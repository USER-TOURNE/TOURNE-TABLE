// ---- Styles (2.1) ---------------------------------------------------------------------
// Eight new looks, picked from the Shape list like the shapes. Each one rides
// on an existing shape underneath (Bloom on Radial, the rest on Stereo), so
// the bar maths, layout, panel and settings all keep working unchanged, and
// a style only adds what it really needs:
//   LED Meter      segmented bars, green / amber / red, peak segment held
//   Line Spectrum  a smooth filled curve through the bar tops, glowing edge
//   Polar Bloom    the Radial bars joined into one filled shape
//   Spectrogram    a scrolling colour history of the bars, with a legend
//   VU Needles     two analog L / R meters with real VU ballistics
//   Stereo Field   the left channel above the centre line, the right below
//   Particles      the bars, plus sparks thrown off their tops on each beat
// Reflection (Appearance) mirrors the bar styles onto the floor beneath them.
//
// Spectrogram, Stereo Field and Particles need the bar levels on the CPU, so
// with them the analysis runs there (Hybrid), as with Terminal.

static const float kVizSpecRate = 60.f;  // Spectrogram rows per second
constexpr int kVizSpecRows = 256;        // rows kept (the texture's height)
constexpr int kVizSparkMax = 512;

bool VizParseShape(PCWSTR v, VizShape* shape, VizStyle* style) {
    struct Entry { const wchar_t* name; VizShape shape; VizStyle style; };
    static const Entry kEntries[] = {
        {L"stereo", VizShape::Stereo, VizStyle::None},         {L"mountain", VizShape::Mountain, VizStyle::None},
        {L"mirror", VizShape::Mirror, VizStyle::None},         {L"wave", VizShape::Wave, VizStyle::None},
        {L"breathe", VizShape::Breathe, VizStyle::None},       {L"dots", VizShape::Dots, VizStyle::None},
        {L"radial", VizShape::Radial, VizStyle::None},         {L"oscilloscope", VizShape::Oscilloscope, VizStyle::None},
        {L"goniometer", VizShape::Goniometer, VizStyle::None}, {L"terminal", VizShape::Terminal, VizStyle::None},
        {L"led", VizShape::Stereo, VizStyle::Led},             {L"line", VizShape::Stereo, VizStyle::Line},
        {L"bloom", VizShape::Radial, VizStyle::Bloom},         {L"spectrogram", VizShape::Stereo, VizStyle::Spectrogram},
        {L"vu", VizShape::Stereo, VizStyle::Vu},               {L"stereo_field", VizShape::Stereo, VizStyle::SplitLR},
        {L"particles", VizShape::Stereo, VizStyle::Particles},
    };
    for (const Entry& e : kEntries) {
        if (v && wcscmp(v, e.name) == 0) {
            *shape = e.shape;
            *style = e.style;
            return true;
        }
    }
    *shape = VizShape::Stereo;
    *style = VizStyle::None;
    return false;
}

// Position in the right-click menu's Shape list: the ten shapes, then the styles.
int VizShapeMenuIndex() {
    return g_settings.style != VizStyle::None ? 9 + (int)g_settings.style : (int)g_settings.shape;
}

bool VizStyleNeedsCpuBars() {
    VizStyle s = g_settings.style;
    return s == VizStyle::Spectrogram || s == VizStyle::SplitLR || s == VizStyle::Particles;
}

// Reflection: horizontal bars standing on the bottom edge only, where there
// is a floor to reflect in.
bool VizReflectionActive() {
    if (g_settings.reflection <= 0) return false;
    if (g_settings.orientation != VizOrientation::Horizontal) return false;
    if (g_settings.verticalAnchor != VizAnchor::Bottom) return false;
    VizStyle st = g_settings.style;
    if (!(st == VizStyle::None || st == VizStyle::Led || st == VizStyle::Line || st == VizStyle::Particles)) return false;
    VizShape s = g_settings.shape;
    return s == VizShape::Stereo || s == VizShape::Mountain || s == VizShape::Mirror || s == VizShape::Wave ||
           s == VizShape::Breathe || s == VizShape::Dots;
}

float VizReflectionDepth(float maxSize) { return maxSize * std::clamp(g_settings.reflection, 0, 100) / 100.f; }

// ---- Scale numbers -----------------------------------------------------------------------
// The Spectrogram legend's quarter ticks and the VU faces carry numbers. They
// are Direct2D text: on the Direct3D renderer they go on the text surface,
// which only redraws when something on it changes, so a still scale costs
// nothing per frame.
namespace {
ComPtr<IDWriteTextFormat> s_scaleFmt;
float s_scaleFmtPx = 0.f;

IDWriteTextFormat* VizScaleFormat(float px) {
    if (!g_dwriteFactory) return nullptr;
    if (!s_scaleFmt || fabsf(s_scaleFmtPx - px) > 0.01f) {
        s_scaleFmt.Reset();
        if (FAILED(g_dwriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                     DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, px, L"",
                                                     &s_scaleFmt)))
            return nullptr;
        s_scaleFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        s_scaleFmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        s_scaleFmtPx = px;
    }
    return s_scaleFmt.Get();
}
}  // namespace

float VizScaleLabelPx(float maxSize) { return std::clamp(maxSize * 0.07f, 8.f * g_dpiScale, 13.f * g_dpiScale); }

bool VizStyleHasScale() { return g_settings.style == VizStyle::Spectrogram || g_settings.style == VizStyle::Vu; }

