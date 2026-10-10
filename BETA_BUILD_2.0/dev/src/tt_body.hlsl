// ---- Resources -----------------------------------------------------------------

// Drawing (vertex / pixel shaders).
StructuredBuffer<float2> gBars REG(t0);     // per bar: level 0..1, peak cap 0..1
StructuredBuffer<float4> gGlobals REG(t1);  // [0] beat pulse, master peak, breathe env, -
                                            // [1] EQ-zone energy low, mid, high, -
StructuredBuffer<float> gWave REG(t2);      // oscilloscope trace, 256 samples, -1..1
StructuredBuffer<float4> gPoints REG(t3);   // goniometer: side, mid, alpha, -
Texture2D<float4> gPlate REG(t4);           // baked background panel, premultiplied
SamplerState gSamp REG(s0);
StructuredBuffer<uint> gCells REG(t10);     // Terminal: glyph cells only, char | colour << 8 | grid index << 16
Texture2D<float4> gGlyphs REG(t11);         // Terminal: printable ASCII baked white, premultiplied
Texture2D<float> gSpec REG(t12);            // Spectrogram: bar levels, one row per 1/60 s, ring
Texture2D<float4> gFxSrc REG(t13);          // FX: the texture a bloom pass reads
Texture2D<float4> gFxBloom REG(t14);        // FX: the blurred bloom, for the composite
SamplerState gLin REG(s1);                  // FX: bilinear, clamped

// Analysis (compute shaders, Workload = GPU).
StructuredBuffer<float> gTierIn REG(t5);     // 3 x N newest samples, one block per tier
StructuredBuffer<float> gWindow REG(t6);     // N window coefficients
StructuredBuffer<float4> gBandDesc REG(t7);  // per band: tier + 4 * EQ zone, k0, k1, centre Hz
StructuredBuffer<float4> gBandOff REG(t8);   // per band: RMS offset dB, peak offset dB, w0, w1
StructuredBuffer<float> gShapeMod REG(t9);   // per bar: wave / breathe modulation from the CPU
RWStructuredBuffer<float> gPower REG(u0);    // 3 x (N/2 + 1) power spectra
RWStructuredBuffer<float4> gBandState REG(u1);  // per band: level, raw dB, fast, slow
RWStructuredBuffer<float2> gBarsOut REG(u2);    // = gBars
RWStructuredBuffer<float4> gBarAux REG(u3);     // per bar: peak, hold timer, fall velocity, -
RWStructuredBuffer<float4> gGlobalsOut REG(u4); // = gGlobals, plus [2] auto gain state
RWStructuredBuffer<uint> gStats REG(u5);        // [0] dominant Hz (float bits), [1] max delta px (bits)

#define TT_PI 3.14159265358979f

// ---- Shared helpers ------------------------------------------------------------

float4 Hsv(float h, float s, float v, float a) {
    h = h - 360.0f * floor(h / 360.0f);
    float c = v * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - abs(hp - 2.0f * floor(hp / 2.0f) - 1.0f));
    float m = v - c;
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (h < 60.0f) { r = c; g = x; }
    else if (h < 120.0f) { r = x; g = c; }
    else if (h < 180.0f) { g = c; b = x; }
    else if (h < 240.0f) { g = x; b = c; }
    else if (h < 300.0f) { r = x; b = c; }
    else { r = c; b = x; }
    return float4(r + m, g + m, b + m, a);
}

float4 BeatFlash(float4 col) {
    if ((fFlags & 2u) == 0u) return col;
    float pulse = gGlobals[0].x;
    if (pulse <= 0.001f) return col;
    float blend = min(1.0f, pulse * fBeatIntensity * fBeatColor.w);
    return float4(col.x + (fBeatColor.x - col.x) * blend, col.y + (fBeatColor.y - col.y) * blend,
                  col.z + (fBeatColor.z - col.z) * blend, col.w);
}

// The Color Mode rules, exactly as the Direct2D path applies them per bar.
// Modes: 0 solid, 1 gradient, 2 reactive, 3 accent, 4 album, 5 dynamic album,
// 6 acrylic, 7 rainbow, 8 Tourne. Accent / album / Tourne arrive already
// resolved into fC1 / fGrad1 / fC2.
float4 BarColor(uint i, float fac, bool radial) {
    uint n = fBarCount;
    float t = (n > 1u) ? (float)i / (float)(n - 1u) : 0.0f;
    float4 col = fC1;
    uint m = fColorMode;
    if (m == 1u || m == 8u) col = lerp(fGrad1, fC2, t);
    else if (m == 2u) col = lerp(fGrad1, fC2, fac);
    else if (m == 5u && !radial) col = lerp(fGrad1, fC2, min(1.0f, t * 0.6f + fac * 0.4f));
    else if (m == 7u) {
        float tr = radial ? (float)i / (float)max(n, 1u) : t;
        col = Hsv(fRainbowBase + tr * 360.0f, 0.85f, 1.0f, fC1.w);
    }
    if (m == 6u) col = float4(fC1.x, fC1.y, fC1.z, min(180.0f, floor(180.0f * fac)) / 255.0f);
    return BeatFlash(col);
}

float4 ScopeColor() {
    float4 col = fScopeColor;
    if ((fFlags & 4u) != 0u) {
        float4 z = gGlobals[1];
        float total = z.x + z.y + z.z;
        if (total > 0.001f) {
            col = float4((1.0f * z.x + 0.4706f * z.y + 0.3529f * z.z) / total,
                         (0.3529f * z.x + 0.8627f * z.y + 0.7059f * z.z) / total,
                         (0.2353f * z.x + 0.3529f * z.y + 1.0f * z.z) / total, 1.0f);
        } else {
            col = float4(0.4706f, 0.8627f, 0.3529f, 1.0f);
        }
    }
    return BeatFlash(col);
}

float4 Premul(float4 c) { return float4(c.x * c.w, c.y * c.w, c.z * c.w, c.w); }

// ---- Drawing -------------------------------------------------------------------
//
// Every primitive is one instanced quad. The vertex shader works out where it
// goes from the bar state and the frame constants (the same geometry rules as
// the Direct2D path), the pixel shader computes exact antialiased coverage from
// a signed distance: saturate(0.5 - d) is the true area coverage for an
// axis-aligned edge at any subpixel position, and within a hair of it for the
// rounded corners and the capsules.

struct Prim {
    uint kind;    // 0 rounded rect, 1 capsule, 2 textured plate, 3 skip, 4 glyph
    float4 a;     // rect: left top right bottom / capsule: ax ay bx by
    float4 radii; // rect: TL TR BR BL / capsule: x = radius
    float4 color; // straight alpha
};

Prim NoPrim() {
    Prim p;
    p.kind = 3u;
    p.a = float4(0.0f, 0.0f, 0.0f, 0.0f);
    p.radii = float4(0.0f, 0.0f, 0.0f, 0.0f);
    p.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    return p;
}

Prim RectPrim(float l, float t, float r, float b, float4 radii, float4 color) {
    Prim p;
    p.kind = 0u;
    p.a = float4(l, t, r, b);
    p.radii = radii;
    p.color = color;
    if (r - l <= 0.0f || b - t <= 0.0f) p.kind = 3u;
    return p;
}

Prim CapsulePrim(float ax, float ay, float bx, float by, float radius, float4 color) {
    Prim p;
    p.kind = 1u;
    p.a = float4(ax, ay, bx, by);
    p.radii = float4(radius, 0.0f, 0.0f, 0.0f);
    p.color = color;
    return p;
}

float BarRange() { return max(0.0f, fMaxSize - fIdleSize); }

Prim BarPrimAt(uint i, float fac, float alpha) {
    float size = fIdleSize + fac * BarRange();
    float lead = (float)i * (fBarW + fBarGap);
    float4 col = BarColor(i, fac, false);
    col.w = col.w * alpha;
    if (fVertical == 0u) {
        float x = fBlock.x + lead;
        float y0, y1;
        if (fAnchor == 0u) { y0 = fBlock.y; y1 = fBlock.y + size; }
        else if (fAnchor == 1u) { y0 = fBlock.y + (fMaxSize - size) * 0.5f; y1 = y0 + size; }
        else { y0 = fBlock.y + (fMaxSize - size); y1 = fBlock.y + fMaxSize; }
        return RectPrim(x, y0, x + fBarW, y1, fRadii, col);
    }
    float y = fBlock.y + lead;
    float x0, x1;
    if (fAnchor == 0u) { x1 = fBlock.x + fMaxSize; x0 = x1 - size; }
    else if (fAnchor == 1u) { float cx = fBlock.x + fMaxSize * 0.5f; x0 = cx - size * 0.5f; x1 = cx + size * 0.5f; }
    else { x0 = fBlock.x; x1 = fBlock.x + size; }
    return RectPrim(x0, y, x1, y + fBarW, fRadii, col);
}

