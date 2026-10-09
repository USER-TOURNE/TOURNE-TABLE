    // ---- 2.0: Workload, Renderer, Opaque Panel --------------------------------------
    {
        PCWSTR v = Wh_GetStringSetting(L"hardware.workload");
        g_settings.workload = (wcscmp(v, L"gpu") == 0)   ? VizWorkload::Gpu
                            : (wcscmp(v, L"cpu") == 0)   ? VizWorkload::Cpu
                            : (wcscmp(v, L"npu") == 0)   ? VizWorkload::Npu
                                                         : VizWorkload::Hybrid;
        Wh_FreeStringSetting(v);
        v = Wh_GetStringSetting(L"hardware.renderer");
        g_settings.renderer = (wcscmp(v, L"direct2d") == 0) ? VizRenderer::Direct2D : VizRenderer::D3D11;
        Wh_FreeStringSetting(v);
        v = Wh_GetStringSetting(L"hardware.opaquePanel");
        g_settings.opaquePanel = (wcscmp(v, L"off") == 0) ? VizOpaquePanel::Off : VizOpaquePanel::Auto;
        Wh_FreeStringSetting(v);
    }

    // ---- 2.0: Analysis ------------------------------------------------------------------
    {
        auto str = [](PCWSTR key, auto&& fn) {
            PCWSTR v = Wh_GetStringSetting(key);
            fn(v ? v : L"");
            Wh_FreeStringSetting(v);
        };
        str(L"analysis.engine", [](PCWSTR v) {
            g_settings.engine = (wcscmp(v, L"classic") == 0) ? VizEngineKind::Classic : VizEngineKind::Precision;
        });
        str(L"analysis.bandLayout", [](PCWSTR v) {
            g_settings.bandLayout = (wcscmp(v, L"iec") == 0)       ? VizBandLayout::Iec
                                  : (wcscmp(v, L"musical") == 0)   ? VizBandLayout::Musical
                                                                   : VizBandLayout::Scale;
        });
        str(L"analysis.octaveFraction", [](PCWSTR v) {
            int f = _wtoi(v);
            g_settings.octaveFraction = (f == 1 || f == 3 || f == 6 || f == 12 || f == 24) ? f : 6;
        });
        g_settings.minFreq = std::clamp(Wh_GetIntSetting(L"analysis.minFrequency"), 10, 1000);
        g_settings.maxFreq = std::clamp(Wh_GetIntSetting(L"analysis.maxFrequency"), 1000, 21000);
        if (g_settings.maxFreq <= g_settings.minFreq * 2) g_settings.maxFreq = std::max(1000, g_settings.minFreq * 4);
        {
            int a4 = Wh_GetIntSetting(L"analysis.tuningA4");
            g_settings.tuningA4 = (float)((a4 >= 400 && a4 <= 480) ? a4 : 440);
        }
        str(L"analysis.weighting", [](PCWSTR v) {
            g_settings.weighting = (wcscmp(v, L"a") == 0) ? VizWeighting::A
                                 : (wcscmp(v, L"c") == 0) ? VizWeighting::C
                                                          : VizWeighting::Z;
        });
        str(L"analysis.tilt", [](PCWSTR v) {
            float t = (float)_wtof(v);
            g_settings.tiltDbPerOct = (*v) ? std::clamp(t, 0.f, 6.f) : 1.5f;
        });
        str(L"analysis.detector", [](PCWSTR v) {
            g_settings.detector = (wcscmp(v, L"peak") == 0) ? VizDetector::Peak : VizDetector::Rms;
        });
        str(L"analysis.levelReference", [](PCWSTR v) {
            g_settings.levelRef = (wcscmp(v, L"band") == 0) ? VizLevelRef::Band : VizLevelRef::ThirdOctave;
        });
        str(L"analysis.window", [](PCWSTR v) {
            g_settings.window = (wcscmp(v, L"hamming") == 0)           ? VizWindowKind::Hamming
                              : (wcscmp(v, L"blackman_harris") == 0)   ? VizWindowKind::BlackmanHarris
                              : (wcscmp(v, L"flat_top") == 0)          ? VizWindowKind::FlatTop
                                                                       : VizWindowKind::Hann;
        });
        str(L"analysis.bassDetail", [](PCWSTR v) {
            g_settings.bassDetail = (*v) ? std::clamp(_wtoi(v), 0, 2) : 2;
        });
        str(L"analysis.channel", [](PCWSTR v) {
            g_settings.channel = (wcscmp(v, L"left") == 0)    ? VizChannel::Left
                               : (wcscmp(v, L"right") == 0)   ? VizChannel::Right
                               : (wcscmp(v, L"mid") == 0)     ? VizChannel::Mid
                               : (wcscmp(v, L"side") == 0)    ? VizChannel::Side
                                                              : VizChannel::Mix;
        });
        g_settings.dbFloor = std::clamp(Wh_GetIntSetting(L"analysis.dbFloor"), -120, -20);
        g_settings.dbCeiling = std::clamp(Wh_GetIntSetting(L"analysis.dbCeiling"), -60, 0);
        if (g_settings.dbCeiling - g_settings.dbFloor < 10) {
            ReportSettingIssue(L"Analysis", L"Display Ceiling (dBFS)", std::to_wstring(g_settings.dbCeiling).c_str(),
                               L"at least 10 dB above Display Floor", L"Display Floor + 10");
            g_settings.dbCeiling = g_settings.dbFloor + 10;
        }
        str(L"analysis.ballistics", [](PCWSTR v) {
            g_settings.ballistics = (wcscmp(v, L"smooth") == 0)     ? VizBallisticsPreset::Smooth
                                  : (wcscmp(v, L"analyzer") == 0)   ? VizBallisticsPreset::Analyzer
                                  : (wcscmp(v, L"vu") == 0)         ? VizBallisticsPreset::Vu
                                  : (wcscmp(v, L"ppm_ebu") == 0)    ? VizBallisticsPreset::PpmEbu
                                  : (wcscmp(v, L"ppm_din") == 0)    ? VizBallisticsPreset::PpmDin
                                  : (wcscmp(v, L"custom") == 0)     ? VizBallisticsPreset::Custom
                                                                    : VizBallisticsPreset::Snappy;
        });
        g_settings.attackMs = std::clamp(Wh_GetIntSetting(L"analysis.attackMs"), 1, 500);
        g_settings.releaseDbPerSec = std::clamp(Wh_GetIntSetting(L"analysis.releaseDbPerSecond"), 1, 200);
        g_settings.peakHoldMs = std::clamp(Wh_GetIntSetting(L"analysis.peakHoldMs"), 0, 5000);
        str(L"analysis.peakFall", [](PCWSTR v) {
            g_settings.peakFall = (wcscmp(v, L"linear") == 0) ? VizPeakFall::Linear : VizPeakFall::Gravity;
        });
        str(L"analysis.readout", [](PCWSTR v) {
            g_settings.readout = (wcscmp(v, L"loudness") == 0)        ? VizReadout::Loudness
                               : (wcscmp(v, L"loudness_full") == 0)   ? VizReadout::LoudnessFull
                               : (wcscmp(v, L"both") == 0)            ? VizReadout::Both
                                                                      : VizReadout::Frequency;
        });
        g_settings.loudnessResetOnTrack = Wh_GetIntSetting(L"analysis.loudnessResetOnTrack") != 0;

        // The IEC and musical layouts decide the bar count. Worked out here at
        // 48 kHz; the range is capped at 21 kHz, below every common device's
        // Nyquist limit, so the count is the same on 44.1 kHz.
        g_settings.layoutBandCount = 0;
        if (g_settings.engine == VizEngineKind::Precision && g_settings.bandLayout != VizBandLayout::Scale) {
            std::vector<ttdsp::Band> b =
                (g_settings.bandLayout == VizBandLayout::Iec)
                    ? ttdsp::IecBands(g_settings.octaveFraction, g_settings.minFreq, g_settings.maxFreq)
                    : ttdsp::MusicalBands(g_settings.octaveFraction, g_settings.tuningA4, g_settings.minFreq,
                                          g_settings.maxFreq);
            g_settings.layoutBandCount = std::clamp((int)b.size(), 1, VIZ_BARS_MAX);
        }

        // Combinations that can't be honoured, said out loud.
        if (g_settings.workload == VizWorkload::Gpu && g_settings.engine == VizEngineKind::Classic) {
            ReportSettingWarning(L"Hardware", L"Workload",
                                 L"GPU analysis is the Precision engine on the GPU. With the Classic engine the "
                                 L"analysis stays on the CPU (Hybrid).");
        }
        if (g_settings.workload == VizWorkload::Gpu && g_settings.renderer == VizRenderer::Direct2D) {
            ReportSettingWarning(L"Hardware", L"Workload",
                                 L"GPU analysis needs the Direct3D 11 renderer, so with Direct2D it runs on the "
                                 L"CPU (Hybrid).");
        }
    }
