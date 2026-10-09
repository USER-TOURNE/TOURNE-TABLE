// ---- Shared by both renderers ---------------------------------------------------------

// Publishes the bounds of what is actually visible, for the occlusion check
// (see the note where 1.4 introduced this).
void VizPublishDrawRect(const VizLayout& layout) {
    int virtualScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    float padL = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingL * g_dpiScale : 0.f;
    float padR = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingR * g_dpiScale : 0.f;
    float padT = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingT * g_dpiScale : 0.f;
    float padB = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingB * g_dpiScale : 0.f;
    float visL = layout.originX + layout.blockX - padL;
    float visT = layout.originY + layout.blockY - padT;
    float visR = layout.originX + layout.blockX + layout.totalWidth + padR;
    float visB = layout.originY + layout.blockY + layout.totalHeight + padB;
    LONG l = (LONG)visL + virtualScreenX, t = (LONG)visT + virtualScreenY;
    LONG r = (LONG)visR + virtualScreenX, b = (LONG)visB + virtualScreenY;
    bool moved = !g_drawRectValid.load(std::memory_order_relaxed) ||
                 l != g_drawRectL.load(std::memory_order_relaxed) ||
                 t != g_drawRectT.load(std::memory_order_relaxed) ||
                 r != g_drawRectR.load(std::memory_order_relaxed) ||
                 b != g_drawRectB.load(std::memory_order_relaxed);
    g_drawRectL.store(l, std::memory_order_relaxed);
    g_drawRectT.store(t, std::memory_order_relaxed);
    g_drawRectR.store(r, std::memory_order_relaxed);
    g_drawRectB.store(b, std::memory_order_relaxed);
    g_drawRectValid.store(true, std::memory_order_relaxed);
    // Media controls anchored to the panel follow it (a drag, a settings
    // change, a nudge). Only on an actual move, so a still panel costs nothing.
    if (moved && g_mediaWnd && g_settings.mediaControlsEnabled && g_settings.mediaAnchor != VizMediaAnchor::Screen)
        PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}

// The background panel's rectangle (layout-local) and corner radii, with the
// same guard as the Direct2D path against negative padding inverting it.
void VizPanelRect(const VizLayout& layout, D2D1_RECT_F* out, float radii[4]) {
    float padL = (float)g_settings.bgPaddingL * g_dpiScale;
    float padR = (float)g_settings.bgPaddingR * g_dpiScale;
    float padT = (float)g_settings.bgPaddingT * g_dpiScale;
    float padB = (float)g_settings.bgPaddingB * g_dpiScale;
    D2D1_RECT_F r = D2D1::RectF(layout.blockX - padL, layout.blockY - padT, layout.blockX + layout.totalWidth + padR,
                                layout.blockY + layout.totalHeight + padB);
    if (r.right - r.left < 1.0f) {
        float mid = (r.left + r.right) * 0.5f;
        r.left = mid - 0.5f;
        r.right = mid + 0.5f;
    }
    if (r.bottom - r.top < 1.0f) {
        float mid = (r.top + r.bottom) * 0.5f;
        r.top = mid - 0.5f;
        r.bottom = mid + 0.5f;
    }
    *out = r;
    radii[0] = g_settings.bgRadiusTL * g_dpiScale;
    radii[1] = g_settings.bgRadiusTR * g_dpiScale;
    radii[2] = g_settings.bgRadiusBR * g_dpiScale;
    radii[3] = g_settings.bgRadiusBL * g_dpiScale;
}

// The oscilloscope trace after Oscilloscope Damping (1.5 semantics, moved out
// of the Direct2D path so both renderers ease it the same way).
float g_scopeDisp[VIZ_WAVE_SAMPLES] = {};
void VizUpdateScopeTrace() {
    float waveSnap[VIZ_WAVE_SAMPLES];
    ReadWaveform(waveSnap);
    float scopeEase = (g_settings.oscilloscopeDamping > 0) ? 1.0f - (g_settings.oscilloscopeDamping / 100.0f) * 0.95f
                                                           : 1.0f;
    if (scopeEase < 1.0f) scopeEase = VizEaseForFrame(scopeEase);
    for (int w = 0; w < VIZ_WAVE_SAMPLES; w++) g_scopeDisp[w] += (waveSnap[w] - g_scopeDisp[w]) * scopeEase;
}