Prim BarPrim(uint i) {
    if (i >= fBarCount) return NoPrim();
    return BarPrimAt(i, max(0.0f, gBars[i].x), 1.0f);
}

// Afterimage (2.1): the bar again at its slowly falling trail level (carried
// where the peak hold usually is), see-through, drawn before the bar.
Prim GhostPrim(uint i) {
    if (i >= fBarCount || fGhostA <= 0.0f) return NoPrim();
    return BarPrimAt(i, max(0.0f, gBars[i].y), fGhostA);
}


Prim CapPrim(uint i) {
    if (i >= fBarCount || (fFlags & 1u) == 0u) return NoPrim();
    float hold = fIdleSize + gBars[i].y * BarRange();
    if (hold <= 0.5f) return NoPrim();
    float lead = (float)i * (fBarW + fBarGap);
    float h = fCapThickness * 0.5f;
    float4 z = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (fVertical == 0u) {
        float x = fBlock.x + lead;
        float cy;
        if (fAnchor == 0u) cy = fBlock.y + hold;
        else if (fAnchor == 1u) cy = fBlock.y + (fMaxSize - hold) * 0.5f;
        else cy = fBlock.y + (fMaxSize - hold);
        return RectPrim(x, cy - h, x + fBarW, cy + h, z, fPeakColor);
    }
    float y = fBlock.y + lead;
    float cx;
    if (fAnchor == 0u) cx = fBlock.x + fMaxSize - hold;
    else if (fAnchor == 1u) cx = fBlock.x + fMaxSize * 0.5f - hold * 0.5f;
    else cx = fBlock.x + hold;
    return RectPrim(cx - h, y, cx + h, y + fBarW, z, fPeakColor);
}

// One dot. Slots per bar: Middle anchor uses 2 * maxDots + 1 (centre, then
// alternating up / down pairs), the other anchors maxDots.
Prim DotPrim(uint id) {
    uint slots = max(fDotSlots, 1u);
    uint i = id / slots;
    uint s = id - i * slots;
    if (i >= fBarCount) return NoPrim();
    float fac = max(0.0f, gBars[i].x);
    float colSize = fIdleSize + fac * BarRange();
    if (colSize < 0.5f) return NoPrim();
    int numDots = (fDotStep > 0.5f) ? (int)(colSize / fDotStep) : 1;
    numDots = max(1, numDots);
    uint d = s;
    bool second = false;  // Middle: the downward (or leftward) dot of a pair
    if (fAnchor == 1u && s > 0u) { d = (s + 1u) / 2u; second = (s & 1u) == 0u; }
    if ((int)d >= numDots) return NoPrim();
    float r = fDotR;
    float lead = (float)i * fDotStep + r;
    float fd = (float)d * fDotStep;
    float cx, cy;
    if (fVertical == 0u) {
        cx = fBlock.x + lead;
        if (fAnchor == 0u) {
            cy = fBlock.y + fd + r;
            if (cy - r > fBlock.y + fMaxSize) return NoPrim();
        } else if (fAnchor == 1u) {
            float c = fBlock.y + fMaxSize * 0.5f;
            if (d == 0u) cy = c;
            else if (!second) { cy = c - fd; if (cy - r < fBlock.y) return NoPrim(); }
            else { cy = c + fd; if (cy + r > fBlock.y + fMaxSize) return NoPrim(); }
        } else {
            cy = fBlock.y + fMaxSize - fd - r;
            if (cy + r < fBlock.y) return NoPrim();
        }
    } else {
        cy = fBlock.y + lead;
        if (fAnchor == 0u) {
            cx = fBlock.x + fMaxSize - fd - r;
            if (cx + r < fBlock.x) return NoPrim();
        } else if (fAnchor == 1u) {
            float c = fBlock.x + fMaxSize * 0.5f;
            if (d == 0u) cx = c;
            else if (!second) { cx = c + fd; if (cx + r > fBlock.x + fMaxSize) return NoPrim(); }
            else { cx = c - fd; if (cx - r < fBlock.x) return NoPrim(); }
        } else {
            cx = fBlock.x + fd + r;
            if (cx - r > fBlock.x + fMaxSize) return NoPrim();
        }
    }
    return RectPrim(cx - r, cy - r, cx + r, cy + r, fDotRadii, BarColor(i, fac, false));
}

Prim RadialPrim(uint i) {
    if (i >= fBarCount) return NoPrim();
    float fac = max(0.0f, gBars[i].x);
    float len = fIdleSize + fac * BarRange();
    if (len < 0.5f) return NoPrim();
    float ang = (float)i / (float)fBarCount * 2.0f * TT_PI - TT_PI * 0.5f;
    float dx = cos(ang), dy = sin(ang);
    return CapsulePrim(fCenter.x + dx * fInnerR, fCenter.y + dy * fInnerR,
                       fCenter.x + dx * (fInnerR + len), fCenter.y + dy * (fInnerR + len),
                       fStrokeW * 0.5f, BarColor(i, fac, true));
}

float2 ScopePoint(uint w) {
    float along = fSweepOrigin + (float)w * fWStep;
    float across = fScopeCenter + gWave[w] * fAmpScale * ((fVertical == 0u) ? -1.0f : 1.0f);
    return (fVertical == 0u) ? float2(along, across) : float2(across, along);
}

Prim ScopePrim(uint w) {
    if (w >= 255u) return NoPrim();
    float2 a = ScopePoint(w);
    float2 b = ScopePoint(w + 1u);
    return CapsulePrim(a.x, a.y, b.x, b.y, max(1.0f, fBarW * 0.5f) * 0.5f, ScopeColor());
}

Prim GonioPrim(uint j) {
    float4 p = gPoints[j];
    if (p.z <= 0.002f) return NoPrim();
    float x = fCenter.x + clamp(p.x, -1.0f, 1.0f) * fGonioR;
    float y = fCenter.y - clamp(p.y, -1.0f, 1.0f) * fGonioR;
    float4 col = ScopeColor();
    col.w = col.w * p.z;
    float r = fGonioDot;
    return RectPrim(x - r, y - r, x + r, y + r, float4(r, r, r, r), col);
}

Prim CorrPrim(uint j) {
    float halfW = fGonioR;
    float y0 = fCorrY, y1 = fCorrY + fCorrH;
    float4 col = ScopeColor();
    float rr = fCorrH * 0.5f;
    if (j == 0u) {
        col.w = col.w * 0.25f;
        return RectPrim(fCenter.x - halfW, y0, fCenter.x + halfW, y1, float4(rr, rr, rr, rr), col);
    }
    float c = fCorr;
    float x = fCenter.x + clamp(c, -1.0f, 1.0f) * halfW;
    float hw = max(1.5f, fCorrH * 0.5f);
    return RectPrim(x - hw, y0 - 1.0f, x + hw, y1 + 1.0f, float4(hw, hw, hw, hw), col);
}

// One terminal cell: a glyph from the atlas (printable ASCII from 32, laid
// out fTermAtlasCols to a row, one cell each), in one of five palette
// colours. Cells are whole pixels and drawn 1:1, so pixel fonts stay sharp.
// Only cells with a glyph are uploaded, each as char | colour << 8 |
// grid index << 16, so the instance count is the number of glyphs.
Prim TermPrim(uint id) {
    uint cols = max(fTermCols, 1u);
    uint cell = gCells[id];
    uint ch = cell & 255u;  // 33-126 ASCII, 128-255 the custom glyphs (2.1)
    uint idx = cell >> 16u;
    if (ch <= 32u || idx >= cols * fTermRows) return NoPrim();
    uint ci = min((cell >> 8u) & 7u, 4u);
    uint col = idx % cols;
    uint row = idx / cols;
    float cw = fTermGeom.z, chh = fTermGeom.w;
    float x0 = fTermGeom.x + (float)col * cw;
    float y0 = fTermGeom.y + (float)row * chh;
    uint g = ch - 32u;
    uint ac = max(fTermAtlasCols, 1u);
    float gx = (float)(g % ac), gy = (float)(g / ac);
    Prim p;
    p.kind = 4u;
    p.a = float4(x0, y0, x0 + cw, y0 + chh);
    p.radii = float4(gx * cw / fTermAtlas.x, gy * chh / fTermAtlas.y, (gx + 1.0f) * cw / fTermAtlas.x,
                     (gy + 1.0f) * chh / fTermAtlas.y);
    p.color = fTermColors[ci];
    return p;
}

// ---- Styles (2.1) ----------------------------------------------------------------
//
// Each style is one more pass over the same instanced quad. Bars, peak caps
// and the frame constants are shared with the shapes above, so a style costs
// one draw call and no new per-frame upload unless it has data of its own
// (Spectrogram rows, Stereo Field levels, Particles).

