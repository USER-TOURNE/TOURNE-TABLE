// ---- Bar levels for the frame being drawn ------------------------------------------
//
// Both renderers draw from the same two arrays: g_vizPeak (each bar's height,
// 0..1) and g_vizPeakHold (its peak cap). This is the one place they are
// worked out, for either engine, so Direct2D and Direct3D 11 can't drift
// apart.
//
//   Classic:   UpdateVisualizerTargets interpolates the 7 bands, then each bar
//              eases toward its target with the per-shape attack / decay, and
//              the caps fall 0.012 per frame. Numbers exactly as in 1.5.
//   Precision: each bar already has its own band with real ballistics, so the
//              Shape mapping is applied directly and the caps hold, then fall
//              with gravity, on elapsed time.

ttdsp::PeakHold g_peakState[VIZ_BARS_MAX];
float g_frameDt = 1.0f / 60.0f;  // seconds since the previous drawn frame (UI thread)
VizBandFrame g_drawBands;        // the UI thread's copy of the latest precision bands

int VizEffectiveBarCount() {
    if (g_settings.engine == VizEngineKind::Precision && g_settings.bandLayout != VizBandLayout::Scale &&
        g_settings.layoutBandCount > 0)
        return std::clamp(g_settings.layoutBandCount, 1, VIZ_BARS_MAX);
    return std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
}

// Wave and Breathe modulation for bar i, from the high-resolution clock. Used
// by the CPU mapping below and uploaded per frame for the GPU one.
float VizShapeMod(int i, int n, double clock) {
    if (g_settings.shape == VizShape::Wave) {
        float phase = (float)i * (2.f * VIZ_PI / (float)std::max(1, n));
        return 0.55f + 0.45f * sinf(VizClockPhase(clock, 3.5, 2.0 * 3.14159265358979323846) - phase);
    }
    if (g_settings.shape == VizShape::Breathe) {
        float seed = VIZ_SEEDS[i % VIZ_BARS_MAX];
        double rate = 0.55 + seed * 0.18;
        return 0.5f + 0.5f * sinf(VizClockPhase(clock, rate, 2.0 * 3.14159265358979323846) + seed * 1.2f);
    }
    return 1.0f;
}

void UpdatePrecisionTargets(int vizBars) {
    ReadBandFrame(g_drawBands);
    const VizBandFrame& f = g_drawBands;
    const int nb = f.count;
    float master = 0.f;
    for (int b = 0; b < nb; b++) master = std::max(master, f.level[b]);

    // Breathe's slow envelope, on elapsed time: the 1.5 per-frame constants
    // 0.04 / 0.015 at 60 FPS are time constants of 0.41 s / 1.10 s.
    {
        float tau = (master > g_vizBreatheEnv) ? 0.408f : 1.103f;
        g_vizBreatheEnv += (master - g_vizBreatheEnv) * (1.f - expf(-g_frameDt / tau));
    }
    auto sample = [&](float t) -> float {
        if (nb <= 0) return 0.f;
        float pos = std::clamp(t, 0.f, 1.f) * (float)(nb - 1);
        int lo = (int)pos;
        int hi = std::min(lo + 1, nb - 1);
        float fr = pos - (float)lo;
        return f.level[lo] * (1.f - fr) + f.level[hi] * fr;
    };
    const double clock = VizClockSeconds();
    float center = (vizBars - 1) * 0.5f;
    for (int i = 0; i < vizBars; i++) {
        float freqT = (vizBars > 1) ? (float)i / (float)(vizBars - 1) : 0.5f;
        float target = 0.f;
        switch (g_settings.shape) {
            case VizShape::Mountain: {
                float dist = fabsf((float)i - center) / std::max(1.f, center);
                target = (sample(dist) + master * (0.2f - dist * 0.12f)) * (1.6f - dist * 0.9f);
                break;
            }
            case VizShape::Mirror: {
                float mirT = 1.f - fabsf((float)i - center) / std::max(1.f, center);
                target = (sample(mirT) + master * (0.1f + mirT * 0.12f)) * 1.3f;
                break;
            }
            case VizShape::Wave:
                target = sample(freqT) * VizShapeMod(i, vizBars, clock) + master * 0.15f;
                break;
            case VizShape::Breathe:
                target = VizShapeMod(i, vizBars, clock) * (0.12f + g_vizBreatheEnv * 0.88f);
                break;
            case VizShape::Oscilloscope:
            case VizShape::Goniometer:
                target = 0.f;
                break;
            default:
                target = sample(freqT);
                break;
        }
        g_vizTarget[i] = std::clamp(target, 0.f, 1.f);
    }
}

