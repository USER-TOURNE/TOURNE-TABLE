// ---- Media Card (2.1) -------------------------------------------------------------------
// Media Controls > Layout = Card turns the three-button strip into a small
// card, after the Rainmeter media modules:
//   album art   the current track's cover; hover it for prev / play / next
//   seek bar    the track position; click or drag it to seek
//   output      the speaker button lists the outputs, one click switches the
//               Windows default (the visualizer follows if it listens to it)
//   volume      drag the slider, or scroll anywhere on the card
// Its look has its own settings: background, border, corner radius, art
// size, and an accent (for the seek and volume fills) taken from the icon
// colour, a colour of your own, the album art or the Windows accent.
// It is the same layered window as the strip, painted in software, and only
// repainted when something on it changes: hover, a click, a new cover, the
// volume, or the seek bar moving by a whole pixel (checked once a second
// while a track plays).

float VizCardGetVolume();                 // default output, 0..1, or -1
void VizCardSetVolume(float v);
void VizCardOutputMenu(HWND hWnd, POINT screenPt);

constexpr UINT_PTR kCardTimer = 0x7701;

bool VizCardActive() { return g_settings.mediaCard; }

struct VizCardGeom {
    int pad, tile, gap, progY, progH, rowY, rowH, w, h;
    float dpi;
};

VizCardGeom VizCardLayout() {
    VizCardGeom g;
    g.dpi = GetMediaControlsDpiScale();
    int s = std::max(1, (int)std::lround(g_settings.mediaIconSize * g.dpi));
    int sp = std::max(0, (int)std::lround(g_settings.mediaIconSpacing * g.dpi));
    g.pad = std::max(GetMediaPlatePaddingPx(), (int)std::lround(8 * g.dpi));
    g.tile = g_settings.cardArtSize > 0 ? std::max(24, (int)std::lround(g_settings.cardArtSize * g.dpi)) : s * 3 + sp * 2;
    g.gap = std::max(4, (int)std::lround(7 * g.dpi));
    g.progH = std::max(3, (int)std::lround(3 * g.dpi));
    g.progY = g.pad + g.tile + g.gap;
    g.rowH = std::max((int)std::lround(18 * g.dpi), (int)std::lround(s * 0.6f));
    g.rowY = g.progY + g.progH + g.gap;
    g.w = g.tile + g.pad * 2;
    g.h = g.rowY + g.rowH + g.pad;
    return g;
}

void VizCardSize(int* w, int* h) {
    VizCardGeom g = VizCardLayout();
    *w = g.w;
    *h = g.h;
}

namespace {
enum CardPart { kPartNone, kPartArt, kPartSeek, kPartSpeaker, kPartVolume };
bool s_cardHover = false, s_cardTracking = false;
int s_cardDrag = kPartNone;
int s_cardHoverPart = kPartNone;
float s_cardSeekPreview = -1.f;  // while dragging the seek bar
float s_cardVolume = -1.f;
int s_cardProgPx = -1;

// Straight-alpha colour over the premultiplied buffer, with coverage.
void CardBlend(BYTE* p, float r, float g, float b, float a) {
    if (a <= 0.f) return;
    BlendPremultipliedOver(p, (BYTE)std::lround(b * a * 255.f), (BYTE)std::lround(g * a * 255.f),
                           (BYTE)std::lround(r * a * 255.f), (BYTE)std::lround(a * 255.f));
}

// Coverage of a pixel by a rounded rect (signed distance, one pixel of AA).
float CardRoundCov(float px, float py, float l, float t, float r, float b, float rad) {
    float cx = (l + r) * 0.5f, cy = (t + b) * 0.5f, hx = (r - l) * 0.5f, hy = (b - t) * 0.5f;
    rad = std::min({rad, hx, hy});
    float qx = fabsf(px - cx) - hx + rad, qy = fabsf(py - cy) - hy + rad;
    float d = sqrtf(std::max(qx, 0.f) * std::max(qx, 0.f) + std::max(qy, 0.f) * std::max(qy, 0.f)) +
              std::min(std::max(qx, qy), 0.f) - rad;
    return std::clamp(0.5f - d, 0.f, 1.f);
}

void CardFillRound(BYTE* buf, int stride, int W, int H, float l, float t, float r, float b, float rad, float cr,
                   float cg, float cb, float ca) {
    int x0 = std::max(0, (int)floorf(l)), x1 = std::min(W, (int)ceilf(r));
    int y0 = std::max(0, (int)floorf(t)), y1 = std::min(H, (int)ceilf(b));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float cov = CardRoundCov(x + 0.5f, y + 0.5f, l, t, r, b, rad);
            if (cov > 0.f) CardBlend(buf + (size_t)y * stride + (size_t)x * 4, cr, cg, cb, ca * cov);
        }
}