// The part of bar i that lies between distances a and b from its base edge.
// Middle anchor is drawn from the bottom (or left) edge here: segmented and
// filled styles read as meters, and a meter grows from one end.
float4 SpanRect(uint i, float a, float b) {
    float lead = (float)i * (fBarW + fBarGap);
    if (fVertical == 0u) {
        float x = fBlock.x + lead;
        if (fAnchor == 0u) return float4(x, fBlock.y + a, x + fBarW, fBlock.y + b);
        float base = fBlock.y + fMaxSize;
        return float4(x, base - b, x + fBarW, base - a);
    }
    float y = fBlock.y + lead;
    if (fAnchor == 0u) {
        float base = fBlock.x + fMaxSize;
        return float4(base - b, y, base - a, y + fBarW);
    }
    return float4(fBlock.x + a, y, fBlock.x + b, y + fBarW);
}

Prim RectPrimV(float4 r, float4 radii, float4 color) { return RectPrim(r.x, r.y, r.z, r.w, radii, color); }

// LED Meter: green to 60 %, amber to 85 %, red above, like a hardware meter.
// Unlit segments stay faintly visible; the peak-hold segment stays lit.
float4 LedZone(float t) {
    if (t >= 0.85f) return float4(1.0f, 0.23f, 0.19f, 1.0f);
    if (t >= 0.6f) return float4(1.0f, 0.69f, 0.0f, 1.0f);
    return float4(0.22f, 0.89f, 0.42f, 1.0f);
}

Prim LedPrim(uint id) {
    uint segs = max(fSegs, 1u);
    uint i = id / segs;
    uint s = id % segs;
    if (i >= fBarCount) return NoPrim();
    float fac = max(0.0f, gBars[i].x);
    float lit = (fIdleSize + fac * BarRange()) / max(fSegStep, 1.0f);
    float4 col = (fColorMode == 0u) ? BeatFlash(LedZone(((float)s + 0.5f) / (float)segs)) : BarColor(i, fac, false);
    bool on = (float)s + 0.5f <= lit;
    if (!on && (fFlags & 1u) != 0u) {
        float hold = fIdleSize + gBars[i].y * BarRange();
        on = hold > 0.5f && (uint)min(floor(hold / max(fSegStep, 1.0f)), (float)(segs - 1u)) == s;
    }
    if (!on) col.w = col.w * 0.1f;
    float a = (float)s * fSegStep;
    return RectPrimV(SpanRect(i, a, a + fSegH), fRadii, col);
}

// Line Spectrum: a Catmull-Rom curve through the bar tops, filled down to the
// base and lit along its edge. Each instance is one column between two
// curve points (kind 5); columns tile exactly in x, so the translucent fill
// and glow are never blended twice where they meet.
float LineSign() {
    if (fVertical == 0u) return (fAnchor == 0u) ? 1.0f : -1.0f;
    return (fAnchor == 0u) ? -1.0f : 1.0f;
}
float LineBase() {
    if (fVertical == 0u) return (fAnchor == 0u) ? fBlock.y : fBlock.y + fMaxSize;
    return (fAnchor == 0u) ? fBlock.x + fMaxSize : fBlock.x;
}
float LineVal(int i) {
    int n = (int)fBarCount;
    i = clamp(i, 0, n - 1);
    return fIdleSize + max(0.0f, gBars[(uint)i].x) * BarRange();
}
float LineAlong(float k) {
    return ((fVertical == 0u) ? fBlock.x : fBlock.y) + k * (fBarW + fBarGap) + fBarW * 0.5f;
}
float CatRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * (2.0f * p1 + (p2 - p0) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (3.0f * p1 - p0 - 3.0f * p2 + p3) * t3);
}

Prim LinePrim(uint id) {
    uint sub = max(fSubdiv, 1u);
    uint seg = id / sub;
    uint j = id % sub;
    if (fBarCount < 2u || seg + 1u >= fBarCount) return NoPrim();
    int k = (int)seg;
    float p0 = LineVal(k - 1), p1 = LineVal(k), p2 = LineVal(k + 1), p3 = LineVal(k + 2);
    float t0 = (float)j / (float)sub, t1 = (float)(j + 1u) / (float)sub;
    float v0 = clamp(CatRom(p0, p1, p2, p3, t0), 0.0f, fMaxSize);
    float v1 = clamp(CatRom(p0, p1, p2, p3, t1), 0.0f, fMaxSize);
    float u0 = LineAlong((float)seg + t0), u1 = LineAlong((float)seg + t1);
    // The first and last columns reach the ends of the bar row.
    float start = (fVertical == 0u) ? fBlock.x : fBlock.y;
    if (seg == 0u && j == 0u) u0 = start;
    if (seg + 2u == fBarCount && j + 1u == sub) u1 = start + (float)fBarCount * (fBarW + fBarGap) - fBarGap;
    float base = LineBase(), sg = LineSign();
    Prim p;
    p.kind = 5u;
    p.a = float4(u0, base + sg * v0, u1, base + sg * v1);
    p.radii = float4(base, max(1.5f, fBarW * 0.2f), fGlowR, fFillA);
    p.color = BarColor(seg + ((t0 >= 0.5f) ? 1u : 0u), max(0.0f, gBars[seg].x), false);
    return p;
}

// Polar Bloom: the Radial bars joined into one filled flower. One wedge per
// bar (kind 6) from the centre to this bar's tip and the next one's. The
// two straight sides are shared with the neighbours and tested with the
// same expression from both sides, so every pixel lands in exactly one
// wedge; only the outer edge is antialiased.
float Cross2(float ax, float ay, float bx, float by) { return ax * by - ay * bx; }

Prim BloomPrim(uint i) {
    uint n = fBarCount;
    if (n < 3u || i >= n) return NoPrim();
    uint j = (i + 1u) % n;
    float li = fIdleSize + max(0.0f, gBars[i].x) * BarRange();
    float lj = fIdleSize + max(0.0f, gBars[j].x) * BarRange();
    float ai = (float)i / (float)n * 2.0f * TT_PI - TT_PI * 0.5f;
    float aj = (float)j / (float)n * 2.0f * TT_PI - TT_PI * 0.5f;
    float ri = fInnerR + li, rj = fInnerR + lj;
    Prim p;
    p.kind = 6u;
    p.a = float4(fCenter.x + cos(ai) * ri, fCenter.y + sin(ai) * ri, fCenter.x + cos(aj) * rj,
                 fCenter.y + sin(aj) * rj);
    p.radii = float4(max(1.0f, fBarW * 0.35f), 0.0f, 0.0f, 0.0f);
    p.color = BarColor(i, max(0.0f, gBars[i].x), true);
    return p;
}

// Spectrogram: one quad (kind 7) reading a history texture, a row of bar
// levels per 1/60 s, newest at the top (left when vertical), plus a colour
// legend with quarter ticks beside it.
float4 SpecColor(float v) {
    v = saturate(v);
    float4 c;
    if (fColorMode != 0u) {
        c = lerp(fGrad1, fC2, v);
    } else if (v < 0.25f) {
        c = lerp(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(0.25f, 0.02f, 0.45f, 1.0f), v * 4.0f);
    } else if (v < 0.5f) {
        c = lerp(float4(0.25f, 0.02f, 0.45f, 1.0f), float4(0.85f, 0.15f, 0.35f, 1.0f), (v - 0.25f) * 4.0f);
    } else if (v < 0.75f) {
        c = lerp(float4(0.85f, 0.15f, 0.35f, 1.0f), float4(1.0f, 0.6f, 0.1f, 1.0f), (v - 0.5f) * 4.0f);
    } else {
        c = lerp(float4(1.0f, 0.6f, 0.1f, 1.0f), float4(1.0f, 1.0f, 0.85f, 1.0f), (v - 0.75f) * 4.0f);
    }
    c.w = saturate(v * 2.5f) * fC1.w;
    return c;
}

Prim SpecPrim(uint id) {
    if (id > 1u || fSpecW == 0u) return NoPrim();
    float thick = (float)fBarCount * (fBarW + fBarGap) - fBarGap;
    float w = (fVertical == 0u) ? thick : fMaxSize;
    float h = (fVertical == 0u) ? fMaxSize : thick;
    Prim p;
    p.kind = 7u;
    p.radii = float4((float)id, 0.0f, 0.0f, 0.0f);
    p.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (id == 0u) {
        p.a = float4(fBlock.x, fBlock.y, fBlock.x + w, fBlock.y + h);
    } else if (fVertical == 0u) {
        float x = fBlock.x + w + fVuBox.x;
        p.a = float4(x, fBlock.y, x + fVuBox.y, fBlock.y + h);
    } else {
        float y = fBlock.y + h + fVuBox.x;
        p.a = float4(fBlock.x, y, fBlock.x + w, y + fVuBox.y);
    }
    return p;
}