// `thick` is the bars' span across the time axis (Spectrogram only).
void VizDrawStyleScale(float blockX, float blockY, float maxSize, float thick, bool horizontal) {
    if (!g_dc || !g_barBrush || !VizStyleHasScale()) return;
    ID2D1SolidColorBrush* b = g_barBrush.Get();
    WCHAR s[24];
    if (g_settings.style == VizStyle::Spectrogram) {
        float px = VizScaleLabelPx(maxSize);
        IDWriteTextFormat* fmt = VizScaleFormat(px);
        if (!fmt) return;
        fmt->SetTextAlignment(horizontal ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_CENTER);
        b->SetColor(D2D1::ColorF(0.92f, 0.92f, 0.92f, 0.85f));
        // Precision maps Display Floor..Ceiling straight onto the bar height,
        // so the ticks are dB; Classic has no fixed dB scale, so percent.
        bool db = g_settings.engine == VizEngineKind::Precision;
        float side = 3.f * g_dpiScale + 6.f * g_dpiScale;
        for (int k = 0; k <= 4; k++) {
            float q = k / 4.f;
            if (db)
                swprintf_s(s, k == 4 ? L"%d dB" : L"%d",
                           (int)lroundf(g_settings.dbFloor + q * (g_settings.dbCeiling - g_settings.dbFloor)));
            else
                swprintf_s(s, L"%d%%", k * 25);
            D2D1_RECT_F r;
            if (horizontal) {
                float y = std::clamp(blockY + maxSize - q * maxSize, blockY + px * 0.6f, blockY + maxSize - px * 0.6f);
                float x = blockX + thick + side + 3.f * g_dpiScale;
                r = D2D1::RectF(x, y - px, x + px * 4.f, y + px);
            } else {
                float x = std::clamp(blockX + q * maxSize, blockX + px * 1.2f, blockX + maxSize - px * 1.2f);
                float y = blockY + thick + side + 2.f * g_dpiScale;
                r = D2D1::RectF(x - px * 2.5f, y, x + px * 2.5f, y + px * 1.3f);
            }
            g_dc->DrawText(s, (UINT32)wcslen(s), fmt, r, b, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
        return;
    }
    // VU: the classic face numbers, inside the tick arc, red above 0 VU.
    float mh = maxSize, mw = maxSize * 1.5f, gap = 8.f * g_dpiScale;
    float px = std::max(7.f * g_dpiScale, mh * 0.075f);
    IDWriteTextFormat* fmt = VizScaleFormat(px);
    if (!fmt) return;
    fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    const float marks[7] = {-20, -10, -7, -5, -3, 0, 3};
    for (int m = 0; m < 2; m++) {
        float ox = blockX + (horizontal ? m * (mw + gap) : 0.f), oy = blockY + (horizontal ? 0.f : m * (mh + gap));
        float pvx = ox + mw * 0.5f, pvy = oy + mh * 0.9f, R = mh * 0.68f * 0.74f;
        for (float dbm : marks) {
            float p = (powf(10.f, dbm / 20.f) - 0.1f) / (1.41254f - 0.1f);
            float an = (-48.f + 96.f * p) * VIZ_PI / 180.f;
            float x = pvx + sinf(an) * R, y = pvy - cosf(an) * R;
            swprintf_s(s, dbm > 0 ? L"+%d" : L"%d", (int)fabsf(dbm));
            b->SetColor(dbm > 0 ? D2D1::ColorF(1.f, 0.27f, 0.23f, 0.95f) : D2D1::ColorF(0.92f, 0.92f, 0.9f, 0.85f));
            g_dc->DrawText(s, (UINT32)wcslen(s), fmt, D2D1::RectF(x - px * 1.5f, y - px * 0.7f, x + px * 1.5f, y + px * 0.7f),
                           b, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
    }
}

// The styles' extra room, applied after the shapes have sized the box.
void VizStyleBox(float* w, float* h, float maxSize, bool horizontal) {
    switch (g_settings.style) {
        case VizStyle::Vu: {
            float mw = maxSize * 1.5f, gap = 8.f * g_dpiScale;
            *w = horizontal ? mw * 2.f + gap : mw;
            *h = horizontal ? maxSize : maxSize * 2.f + gap;
            break;
        }
        case VizStyle::Spectrogram: {
            float legend = 3.f * g_dpiScale + 6.f * g_dpiScale, px = VizScaleLabelPx(maxSize);
            if (horizontal) *w += legend + 3.f * g_dpiScale + px * 3.4f;  // plus the scale numbers
            else *h += legend + 2.f * g_dpiScale + px * 1.3f;
            break;
        }
        default: break;
    }
    if (VizReflectionActive()) *h += VizReflectionDepth(maxSize);
}

// ---- Per-frame data, on the render thread ----------------------------------------------
float g_vizSplitL[VIZ_BARS_MAX] = {}, g_vizSplitR[VIZ_BARS_MAX] = {};
float g_vizVu[4] = {};  // needle L, R (0..1 of the scale), peak LED L, R
std::vector<uint8_t> g_vizSpecRing;  // bars x kVizSpecRows, newest row at g_vizSpecHead
int g_vizSpecW = 0, g_vizSpecHead = 0;
uint32_t g_vizSpecSerial = 0;  // rows pushed so far
float g_vizSparkBuf[kVizSparkMax * 4] = {};
int g_vizSparkCount = 0;
uint32_t g_vizSparkSerial = 0;

namespace {
struct VizSpark { float x, y, vx, vy, life, band, r; };
std::vector<VizSpark> s_sparks;
double s_styleClock = 0.0, s_specAcc = 0.0;
uint32_t s_stereoSerial = 0;
float s_hist[2][2048] = {};  // newest stereo samples, L and R
int s_histPos = 0;
float s_vuIn[2] = {}, s_vuPos[2] = {-0.03f, -0.03f}, s_vuVel[2] = {};
double s_vuLastBlock = 0.0;
float s_lastPulse = 0.f;
uint32_t s_rng = 2463534242u;

float Rand01() {
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return (float)(s_rng & 0xFFFFFF) / 16777216.f;
}

// VU scale position (0 at -20 VU, 1 at +3 VU) of a linear level, with
// 0 VU = -18 dBFS. Real VU meters are close to linear in voltage, so the
// scale's marks crowd toward the top as they do here.
float VuPosOf(float rms) { return std::clamp((rms * 7.943f - 0.1f) / (1.41254f - 0.1f), -0.03f, 1.08f); }

// New stereo samples from the engine (the Goniometer feed): VU input levels
// and peak LEDs, and the history Stereo Field analyses.
void TakeStereo(double now) {
    uint32_t serial = g_gonioSerial.load(std::memory_order_acquire);
    if (serial == s_stereoSerial) {
        if (now - s_vuLastBlock > 0.25) s_vuIn[0] = s_vuIn[1] = 0.f;  // nothing new: the signal stopped
        return;
    }
    s_stereoSerial = serial;
    s_vuLastBlock = now;
    double ms[2] = {0, 0};
    float pk[2] = {0, 0};
    size_t n = 0;
    {
        std::lock_guard<std::mutex> lock(g_gonioMutex);
        n = g_gonioXY.size() / 2;
        for (size_t i = 0; i < n; i++) {
            float side = g_gonioXY[2 * i], mid = g_gonioXY[2 * i + 1];
            float l = mid + side, r = mid - side;
            ms[0] += (double)l * l;
            ms[1] += (double)r * r;
            pk[0] = std::max(pk[0], fabsf(l));
            pk[1] = std::max(pk[1], fabsf(r));
            s_hist[0][s_histPos] = l;
            s_hist[1][s_histPos] = r;
            s_histPos = (s_histPos + 1) & 2047;
        }
    }
    if (!n) return;
    for (int c = 0; c < 2; c++) {
        s_vuIn[c] = VuPosOf((float)sqrt(ms[c] / (double)n));
        if (pk[c] >= 0.708f) g_vizVu[2 + c] = 1.f;  // -3 dBFS sample peak
    }
}

// VU ballistics, IEC 60268-17: a second-order needle, 99 % of a step in
// 300 ms with 1.5 % overshoot (zeta 0.8, omega 13.1 rad/s).
void StepVu(float dt) {
    const float wn = 13.1f, z = 0.8f;
    int steps = std::max(1, (int)ceilf(dt / 0.002f));
    float h = dt / steps;
    for (int c = 0; c < 2; c++) {
        for (int s = 0; s < steps; s++) {
            float a = wn * wn * (s_vuIn[c] - s_vuPos[c]) - 2.f * z * wn * s_vuVel[c];
            s_vuVel[c] += a * h;
            s_vuPos[c] += s_vuVel[c] * h;
        }
        s_vuPos[c] = std::clamp(s_vuPos[c], -0.04f, 1.1f);
        g_vizVu[c] = s_vuPos[c];
        g_vizVu[2 + c] = std::max(0.f, g_vizVu[2 + c] - dt / 0.6f);
    }
}

// Stereo Field: a 2048-point spectrum per channel, log-spaced bars from
// 30 Hz, 72 dB of range. Only when new samples came in.
void StepSplit(int bars, float dt, bool fresh) {
    static ttdsp::RealFft fft;
    static std::vector<float> win, buf, re, im;
    static float target[2][VIZ_BARS_MAX] = {};
    const int N = 2048;
    if (fft.Size() != N) {
        fft.Init(N);
        win.resize(N);
        for (int i = 0; i < N; i++) win[i] = 0.5f - 0.5f * cosf(2.f * VIZ_PI * i / (N - 1));
        buf.resize(N);
        re.resize(N / 2 + 1);
        im.resize(N / 2 + 1);
    }
    if (fresh) {
        float sr = (float)std::max<uint32_t>(8000, g_vizStereoRate.load(std::memory_order_relaxed));
        float fmax = std::min(18000.f, sr * 0.45f), fmin = 30.f;
        const double norm = 1.0 / ((N / 4.0) * (N / 4.0));  // full-scale sine through Hann = 0 dB
        for (int c = 0; c < 2; c++) {
            for (int i = 0; i < N; i++) buf[i] = s_hist[c][(s_histPos + i) & 2047] * win[i];
            fft.Forward(buf.data(), re.data(), im.data());
            for (int b = 0; b < bars; b++) {
                float f0 = fmin * powf(fmax / fmin, (float)b / bars), f1 = fmin * powf(fmax / fmin, (float)(b + 1) / bars);
                int k0 = std::clamp((int)(f0 * N / sr), 1, N / 2), k1 = std::clamp((int)(f1 * N / sr), k0, N / 2);
                double p = 0;
                for (int k = k0; k <= k1; k++) p = std::max(p, (double)re[k] * re[k] + (double)im[k] * im[k]);
                float db = 10.f * log10f((float)std::max(p * norm, 1e-12));
                target[c][b] = std::clamp((db + 72.f) / 66.f, 0.f, 1.f);
            }
        }
    }
    float att = 1.f - expf(-dt / 0.012f);
    float fall = dt * 1.4f;
    for (int b = 0; b < bars; b++) {
        float* lv[2] = {&g_vizSplitL[b], &g_vizSplitR[b]};
        for (int c = 0; c < 2; c++) {
            float t = target[c][b];
            *lv[c] = t > *lv[c] ? *lv[c] + (t - *lv[c]) * att : std::max(t, *lv[c] - fall);
        }
    }
}

// Spectrogram: a row of the bars' levels every 1/60 s into the ring.
void StepSpectrogram(int bars, float dt) {
    if (bars != g_vizSpecW || (int)g_vizSpecRing.size() != bars * kVizSpecRows) {
        g_vizSpecW = bars;
        g_vizSpecRing.assign((size_t)bars * kVizSpecRows, 0);
        g_vizSpecHead = 0;
        g_vizSpecSerial += kVizSpecRows;  // a full re-upload
        s_specAcc = 0.0;
    }
    s_specAcc += dt * kVizSpecRate;
    int rows = std::min(8, (int)s_specAcc);
    s_specAcc -= rows;
    for (int r = 0; r < rows; r++) {
        g_vizSpecHead = (g_vizSpecHead + 1) % kVizSpecRows;
        uint8_t* row = &g_vizSpecRing[(size_t)g_vizSpecHead * bars];
        for (int i = 0; i < bars; i++) row[i] = (uint8_t)lroundf(std::clamp(g_vizPeak[i], 0.f, 1.f) * 255.f);
        g_vizSpecSerial++;
    }
}

// Particles: on each beat, sparks leave the tops of the louder bars, thrown
// no higher than the box, and fall back under gravity.
void StepSparks(int bars, float dt) {
    const bool horizontal = g_settings.orientation == VizOrientation::Horizontal;
    const bool top = g_settings.verticalAnchor == VizAnchor::Top;
    const float barW = std::max(1.f, VizPx((float)std::max(1, g_settings.barWidth)));
    const float barGap = VizPx((float)std::max(0, g_settings.barGap));
    const float maxSize = std::max(2.f, VizPx((float)std::max(2, g_settings.barMaxSize)));
    const float idle = VizPx((float)std::max(0, g_settings.barIdleSize));
    const float G = 900.f * g_dpiScale;
    // Growth direction of the bars, in block coordinates.
    const float gx = horizontal ? 0.f : (top ? -1.f : 1.f);
    const float gy = horizontal ? (top ? 1.f : -1.f) : 0.f;
    float pulse = g_beatPulse.load(std::memory_order_relaxed);
    if (pulse > s_lastPulse + 0.3f && bars > 0) {
        for (int k = 0; k < 18 && (int)s_sparks.size() < kVizSparkMax; k++) {
            int i = std::min(bars - 1, (int)(Rand01() * bars));
            for (int t = 0; t < 4 && g_vizPeak[i] < 0.25f; t++) i = std::min(bars - 1, (int)(Rand01() * bars));
            float len = idle + std::max(0.f, g_vizPeak[i]) * std::max(0.f, maxSize - idle);
            float along = i * (barW + barGap) + barW * 0.5f;
            float base = horizontal ? (top ? 0.f : maxSize) : (top ? maxSize : 0.f);
            float tip = base + (horizontal ? gy : gx) * len;
            float room = std::max(0.f, maxSize - len);
            float v = sqrtf(2.f * G * room) * (0.45f + 0.55f * Rand01());
            float side = (Rand01() - 0.5f) * 80.f * g_dpiScale;
            VizSpark s;
            s.x = horizontal ? along : tip;
            s.y = horizontal ? tip : along;
            s.vx = gx * v + (horizontal ? side : 0.f);
            s.vy = gy * v + (horizontal ? 0.f : side);
            s.life = 1.f;
            s.band = (float)i;
            s.r = std::min(15.f, (1.2f + 1.4f * Rand01()) * g_dpiScale);
            s_sparks.push_back(s);
        }
    }
    s_lastPulse = pulse;
    const float boxW = horizontal ? bars * (barW + barGap) - barGap : maxSize;
    const float boxH = horizontal ? maxSize : bars * (barW + barGap) - barGap;
    size_t out = 0;
    for (size_t k = 0; k < s_sparks.size(); k++) {
        VizSpark s = s_sparks[k];
        s.vx -= gx * G * dt;
        s.vy -= gy * G * dt;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.life -= dt / 1.1f;
        bool inside = s.x >= -s.r && s.y >= -s.r && s.x <= boxW + s.r && s.y <= boxH + s.r;
        if (s.life > 0.f && inside) s_sparks[out++] = s;
    }
    s_sparks.resize(out);
    g_vizSparkCount = (int)out;
    for (size_t k = 0; k < out; k++) {
        const VizSpark& s = s_sparks[k];
        g_vizSparkBuf[k * 4] = s.x;
        g_vizSparkBuf[k * 4 + 1] = s.y;
        g_vizSparkBuf[k * 4 + 2] = s.life;
        g_vizSparkBuf[k * 4 + 3] = s.band * 16.f + s.r;
    }
    if (out) g_vizSparkSerial++;
}
}  // namespace

// Called once per drawn frame, after VizComputeBarFrame.
void VizStylesFrame() {
    VizStyle st = g_settings.style;
    if (st == VizStyle::None || st == VizStyle::Led || st == VizStyle::Line || st == VizStyle::Bloom) return;
    double now = VizClockSeconds();
    float dt = s_styleClock > 0.0 ? (float)std::clamp(now - s_styleClock, 0.0, 0.1) : 1.f / 60.f;
    s_styleClock = now;
    int bars = VizEffectiveBarCount();
    if (st == VizStyle::Vu || st == VizStyle::SplitLR) {
        uint32_t before = s_stereoSerial;
        TakeStereo(now);
        if (st == VizStyle::Vu) StepVu(dt);
        else StepSplit(bars, dt, s_stereoSerial != before);
    } else if (st == VizStyle::Spectrogram) {
        StepSpectrogram(bars, dt);
    } else if (st == VizStyle::Particles) {
        StepSparks(bars, dt);
    }
}

// ---- Direct2D (the 1.5 path, and the fallback) ---------------------------------------------
// The same looks drawn with plain Direct2D calls. Fine for a fallback; the
// Direct3D 11 renderer is the one to use for these (and the only one that
// draws Reflection).
namespace {
RGBA StyleBarColor(int i, int n, float fac, RGBA c1, RGBA cGrad1, RGBA c2, float rainbowBase, bool radial) {
    RGBA col = c1;
    VizColorMode m = g_settings.colorMode;
    float t = n > 1 ? (float)i / (n - 1) : 0.f;
    if (m == VizColorMode::Gradient || m == VizColorMode::Tourne) col = LerpColor(cGrad1, c2, t);
    else if (m == VizColorMode::ReactiveGradient) col = LerpColor(cGrad1, c2, fac);
    else if (m == VizColorMode::DynamicAlbum && !radial) col = LerpColor(cGrad1, c2, std::min(1.f, t * 0.6f + fac * 0.4f));
    else if (m == VizColorMode::RainbowCycle)
        col = HSVtoRGB(fmodf(rainbowBase + (radial ? (float)i / std::max(1, n) : t) * 360.f, 360.f), 0.85f, 1.0f, c1.a);
    if (m == VizColorMode::Acrylic) col = {(BYTE)std::clamp((int)(180.f * fac), 0, 180), c1.r, c1.g, c1.b};
    return col;
}

void SetBrush(RGBA c, float alphaScale = 1.f) {
    g_barBrush->SetColor(D2D1::ColorF(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f * alphaScale));
}

RGBA SpecColorCpu(float v) {
    v = std::clamp(v, 0.f, 1.f);
    static const float stops[5][3] = {{0, 0, 0}, {0.25f, 0.02f, 0.45f}, {0.85f, 0.15f, 0.35f}, {1, 0.6f, 0.1f}, {1, 1, 0.85f}};
    int k = std::min(3, (int)(v * 4.f));
    float f = v * 4.f - k;
    auto ch = [&](int c) { return (BYTE)lroundf((stops[k][c] + (stops[k + 1][c] - stops[k][c]) * f) * 255.f); };
    return {(BYTE)lroundf(std::clamp(v * 2.5f, 0.f, 1.f) * 255.f), ch(0), ch(1), ch(2)};
}
}  // namespace

// Draws the current style and returns true, or returns false for the shape
// path to draw (Particles draw their sparks here, then let the bars draw).
bool VizDrawStyleD2D(float blockX, float blockY, float totalWidth, float totalHeight, int barCount, float barW,
                     float barGap, float maxSize, float idleSize, bool horizontal, RGBA c1, RGBA cGrad1, RGBA c2,
                     float rainbowBase) {
    const VizStyle st = g_settings.style;
    if (st == VizStyle::None || !g_dc || !g_barBrush) return false;
    const float range = std::max(0.f, maxSize - idleSize);
    const bool top = g_settings.verticalAnchor == VizAnchor::Top;
    auto lenOf = [&](float lev) { return idleSize + std::max(0.f, lev) * range; };
    // Rect of bar i between distances a and b from its base edge (SpanRect in the shader).
    auto span = [&](int i, float a, float b) {
        float lead = i * (barW + barGap);
        if (horizontal) {
            float x = blockX + lead;
            if (top) return D2D1::RectF(x, blockY + a, x + barW, blockY + b);
            float base = blockY + maxSize;
            return D2D1::RectF(x, base - b, x + barW, base - a);
        }
        float y = blockY + lead;
        if (top) return D2D1::RectF(blockX + maxSize - b, y, blockX + maxSize - a, y + barW);
        return D2D1::RectF(blockX + a, y, blockX + b, y + barW);
    };

    if (st == VizStyle::Particles) {
        for (int k = 0; k < g_vizSparkCount; k++) {
            const float* p = &g_vizSparkBuf[k * 4];
            int band = std::min(std::max(barCount - 1, 0), (int)(p[3] / 16.f));
            float r = p[3] - band * 16.f;
            SetBrush(StyleBarColor(band, barCount, 1.f, c1, cGrad1, c2, rainbowBase, false), p[2]);
            g_dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(blockX + p[0], blockY + p[1]), r, r), g_barBrush.Get());
        }
        return false;
    }

    if (st == VizStyle::Led) {
        float segH = std::max(2.f * g_dpiScale, roundf(barW * 0.5f));
        float step = segH + std::max(1.f, roundf(1.5f * g_dpiScale));
        int segs = std::max(1, (int)((maxSize + step - segH) / step));
        for (int i = 0; i < barCount; i++) {
            float lit = lenOf(g_vizPeak[i]) / step;
            float hold = g_settings.peakHoldEnabled ? lenOf(g_vizPeakHold[i]) : 0.f;
            int holdSeg = hold > 0.5f ? std::min(segs - 1, (int)(hold / step)) : -1;
            for (int s = 0; s < segs; s++) {
                float t = (s + 0.5f) / segs;
                RGBA col = g_settings.colorMode == VizColorMode::Solid
                               ? (t >= 0.85f ? RGBA{255, 255, 59, 48} : t >= 0.6f ? RGBA{255, 255, 176, 0} : RGBA{255, 56, 227, 107})
                               : StyleBarColor(i, barCount, g_vizPeak[i], c1, cGrad1, c2, rainbowBase, false);
                bool on = s + 0.5f <= lit || s == holdSeg;
                SetBrush(col, on ? 1.f : 0.1f);
                g_dc->FillRectangle(span(i, s * step, s * step + segH), g_barBrush.Get());
            }
        }
        return true;
    }

    if (st == VizStyle::SplitLR) {
        for (int i = 0; i < barCount; i++) {
            float lead = i * (barW + barGap);
            for (int c = 0; c < 2; c++) {
                float lev = c ? g_vizSplitR[i] : g_vizSplitL[i];
                float s = lenOf(lev) * 0.5f;
                if (s < 0.25f) continue;
                SetBrush(StyleBarColor(i, barCount, lev, c1, cGrad1, c2, rainbowBase, false));
                D2D1_RECT_F r;
                if (horizontal) {
                    float x = blockX + lead, cy = blockY + maxSize * 0.5f;
                    r = c ? D2D1::RectF(x, cy + 0.5f, x + barW, cy + 0.5f + s) : D2D1::RectF(x, cy - 0.5f - s, x + barW, cy - 0.5f);
                } else {
                    float y = blockY + lead, cx = blockX + maxSize * 0.5f;
                    r = c ? D2D1::RectF(cx + 0.5f, y, cx + 0.5f + s, y + barW) : D2D1::RectF(cx - 0.5f - s, y, cx - 0.5f, y + barW);
                }
                g_dc->FillRectangle(r, g_barBrush.Get());
            }
        }
        return true;
    }

    if (st == VizStyle::Line || st == VizStyle::Bloom) {
        ComPtr<ID2D1PathGeometry> geo;
        ComPtr<ID2D1GeometrySink> sink;
        if (!g_d2dFactory || barCount < 3 || FAILED(g_d2dFactory->CreatePathGeometry(&geo)) || FAILED(geo->Open(&sink)))
            return true;
        RGBA col = StyleBarColor(barCount / 2, barCount, 0.5f, c1, cGrad1, c2, rainbowBase, st == VizStyle::Bloom);
        if (st == VizStyle::Line) {
            float sg = horizontal ? (top ? 1.f : -1.f) : (top ? -1.f : 1.f);
            float base = horizontal ? (top ? blockY : blockY + maxSize) : (top ? blockX + maxSize : blockX);
            float start = horizontal ? blockX : blockY;
            auto val = [&](int i) { return lenOf(g_vizPeak[std::clamp(i, 0, barCount - 1)]); };
            auto pt = [&](float along, float v) {
                float c = base + sg * std::clamp(v, 0.f, maxSize);
                return horizontal ? D2D1::Point2F(along, c) : D2D1::Point2F(c, along);
            };
            sink->BeginFigure(pt(start, 0.f), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(pt(start, val(0)));
            const int sub = 4;
            for (int k = 0; k + 1 < barCount; k++) {
                float p0 = val(k - 1), p1 = val(k), p2 = val(k + 1), p3 = val(k + 2);
                for (int j = 1; j <= sub; j++) {
                    float t = (float)j / sub, t2 = t * t, t3 = t2 * t;
                    float v = 0.5f * (2 * p1 + (p2 - p0) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (3 * p1 - p0 - 3 * p2 + p3) * t3);
                    sink->AddLine(pt(start + (k + t) * (barW + barGap) + barW * 0.5f, v));
                }
            }
            float end = start + barCount * (barW + barGap) - barGap;
            sink->AddLine(pt(end, val(barCount - 1)));
            sink->AddLine(pt(end, 0.f));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        } else {
            float cx = blockX + totalWidth * 0.5f, cy = blockY + totalHeight * 0.5f, innerR = maxSize * 0.15f;
            for (int i = 0; i < barCount; i++) {
                float a = (float)i / barCount * 2.f * VIZ_PI - VIZ_PI * 0.5f, r = innerR + lenOf(g_vizPeak[i]);
                D2D1_POINT_2F p = D2D1::Point2F(cx + cosf(a) * r, cy + sinf(a) * r);
                if (i == 0) sink->BeginFigure(p, D2D1_FIGURE_BEGIN_FILLED);
                else sink->AddLine(p);
            }
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        }
        sink->Close();
        SetBrush(col, st == VizStyle::Line ? 0.35f : 0.55f);
        g_dc->FillGeometry(geo.Get(), g_barBrush.Get());
        SetBrush(col);
        g_dc->DrawGeometry(geo.Get(), g_barBrush.Get(), std::max(1.5f, barW * 0.2f));
        return true;
    }

    if (st == VizStyle::Spectrogram) {
        static ComPtr<ID2D1Bitmap> bmp;
        static ID2D1DeviceContext* bmpDc = nullptr;
        static uint32_t bmpSerial = 0;
        static std::vector<uint32_t> px;
        int w = g_vizSpecW, rows = kVizSpecRows;
        if (w <= 0) return true;
        float thick = barCount * (barW + barGap) - barGap;
        if (!bmp || bmpDc != g_dc.Get() || bmp->GetPixelSize().width != (UINT32)w) {
            bmp.Reset();
            bmpDc = g_dc.Get();
            D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            if (FAILED(g_dc->CreateBitmap(D2D1::SizeU(w, rows), nullptr, 0, bp, &bmp))) return true;
            bmpSerial = g_vizSpecSerial - 1;
        }
        if (bmpSerial != g_vizSpecSerial) {  // newest row first
            bmpSerial = g_vizSpecSerial;
            px.resize((size_t)w * rows);
            for (int a = 0; a < rows; a++) {
                const uint8_t* src = &g_vizSpecRing[(size_t)((g_vizSpecHead - a + rows) % rows) * w];
                for (int i = 0; i < w; i++) {
                    RGBA c = g_settings.colorMode == VizColorMode::Solid ? SpecColorCpu(src[i] / 255.f)
                                                                         : LerpColor(cGrad1, c2, src[i] / 255.f);
                    if (g_settings.colorMode != VizColorMode::Solid) c.a = (BYTE)std::min(255, src[i] * 5 / 2);
                    float al = c.a / 255.f * c1.a / 255.f;
                    px[(size_t)a * w + i] = ((uint32_t)lroundf(al * 255.f) << 24) | ((uint32_t)lroundf(c.r * al) << 16) |
                                            ((uint32_t)lroundf(c.g * al) << 8) | (uint32_t)lroundf(c.b * al);
                }
            }
            bmp->CopyFromMemory(nullptr, px.data(), w * 4);
        }
        float vis = std::min((float)rows, maxSize);
        D2D1_RECT_F dst = horizontal ? D2D1::RectF(blockX, blockY, blockX + thick, blockY + maxSize)
                                     : D2D1::RectF(blockX, blockY, blockX + maxSize, blockY + thick);
        if (horizontal) {
            g_dc->DrawBitmap(bmp.Get(), dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1::RectF(0, 0, (float)w, vis));
        } else {  // time runs left to right: rotate the image a quarter turn
            D2D1_MATRIX_3X2_F old;
            g_dc->GetTransform(&old);
            D2D1_POINT_2F o = D2D1::Point2F(blockX, blockY);
            g_dc->SetTransform(D2D1::Matrix3x2F(0, 1, 1, 0, 0, 0) * D2D1::Matrix3x2F::Translation(o.x, o.y) * old);
            g_dc->DrawBitmap(bmp.Get(), D2D1::RectF(0, 0, thick, maxSize), 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                             D2D1::RectF(0, 0, (float)w, vis));
            g_dc->SetTransform(old);
        }
        // Legend, hot end at the top (right when vertical), quarter ticks.
        float gap = 3.f * g_dpiScale, lw = 6.f * g_dpiScale;
        for (int k = 0; k < 32; k++) {
            float v = (k + 0.5f) / 32.f;
            RGBA c = g_settings.colorMode == VizColorMode::Solid ? SpecColorCpu(v) : LerpColor(cGrad1, c2, v);
            c.a = std::max<BYTE>(c.a, 89);  // the legend stays readable at the quiet end
            SetBrush(c);
            float a0 = k / 32.f, a1 = (k + 1) / 32.f;
            D2D1_RECT_F r = horizontal ? D2D1::RectF(dst.right + gap, dst.bottom - a1 * maxSize, dst.right + gap + lw, dst.bottom - a0 * maxSize)
                                       : D2D1::RectF(dst.left + a0 * maxSize, dst.bottom + gap, dst.left + a1 * maxSize, dst.bottom + gap + lw);
            g_dc->FillRectangle(r, g_barBrush.Get());
        }
        VizDrawStyleScale(blockX, blockY, maxSize, thick, horizontal);
        return true;
    }

    if (st == VizStyle::Vu) {
        float mh = maxSize, mw = maxSize * 1.5f, gap = 8.f * g_dpiScale;
        const float marks[11] = {-20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3};
        auto posOf = [](float db) { return (powf(10.f, db / 20.f) - 0.1f) / (1.41254f - 0.1f); };
        ID2D1SolidColorBrush* b = g_barBrush.Get();
        for (int m = 0; m < 2; m++) {
            float ox = blockX + (horizontal ? m * (mw + gap) : 0.f), oy = blockY + (horizontal ? 0.f : m * (mh + gap));
            float pvx = ox + mw * 0.5f, pvy = oy + mh * 0.9f, R = mh * 0.68f, lw = std::max(1.2f, mh * 0.016f);
            auto at = [&](float p, float r) {
                float an = (-48.f + 96.f * p) * VIZ_PI / 180.f;
                return D2D1::Point2F(pvx + sinf(an) * r, pvy - cosf(an) * r);
            };
            b->SetColor(D2D1::ColorF(0.05f, 0.05f, 0.06f, 0.6f));
            g_dc->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(ox, oy, ox + mw, oy + mh), mh * 0.08f, mh * 0.08f), b);
            for (int k = 0; k < 11; k++) {
                float p = posOf(marks[k]);
                b->SetColor(marks[k] > 0 ? D2D1::ColorF(1.f, 0.27f, 0.23f, 0.95f) : D2D1::ColorF(0.92f, 0.92f, 0.9f, 0.85f));
                g_dc->DrawLine(at(p, (k == 0 || k == 7) ? R * 0.84f : R * 0.9f), at(p, R), b, lw);
            }
            for (int k = 0; k < 16; k++) {
                bool hot = (k + 0.5f) / 16.f > posOf(0.f);
                b->SetColor(hot ? D2D1::ColorF(1.f, 0.27f, 0.23f, 0.95f) : D2D1::ColorF(0.92f, 0.92f, 0.9f, 0.85f));
                g_dc->DrawLine(at(k / 16.f, R * 0.9f), at((k + 1) / 16.f, R * 0.9f), b, hot ? lw * 1.8f : lw, g_roundCapStrokeStyle.Get());
            }
            SetBrush(RGBA{255, c1.r, c1.g, c1.b});
            g_dc->DrawLine(at(g_vizVu[m], R * 0.12f), at(g_vizVu[m], R * 1.02f), b, std::max(1.6f, mh * 0.024f),
                           g_roundCapStrokeStyle.Get());
            b->SetColor(D2D1::ColorF(0.25f, 0.25f, 0.27f, 1.f));
            g_dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(pvx, pvy), mh * 0.05f, mh * 0.05f), b);
            b->SetColor(D2D1::ColorF(1.f, 0.18f, 0.12f, 0.18f + 0.82f * std::clamp(g_vizVu[2 + m], 0.f, 1.f)));
            g_dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(ox + mw - mh * 0.12f, oy + mh * 0.12f), mh * 0.045f, mh * 0.045f), b);
        }
        VizDrawStyleScale(blockX, blockY, maxSize, 0.f, horizontal);
        return true;
    }
    return false;
}

