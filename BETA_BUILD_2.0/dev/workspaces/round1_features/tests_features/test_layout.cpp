// Vertical layout reservation (ComputeVizLayout) vs where the overlays are
// drawn (VizProgressRect, VizDrawTextOverlays in part_text.cpp). The reserve
// formulas are copied from the old and new splice2.py ComputeVizLayout
// replacement; the draw math is copied from part_text.cpp. A case passes when
// the drawn element's bottom edge is inside the surface height.
#include <algorithm>
#include <cmath>
#include <cstdio>

struct Case {
    const char* name;
    float dpi;
    bool bg;
    int padT, padB;      // Background Padding Top / Bottom
    float totalHeight;   // bars (logical px * dpi already)
    // progress bar
    bool prog; int progGap, progH;
    // now playing: 0 off, 1 PanelTop, 2 PanelBottom
    int np; bool twoLines; int fontSize; int npBgPadding;
};

static float reserveFor(float o) { return std::ceil(o / 32.0f) * 32.0f; }

struct Out { float surfaceH, barBottom, npBottom; };

static Out Eval(const Case& k, bool fixed) {
    const float dpi = k.dpi;
    float padT = k.bg ? std::round(k.padT * dpi) : 0.f;  // VizPx with Pixel Snap
    float padB = k.bg ? std::round(k.padB * dpi) : 0.f;
    float margin = 4.0f * dpi;  // max(4 dpi, barW) with thin bars
    float fontPx = (float)std::max(6, k.fontSize) * dpi;
    float textTop = 0, textBottom = 0;
    if (k.np) {
        float npPanel = (float)k.npBgPadding * dpi;
        float npOffY = reserveFor(0.f + npPanel);
        if (!fixed) {
            textTop = std::max(textTop, npOffY);
            textBottom = std::max(textBottom, npOffY);
        } else {
            float npHeight = fontPx * (k.twoLines ? 2.7f : 1.6f);
            float overflow = (k.np == 2) ? npHeight - padB : npHeight - padT - k.totalHeight - padB;
            textTop = std::max(textTop, npOffY);
            textBottom = std::max(textBottom, std::max(0.f, overflow) + npOffY);
            textBottom = std::max(textBottom, npOffY);
        }
    }
    if (k.prog) {
        float need = (float)(std::max(1, k.progH) + k.progGap) * dpi + 2.0f * dpi;
        // placement = PanelBottom
        if (fixed) textBottom = std::max(textBottom, std::max(0.f, need - padB));
    }
    float insetT = padT + margin + textTop, insetB = padB + margin + textBottom;
    float blockY = insetT;
    Out o{};
    o.surfaceH = std::max(1.0f, std::ceil(k.totalHeight + insetT + insetB) + 1.0f);
    // VizProgressRect, PanelBottom.
    float h = (float)std::max(1, k.progH) * dpi, gap = (float)k.progGap * dpi;
    float y = blockY + k.totalHeight + gap;
    o.barBottom = k.prog ? std::round(y) + std::round(h) : 0.f;
    // VizDrawTextOverlays, PanelTop / PanelBottom (npOffY offset 0); the text
    // panel then extends npBgPadding beyond the text box.
    if (k.np) {
        float npHeight = fontPx * (k.twoLines ? 2.7f : 1.6f);
        float pT = k.bg ? (float)k.padT * dpi : 0.f, pB = k.bg ? (float)k.padB * dpi : 0.f;
        float ny = (k.np == 1) ? blockY - pT + std::max(0.f, (pT - npHeight) * 0.5f)
                               : blockY + k.totalHeight + std::max(0.f, (pB - npHeight) * 0.5f);
        o.npBottom = ny + npHeight + (float)k.npBgPadding * dpi;
    }
    return o;
}

int main() {
    const Case cases[] = {
        {"bar, bg off, gap 4 h 3, 1.0x", 1.0f, false, 0, 0, 100, true, 4, 3, 0, false, 16, 6},
        {"bar, bg off, gap 6 h 4, 1.5x", 1.5f, false, 0, 0, 120, true, 6, 4, 0, false, 16, 6},
        {"bar, bg on padB 2, gap 8 h 6, 1.25x", 1.25f, true, 10, 2, 80, true, 8, 6, 0, false, 16, 6},
        {"bar, bg on padB 20, gap 4 h 3, 1.0x", 1.0f, true, 10, 20, 80, true, 4, 3, 0, false, 16, 6},
        {"NP PanelBottom 2 lines 16pt, padB 4, 1.5x", 1.5f, true, 8, 4, 100, false, 0, 0, 2, true, 16, 6},
        {"NP PanelBottom 1 line 24pt, bg off, panelpad 0", 1.0f, false, 0, 0, 100, false, 0, 0, 2, false, 24, 0},
        {"NP PanelBottom 2 lines 12pt, padB 40 (fits)", 1.0f, true, 8, 40, 100, false, 0, 0, 2, true, 12, 6},
        {"NP PanelTop 2 lines 20pt, short bars 20px, padT 4", 1.0f, true, 4, 4, 20, false, 0, 0, 1, true, 20, 0},
        {"NP PanelTop 1 line 16pt, tall bars (fits)", 1.0f, true, 4, 4, 120, false, 0, 0, 1, false, 16, 6},
        {"bar + NP PanelBottom, bg off, 2.0x", 2.0f, false, 0, 0, 60, true, 6, 4, 2, true, 14, 6},
    };
    int fails = 0;
    printf("%-50s | %-34s | %-34s\n", "case", "before: surfaceH / bar / np bottom", "after: surfaceH / bar / np bottom");
    for (const Case& k : cases) {
        Out a = Eval(k, false), b = Eval(k, true);
        auto st = [](const Out& o, bool prog, bool np) {
            bool clip = (prog && o.barBottom > o.surfaceH) || (np && o.npBottom > o.surfaceH + 0.5f);
            return clip;
        };
        bool ca = st(a, k.prog, k.np), cb = st(b, k.prog, k.np);
        char sa[64], sb[64];
        snprintf(sa, sizeof sa, "%5.0f / %5.0f / %6.1f %s", a.surfaceH, a.barBottom, a.npBottom, ca ? "CLIP" : "ok");
        snprintf(sb, sizeof sb, "%5.0f / %5.0f / %6.1f %s", b.surfaceH, b.barBottom, b.npBottom, cb ? "CLIP" : "ok");
        printf("%-50s | %-34s | %-34s\n", k.name, sa, sb);
        if (cb) fails++;
        if (!ca && b.surfaceH > a.surfaceH + 0.5f && !ca) printf("    (grew although it fit before)\n");
    }
    printf(fails ? "FAIL\n" : "PASS\n");
    return fails ? 1 : 0;
}