// VU Needles: two analog meters (L, R) built from rects and capsules, 32
// instances each: face, 11 scale marks, a 16-piece arc (red from 0 VU),
// needle, pivot and peak LED. The needle positions come from the CPU with
// real VU ballistics (99 % in 300 ms, 1.5 % overshoot).
float VuPos(float db) { return (pow(10.0f, db / 20.0f) - 0.1f) / (1.41254f - 0.1f); }
float VuMark(uint k) {
    if (k == 0u) return -20.0f;
    if (k == 1u) return -10.0f;
    if (k == 2u) return -7.0f;
    if (k == 3u) return -5.0f;
    if (k == 4u) return -3.0f;
    if (k == 5u) return -2.0f;
    if (k == 6u) return -1.0f;
    if (k == 7u) return 0.0f;
    if (k == 8u) return 1.0f;
    if (k == 9u) return 2.0f;
    return 3.0f;
}

Prim VuPrim(uint id) {
    uint m = id / 32u, k = id % 32u;
    if (m > 1u) return NoPrim();
    float ox = fBlock.x + (float)m * fVuBox.z, oy = fBlock.y + (float)m * fVuBox.w;
    float w = fVuBox.x, h = fVuBox.y;
    float pvx = ox + w * 0.5f, pvy = oy + h * 0.9f;
    float R = h * 0.68f;
    float lw = max(0.6f, h * 0.008f);
    float4 ink = float4(0.92f, 0.92f, 0.9f, 0.85f);
    float4 red = float4(1.0f, 0.27f, 0.23f, 0.95f);
    float zero = VuPos(0.0f);
    if (k == 0u) {
        float rr = h * 0.08f;
        return RectPrim(ox, oy, ox + w, oy + h, float4(rr, rr, rr, rr), float4(0.05f, 0.05f, 0.06f, 0.6f));
    }
    if (k <= 11u) {
        float db = VuMark(k - 1u);
        float an = (-48.0f + 96.0f * VuPos(db)) * TT_PI / 180.0f;
        float dx = sin(an), dy = -cos(an);
        float r0 = (k == 1u || k == 8u) ? R * 0.84f : R * 0.9f;
        return CapsulePrim(pvx + dx * r0, pvy + dy * r0, pvx + dx * R, pvy + dy * R, lw, db > 0.0f ? red : ink);
    }
    if (k <= 27u) {
        float t0 = (float)(k - 12u) / 16.0f, t1 = (float)(k - 11u) / 16.0f;
        float a0 = (-48.0f + 96.0f * t0) * TT_PI / 180.0f, a1 = (-48.0f + 96.0f * t1) * TT_PI / 180.0f;
        bool hot = (t0 + t1) * 0.5f > zero;
        float ra = R * 0.9f;
        return CapsulePrim(pvx + sin(a0) * ra, pvy - cos(a0) * ra, pvx + sin(a1) * ra, pvy - cos(a1) * ra,
                           hot ? lw * 1.8f : lw, hot ? red : ink);
    }
    float pos = (m == 0u) ? fVu.x : fVu.y;
    if (k == 28u) {
        float an = (-48.0f + 96.0f * pos) * TT_PI / 180.0f;
        float dx = sin(an), dy = -cos(an);
        float4 col = BeatFlash(fC1);
        col.w = 1.0f;
        return CapsulePrim(pvx + dx * R * 0.12f, pvy + dy * R * 0.12f, pvx + dx * R * 1.02f, pvy + dy * R * 1.02f,
                           max(0.8f, h * 0.012f), col);
    }
    if (k == 29u) {
        float r = h * 0.05f;
        return RectPrim(pvx - r, pvy - r, pvx + r, pvy + r, float4(r, r, r, r), float4(0.25f, 0.25f, 0.27f, 1.0f));
    }
    if (k == 30u) {
        float led = (m == 0u) ? fVu.z : fVu.w;
        float r = h * 0.045f;
        float cx = ox + w - h * 0.12f, cy = oy + h * 0.12f;
        return RectPrim(cx - r, cy - r, cx + r, cy + r, float4(r, r, r, r),
                        float4(1.0f, 0.18f, 0.12f, lerp(0.18f, 1.0f, saturate(led))));
    }
    return NoPrim();
}

// Stereo Field: left channel grows up from the centre line, right channel
// down (left and right when vertical). gBars holds L in x and R in y.
Prim SplitPrim(uint id) {
    uint i = id >> 1u, ch = id & 1u;
    if (i >= fBarCount) return NoPrim();
    float lev = max(0.0f, (ch == 0u) ? gBars[i].x : gBars[i].y);
    float s = (fIdleSize + lev * BarRange()) * 0.5f;
    if (s < 0.25f) return NoPrim();
    float lead = (float)i * (fBarW + fBarGap);
    float4 col = BarColor(i, lev, false);
    if (fVertical == 0u) {
        float x = fBlock.x + lead, cy = fBlock.y + fMaxSize * 0.5f;
        if (ch == 0u) return RectPrim(x, cy - 0.5f - s, x + fBarW, cy - 0.5f, fRadii, col);
        return RectPrim(x, cy + 0.5f, x + fBarW, cy + 0.5f + s, fRadii, col);
    }
    float y = fBlock.y + lead, cx = fBlock.x + fMaxSize * 0.5f;
    if (ch == 0u) return RectPrim(cx - 0.5f - s, y, cx - 0.5f, y + fBarW, fRadii, col);
    return RectPrim(cx + 0.5f, y, cx + 0.5f + s, y + fBarW, fRadii, col);
}

// Particles: sparks thrown off the bar tops on each beat, simulated on the
// CPU and uploaded as (x, y, alpha, bar * 16 + radius) relative to the block.
Prim SparkPrim(uint j) {
    float4 p = gPoints[j];
    if (p.z <= 0.002f) return NoPrim();
    float band = floor(p.w / 16.0f);
    float r = p.w - band * 16.0f;
    float x = fBlock.x + p.x, y = fBlock.y + p.y;
    float4 col = BarColor(min((uint)band, max(fBarCount, 1u) - 1u), 1.0f, false);
    col.w = col.w * saturate(p.z);
    return RectPrim(x - r, y - r, x + r, y + r, float4(r, r, r, r), col);
}

// Reflection: the pass mirrored about the base line, faded out over
// fReflDepth in the pixel shader (flag 16 on the kind).
Prim ReflectPrim(Prim p) {
    float B2 = 2.0f * fReflBase;
    uint kb = p.kind & 15u;  // the glow flag may be set
    if (kb == 0u) {
        p.a = float4(p.a.x, B2 - p.a.w, p.a.z, B2 - p.a.y);
        p.radii = float4(p.radii.w, p.radii.z, p.radii.y, p.radii.x);
    } else if (kb == 5u) {
        p.a = float4(p.a.x, B2 - p.a.y, p.a.z, B2 - p.a.w);
        p.radii.x = B2 - p.radii.x;
    } else {
        return NoPrim();
    }
    p.color.w = p.color.w * fReflAlpha;
    p.kind = p.kind | 16u;
    return p;
}

Prim BuildPrim(uint passId, uint id) {
    if (passId == 0u) {
        Prim p = RectPrim(fPlateRect.x, fPlateRect.y, fPlateRect.z, fPlateRect.w,
                          float4(0.0f, 0.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 1.0f));
        if (p.kind == 0u) p.kind = 2u;
        return p;
    }
    if (passId == 1u) return BarPrim(id);
    if (passId == 2u) return CapPrim(id);
    if (passId == 3u) return DotPrim(id);
    if (passId == 4u) return RadialPrim(id);
    if (passId == 5u) return ScopePrim(id);
    if (passId == 6u) return GonioPrim(id);
    if (passId == 7u) return CorrPrim(id);
    if (passId == 8u) return TermPrim(id);
    if (passId == 9u) return LedPrim(id);
    if (passId == 10u) return LinePrim(id);
    if (passId == 11u) return BloomPrim(id);
    if (passId == 12u) return SpecPrim(id);
    if (passId == 13u) return VuPrim(id);
    if (passId == 14u) return SplitPrim(id);
    if (passId == 15u) return SparkPrim(id);
    if (passId == 16u) return GhostPrim(id);
    return NoPrim();
}

struct VsOut {
    float4 pos SEM(SV_Position);
    float2 pix SEM(TEXCOORD0);
    NOINTERP float4 shape SEM(TEXCOORD1);
    NOINTERP float4 radii SEM(TEXCOORD2);
    NOINTERP float4 color SEM(TEXCOORD3);
    NOINTERP uint kind SEM(TEXCOORD4);  // low 4 bits: Prim kind; 16: reflected; 32: FX; 64: dash/hollow; 128: tilt
};