// Energy per EQ zone, for the multiband oscilloscope colour.
void VizZoneEnergies(float out[3]) {
    out[0] = out[1] = out[2] = 0.f;
    if (g_settings.engine == VizEngineKind::Precision) {
        for (int z = 0; z < 3; z++) out[z] = g_drawBands.zone[z];
        return;
    }
    float bandsSnap[VIZ_NUM_BANDS];
    ReadBands(bandsSnap);
    for (int b = 0; b < VIZ_NUM_BANDS; b++) out[VIZ_BAND_EQ_ZONE[b]] += bandsSnap[b];
}

// Colour Mode inputs, resolved the way the Direct2D path does it.
void VizResolveColors(RGBA* c1, RGBA* cGrad1, RGBA* c2) {
    *c1 = {g_settings.colorA, g_settings.colorR, g_settings.colorG, g_settings.colorB};
    if (g_settings.colorMode == VizColorMode::Accent) {
        DWORD dw = GetWindowsAccentColor();
        *c1 = {0xFF, (BYTE)((dw >> 16) & 0xFF), (BYTE)((dw >> 8) & 0xFF), (BYTE)(dw & 0xFF)};
    } else if (g_settings.colorMode == VizColorMode::AlbumArt || g_settings.colorMode == VizColorMode::DynamicAlbum) {
        DWORD dw = g_albumArtColor.load(std::memory_order_relaxed);
        *c1 = {0xFF, (BYTE)((dw >> 16) & 0xFF), (BYTE)((dw >> 8) & 0xFF), (BYTE)(dw & 0xFF)};
    }
    *c2 = {g_settings.grad2A, g_settings.grad2R, g_settings.grad2G, g_settings.grad2B};
    *cGrad1 = {g_settings.grad1A, g_settings.grad1R, g_settings.grad1G, g_settings.grad1B};
    if (g_settings.colorMode == VizColorMode::Tourne) {
        *cGrad1 = {255, 20, 184, 166};
        *c2 = {255, 200, 29, 51};
    }
    if (g_settings.colorMode == VizColorMode::DynamicAlbum) {
        DWORD dw = g_albumArtColorSecondary.load(std::memory_order_relaxed);
        *c2 = {0xFF, (BYTE)((dw >> 16) & 0xFF), (BYTE)((dw >> 8) & 0xFF), (BYTE)(dw & 0xFF)};
        *cGrad1 = *c1;
    }
}

// ---- Text overlays --------------------------------------------------------------------
//
// The Now Playing label and the readout, worked out once per frame as plain
// values (VizBuildTextFrame) and drawn from them (VizDrawTextOverlays). The
// split is what lets the Direct3D 11 renderer redraw the text surface only
// when one of those values changes.
struct VizTextFrame {
    std::wstring np;            // one line: "Artist - Title"
    std::wstring npTitle, npArtist;
    float npAlpha = 0.f;
    float progress = -1.f;      // track position 0..1, or -1 for no bar
    std::wstring pf;
    bool pfWide = false;  // a loudness readout: wider box, never wrapped
};

static void AppendLufs(std::wstring& s, const wchar_t* label, double v) {
    wchar_t b[32];
    if (std::isfinite(v) && v > -70.0)
        swprintf_s(b, L"%s %.1f", label, v);
    else
        swprintf_s(b, L"%s --", label);
    if (!s.empty()) s += L"  ";
    s += b;
}

