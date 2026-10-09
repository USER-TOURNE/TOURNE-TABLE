// ---- Terminal shape -----------------------------------------------------------------
//
// The visualizer drawn as text: a grid of character cells in a monospace
// font, each cell one glyph in one of five colours. Three styles:
//
//   Columns    every bar a column of glyphs (Terminal Column Glyph), turning
//              the hot colour above Hot Threshold, with the peak cap as a
//              glyph of its own. The cava / btop look.
//   Waterfall  a spectrogram in characters: each row is one moment, each
//              column one band, the glyph picked from Terminal Glyph Ramp by
//              level, newest row on top, scrolling at Terminal Scroll Rate.
//   Meters     text meters, "Bass:    [||||||        48%]", for bass, mid,
//              treble and the loudest band.
//
// The grid is built here on the CPU from the same per-bar levels the other
// shapes draw (so every engine, layout and ballistics setting applies), then
// drawn by the Direct3D 11 renderer from a baked glyph atlas in one instanced
// call, or by Direct2D as text runs.

constexpr int VIZ_TERM_MAX_CELLS = 65536;

struct VizTermGrid {
    int cols = 0, rows = 0;
    std::vector<uint32_t> cells;  // char | colour << 8; colours: 0 dim, 1 low, 2 high, 3 label, 4 peak
};
VizTermGrid g_termGrid;
std::vector<float> g_termHistory;  // waterfall: rows x cols levels, row 0 newest
float g_termScrollAcc = 0.f;

// Cell size for the terminal font, in whole pixels so glyphs land 1:1.
ComPtr<IDWriteTextFormat> g_termFormat;
std::wstring g_termFormatFont;
float g_termFormatPx = -1.f;
int g_termCellW = 8, g_termCellH = 16;

bool VizTermEnsureFormat() {
    float px = (float)std::clamp(g_settings.termFontSize, 6, 96) * g_dpiScale;
    if (g_termFormat && g_termFormatFont == g_settings.termFont && g_termFormatPx == px) return true;
    g_termFormat.Reset();
    if (!g_dwriteFactory) return false;
    if (FAILED(g_dwriteFactory->CreateTextFormat(g_settings.termFont.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                                 DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, px, L"en-us",
                                                 &g_termFormat)))
        return false;
    g_termFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    g_termFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_termFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    g_termFormatFont = g_settings.termFont;
    g_termFormatPx = px;
    // Cell = the advance of "M" and the font's line height, rounded up. For a
    // proportional font every glyph still gets an M-wide cell, which reads as
    // monospaced; a real monospace font is the intended use.
    ComPtr<IDWriteTextLayout> lay;
    DWRITE_TEXT_METRICS m{};
    if (SUCCEEDED(g_dwriteFactory->CreateTextLayout(L"M", 1, g_termFormat.Get(), 1000.f, 1000.f, &lay)) && lay &&
        SUCCEEDED(lay->GetMetrics(&m))) {
        g_termCellW = std::max(1, (int)ceilf(m.widthIncludingTrailingWhitespace - 0.01f));
        g_termCellH = std::max(1, (int)ceilf(m.height - 0.01f));
    } else {
        g_termCellW = std::max(1, (int)ceilf(px * 0.6f));
        g_termCellH = std::max(1, (int)ceilf(px * 1.2f));
    }
    return true;
}

void VizTermGridSize(int* cols, int* rows) {
    if (g_settings.termStyle == VizTermStyle::Meters) {
        *cols = std::clamp(g_settings.termMeterColumns, 20, 200);
        *rows = 4;
    } else {
        *cols = VizEffectiveBarCount();
        *rows = std::clamp(g_settings.termRows, 2, 128);
    }
    while (*cols * *rows > VIZ_TERM_MAX_CELLS && *rows > 2) (*rows)--;
}

// The grid's size in pixels: what the layout reserves for this shape instead
// of a bar group. Bar Width, Gap and Max Size don't apply; the font does.
void VizTermBox(float* w, float* h) {
    int cols = 0, rows = 0;
    VizTermGridSize(&cols, &rows);
    VizTermEnsureFormat();
    *w = (float)(cols * g_termCellW);
    *h = (float)(rows * g_termCellH);
}

static inline uint32_t TermCell(wchar_t c, int color) {
    uint32_t ch = (c >= 32 && c < 127) ? (uint32_t)c : (uint32_t)'?';
    return ch | ((uint32_t)color << 8);
}