// Quad corner `vid` (triangle strip 0..3) of the primitive's bounds. The
// bounds reach one pixel past the shape so its antialiased edge is covered;
// the plate is drawn exactly 1:1.
VsOut EmitVertex(Prim p, uint vid) {
    VsOut o;
    float2 lo, hi;
    uint kb = p.kind & 15u;
    if (kb == 5u) {
        float c0 = min(min(p.a.y, p.a.w), p.radii.x) - p.radii.z * 2.0f - 1.0f;
        float c1 = max(max(p.a.y, p.a.w), p.radii.x) + p.radii.z * 2.0f + 1.0f;
        lo = (fVertical == 0u) ? float2(p.a.x - 1.0f, c0) : float2(c0, p.a.x - 1.0f);
        hi = (fVertical == 0u) ? float2(p.a.z + 1.0f, c1) : float2(c1, p.a.z + 1.0f);
    } else if (kb == 6u) {
        lo = float2(min(fCenter.x, min(p.a.x, p.a.z)) - 1.5f, min(fCenter.y, min(p.a.y, p.a.w)) - 1.5f);
        hi = float2(max(fCenter.x, max(p.a.x, p.a.z)) + 1.5f, max(fCenter.y, max(p.a.y, p.a.w)) + 1.5f);
    } else if (kb == 1u) {
        float r = p.radii.x + 1.0f;
        lo = float2(min(p.a.x, p.a.z) - r, min(p.a.y, p.a.w) - r);
        hi = float2(max(p.a.x, p.a.z) + r, max(p.a.y, p.a.w) + r);
    } else if (kb == 2u || kb == 4u || kb == 7u) {
        lo = float2(p.a.x, p.a.y);
        hi = float2(p.a.z, p.a.w);
    } else {
        lo = float2(p.a.x - 1.0f, p.a.y - 1.0f);
        hi = float2(p.a.z + 1.0f, p.a.w + 1.0f);
    }
    if ((p.kind & 128u) != 0u) {  // Tilt: the sheared shape's reach
        if (fVertical == 0u) {
            float d0 = fBarTiltK * (fBarPivot - lo.y), d1 = fBarTiltK * (fBarPivot - hi.y);
            lo.x = lo.x + min(min(d0, d1), 0.0f) - 1.0f;
            hi.x = hi.x + max(max(d0, d1), 0.0f) + 1.0f;
        } else {
            float d0 = fBarTiltK * (lo.x - fBarPivot), d1 = fBarTiltK * (hi.x - fBarPivot);
            lo.y = lo.y + min(min(d0, d1), 0.0f) - 1.0f;
            hi.y = hi.y + max(max(d0, d1), 0.0f) + 1.0f;
        }
    }
    if ((p.kind & 32u) != 0u) {  // room for the glow and the shadow
        float gr = max(fFxGlow > 0.0f ? fFxGlowR * 2.0f : 0.0f,
                       fFxShadowColor.w > 0.0f ? fFxShadowSoft + max(abs(fFxShadowX), abs(fFxShadowY)) + 1.0f : 0.0f);
        lo = float2(lo.x - gr, lo.y - gr);
        hi = float2(hi.x + gr, hi.y + gr);
    }
    float cx = ((vid & 1u) != 0u) ? 1.0f : 0.0f;
    float cy = ((vid & 2u) != 0u) ? 1.0f : 0.0f;
    float2 pix = float2(lo.x + (hi.x - lo.x) * cx, lo.y + (hi.y - lo.y) * cy);
    o.pix = pix;
    o.pos = float4(pix.x / fViewport.x * 2.0f - 1.0f, 1.0f - pix.y / fViewport.y * 2.0f, 0.0f, 1.0f);
    if (p.kind == 3u) o.pos = float4(-2.0f, -2.0f, 0.0f, 1.0f);  // all four corners equal: no area
    o.shape = p.a;
    o.radii = p.radii;
    o.color = Premul(p.color);
    o.kind = p.kind;
    return o;
}

VsOut VSMain(uint vid SEM(SV_VertexID), uint iid SEM(SV_InstanceID)) {
    Prim p = BuildPrim(pPass, iid);
    // Glow: passes created with pPad1 = 1, rects and capsules only.
    if (pPad1 != 0u && (fFxGlow > 0.0f || fFxLineW > 0.0f || fFxShadowColor.w > 0.0f) && (p.kind == 0u || p.kind == 1u))
        p.kind = p.kind | 32u;
    // Bar modifiers: the bars and their Afterimage; the caps tilt with them.
    bool barPass = pPass == 1u || pPass == 16u;
    if (barPass && (fModFlags & 11u) != 0u && (p.kind & 15u) <= 1u)
        p.kind = p.kind | 64u;
    if ((barPass || pPass == 2u) && (fModFlags & 4u) != 0u && (p.kind & 15u) <= 1u)
        p.kind = p.kind | 128u;
    if (pPad0 != 0u) p = ReflectPrim(p);
    return EmitVertex(p, vid);
}

float4 LineShade(VsOut i) {
    float u = (fVertical == 0u) ? i.pix.x : i.pix.y;
    float c = (fVertical == 0u) ? i.pix.y : i.pix.x;
    float4 a = i.shape;
    float base = i.radii.x;
    float xcov = saturate(min(u + 0.5f, a.z) - max(u - 0.5f, a.x));
    float k = (a.w - a.y) / max(a.z - a.x, 1e-4f);
    float cl = a.y + k * (u - a.x);
    float sb = (base >= cl) ? 1.0f : -1.0f;
    float d = (c - cl) / sqrt(1.0f + k * k);  // perpendicular distance to the curve
    float dd = d * sb;                         // positive toward the base
    float depth = max(abs(base - cl), 1.0f);
    float fill = saturate(dd + 0.5f) * saturate((base - c) * sb + 0.5f) * i.radii.w *
                 lerp(1.0f, 0.15f, saturate(dd / depth));
    float edge = saturate(i.radii.y * 0.5f + 0.5f - abs(d));
    float glow = 0.0f;
    if (i.radii.z > 0.0f) glow = 0.4f * exp(-(d * d) / (i.radii.z * i.radii.z * 0.5f));
    float alpha = 1.0f - (1.0f - edge) * (1.0f - fill) * (1.0f - glow);
    return i.color * (alpha * xcov);
}

float4 BloomShade(VsOut i) {
    float px = i.pix.x - fCenter.x, py = i.pix.y - fCenter.y;
    float dix = i.shape.x - fCenter.x, diy = i.shape.y - fCenter.y;
    float djx = i.shape.z - fCenter.x, djy = i.shape.w - fCenter.y;
    if (!(Cross2(dix, diy, px, py) >= 0.0f && Cross2(djx, djy, px, py) < 0.0f)) return float4(0.0f, 0.0f, 0.0f, 0.0f);
    float ex = djx - dix, ey = djy - diy;
    float el = max(sqrt(ex * ex + ey * ey), 1e-4f);
    float dOut = Cross2(ex, ey, px - dix, py - diy) / el;  // distance inside the outer edge
    float cov = saturate(dOut + 0.5f);
    float r = sqrt(px * px + py * py) / max(sqrt(dix * dix + diy * diy), 1.0f);
    float fill = lerp(0.2f, 0.75f, saturate(r));
    float rim = saturate(i.radii.x + 0.5f - dOut);
    return i.color * (max(fill, rim) * cov);
}

float4 SpecShade(VsOut i) {
    float rx = saturate((i.pix.x - i.shape.x) / max(1.0f, i.shape.z - i.shape.x));
    float ry = saturate((i.pix.y - i.shape.y) / max(1.0f, i.shape.w - i.shape.y));
    if (i.radii.x > 0.5f) {  // legend: hot end at the top (right when vertical)
        float v = (fVertical == 0u) ? 1.0f - ry : rx;
        float len = (fVertical == 0u) ? i.shape.w - i.shape.y : i.shape.z - i.shape.x;
        float q = v * 4.0f;
        bool tick = abs(q - floor(q + 0.5f)) * len * 0.25f < 0.5f;
        float4 c = SpecColor(v);
        c.w = max(c.w, 0.35f);
        if (tick) c = float4(1.0f, 1.0f, 1.0f, 0.9f);
        return Premul(c);
    }
    float along = (fVertical == 0u) ? rx : ry;
    float age = (fVertical == 0u) ? ry : rx;
    uint w = max(fSpecW, 1u), rows = max(fSpecRows, 1u), tex = max(fSpecTex, 1u);
    uint bar = min((uint)(along * (float)w), w - 1u);
    uint ago = min((uint)(age * (float)rows), rows - 1u);
    uint row = (fSpecHead + tex - ago) % tex;
    return Premul(SpecColor(gSpec.Load(int3((int)bar, (int)row, 0))));
}