// A small speaker: body, cone, and one or two sound waves by volume.
void CardSpeaker(BYTE* buf, int stride, int ox, int oy, int sz, float vol, float ir, float ig, float ib, float ia) {
    float s = (float)sz;
    for (int y = 0; y < sz; y++)
        for (int x = 0; x < sz; x++) {
            float cov = 0.f;
            for (int k = 0; k < 4; k++) {
                float px = x + ((k & 1) + 0.5f) / 2.f, py = y + ((k >> 1) + 0.5f) / 2.f;
                float u = px / s, v = py / s;
                bool in = (u >= 0.14f && u <= 0.3f && v >= 0.38f && v <= 0.62f) ||
                          PointInTriangle(px, py, s * 0.3f, s * 0.38f, s * 0.3f, s * 0.62f, s * 0.5f, s * 0.2f) ||
                          PointInTriangle(px, py, s * 0.3f, s * 0.62f, s * 0.5f, s * 0.8f, s * 0.5f, s * 0.2f);
                float dx = u - 0.5f, dy = v - 0.5f, rr = sqrtf(dx * dx + dy * dy);
                bool arcs = dx > 0.05f && fabsf(dy) < dx * 1.1f;
                if (arcs && vol > 0.01f && fabsf(rr - 0.2f) < 0.035f) in = true;
                if (arcs && vol > 0.5f && fabsf(rr - 0.33f) < 0.035f) in = true;
                if (in) cov += 0.25f;
            }
            if (cov > 0.f) CardBlend(buf + (size_t)(oy + y) * stride + (size_t)(ox + x) * 4, ir, ig, ib, ia * cov);
        }
}

int CardHit(const VizCardGeom& g, int x, int y) {
    int slack = (int)std::lround(5 * g.dpi);
    if (x >= g.pad && x < g.pad + g.tile && y >= g.pad && y < g.pad + g.tile) return kPartArt;
    if (x >= g.pad && x < g.pad + g.tile && y >= g.progY - slack && y < g.progY + g.progH + slack) return kPartSeek;
    if (y >= g.rowY && y < g.rowY + g.rowH) {
        if (x >= g.pad && x < g.pad + g.rowH) return kPartSpeaker;
        if (x >= g.pad + g.rowH && x < g.pad + g.tile + slack) return kPartVolume;
    }
    return kPartNone;
}

float CardSliderFrac(const VizCardGeom& g, int x) {
    float l = (float)(g.pad + g.rowH + g.gap), r = (float)(g.pad + g.tile) - 6.f * g.dpi;
    return std::clamp((x - l) / std::max(1.f, r - l), 0.f, 1.f);
}

void CardRepaint() {
    if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}

// Seek to a fraction of the track, through the media command thread.
void CardSeek(float frac) {
    int64_t start = g_tlStart.load(std::memory_order_relaxed), end = g_tlEnd.load(std::memory_order_relaxed);
    if (!g_tlValid.load(std::memory_order_relaxed) || end <= start) return;
    g_mediaSeekTicks.store(start + (int64_t)((double)(end - start) * std::clamp(frac, 0.f, 1.f)),
                           std::memory_order_relaxed);
    SendMediaCommand(3);
}
}  // namespace