void VizComputeBarFrame() {
    const int barCount = VizEffectiveBarCount();
    if (g_settings.engine == VizEngineKind::Precision) {
        UpdatePrecisionTargets(barCount);
        const bool gravity = g_settings.peakFall == VizPeakFall::Gravity;
        for (int i = 0; i < barCount; i++) {
            g_vizPeak[i] = g_vizTarget[i];
            if (g_settings.peakHoldEnabled) {
                // Gravity 2 bar-heights / s^2 drops a full-height cap in 1 s;
                // linear falls a full height in 1.4 s, close to 1.5's
                // 0.012 per 60 FPS frame.
                ttdsp::PeakHoldStep(g_peakState[i], g_vizPeak[i], g_frameDt, (double)g_settings.peakHoldMs,
                                    gravity, 2.0f, 0.72f);
                g_vizPeakHold[i] = g_peakState[i].level;
            } else {
                g_peakState[i] = ttdsp::PeakHold();
                g_vizPeakHold[i] = 0.f;
            }
        }
        return;
    }

    UpdateVisualizerTargets();
    float attack = 0.55f, decay = 0.18f;
    switch (g_settings.shape) {
        case VizShape::Stereo: attack = 0.72f; decay = 0.22f; break;
        case VizShape::Mirror: attack = 0.52f; decay = 0.20f; break;
        case VizShape::Wave: attack = 0.34f; decay = 0.17f; break;
        case VizShape::Breathe: attack = 0.20f; decay = 0.11f; break;
        default: break;
    }
    if (g_settings.smoothing > 0) {
        float smoothFactor = 1.0f - (g_settings.smoothing / 100.0f) * 0.9f;
        attack *= smoothFactor;
        decay *= smoothFactor;
    }
    attack = VizEaseForFrame(attack);
    decay = VizEaseForFrame(decay);
    for (int i = 0; i < barCount; i++) {
        float tgt = g_vizTarget[i], cur = g_vizPeak[i];
        float next = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
        g_vizPeak[i] = (fabsf(next - cur) > 0.0005f) ? next : tgt;
        float fac = std::max(0.f, g_vizPeak[i]);
        if (g_settings.peakHoldEnabled) {
            g_vizPeakHold[i] =
                (fac >= g_vizPeakHold[i]) ? fac : std::max(0.f, g_vizPeakHold[i] - 0.012f * g_frameScale);
        } else {
            g_vizPeakHold[i] = 0.f;
        }
    }
}