// Levels for the Meters style: bass / mid / treble as the average drawn level
// of each EQ zone, and "Volume" as the loudest band.
void VizTermMeterLevels(float out[4]) {
    if (g_settings.engine == VizEngineKind::Precision) {
        const VizBandFrame& f = g_drawBands;
        float mx = 0.f;
        for (int b = 0; b < f.count; b++) mx = std::max(mx, f.level[b]);
        for (int z = 0; z < 3; z++) out[z] = (f.zoneCount[z] > 0.f) ? f.zone[z] / f.zoneCount[z] : 0.f;
        out[3] = mx;
    } else {
        float b[VIZ_NUM_BANDS];
        ReadBands(b);
        float n[3] = {0, 0, 0};
        out[0] = out[1] = out[2] = out[3] = 0.f;
        for (int i = 0; i < VIZ_NUM_BANDS; i++) {
            out[VIZ_BAND_EQ_ZONE[i]] += b[i];
            n[VIZ_BAND_EQ_ZONE[i]] += 1.f;
            out[3] = std::max(out[3], b[i]);
        }
        for (int z = 0; z < 3; z++) out[z] = n[z] > 0.f ? out[z] / n[z] : 0.f;
    }
    for (int i = 0; i < 4; i++) out[i] = std::clamp(out[i], 0.f, 1.f);
}

// ---- waterfall scroll: begin (src/tests_features/test_waterfall.cpp compiles this block from v2b.cpp)
// Scrolls the waterfall history (rows x cols, row 0 newest) down by `steps`
// rows and writes the current levels (`newest`, clamped at 0) into row 0. The
// rows a multi-row step opens up between the new row 0 and the previous newest
// row are moments the frame skipped over: they are filled by interpolating
// between the two, so a long frame or a fast Scroll Rate leaves neither stale
// lines nor blank stripes. A step of the whole height or more leaves no older
// row to keep, so every row starts again from the current levels.
void VizTermScrollHistory(float* hist, int rows, int cols, int steps, const float* newest) {
    if (!hist || rows <= 0 || cols <= 0) return;
    const size_t rowLen = (size_t)cols;
    if (steps >= rows) {
        for (int c = 0; c < cols; c++) hist[c] = std::max(0.f, newest[c]);
        for (int r = 1; r < rows; r++) memcpy(hist + (size_t)r * rowLen, hist, sizeof(float) * rowLen);
        return;
    }
    if (steps > 0) {
        memmove(hist + (size_t)steps * rowLen, hist, sizeof(float) * (size_t)(rows - steps) * rowLen);
        const float* prev = hist + (size_t)steps * rowLen;  // the previous newest row
        for (int r = 1; r < steps; r++) {
            const float t = (float)r / (float)steps;
            float* row = hist + (size_t)r * rowLen;
            for (int c = 0; c < cols; c++) {
                float now = std::max(0.f, newest[c]);
                row[c] = now + (prev[c] - now) * t;
            }
        }
    }
    for (int c = 0; c < cols; c++) hist[c] = std::max(0.f, newest[c]);
}
// ---- waterfall scroll: end

void VizBuildTermGrid() {
    VizTermGrid& g = g_termGrid;
    VizTermGridSize(&g.cols, &g.rows);
    g.cells.assign((size_t)g.cols * g.rows, TermCell(L' ', 0));
    const float hot = std::clamp(g_settings.termHotThreshold, 1, 100) / 100.0f;
    auto at = [&](int c, int r) -> uint32_t& { return g.cells[(size_t)r * g.cols + c]; };

    if (g_settings.termStyle == VizTermStyle::Meters) {
        static const wchar_t* kLabels[4] = {L"Bass:", L"Mid:", L"Treble:", L"Volume:"};
        float lv[4];
        VizTermMeterLevels(lv);
        const int labelW = 9, barN = std::max(1, g.cols - labelW - 6);
        for (int r = 0; r < 4; r++) {
            int c = 0;
            for (const wchar_t* p = kLabels[r]; *p && c < labelW; p++) at(c++, r) = TermCell(*p, 3);
            c = labelW;
            at(c++, r) = TermCell(L'[', 1);
            int filled = (int)lroundf(lv[r] * barN);
            for (int j = 0; j < barN; j++)
                at(c++, r) = (j < filled) ? TermCell(g_settings.termColumnGlyph, ((j + 1) > hot * barN) ? 2 : 1)
                                          : TermCell(L' ', 0);
            wchar_t pct[8];
            swprintf_s(pct, L"%3d%%", (int)lroundf(lv[r] * 100.f));
            for (const wchar_t* p = pct; *p && c < g.cols - 1; p++) at(c++, r) = TermCell(*p, 1);
            if (c < g.cols) at(c, r) = TermCell(L']', 1);
        }
        return;
    }

    if (g_settings.termStyle == VizTermStyle::Waterfall) {
        const std::wstring& ramp = g_settings.termRamp;
        const int nr = (int)ramp.size();
        size_t need = (size_t)g.cols * g.rows;
        if (g_termHistory.size() != need) {
            g_termHistory.assign(need, 0.f);
            g_termScrollAcc = 0.f;
        }
        // Scroll by whole rows at the configured rate; the newest row always
        // shows the current levels, so the top line stays live between steps.
        g_termScrollAcc += g_frameDt * (float)std::clamp(g_settings.termScrollRate, 1, 120);
        int steps = std::min((int)g_termScrollAcc, g.rows);
        g_termScrollAcc -= (float)(int)g_termScrollAcc;
        VizTermScrollHistory(g_termHistory.data(), g.rows, g.cols, steps, g_vizPeak);
        for (int r = 0; r < g.rows; r++) {
            for (int c = 0; c < g.cols; c++) {
                float v = g_termHistory[(size_t)r * g.cols + c];
                if (nr <= 0 || v < 0.02f) continue;
                int idx = std::clamp((int)(v * (nr - 1) + 0.5f), 0, nr - 1);
                wchar_t ch = ramp[idx];
                if (ch == L' ') continue;
                at(c, r) = TermCell(ch, v > hot ? 2 : (idx == 0 ? 0 : 1));
            }
        }
        return;
    }

    // Columns.
    for (int c = 0; c < g.cols; c++) {
        float v = std::max(0.f, g_vizPeak[c]) * g.rows;
        int full = std::min(g.rows, (int)v);
        float frac = v - (float)full;
        for (int k = 0; k < full; k++) {
            int r = g.rows - 1 - k;
            at(c, r) = TermCell(g_settings.termColumnGlyph, ((float)(k + 1) / g.rows > hot) ? 2 : 1);
        }
        if (full < g.rows && frac >= 0.5f) {
            int r = g.rows - 1 - full;
            at(c, r) = TermCell(L'.', ((float)(full + 1) / g.rows > hot) ? 2 : 1);
        }
        if (g_settings.peakHoldEnabled) {
            int pr = (int)lroundf(g_vizPeakHold[c] * g.rows);
            if (pr > full && pr >= 1 && pr <= g.rows) at(c, g.rows - pr) = TermCell(g_settings.termPeakGlyph, 4);
        }
    }
}