// ---- Reflection on the Direct2D renderer ------------------------------------------------
// The bars go into an offscreen bitmap the size of the target, which is then
// drawn twice: as is, and mirrored about the base line through a layer whose
// opacity fades from 40 % to nothing over the reflection depth, as on
// Direct3D 11. Skipped for the frames where the whole scene fades in or out,
// since the target can't change under a pushed layer.
namespace {
ComPtr<ID2D1Bitmap1> s_reflBmp;
ComPtr<ID2D1Image> s_reflOld;
ComPtr<ID2D1LinearGradientBrush> s_reflFade;
ID2D1DeviceContext* s_reflDc = nullptr;
}  // namespace

bool VizReflD2DBegin(bool fadeLayer) {
    if (fadeLayer || !g_dc || !VizReflectionActive()) return false;
    ComPtr<ID2D1Image> old;
    g_dc->GetTarget(&old);
    ComPtr<ID2D1Bitmap1> tb;
    if (!old || FAILED(old.As(&tb))) return false;
    D2D1_SIZE_U sz = tb->GetPixelSize();
    if (!s_reflBmp || s_reflDc != g_dc.Get() || s_reflBmp->GetPixelSize().width != sz.width ||
        s_reflBmp->GetPixelSize().height != sz.height) {
        s_reflBmp.Reset();
        s_reflFade.Reset();
        float dx = 96.f, dy = 96.f;
        tb->GetDpi(&dx, &dy);
        D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dx, dy);
        if (FAILED(g_dc->CreateBitmap(sz, nullptr, 0, bp, &s_reflBmp))) return false;
        D2D1_GRADIENT_STOP stops[2] = {{0.f, D2D1::ColorF(0, 0, 0, 0.4f)}, {1.f, D2D1::ColorF(0, 0, 0, 0.f)}};
        ComPtr<ID2D1GradientStopCollection> sc;
        if (FAILED(g_dc->CreateGradientStopCollection(stops, 2, &sc)) ||
            FAILED(g_dc->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, 1)),
                                                   sc.Get(), &s_reflFade))) {
            s_reflBmp.Reset();
            return false;
        }
        s_reflDc = g_dc.Get();
    }
    s_reflOld = old;
    g_dc->SetTarget(s_reflBmp.Get());
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    return true;
}