// Coverage of the pixel centred at `pix` by a rectangle with a separate radius
// per corner (TL TR BR BL, y down), each clamped to half the shorter side as
// Direct2D does.
//
// The straight edges use the exact overlap of the pixel with the rectangle on
// each axis, multiplied: that is the true area for any rectangle at any
// subpixel position, including bars thinner than a pixel and square corners
// (where a distance field would give min(x, y) instead of x * y, up to a
// quarter of a pixel wrong). Inside a rounded corner's square the arc takes
// over, from the distance to the corner circle.
float RectCoverage(float2 pix, float4 rect, float4 radii) {
    float ox = saturate(min(pix.x + 0.5f, rect.z) - max(pix.x - 0.5f, rect.x));
    float oy = saturate(min(pix.y + 0.5f, rect.w) - max(pix.y - 0.5f, rect.y));
    float cov = ox * oy;
    float2 c = float2((rect.x + rect.z) * 0.5f, (rect.y + rect.w) * 0.5f);
    float2 h = float2((rect.z - rect.x) * 0.5f, (rect.w - rect.y) * 0.5f);
    float2 p = float2(pix.x - c.x, pix.y - c.y);
    float r = (p.x > 0.0f) ? ((p.y > 0.0f) ? radii.z : radii.y) : ((p.y > 0.0f) ? radii.w : radii.x);
    r = clamp(r, 0.0f, min(h.x, h.y));
    float qx = abs(p.x) - h.x + r;
    float qy = abs(p.y) - h.y + r;
    if (r > 0.0f && qx > 0.0f && qy > 0.0f) {
        float d = length(float2(qx, qy)) - r;
        cov = min(cov, saturate(0.5f - d));
    }
    return cov;
}

float SdCapsule(float2 pix, float4 seg, float radius) {
    float2 pa = float2(pix.x - seg.x, pix.y - seg.y);
    float2 ba = float2(seg.z - seg.x, seg.w - seg.y);
    float bb = dot(ba, ba);
    float h = (bb > 1e-8f) ? saturate(dot(pa, ba) / bb) : 0.0f;
    return length(float2(pa.x - ba.x * h, pa.y - ba.y * h)) - radius;
}

// Glow (FX, 2.1): signed distance to a rounded rect (negative inside), so
// the glow can fall off with the distance outside the shape.
float SdRoundRect(float2 pix, float4 rect, float4 radii) {
    float cx = (rect.x + rect.z) * 0.5f, cy = (rect.y + rect.w) * 0.5f;
    float hx = (rect.z - rect.x) * 0.5f, hy = (rect.w - rect.y) * 0.5f;
    float px = pix.x - cx, py = pix.y - cy;
    float r = (px > 0.0f) ? ((py > 0.0f) ? radii.z : radii.y) : ((py > 0.0f) ? radii.w : radii.x);
    r = clamp(r, 0.0f, min(hx, hy));
    float qx = abs(px) - hx + r, qy = abs(py) - hy + r;
    return length(float2(max(qx, 0.0f), max(qy, 0.0f))) + min(max(qx, qy), 0.0f) - r;
}

float ShapeCoverage(VsOut i, uint kb) {
    if (kb == 1u) return saturate(0.5f - SdCapsule(i.pix, i.shape, i.radii.x));
    return RectCoverage(i.pix, i.shape, i.radii);
}

// The bar's own pixels, with Hollow (only a band just inside the edge) and
// Dashed (gaps along the growth direction, fixed to the base line so the
// dashes stay still while the bar grows) applied.
float Coverage(VsOut i, uint kb) {
    float cov = ShapeCoverage(i, kb);
    if ((i.kind & 64u) == 0u) return cov;
    if ((fModFlags & 1u) != 0u) {
        float d = (kb == 1u) ? SdCapsule(i.pix, i.shape, i.radii.x) : SdRoundRect(i.pix, i.shape, i.radii);
        float w = max(fHollowW, 0.25f);
        cov = min(cov, saturate(0.5f - (abs(d + w * 0.5f) - w * 0.5f)));
    }
    if ((fModFlags & 2u) != 0u) {
        float period = max(fBarDash + fBarDashGap, 1.0f);
        float t = (fVertical == 0u) ? abs(i.pix.y - fBarPivot) : abs(i.pix.x - fBarPivot);
        float u = t - floor(t / period) * period;
        float m = saturate(fBarDash - u + 0.5f) * saturate(u + 0.5f) + saturate(u - period + 0.5f);
        cov = cov * saturate(m);
    }
    if ((fModFlags & 8u) != 0u) {  // Mirror: the two halves pulled apart from the centre line
        float t = (fVertical == 0u) ? abs(i.pix.y - fBarPivot) : abs(i.pix.x - fBarPivot);
        cov = cov * saturate(t - fMirrorGap * 0.5f + 0.5f);
    }
    return cov;
}

// Tilt: the pixel back in the unsheared bar's space.
float2 Untilt(float2 pix) {
    if (fVertical == 0u) return float2(pix.x - fBarTiltK * (fBarPivot - pix.y), pix.y);
    return float2(pix.x, pix.y - fBarTiltK * (pix.x - fBarPivot));
}


// Glow, Outline and Shadow together, from one signed distance (and one more
// for the shadow's offset copy). Outline is a band just inside the edge, so
// it never changes a bar's size; the shadow sits behind the bar's own pixels.
float4 FxShade(VsOut i, uint kb) {
    float d = (kb == 1u) ? SdCapsule(i.pix, i.shape, i.radii.x) : SdRoundRect(i.pix, i.shape, i.radii);
    float cov = Coverage(i, kb);
    float4 col = i.color * cov;
    if (fFxLineW > 0.0f) {
        float o = saturate(0.5f - (abs(d + fFxLineW * 0.5f) - fFxLineW * 0.5f)) * fFxLineColor.w * i.color.w;
        col = col * (1.0f - o) + float4(fFxLineColor.x, fFxLineColor.y, fFxLineColor.z, 1.0f) * o;
    }
    if (fFxGlow > 0.0f && d > 0.0f) {
        float g = exp(-(d * d) / max(fFxGlowR * fFxGlowR * 0.5f, 0.01f));
        col = col + i.color * ((1.0f - cov) * fFxGlow * 0.65f * g);
    }
    if (fFxShadowColor.w > 0.0f) {
        float2 sp = float2(i.pix.x - fFxShadowX, i.pix.y - fFxShadowY);
        float ds = (kb == 1u) ? SdCapsule(sp, i.shape, i.radii.x) : SdRoundRect(sp, i.shape, i.radii);
        float s = saturate((fFxShadowSoft * 0.5f + 0.5f - ds) / (fFxShadowSoft + 1.0f)) * fFxShadowColor.w * i.color.w;
        col = col + float4(fFxShadowColor.x, fFxShadowColor.y, fFxShadowColor.z, 1.0f) * (s * (1.0f - col.w));
    }
    return col;
}

float4 PSMain(VsOut iIn) SEM(SV_Target) {
    VsOut i = iIn;
    if ((i.kind & 128u) != 0u) i.pix = Untilt(i.pix);
    uint kb = i.kind & 15u;
    float fade = 1.0f;
    if ((i.kind & 16u) != 0u) fade = saturate(1.0f - (i.pix.y - fReflBase) * fReflDir / max(fReflDepth, 1.0f));
    if (kb == 5u) return LineShade(i) * (fade * fSceneAlpha);
    if (kb == 6u) return BloomShade(i) * fSceneAlpha;
    if (kb == 7u) return SpecShade(i) * fSceneAlpha;
    if (kb == 2u) {
        float2 uv = float2((i.pix.x - i.shape.x) / max(1.0f, i.shape.z - i.shape.x),
                           (i.pix.y - i.shape.y) / max(1.0f, i.shape.w - i.shape.y));
        return gPlate.SampleLevel(gSamp, uv, 0.0f) * fSceneAlpha;
    }
    if (kb == 4u) {
        float fx = (i.pix.x - i.shape.x) / max(1.0f, i.shape.z - i.shape.x);
        float fy = (i.pix.y - i.shape.y) / max(1.0f, i.shape.w - i.shape.y);
        float2 uv = float2(i.radii.x + (i.radii.z - i.radii.x) * fx, i.radii.y + (i.radii.w - i.radii.y) * fy);
        float cov = gGlyphs.SampleLevel(gSamp, uv, 0.0f).w;
        return i.color * (cov * fSceneAlpha);
    }
    if ((i.kind & 32u) != 0u) return FxShade(i, kb) * (fade * fSceneAlpha);
    float cov = Coverage(i, kb);
    return i.color * (cov * fade * fSceneAlpha);
}

// ---- Bloom (FX, 2.1) ---------------------------------------------------------------
// The bars are drawn into a scene texture, which is box-filtered down to a
// quarter of its size, blurred there in two 9-tap Gaussian passes (five
// bilinear reads each), and added back over the scene as light. All four
// passes are one full-surface triangle and run only on frames that draw.
struct FxOut {
    float4 pos SEM(SV_Position);
    float2 uv SEM(TEXCOORD0);
};