// Terminal palette as straight-alpha floats, in cell colour order.
void VizTermPalette(float out[5][4]) {
    const BYTE* c[5][4] = {
        {&g_settings.termDimA, &g_settings.termDimR, &g_settings.termDimG, &g_settings.termDimB},
        {&g_settings.termLowA, &g_settings.termLowR, &g_settings.termLowG, &g_settings.termLowB},
        {&g_settings.termHighA, &g_settings.termHighR, &g_settings.termHighG, &g_settings.termHighB},
        {&g_settings.termLabelA, &g_settings.termLabelR, &g_settings.termLabelG, &g_settings.termLabelB},
        {&g_settings.peakHoldA, &g_settings.peakHoldR, &g_settings.peakHoldG, &g_settings.peakHoldB}};
    for (int i = 0; i < 5; i++) {
        out[i][0] = *c[i][1] / 255.f;
        out[i][1] = *c[i][2] / 255.f;
        out[i][2] = *c[i][3] / 255.f;
        out[i][3] = *c[i][0] / 255.f;
    }
}

// Direct2D fallback: each row as runs of same-coloured text.
void VizDrawTermGridD2D(float originX, float originY) {
    if (!VizTermEnsureFormat() || !g_barBrush) return;
    const VizTermGrid& g = g_termGrid;
    float pal[5][4];
    VizTermPalette(pal);
    D2D1_TEXT_ANTIALIAS_MODE prev = g_dc->GetTextAntialiasMode();
    g_dc->SetTextAntialiasMode(g_settings.textPixel ? D2D1_TEXT_ANTIALIAS_MODE_ALIASED
                                                    : D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    std::wstring run;
    for (int r = 0; r < g.rows; r++) {
        int c = 0;
        while (c < g.cols) {
            uint32_t cell = g.cells[(size_t)r * g.cols + c];
            if ((cell & 127u) <= 32u) {
                c++;
                continue;
            }
            int color = (int)((cell >> 8) & 7u);
            int start = c;
            run.clear();
            while (c < g.cols) {
                uint32_t k = g.cells[(size_t)r * g.cols + c];
                bool blank = (k & 127u) <= 32u;
                if (!blank && (int)((k >> 8) & 7u) != color) break;
                run.push_back(blank ? L' ' : (wchar_t)(k & 127u));
                c++;
            }
            while (!run.empty() && run.back() == L' ') run.pop_back();
            int ci = std::min(color, 4);
            g_barBrush->SetColor(D2D1::ColorF(pal[ci][0], pal[ci][1], pal[ci][2], pal[ci][3]));
            float x = originX + start * g_termCellW, y = originY + r * g_termCellH;
            g_dc->DrawText(run.c_str(), (UINT32)run.size(), g_termFormat.Get(),
                           D2D1::RectF(x, y, x + (float)(run.size() + 1) * g_termCellW, y + g_termCellH),
                           g_barBrush.Get());
        }
    }
    g_dc->SetTextAntialiasMode(prev);
}