void VizBuildTextFrame(VizTextFrame& t) {
    t = VizTextFrame();
    if (g_settings.nowPlayingEnabled && g_dwriteTextFormat && g_nowPlayingBrush) {
        ULONGLONG changedAt = g_nowPlayingChangedTick.load(std::memory_order_relaxed);
        ULONGLONG npElapsed = GetTickCount64() - changedAt;
        ULONGLONG showMs = (ULONGLONG)std::max(0, g_settings.nowPlayingDisplaySeconds) * 1000ULL;
        constexpr ULONGLONG kNpFadeMs = 800;
        if (changedAt != 0 && showMs == 0)
            t.npAlpha = 1.0f;  // Display Seconds 0: always shown, a widget rather than a toast
        else if (changedAt != 0 && npElapsed < showMs + kNpFadeMs)
            t.npAlpha = (npElapsed < showMs) ? 1.0f : 1.0f - (float)(npElapsed - showMs) / (float)kNpFadeMs;
        if (t.npAlpha > 0.01f) {
            std::lock_guard<std::mutex> lock(g_nowPlayingMutex);
            t.np = g_nowPlayingDisplay;
            t.npTitle = g_nowPlayingTitle;
            t.npArtist = g_nowPlayingArtist;
        }
    }
    if (g_settings.progressEnabled && g_progressBrush) t.progress = VizTrackProgress();
    // The readout. The dominant frequency and the correlation move on almost
    // every frame, and every new string costs a text-surface redraw, a
    // DirectWrite layout and a Present; nobody reads a number at 144 Hz. So
    // the string is rebuilt at most kReadoutMs apart (10 per second; momentary
    // loudness only moves every 100 ms anyway) and held in between, and the
    // frequency has hysteresis: the value shown changes only once the
    // measured one has moved more than a quarter of a semitone (1/48 octave,
    // about 1.45 %) from it, which keeps a held note from flickering between
    // neighbouring values while any real change of pitch still shows within
    // the next refresh. Changing the Readout setting rebuilds at once.
    static std::wstring s_pf;
    static int s_pfMode = -1;
    static ULONGLONG s_pfTick = 0;
    static float s_pfHz = 0.f;
    if (!(g_settings.peakFreqEnabled && g_dwriteTextFormat && g_nowPlayingBrush)) {
        s_pfMode = -1;  // shown again later: built fresh, not a stale string
        return;
    }
    constexpr ULONGLONG kReadoutMs = 100;
    constexpr float kHzHysteresisOct = 1.0f / 48.0f;
    const VizReadout r = g_settings.readout;
    const ULONGLONG now = GetTickCount64();
    if ((int)r != s_pfMode || now - s_pfTick >= kReadoutMs) {
        s_pfMode = (int)r;
        s_pfTick = now;
        std::wstring freq;
        float hz = g_dominantFreqHz.load(std::memory_order_relaxed);
        if (!(hz > 0.f))
            s_pfHz = 0.f;
        else if (!(s_pfHz > 0.f) || fabsf(log2f(hz / s_pfHz)) > kHzHysteresisOct)
            s_pfHz = hz;
        if (s_pfHz > 0.f) {
            wchar_t b[32];
            if (s_pfHz >= 1000.f) swprintf_s(b, L"%.1f kHz", s_pfHz / 1000.f);
            else swprintf_s(b, L"%.0f Hz", s_pfHz);
            freq = b;
        }
        if (r == VizReadout::Frequency) {
            s_pf = freq;
        } else {
            VizMeterValues m;
            {
                std::lock_guard<std::mutex> lock(g_meterMutex);
                m = g_meters;
            }
            std::wstring s;
            if (r == VizReadout::Both && !freq.empty()) s = freq;
            AppendLufs(s, L"M", m.momentary);
            AppendLufs(s, L"S", m.shortTerm);
            AppendLufs(s, L"I", m.integrated);
            s += L" LUFS";
            if (r == VizReadout::LoudnessFull) {
                wchar_t b[96];
                double plr = (std::isfinite(m.truePeak) && std::isfinite(m.integrated) && m.integrated > -70.0)
                                 ? m.truePeak - m.integrated
                                 : NAN;
                if (std::isfinite(m.truePeak) && m.truePeak > -100.0)
                    swprintf_s(b, L"  TP %.1f dBTP", m.truePeak);
                else
                    swprintf_s(b, L"  TP --");
                s += b;
                if (std::isfinite(plr)) swprintf_s(b, L"  PLR %.1f", plr);
                else swprintf_s(b, L"  PLR --");
                s += b;
                swprintf_s(b, L"  r %+.2f", m.correlation);
                s += b;
            }
            s_pf = s;
        }
    }
    t.pf = s_pf;
    t.pfWide = r != VizReadout::Frequency;
}

// Rough single-line width for the room a wide readout needs, used when sizing
// the layout before any text has been measured.
float VizReadoutWidthEstimate() {
    float fontPx = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale;
    int chars = 0;
    switch (g_settings.readout) {
        case VizReadout::Loudness: chars = 32; break;
        case VizReadout::LoudnessFull: chars = 70; break;
        case VizReadout::Both: chars = 42; break;
        default: return 0.f;
    }
    return fontPx * 0.58f * chars;
}