FxOut VSFull(uint vid SEM(SV_VertexID)) {
    FxOut o;
    float u = (vid == 1u) ? 2.0f : 0.0f;
    float v = (vid == 2u) ? 2.0f : 0.0f;
    o.uv = float2(u, v);
    o.pos = float4(u * 2.0f - 1.0f, 1.0f - v * 2.0f, 0.0f, 1.0f);
    return o;
}

// Scene to quarter size: four bilinear reads cover the 4 x 4 block.
float4 PSBloomDown(FxOut i) SEM(SV_Target) {
    float tx = fFxTexel.z, ty = fFxTexel.w;
    float4 a = gFxSrc.SampleLevel(gLin, float2(i.uv.x - tx, i.uv.y - ty), 0.0f);
    float4 b = gFxSrc.SampleLevel(gLin, float2(i.uv.x + tx, i.uv.y - ty), 0.0f);
    float4 c = gFxSrc.SampleLevel(gLin, float2(i.uv.x - tx, i.uv.y + ty), 0.0f);
    float4 d = gFxSrc.SampleLevel(gLin, float2(i.uv.x + tx, i.uv.y + ty), 0.0f);
    return (a + b + c + d) * 0.25f;
}

float4 BloomBlur(float2 uv, float dx, float dy) {
    // Weights of a 9-tap Gaussian folded into 5 bilinear reads.
    float4 s = gFxSrc.SampleLevel(gLin, uv, 0.0f) * 0.2270270f;
    s = s + gFxSrc.SampleLevel(gLin, float2(uv.x + dx * 1.3846154f, uv.y + dy * 1.3846154f), 0.0f) * 0.3162162f;
    s = s + gFxSrc.SampleLevel(gLin, float2(uv.x - dx * 1.3846154f, uv.y - dy * 1.3846154f), 0.0f) * 0.3162162f;
    s = s + gFxSrc.SampleLevel(gLin, float2(uv.x + dx * 3.2307692f, uv.y + dy * 3.2307692f), 0.0f) * 0.0702703f;
    s = s + gFxSrc.SampleLevel(gLin, float2(uv.x - dx * 3.2307692f, uv.y - dy * 3.2307692f), 0.0f) * 0.0702703f;
    return s;
}

// The step between taps grows with Bloom Radius (in quarter-size texels).
float BloomStep() { return max(1.0f, fFxBloomR / 16.0f); }
float4 PSBloomH(FxOut i) SEM(SV_Target) { return BloomBlur(i.uv, fFxTexel.x * BloomStep(), 0.0f); }
float4 PSBloomV(FxOut i) SEM(SV_Target) { return BloomBlur(i.uv, 0.0f, fFxTexel.y * BloomStep()); }

// Scene plus its bloom as light, then blended "over" whatever is beneath
// (the plate). Premultiplied: the bloom adds colour and some coverage.
float4 PSBloomComposite(FxOut i) SEM(SV_Target) {
    float4 sc = gFxSrc.SampleLevel(gLin, i.uv, 0.0f);
    float4 bl = gFxBloom.SampleLevel(gLin, i.uv, 0.0f) * (fFxBloom * 1.5f);
    return float4(sc.x + bl.x, sc.y + bl.y, sc.z + bl.z, saturate(sc.w + bl.w * (1.0f - sc.w)));
}


// ---- Analysis on the GPU (Workload = GPU) ----------------------------------------
//
// The same maths as ttdsp, split into four dispatches:
//   CsFft    one group per tier that has new samples: window, real FFT
//            (N/2-point complex radix-2 in group shared memory, then the
//            real-input post-twiddle), power spectrum.
//   CsBands  one thread per band: bin sum or peak, dB with the band's offset,
//            display mapping, ballistics.
//   CsReduce one group: master peak, EQ-zone energies, beat onset, breathe
//            envelope, auto gain, dominant frequency.
//   CsShape  one thread per bar: Shape mapping, peak hold, motion for the
//            skip-unchanged-frames test.
// Nothing comes back to the CPU each frame except an 8-byte stats block, read
// two frames late without waiting.

GROUPSHARED float2 gsFft[2048];
GROUPSHARED float4 gsRed[256];
GROUPSHARED float4 gsRed2[256];

uint BitRev(uint v, uint bits) { return reversebits(v) >> (32u - bits); }

NUMTHREADS(256, 1, 1)
void CsFft(uint3 gid SEM(SV_GroupID), uint3 tid3 SEM(SV_GroupThreadID)) {
    uint tid = tid3.x;
    uint tier = (cTierList >> (2u * gid.x)) & 3u;
    uint n = cN, m = cHalf;
    uint base = tier * n;
    for (uint k = tid; k < m; k += 256u) {
        float x0 = gTierIn[base + 2u * k] * gWindow[2u * k];
        float x1 = gTierIn[base + 2u * k + 1u] * gWindow[2u * k + 1u];
        gsFft[BitRev(k, cLog2Half)] = float2(x0, x1);
    }
    BARRIER;
    for (uint len = 2u; len <= m; len <<= 1u) {
        uint halfLen = len >> 1u;
        for (uint j = tid; j < m / 2u; j += 256u) {
            uint grp = j / halfLen;
            uint pos = j - grp * halfLen;
            uint a = grp * len + pos;
            uint b = a + halfLen;
            float ang = -2.0f * TT_PI * (float)pos / (float)len;
            float wr = cos(ang), wi = sin(ang);
            float2 za = gsFft[a];
            float2 zb = gsFft[b];
            float vr = zb.x * wr - zb.y * wi;
            float vi = zb.x * wi + zb.y * wr;
            gsFft[a] = float2(za.x + vr, za.y + vi);
            gsFft[b] = float2(za.x - vr, za.y - vi);
        }
        BARRIER;
    }
    uint outBase = tier * (m + 1u);
    for (uint k2 = tid; k2 <= m; k2 += 256u) {
        uint ka = (k2 == m) ? 0u : k2;
        uint kb = (k2 == 0u) ? 0u : m - k2;
        float2 za = gsFft[ka];
        float2 zb = gsFft[kb];
        float br = zb.x, bi = -zb.y;
        float er = 0.5f * (za.x + br), ei = 0.5f * (za.y + bi);
        float dr = za.x - br, di = za.y - bi;
        float orr = 0.5f * di, oi = -0.5f * dr;
        float ang = -2.0f * TT_PI * (float)k2 / (float)n;
        float pr = cos(ang), pim = sin(ang);
        float re = er + pr * orr - pim * oi;
        float im = ei + pr * oi + pim * orr;
        gPower[outBase + k2] = re * re + im * im;
    }
}

float CurveOf(float x) {
    if (x <= 0.0f) return 0.0f;
    if (cCurve == 0u) return min(1.0f, (1.0f - exp(-2.0f * x)) / (1.0f - exp(-2.0f)));
    if (cCurve == 2u) return min(1.0f, pow(x, 0.6f));
    if (cCurve == 3u) return min(1.0f, x);
    float knee = 0.7f;
    return (x <= knee) ? x : knee + (1.0f - knee) * tanh((x - knee) / (1.0f - knee));
}

float Ballistic(float y, float x) {
    if (x > y) return y + (x - y) * (1.0f - exp(-cDt * 1000.0f / max(0.1f, cAttackMs)));
    if (cReleaseLinear != 0u) return max(x, y - cReleaseDbPerSec * cDt / max(1.0f, cRangeDb));
    return y + (x - y) * (1.0f - exp(-cDt * 1000.0f / max(0.1f, cReleaseMs)));
}

float DbOf(float p) { return 10.0f * log10(max(p, 1e-30f)); }

NUMTHREADS(64, 1, 1)
void CsBands(uint3 did SEM(SV_DispatchThreadID)) {
    uint b = did.x;
    if (b >= cNumBands) return;
    float4 d = gBandDesc[b];
    float4 off = gBandOff[b];
    uint tier = ((uint)d.x) & 3u;
    uint k0 = (uint)d.y, k1 = (uint)d.z;
    uint pb = tier * (cHalf + 1u);
    float db;
    if (cDetector == 1u) {
        float mx = 0.0f;
        for (uint k = k0; k <= k1; k++) mx = max(mx, gPower[pb + k]);
        db = DbOf(mx) + off.y;
    } else {
        float s;
        if (k0 == k1) s = gPower[pb + k0] * off.z;
        else {
            s = gPower[pb + k0] * off.z + gPower[pb + k1] * off.w;
            for (uint k = k0 + 1u; k < k1; k++) s += gPower[pb + k];
        }
        db = DbOf(s) + off.x;
    }
    float gain = cSensDb + gGlobalsOut[2].x;  // auto gain from the previous frame
    float x = clamp(CurveOf((db + gain - cFloorDb) / max(1.0f, cRangeDb)), 0.0f, 1.0f);
    float4 st = gBandState[b];
    st.x = Ballistic(st.x, x);
    st.y = db;
    gBandState[b] = st;
}