// `baseY` is the bars' base line in the drawing's own coordinates.
void VizReflD2DEnd(float baseY, float depth) {
    g_dc->SetTarget(s_reflOld.Get());
    s_reflOld.Reset();
    D2D1_MATRIX_3X2_F old;
    g_dc->GetTransform(&old);
    g_dc->SetTransform(D2D1::IdentityMatrix());  // the bitmap is already in target space
    g_dc->DrawImage(s_reflBmp.Get());
    float base = old._22 * baseY + old._32, d = depth * fabsf(old._22);
    if (d >= 1.f) {
        D2D1_SIZE_F ts = g_dc->GetSize();
        s_reflFade->SetStartPoint(D2D1::Point2F(0, base));
        s_reflFade->SetEndPoint(D2D1::Point2F(0, base + d));
        g_dc->PushLayer(D2D1::LayerParameters1(D2D1::RectF(0, base, ts.width, base + d), nullptr,
                                               D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), 1.f,
                                               s_reflFade.Get()),
                        nullptr);
        g_dc->SetTransform(D2D1::Matrix3x2F::Scale(1.f, -1.f, D2D1::Point2F(0, base)));
        g_dc->DrawImage(s_reflBmp.Get());
        g_dc->SetTransform(D2D1::IdentityMatrix());
        g_dc->PopLayer();
    }
    g_dc->SetTransform(old);
}