// Copies what the engine thread needs out of the settings, as one value, and
// tells it. UI thread, from LoadSettings.
void VizPublishEngineConfig() {
    VizEngineConfig c;
    c.precision = g_settings.engine == VizEngineKind::Precision;
    c.workload = g_settings.workload;
    // GPU analysis needs both the Precision engine and the Direct3D 11
    // renderer; without either it is Hybrid (LoadSettings has said why).
    if (c.workload == VizWorkload::Gpu &&
        (!c.precision || g_settings.renderer != VizRenderer::D3D11 || g_settings.shape == VizShape::Terminal))
        c.workload = VizWorkload::Hybrid;
    auto& s = c.spec;
    s.fftSize = g_settings.fftSize;
    if (c.workload == VizWorkload::Gpu) s.fftSize = std::min(s.fftSize, 4096);  // GPU FFT limit
    s.maxTier = g_settings.bassDetail;
    s.window = (ttdsp::WindowKind)(int)g_settings.window;
    s.layout = (ttdsp::BandLayout)(int)g_settings.bandLayout;
    s.scale = (ttdsp::FreqScale)(int)g_settings.freqScale;
    s.octaveFraction = g_settings.octaveFraction;
    s.fmin = g_settings.minFreq;
    s.fmax = g_settings.maxFreq;
    s.a4 = g_settings.tuningA4;
    s.bars = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
    s.weighting = (ttdsp::Weighting)(int)g_settings.weighting;
    s.tiltDbPerOct = g_settings.tiltDbPerOct;
    s.detector = (ttdsp::Detector)(int)g_settings.detector;
    s.levelRef = (ttdsp::LevelRef)(int)g_settings.levelRef;
    // EQ Preset as dB per zone, from the 1.4 multipliers.
    auto eq = GetVizEQMultipliers(g_settings.eq);
    s.zoneDb[0] = 20.0 * log10(std::max(0.01f, eq.low));
    s.zoneDb[1] = 20.0 * log10(std::max(0.01f, eq.mid));
    s.zoneDb[2] = 20.0 * log10(std::max(0.01f, eq.high));

    if (g_settings.ballistics == VizBallisticsPreset::Custom) {
        c.ball.attackMs = g_settings.attackMs;
        c.ball.release = ttdsp::ReleaseKind::Linear;
        c.ball.releaseDbPerSec = g_settings.releaseDbPerSec;
    } else {
        c.ball = ttdsp::BallisticsPreset((int)g_settings.ballistics);
    }
    // Motion Smoothing slows whichever ballistics are in use, up to 6x.
    double slow = 1.0 + g_settings.smoothing / 20.0;
    c.ball.attackMs *= slow;
    c.ball.releaseMs *= slow;
    c.ball.releaseDbPerSec /= slow;

    c.disp.floorDb = (float)g_settings.dbFloor;
    c.disp.ceilDb = (float)g_settings.dbCeiling;
    c.disp.curve = (ttdsp::Curve)(int)g_settings.sensitivityCurve;
    // Sensitivity keeps its 0-300 range and its default of 150: here it is a
    // gain of 0.2 dB per step around the default, -30 to +30 dB.
    c.sensDb = (g_settings.sensitivity - 150) * 0.2f;
    c.autoGain = g_settings.autoGain;
    c.autoGainMaxDb = g_settings.autoGainMaxDb;
    c.channel = g_settings.channel;
    bool readout = g_settings.peakFreqEnabled;
    c.wantDominant = readout && (g_settings.readout == VizReadout::Frequency || g_settings.readout == VizReadout::Both);
    c.wantLoudness = readout && g_settings.readout != VizReadout::Frequency;
    c.wantGonio = g_settings.shape == VizShape::Goniometer;
    c.beat = g_settings.beatFlashEnabled;
    c.loudnessResetOnTrack = g_settings.loudnessResetOnTrack;
    {
        std::lock_guard<std::mutex> lock(g_engineCfgMutex);
        g_engineCfg = c;
    }
    g_engineCfgGen.fetch_add(1, std::memory_order_acq_rel);
    // Audio source: a different one reopens the stream on the engine thread.
    {
        std::lock_guard<std::mutex> lock(g_audioSourceMutex);
        std::wstring key = g_settings.audioSourceKey == L"default_output" ? L"" : g_settings.audioSourceKey;
        if (key != g_audioSourceKeyShared) {
            g_audioSourceKeyShared = key;
            g_deviceChanged.store(true, std::memory_order_relaxed);
        }
    }
    if (g_engineWake) SetEvent(g_engineWake);
}