// Where the track progress bar goes, layout-local. False when it isn't shown.
bool VizProgressRect(const VizLayout& layout, D2D1_RECT_F* out) {
    float h = (float)std::max(1, g_settings.progressHeight) * g_dpiScale;
    float gap = (float)g_settings.progressGap * g_dpiScale;
    D2D1_RECT_F panel;
    float radii[4];
    VizPanelRect(layout, &panel, radii);
    bool hasPanel = g_settings.backgroundEnabled;
    float l = hasPanel ? panel.left : layout.blockX, r = hasPanel ? panel.right : layout.blockX + layout.totalWidth;
    float top = hasPanel ? panel.top : layout.blockY, bottom = hasPanel ? panel.bottom : layout.blockY + layout.totalHeight;
    float y;
    switch (g_settings.progressPlacement) {
        case VizProgressPlacement::Above: y = top - gap - h; break;
        case VizProgressPlacement::PanelBottom:
            // Inside the panel, under the bars, as wide as the bars.
            l = layout.blockX;
            r = layout.blockX + layout.totalWidth;
            y = layout.blockY + layout.totalHeight + gap;
            break;
        default: y = bottom + gap; break;
    }
    // Whole pixels: a 2 px bar on a half pixel would read as a 3 px smear.
    *out = D2D1::RectF(roundf(l), roundf(y), roundf(r), roundf(y) + roundf(h));
    return out->right > out->left;
}

