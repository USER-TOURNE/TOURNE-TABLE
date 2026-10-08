// ---- Resources -----------------------------------------------------------------

// Drawing (vertex / pixel shaders).
StructuredBuffer<float2> gBars REG(t0);     // per bar: level 0..1, peak cap 0..1
StructuredBuffer<float4> gGlobals REG(t1);  // [0] beat pulse, master peak, breathe env, -
                                            // [1] EQ-zone energy low, mid, high, -
StructuredBuffer<float> gWave REG(t2);      // oscilloscope trace, 256 samples, -1..1
StructuredBuffer<float4> gPoints REG(t3);   // goniometer: side, mid, alpha, -
Texture2D<float4> gPlate REG(t4);           // baked background panel, premultiplied
SamplerState gSamp REG(s0);

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
    uint kind;    // 0 rounded rect, 1 capsule, 2 textured plate, 3 skip
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

Prim BarPrim(uint i) {
    if (i >= fBarCount) return NoPrim();
    float fac = max(0.0f, gBars[i].x);
    float size = fIdleSize + fac * BarRange();
    float lead = (float)i * (fBarW + fBarGap);
    float4 col = BarColor(i, fac, false);
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

Prim BuildPrim(uint pass, uint id) {
    if (pass == 0u) {
        Prim p = RectPrim(fPlateRect.x, fPlateRect.y, fPlateRect.z, fPlateRect.w,
                          float4(0.0f, 0.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 1.0f));
        if (p.kind == 0u) p.kind = 2u;
        return p;
    }
    if (pass == 1u) return BarPrim(id);
    if (pass == 2u) return CapPrim(id);
    if (pass == 3u) return DotPrim(id);
    if (pass == 4u) return RadialPrim(id);
    if (pass == 5u) return ScopePrim(id);
    if (pass == 6u) return GonioPrim(id);
    if (pass == 7u) return CorrPrim(id);
    return NoPrim();
}

struct VsOut {
    float4 pos SEM(SV_Position);
    float2 pix SEM(TEXCOORD0);
    NOINTERP float4 shape SEM(TEXCOORD1);
    NOINTERP float4 radii SEM(TEXCOORD2);
    NOINTERP float4 color SEM(TEXCOORD3);
    NOINTERP uint kind SEM(TEXCOORD4);
};

// Quad corner `vid` (triangle strip 0..3) of the primitive's bounds. The
// bounds reach one pixel past the shape so its antialiased edge is covered;
// the plate is drawn exactly 1:1.
VsOut EmitVertex(Prim p, uint vid) {
    VsOut o;
    float2 lo, hi;
    if (p.kind == 1u) {
        float r = p.radii.x + 1.0f;
        lo = float2(min(p.a.x, p.a.z) - r, min(p.a.y, p.a.w) - r);
        hi = float2(max(p.a.x, p.a.z) + r, max(p.a.y, p.a.w) + r);
    } else if (p.kind == 2u) {
        lo = float2(p.a.x, p.a.y);
        hi = float2(p.a.z, p.a.w);
    } else {
        lo = float2(p.a.x - 1.0f, p.a.y - 1.0f);
        hi = float2(p.a.z + 1.0f, p.a.w + 1.0f);
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
    return EmitVertex(BuildPrim(pPass, iid), vid);
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

float Coverage(VsOut i) {
    if (i.kind == 1u) return saturate(0.5f - SdCapsule(i.pix, i.shape, i.radii.x));
    return RectCoverage(i.pix, i.shape, i.radii);
}

float4 PSMain(VsOut i) SEM(SV_Target) {
    if (i.kind == 2u) {
        float2 uv = float2((i.pix.x - i.shape.x) / max(1.0f, i.shape.z - i.shape.x),
                           (i.pix.y - i.shape.y) / max(1.0f, i.shape.w - i.shape.y));
        return gPlate.SampleLevel(gSamp, uv, 0.0f) * fSceneAlpha;
    }
    return i.color * (Coverage(i) * fSceneAlpha);
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
    BARRIER();
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
        BARRIER();
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
    BARRIER();
    for (uint s = 128u; s > 0u; s >>= 1u) {
        if (tid < s) {
            float4 a = gsRed[tid], c = gsRed[tid + s];
            gsRed[tid] = float4(max(a.x, c.x), a.y + c.y, a.z + c.z, a.w + c.w);
            float4 a2 = gsRed2[tid], c2 = gsRed2[tid + s];
            gsRed2[tid] = float4(max(a2.x, c2.x), max(a2.y, c2.y), 0.0f, 0.0f);
        }
        BARRIER();
    }
    float4 red = gsRed[0];
    float4 red2 = gsRed2[0];
    BARRIER();

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
    BARRIER();
    for (uint s2 = 128u; s2 > 0u; s2 >>= 1u) {
        if (tid < s2) {
            float4 a = gsRed[tid], c = gsRed[tid + s2];
            if (c.x > a.x) gsRed[tid] = c;
        }
        BARRIER();
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