void VizPaintCard(BYTE* buf, int stride, int W, int H) {
    VizCardGeom g = VizCardLayout();
    const float ir = g_settings.mediaIconColorR / 255.f, ig = g_settings.mediaIconColorG / 255.f,
                ib = g_settings.mediaIconColorB / 255.f, ia = g_settings.mediaIconColorA / 255.f;
    // Accent: the seek and volume fills and their knobs.
    float ar = ir, ag = ig, ab = ib, aa = ia;
    if (g_settings.cardAccentSource == 1) {
        ar = g_settings.cardAccentR / 255.f;
        ag = g_settings.cardAccentG / 255.f;
        ab = g_settings.cardAccentB / 255.f;
        aa = g_settings.cardAccentA / 255.f;
    } else if (g_settings.cardAccentSource >= 2) {
        DWORD dw = g_settings.cardAccentSource == 2 ? g_albumArtColor.load(std::memory_order_relaxed)
                                                    : GetWindowsAccentColor();
        ar = ((dw >> 16) & 0xFF) / 255.f;
        ag = ((dw >> 8) & 0xFF) / 255.f;
        ab = (dw & 0xFF) / 255.f;
        aa = 1.f;
    }
    // Background, then the border drawn as a ring inside the edge. The
    // background is never fully transparent: a layered window lets clicks
    // through pixels with zero alpha, and the card should take every click
    // inside it.
    const float cardR = g_settings.cardRadius * g.dpi;
    CardFillRound(buf, stride, W, H, 0.f, 0.f, (float)W, (float)H, cardR, g_settings.cardBgR / 255.f,
                  g_settings.cardBgG / 255.f, g_settings.cardBgB / 255.f, std::max(1, (int)g_settings.cardBgA) / 255.f);
    if (g_settings.cardBorderSize > 0 && g_settings.cardBorderA > 0) {
        float bw = std::min(g_settings.cardBorderSize * g.dpi, std::min(W, H) * 0.5f);
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                float px = x + 0.5f, py = y + 0.5f;
                float ring = CardRoundCov(px, py, 0.f, 0.f, (float)W, (float)H, cardR) -
                             CardRoundCov(px, py, bw, bw, W - bw, H - bw, std::max(0.f, cardR - bw));
                if (ring > 0.f)
                    CardBlend(buf + (size_t)y * stride + (size_t)x * 4, g_settings.cardBorderR / 255.f,
                              g_settings.cardBorderG / 255.f, g_settings.cardBorderB / 255.f,
                              g_settings.cardBorderA / 255.f * ring);
            }
    }

    // Album art, scaled bilinearly into the tile with rounded corners.
    const float tl = (float)g.pad, tt = (float)g.pad, ts = (float)g.tile;
    const float rad = std::max(0.f, cardR - g.pad * 0.5f);  // follows the card's corners
    bool art = false;
    {
        std::lock_guard<std::mutex> lock(g_artTileMutex);
        if (!g_artTile.empty() && g_artTileW > 0 && g_artTileH > 0) {
            art = true;
            const int aw = g_artTileW, ah = g_artTileH;
            for (int y = 0; y < g.tile; y++)
                for (int x = 0; x < g.tile; x++) {
                    float cov = CardRoundCov(tl + x + 0.5f, tt + y + 0.5f, tl, tt, tl + ts, tt + ts, rad);
                    if (cov <= 0.f) continue;
                    float u = (x + 0.5f) / ts * aw - 0.5f, v = (y + 0.5f) / ts * ah - 0.5f;
                    int x0 = std::clamp((int)floorf(u), 0, aw - 1), y0 = std::clamp((int)floorf(v), 0, ah - 1);
                    int x1 = std::min(x0 + 1, aw - 1), y1 = std::min(y0 + 1, ah - 1);
                    float fx = std::clamp(u - x0, 0.f, 1.f), fy = std::clamp(v - y0, 0.f, 1.f);
                    float c[4];
                    for (int k = 0; k < 4; k++) {
                        auto px = [&](int xx, int yy) { return g_artTile[((size_t)yy * aw + xx) * 4 + k] / 255.f; };
                        c[k] = (px(x0, y0) * (1 - fx) + px(x1, y0) * fx) * (1 - fy) +
                               (px(x0, y1) * (1 - fx) + px(x1, y1) * fx) * fy;
                    }
                    CardBlend(buf + (size_t)(g.pad + y) * stride + (size_t)(g.pad + x) * 4, c[2], c[1], c[0], c[3] * cov);
                }
        }
    }
    if (!art) CardFillRound(buf, stride, W, H, tl, tt, tl + ts, tt + ts, rad, 1.f, 1.f, 1.f, 0.06f);

    // Controls over the art: on hover, or always when there is no cover.
    if (!art || s_cardHoverPart == kPartArt) {
        if (art) CardFillRound(buf, stride, W, H, tl, tt, tl + ts, tt + ts, rad, 0.f, 0.f, 0.f, 0.45f);
        int gs = std::max(8, g.tile / 4);
        int gy = g.pad + (g.tile - gs) / 2;
        bool playing = g_mediaIsPlaying.load(std::memory_order_relaxed);
        int slot = g.tile / 3;
        DrawBuiltinGlyph(buf, stride, g.pad + (slot - gs) / 2, gy, gs, 0);
        DrawBuiltinGlyph(buf, stride, g.pad + slot + (slot - gs) / 2, gy, gs, playing ? 2 : 1);
        DrawBuiltinGlyph(buf, stride, g.pad + 2 * slot + (slot - gs) / 2, gy, gs, 3);
    }

    // Seek bar.
    float prog = s_cardSeekPreview >= 0.f ? s_cardSeekPreview : VizTrackProgress();
    bool seekHot = s_cardHoverPart == kPartSeek || s_cardDrag == kPartSeek;
    float ph = seekHot ? g.progH + 2.f * g.dpi : (float)g.progH;
    float py = g.progY + g.progH * 0.5f - ph * 0.5f;
    CardFillRound(buf, stride, W, H, tl, py, tl + ts, py + ph, ph * 0.5f, ir, ig, ib, ia * 0.22f);
    if (prog >= 0.f) {
        float px = tl + ts * std::clamp(prog, 0.f, 1.f);
        CardFillRound(buf, stride, W, H, tl, py, std::max(px, tl + ph), py + ph, ph * 0.5f, ar, ag, ab, aa * 0.9f);
        if (seekHot) {
            float kr = ph * 1.1f;
            CardFillRound(buf, stride, W, H, px - kr, py + ph * 0.5f - kr, px + kr, py + ph * 0.5f + kr, kr, ar, ag, ab, aa);
        }
        s_cardProgPx = (int)lroundf(ts * std::clamp(prog, 0.f, 1.f));
    }

    // Output button and volume slider.
    s_cardVolume = VizCardGetVolume();
    int spk = g.rowH;
    float spA = s_cardHoverPart == kPartSpeaker ? ia : ia * 0.8f;
    CardSpeaker(buf, stride, g.pad, g.rowY, spk, std::max(0.f, s_cardVolume), ir, ig, ib, spA);
    if (s_cardVolume >= 0.f) {
        float l = (float)(g.pad + g.rowH + g.gap), r = (float)(g.pad + g.tile) - 6.f * g.dpi;
        float cy = g.rowY + g.rowH * 0.5f, th = std::max(2.f, 3.f * g.dpi);
        CardFillRound(buf, stride, W, H, l, cy - th * 0.5f, r, cy + th * 0.5f, th * 0.5f, ir, ig, ib, ia * 0.22f);
        float kx = l + (r - l) * s_cardVolume;
        CardFillRound(buf, stride, W, H, l, cy - th * 0.5f, std::max(kx, l + th), cy + th * 0.5f, th * 0.5f, ar, ag, ab, aa * 0.9f);
        float kr = (s_cardHoverPart == kPartVolume || s_cardDrag == kPartVolume) ? 6.f * g.dpi : 4.5f * g.dpi;
        CardFillRound(buf, stride, W, H, kx - kr, cy - kr, kx + kr, cy + kr, kr, ar, ag, ab, aa);
    }
}