// Owned range of tier t for the dominant-frequency search: above the next
// tier's clean passband, below this one's.
void TierRange(uint t, OUT(float) lo, OUT(float) hi, OUT(float) fs) {
    fs = (t == 0u) ? cTierRate0 : ((t == 1u) ? cTierRate1 : cTierRate2);
    hi = (t == 0u) ? min(cFmax, 0.49f * fs) : 0.4f * fs;
    float nextFs = (t == 0u) ? cTierRate1 : ((t == 1u) ? cTierRate2 : 0.0f);
    lo = (nextFs > 0.0f) ? 0.4f * nextFs : cFmin;
}

NUMTHREADS(256, 1, 1)
void CsReduce(uint3 tid3 SEM(SV_GroupThreadID)) {
    uint tid = tid3.x;
    // Pass 1: master peak, zone energies, bass, loudest raw band (auto gain).
    float mx = 0.0f, zl = 0.0f, zm = 0.0f, zh = 0.0f, bass = 0.0f, rawMax = -300.0f;
    for (uint b = tid; b < cNumBands; b += 256u) {
        float4 st = gBandState[b];
        uint zone = ((uint)gBandDesc[b].x) >> 2u;
        mx = max(mx, st.x);
        if (zone == 0u) zl += st.x;
        else if (zone == 1u) zm += st.x;
        else zh += st.x;
        if (zone == 0u && gBandDesc[b].w < 150.0f) bass = max(bass, st.x);
        rawMax = max(rawMax, st.y);
    }
    gsRed[tid] = float4(mx, zl, zm, zh);
    gsRed2[tid] = float4(bass, rawMax, 0.0f, 0.0f);
    BARRIER;
    for (uint s = 128u; s > 0u; s >>= 1u) {
        if (tid < s) {
            float4 a = gsRed[tid], c = gsRed[tid + s];
            gsRed[tid] = float4(max(a.x, c.x), a.y + c.y, a.z + c.z, a.w + c.w);
            float4 a2 = gsRed2[tid], c2 = gsRed2[tid + s];
            gsRed2[tid] = float4(max(a2.x, c2.x), max(a2.y, c2.y), 0.0f, 0.0f);
        }
        BARRIER;
    }
    float4 red = gsRed[0];
    float4 red2 = gsRed2[0];
    BARRIER;

    // Pass 2: dominant frequency. Each thread keeps its best local maximum.
    float bestDb = -1e9f;
    float bestBin = 0.0f;
    float bestTier = 0.0f;
    if (cDomOn != 0u) {
        for (uint t = 0u; t < 3u; t++) {
            float lo, hi, fs;
            TierRange(t, lo, hi, fs);
            if (fs <= 0.0f) continue;
            if (t > 0u && ((cTierCount >> (8u + t)) & 1u) == 0u) continue;  // tier unused
            float df = fs / (float)cN;
            uint k0 = max(2u, (uint)ceil(lo / df));
            uint k1 = min(cHalf - 2u, (uint)floor(hi / df));
            uint pb = t * (cHalf + 1u);
            for (uint k = k0 + tid; k <= k1; k += 256u) {
                float p = gPower[pb + k];
                if (p >= gPower[pb + k - 1u] && p >= gPower[pb + k + 1u]) {
                    float dbv = DbOf(p) + cPeakCal;
                    if (dbv > bestDb) { bestDb = dbv; bestBin = (float)k; bestTier = (float)t; }
                }
            }
        }
    }
    gsRed[tid] = float4(bestDb, bestBin, bestTier, 0.0f);
    BARRIER;
    for (uint s2 = 128u; s2 > 0u; s2 >>= 1u) {
        if (tid < s2) {
            float4 a = gsRed[tid], c = gsRed[tid + s2];
            if (c.x > a.x) gsRed[tid] = c;
        }
        BARRIER;
    }

    if (tid == 0u) {
        float4 g0 = gGlobalsOut[0];
        float4 g2 = gGlobalsOut[2];
        // Beat: the bass envelope pulling ahead of its own 60 ms average.
        float fast = red2.x;
        float slow = g2.z + (fast - g2.z) * (1.0f - exp(-cDt / 0.06f));
        float pulse = max(0.0f, g0.x - 4.8f * cDt);
        if (fast - slow > 0.12f) pulse = 1.0f;
        // Breathe envelope, as the Direct2D path eases it, from elapsed time.
        float env = g0.z;
        float tau = (red.x > env) ? cBreatheUp : cBreatheDown;
        env += (red.x - env) * (1.0f - exp(-cDt / max(0.01f, tau)));
        // Auto gain: lift the loudest band toward 85% of the range, boost only,
        // frozen through silence.
        float ag = g2.x;
        if (cAutoGain != 0u) {
            float loud = red2.y + cSensDb;
            float target = cFloorDb + 0.85f * cRangeDb;
            if (loud > cFloorDb + 0.1f * cRangeDb) {
                float want = clamp(target - loud, 0.0f, cAutoGainMaxDb);
                ag += (want - ag) * (1.0f - exp(-cDt / 0.4f));
            }
        } else {
            ag = 0.0f;
        }
        gGlobalsOut[0] = float4(pulse, red.x, env, 0.0f);
        float4 g1 = gGlobalsOut[1];
        gGlobalsOut[1] = float4(red.y, red.z, red.w, g1.w);
        gGlobalsOut[2] = float4(ag, 0.0f, slow, 0.0f);
        float hz = 0.0f;
        float4 best = gsRed[0];
        if (cDomOn != 0u && best.x > -90.0f) {
            uint t = (uint)best.z;
            uint k = (uint)best.y;
            float lo, hi, fs;
            TierRange(t, lo, hi, fs);
            uint pb = t * (cHalf + 1u);
            float a = DbOf(gPower[pb + k - 1u]), bq = DbOf(gPower[pb + k]), c = DbOf(gPower[pb + k + 1u]);
            float den = a - 2.0f * bq + c;
            float delta = (abs(den) > 1e-9f) ? clamp(0.5f * (a - c) / den, -0.5f, 0.5f) : 0.0f;
            hz = ((float)k + delta) * fs / (float)cN;
        }
        gStats[0] = asuint(hz);
        gStats[1] = 0u;
    }
}

float SampleBands(float t) {
    float pos = clamp(t, 0.0f, 1.0f) * (float)(cNumBands - 1u);
    uint lo = (uint)pos;
    uint hi = min(lo + 1u, cNumBands - 1u);
    float f = pos - (float)lo;
    return gBandState[lo].x * (1.0f - f) + gBandState[hi].x * f;
}

NUMTHREADS(64, 1, 1)
void CsShape(uint3 did SEM(SV_DispatchThreadID)) {
    uint i = did.x;
    if (i >= cNumBars) return;
    uint n = cNumBars;
    float master = gGlobalsOut[0].y;
    float freqT = (n > 1u) ? (float)i / (float)(n - 1u) : 0.5f;
    float center = (float)(n - 1u) * 0.5f;
    float target = 0.0f;
    uint shape = cShape;
    if (shape == 1u) {  // Mountain
        float dist = abs((float)i - center) / max(1.0f, center);
        float e = SampleBands(dist);
        target = (e + master * (0.2f - dist * 0.12f)) * (1.6f - dist * 0.9f);
    } else if (shape == 2u) {  // Mirror
        float mirT = 1.0f - abs((float)i - center) / max(1.0f, center);
        target = (SampleBands(mirT) + master * (0.1f + mirT * 0.12f)) * 1.3f;
    } else if (shape == 3u) {  // Wave
        target = SampleBands(freqT) * gShapeMod[i] + master * 0.15f;
    } else if (shape == 4u) {  // Breathe
        target = gShapeMod[i] * (0.12f + gGlobalsOut[0].z * 0.88f);
    } else if (shape == 7u || shape == 8u) {
        target = 0.0f;
    } else {
        target = SampleBands(freqT);
    }
    target = clamp(target, 0.0f, 1.0f);

    float4 aux = gBarAux[i];  // peak, timer, velocity, previous level
    float pk = aux.x, timer = aux.y, vel = aux.z;
    if (target >= pk) { pk = target; timer = 0.0f; vel = 0.0f; }
    else {
        timer += cDt;
        if (timer * 1000.0f >= cPeakHoldMs) {
            if (cPeakGravity != 0u) { vel += cGravity * cDt; pk -= vel * cDt; }
            else pk -= cLinFall * cDt;
            if (pk < target) { pk = target; vel = 0.0f; }
            pk = max(pk, 0.0f);
        }
    }
    float moved = max(abs(target - aux.w), abs(pk - gBarsOut[i].y)) * cBarRangePx;
    gBarAux[i] = float4(pk, timer, vel, target);
    gBarsOut[i] = float2(target, pk);
    InterlockedMax(gStats[1], asuint(moved));
}