void VizDrawTextOverlays(const VizTextFrame& t, const VizLayout& layout, bool smooth) {
    const float blockX = layout.blockX, blockY = layout.blockY;
    const float totalWidth = layout.totalWidth, totalHeight = layout.totalHeight;
    const bool pixel = g_settings.textPixel;
    // Pixel-sharp text: no antialiasing, and every box on a whole pixel, so a
    // pixel font at its design size lands exactly on the grid.
    g_dc->SetTextAntialiasMode(pixel ? D2D1_TEXT_ANTIALIAS_MODE_ALIASED : D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
    auto snap = [&](D2D1_RECT_F r) {
        if (!pixel && !g_settings.pixelSnap) return r;
        float dx = roundf(r.left) - r.left, dy = roundf(r.top) - r.top;
        return D2D1::RectF(r.left + dx, r.top + dy, r.right + dx, r.bottom + dy);
    };

    if (t.progress >= 0.f && g_progressBrush) {
        D2D1_RECT_F pr;
        if (VizProgressRect(layout, &pr)) {
            g_progressBrush->SetColor(D2D1::ColorF(g_settings.progressTrackR / 255.f, g_settings.progressTrackG / 255.f,
                                                   g_settings.progressTrackB / 255.f, g_settings.progressTrackA / 255.f));
            g_dc->FillRectangle(pr, g_progressBrush.Get());
            float fillR = pr.left + roundf((pr.right - pr.left) * std::clamp(t.progress, 0.f, 1.f));
            if (fillR > pr.left) {
                g_progressBrush->SetColor(D2D1::ColorF(g_settings.progressR / 255.f, g_settings.progressG / 255.f,
                                                       g_settings.progressB / 255.f, g_settings.progressA / 255.f));
                g_dc->FillRectangle(D2D1::RectF(pr.left, pr.top, fillR, pr.bottom), g_progressBrush.Get());
            }
        }
    }

    if (!t.np.empty() && t.npAlpha > 0.01f) {
        g_nowPlayingBrush->SetColor(D2D1::ColorF(g_settings.nowPlayingR / 255.0f, g_settings.nowPlayingG / 255.0f,
                                                 g_settings.nowPlayingB / 255.0f,
                                                 (g_settings.nowPlayingA / 255.0f) * t.npAlpha));
        if (g_npArtistBrush)
            g_npArtistBrush->SetColor(D2D1::ColorF(g_settings.npArtistR / 255.0f, g_settings.npArtistG / 255.0f,
                                                   g_settings.npArtistB / 255.0f,
                                                   (g_settings.npArtistA / 255.0f) * t.npAlpha));
        const float fontPx = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale;
        const bool two = g_settings.npLayout == VizNpLayout::TwoLines && !t.npTitle.empty() && !t.npArtist.empty();
        // Two lines: title, then artist. One line: "Artist - Title", the
        // artist part in the artist colour.
        std::wstring text = two ? t.npTitle + L"\n" + t.npArtist : t.np;
        UINT32 artistAt = 0, artistLen = 0;
        if (two) {
            artistAt = (UINT32)t.npTitle.size() + 1;
            artistLen = (UINT32)t.npArtist.size();
        } else if (!t.npArtist.empty() && !t.npTitle.empty() && t.np.rfind(t.npArtist, 0) == 0) {
            artistLen = (UINT32)t.npArtist.size();
        }
        float npMargin = 8.0f * g_dpiScale;
        float npHeight = fontPx * (two ? 2.7f : 1.6f);
        float npOffX = EffectiveNowPlayingOffsetX();
        float npOffY = EffectiveNowPlayingOffsetY();
        D2D1_RECT_F npRect;
        if (g_settings.npPlacement == VizNpPlacement::Above) {
            // With the progress bar above too, the text goes above the bar.
            // The bar hangs off the panel's top edge and the text off the
            // bars, so with less top padding than the text is tall they used
            // to overlap. Decided by the settings, not by whether a track
            // position is known this frame, so the text doesn't jump when
            // the bar comes and goes. ComputeVizLayout reserves room for
            // both, one above the other.
            float npBottom = blockY - npMargin;
            D2D1_RECT_F pr;
            if (g_settings.progressEnabled && g_settings.progressPlacement == VizProgressPlacement::Above &&
                VizProgressRect(layout, &pr))
                npBottom = pr.top - npMargin;
            npRect = D2D1::RectF(blockX - layout.textAnchorSide, npBottom - npHeight,
                                 blockX + totalWidth + layout.textAnchorSide, npBottom);
        } else {
            // Inside the panel: in the band of padding above (or below) the
            // bars, as wide as the bars, so Left / Right line up with them.
            float padT = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingT * g_dpiScale : 0.f;
            float padB = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingB * g_dpiScale : 0.f;
            float y = (g_settings.npPlacement == VizNpPlacement::PanelTop)
                          ? blockY - padT + std::max(0.f, (padT - npHeight) * 0.5f)
                          : blockY + totalHeight + std::max(0.f, (padB - npHeight) * 0.5f);
            npRect = D2D1::RectF(blockX, y, blockX + totalWidth, y + npHeight);
        }
        npRect = snap(D2D1::RectF(npRect.left + npOffX, npRect.top + npOffY, npRect.right + npOffX,
                                  npRect.bottom + npOffY));
        TextPanelStyle npPanel{g_settings.npBgA, g_settings.npBgR, g_settings.npBgG, g_settings.npBgB,
                               g_settings.npBgBorderA, g_settings.npBgBorderR, g_settings.npBgBorderG,
                               g_settings.npBgBorderB, g_settings.npBgPadding, g_settings.npBgCornerRadius,
                               g_settings.npBgBorderSize};
        // Build the layout here (alignment, the artist's colour) and hand it
        // to DrawOverlayText through its cache, which then measures the panel
        // from it and draws it as is.
        static uint64_t s_npKey = 0;
        uint64_t key = (uint64_t)g_settings.npAlign * 7u + (uint64_t)two * 3u + (uint64_t)artistLen * 131u +
                       (uint64_t)(uintptr_t)g_npArtistBrush.Get();
        float boxW = npRect.right - npRect.left, boxH = npRect.bottom - npRect.top;
        VizTextLayoutCache& c = g_npLayoutCache;
        bool same = c.layout && s_npKey == key && c.w == boxW && c.h == boxH && c.text == text;
        if (!same && g_dwriteFactory && g_dwriteTextFormat) {
            c.Reset();
            if (SUCCEEDED(g_dwriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), g_dwriteTextFormat.Get(),
                                                            boxW, boxH, &c.layout)) &&
                c.layout) {
                c.layout->SetTextAlignment(g_settings.npAlign == VizTextAlignH::Left    ? DWRITE_TEXT_ALIGNMENT_LEADING
                                           : g_settings.npAlign == VizTextAlignH::Right ? DWRITE_TEXT_ALIGNMENT_TRAILING
                                                                                        : DWRITE_TEXT_ALIGNMENT_CENTER);
                c.layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                if (artistLen && g_npArtistBrush) {
                    DWRITE_TEXT_RANGE r{artistAt, artistLen};
                    c.layout->SetDrawingEffect(g_npArtistBrush.Get(), r);
                    if (two) c.layout->SetFontWeight(DWRITE_FONT_WEIGHT_NORMAL, r);
                }
                c.text = text;
                c.w = boxW;
                c.h = boxH;
                s_npKey = key;
            } else {
                c.Reset();
            }
        }
        (void)smooth;
        DrawOverlayText(text.c_str(), (UINT32)text.size(), npRect, g_nowPlayingBrush.Get(), npPanel, t.npAlpha,
                        &g_npLayoutCache);
    }
    if (!t.pf.empty()) {
        g_nowPlayingBrush->SetColor(D2D1::ColorF(g_settings.nowPlayingR / 255.0f, g_settings.nowPlayingG / 255.0f,
                                                 g_settings.nowPlayingB / 255.0f, g_settings.nowPlayingA / 255.0f));
        float pfMargin = 4.0f * g_dpiScale;
        float pfHeight = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale * 1.4f;
        float pfWidth = t.pfWide ? totalWidth + 2.0f * layout.textAnchorSide
                                 : std::min(120.0f * g_dpiScale, totalWidth + 2.0f * layout.textAnchorSide);
        float pfOffX = EffectivePeakFreqOffsetX();
        float pfOffY = EffectivePeakFreqOffsetY();
        float pfX;
        switch (g_settings.peakFreqAlignH) {
            case VizTextAlignH::Left: pfX = blockX - layout.textAnchorSide; break;
            case VizTextAlignH::Center: pfX = blockX + (totalWidth - pfWidth) * 0.5f; break;
            default: pfX = blockX + totalWidth + layout.textAnchorSide - pfWidth; break;
        }
        pfX += pfOffX;
        float pfY;
        switch (g_settings.peakFreqAlignV) {
            case VizTextAlignV::Above: pfY = blockY - pfHeight - pfMargin; break;
            case VizTextAlignV::Middle: pfY = blockY + (totalHeight - pfHeight) * 0.5f; break;
            case VizTextAlignV::Bottom: pfY = blockY + totalHeight - pfHeight - pfMargin; break;
            case VizTextAlignV::Below: pfY = blockY + totalHeight + pfMargin; break;
            default: pfY = blockY + pfMargin; break;
        }
        pfY += pfOffY;
        D2D1_RECT_F pfRect = snap(D2D1::RectF(pfX, pfY, pfX + pfWidth, pfY + pfHeight));
        TextPanelStyle pfPanel{g_settings.pfBgA, g_settings.pfBgR, g_settings.pfBgG, g_settings.pfBgB,
                               g_settings.pfBgBorderA, g_settings.pfBgBorderR, g_settings.pfBgBorderG,
                               g_settings.pfBgBorderB, g_settings.pfBgPadding, g_settings.pfBgCornerRadius,
                               g_settings.pfBgBorderSize};
        DrawOverlayText(t.pf.c_str(), (UINT32)t.pf.length(), pfRect, g_nowPlayingBrush.Get(), pfPanel, 1.0f,
                        (smooth || t.pfWide) ? &g_pfLayoutCache : nullptr, t.pfWide);
    }
}