// Mouse and timer messages for the card. Returns true when handled.
bool VizCardMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    VizCardGeom g = VizCardLayout();
    int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
    switch (msg) {
        case WM_MOUSEMOVE: {
            if (!s_cardTracking) {
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
                s_cardTracking = TrackMouseEvent(&tme) != FALSE;
            }
            s_cardHover = true;
            int part = s_cardDrag != kPartNone ? s_cardDrag : CardHit(g, x, y);
            bool repaint = part != s_cardHoverPart;
            s_cardHoverPart = part;
            if (s_cardDrag == kPartVolume) {
                VizCardSetVolume(CardSliderFrac(g, x));
                repaint = true;
            } else if (s_cardDrag == kPartSeek) {
                s_cardSeekPreview = std::clamp((x - g.pad) / (float)std::max(1, g.tile), 0.f, 1.f);
                repaint = true;
            }
            if (repaint) CardRepaint();
            return true;
        }
        case WM_MOUSELEAVE:
            s_cardTracking = false;
            s_cardHover = false;
            if (s_cardDrag == kPartNone && s_cardHoverPart != kPartNone) {
                s_cardHoverPart = kPartNone;
                CardRepaint();
            }
            return true;
        case WM_LBUTTONDOWN: {
            int part = CardHit(g, x, y);
            if (part == kPartSeek || part == kPartVolume) {
                s_cardDrag = part;
                SetCapture(hWnd);
                if (part == kPartVolume) VizCardSetVolume(CardSliderFrac(g, x));
                else s_cardSeekPreview = std::clamp((x - g.pad) / (float)std::max(1, g.tile), 0.f, 1.f);
                CardRepaint();
            }
            return true;
        }
        case WM_LBUTTONUP: {
            int drag = s_cardDrag;
            s_cardDrag = kPartNone;
            if (GetCapture() == hWnd) ReleaseCapture();
            if (drag == kPartSeek) {
                CardSeek(s_cardSeekPreview);
                s_cardSeekPreview = -1.f;
                CardRepaint();
                return true;
            }
            if (drag == kPartVolume) {
                CardRepaint();
                return true;
            }
            int part = CardHit(g, x, y);
            if (part == kPartArt) {
                int third = std::clamp((x - g.pad) * 3 / std::max(1, g.tile), 0, 2);
                SendMediaCommand(third);
            } else if (part == kPartSpeaker) {
                POINT pt;
                GetCursorPos(&pt);
                VizCardOutputMenu(hWnd, pt);
                CardRepaint();
            }
            return true;
        }
        case WM_MOUSEWHEEL: {
            float v = VizCardGetVolume();
            if (v >= 0.f) {
                VizCardSetVolume(v + 0.02f * (GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA));
                CardRepaint();
            }
            return true;
        }
        case WM_TIMER:
            if (wParam != kCardTimer) return false;
            // Repaint only when the seek bar moved by a pixel, or the volume
            // changed from elsewhere (the volume flyout, a keyboard key).
            if (g_mediaIsPlaying.load(std::memory_order_relaxed) && s_cardDrag == kPartNone) {
                float p = VizTrackProgress();
                if (p >= 0.f && (int)lroundf(g.tile * std::clamp(p, 0.f, 1.f)) != s_cardProgPx) CardRepaint();
            }
            if (fabsf(VizCardGetVolume() - s_cardVolume) > 0.004f) CardRepaint();
            return true;
    }
    return false;
}

// The card's once-a-second check runs only while the card is showing.
void VizCardTimer(HWND hWnd, bool on) {
    static bool s_on = false;
    if (on == s_on || !hWnd) return;
    s_on = on;
    if (on) SetTimer(hWnd, kCardTimer, 1000, nullptr);
    else KillTimer(hWnd, kCardTimer);
}