// ---- Direct3D 11 frame ------------------------------------------------------------------
// Returns false to fall back to the Direct2D path for this frame.
bool RenderVisualizerD3D(float sceneAlpha) {
    VizLayout layout;
    if (!ComputeVizLayout(&layout)) {
        // The Direct2D path draws this frame on the text surface.
        ttgfx::TextAttach();
        ttgfx::g.textForce = true;
        return false;
    }
    VizPublishDrawRect(layout);
    const bool dragPause = g_dragRenderPauseActive.load(std::memory_order_relaxed);

    ttgfx::FrameInputs in = {};
    in.layout = &layout;
    in.sceneAlpha = sceneAlpha;
    in.dragPause = dragPause;
    in.hasPanel = g_settings.backgroundEnabled && g_backgroundBrush;
    if (in.hasPanel) VizPanelRect(layout, &in.bgRect, in.bgRadii);

    if (sceneAlpha > 0.001f) {
        {
            float pulse = g_beatPulse.load(std::memory_order_relaxed);
            if (pulse > 0.f)
                g_beatPulse.store(std::max(0.f, pulse - 0.08f * g_frameScale), std::memory_order_relaxed);
        }
        if (!dragPause) {
            VizComputeBarFrame();
            if (g_settings.shape == VizShape::Oscilloscope) VizUpdateScopeTrace();
            if (g_settings.shape == VizShape::Terminal) VizBuildTermGrid();
        }
        VizResolveColors(&in.c1, &in.cGrad1, &in.c2);
        in.rainbowBase = VizClockPhase(VizClockSeconds(), (double)g_settings.rainbowSpeed, 360.0);
        memcpy(in.scopeDisp, g_scopeDisp, sizeof(in.scopeDisp));
        VizZoneEnergies(in.zones);
        {
            std::lock_guard<std::mutex> lock(g_meterMutex);
            in.correlation = (float)g_meters.correlation;
        }
    }
    if (!ttgfx::Render(in)) {
        ttgfx::TextAttach();
        ttgfx::g.textForce = true;
        return false;
    }

    // Text surface: redrawn only when what it shows changes.
    VizTextFrame tf;
    if (sceneAlpha > 0.001f && !dragPause) VizBuildTextFrame(tf);
    uint64_t key = 1469598103934665603ull;
    for (wchar_t c : tf.np) ttgfx::Mix(key, (uint64_t)c);
    ttgfx::MixF(key, tf.npAlpha, 255.f);
    for (wchar_t c : tf.pf) ttgfx::Mix(key, (uint64_t)c);
    for (wchar_t c : tf.npArtist) ttgfx::Mix(key, (uint64_t)c);
    // The progress bar redraws the text surface once per pixel it grows.
    bool progressShown = false;
    if (tf.progress >= 0.f) {
        D2D1_RECT_F pr;
        progressShown = g_progressBrush && VizProgressRect(layout, &pr);
        float wpx = progressShown ? pr.right - pr.left : 0.f;
        ttgfx::Mix(key, 1000003u + (uint64_t)lroundf(std::clamp(tf.progress, 0.f, 1.f) * wpx));
    }
    ttgfx::MixF(key, sceneAlpha, 255.f);
    ttgfx::MixF(key, layout.blockX, 64.f);
    ttgfx::MixF(key, layout.blockY, 64.f);
    ttgfx::Mix(key, (uint64_t)g_swapChainWidth * 65536u + g_swapChainHeight);
    if (!ttgfx::g.textForce && key == ttgfx::g.textKey) return true;
    ttgfx::g.textKey = key;
    ttgfx::g.textForce = false;
    // Nothing on it (what VizDrawTextOverlays would draw): one cleared frame,
    // then its content comes off the text visual until something shows again,
    // so an empty text surface costs DWM nothing (see ttgfx::TextDetach).
    const bool anyText = (!tf.np.empty() && tf.npAlpha > 0.01f) || !tf.pf.empty() || progressShown;
    if (!anyText) {
        if (!ttgfx::TextIsDetached()) {
            g_dc->BeginDraw();
            g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
            HRESULT hrEnd = g_dc->EndDraw();
            HRESULT hrPresent = g_swapChain->Present(0, 0);
            VizCheckDeviceLost(hrEnd, hrPresent);
            ttgfx::TextDetach();
        }
        return true;
    }
    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    bool fade = sceneAlpha < 0.999f;
    if (fade)
        g_dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                              D2D1::IdentityMatrix(), sceneAlpha),
                        nullptr);
    VizDrawTextOverlays(tf, layout, true);
    if (fade) g_dc->PopLayer();
    HRESULT hrEnd = g_dc->EndDraw();
    HRESULT hrPresent = g_swapChain->Present(0, 0);
    VizCheckDeviceLost(hrEnd, hrPresent);
    ttgfx::TextAttach();  // after the Present, so the visual gets its content back with the text on it
    ttgfx::g.textForce = false;  // TextAttach asks for a redraw; this frame was it
    return true;
}
