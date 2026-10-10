// ---- Constant buffers --------------------------------------------------------
// Mirrored field for field by VizGfxFrameCB / VizGfxPassCB / VizGfxCsCB on the
// C++ side; every row is 16 bytes so HLSL packing and C++ layout agree.

TT_CBUFFER(FrameCB, b0) {
    float2 fViewport;  // swap chain size, px
    float2 fBlock;     // bar group top-left, px
    float fBarW, fBarGap, fMaxSize, fIdleSize;
    float4 fRadii;     // bar corners TL TR BR BL, px
    float4 fDotRadii;  // dot corners TL TR BR BL, px
    uint fBarCount, fShape, fVertical, fAnchor;     // anchor: 0 top, 1 middle, 2 bottom
    uint fColorMode, fFlags, fMaxDots, fDotSlots;  // flags: 1 peak caps, 2 beat flash, 4 multiband scope
    float4 fC1, fGrad1, fC2, fPeakColor, fBeatColor;  // straight alpha, 0..1
    float fBeatIntensity, fRainbowBase, fSceneAlpha, fCapThickness;
    float fDotStep, fDotR, fInnerR, fStrokeW;
    float2 fCenter;
    float fAmpScale, fSweepLen;
    float fScopeCenter, fSweepOrigin, fWStep, fGonioR;
    float4 fPlateRect;
    float4 fScopeColor;
    float fGonioDot, fCorrY, fCorrH, fCorr;
    // Terminal shape: palette (dim, low, high, label, peak), grid origin and
    // cell size in px, grid and glyph-atlas dimensions.
    float4 fTermColors[5];
    float4 fTermGeom;   // origin x, origin y, cell width, cell height
    float4 fTermAtlas;  // atlas width, atlas height, -, -
    uint fTermCols, fTermRows, fTermAtlasCols, fTermPad;
    // Styles (2.1): which one (0 none, 1 LED, 2 Line, 3 Bloom, 4 Spectrogram,
    // 5 VU, 6 Stereo Field, 7 Particles), LED segments per bar, Line
    // subdivisions per bar gap, Spectrogram history rows in use.
    uint fStyle, fSegs, fSubdiv, fSpecRows;
    // Reflection: base line y, direction (+1 down), depth px, start opacity.
    float fReflBase, fReflDir, fReflDepth, fReflAlpha;
    // LED segment pitch and height, Line glow radius and fill opacity.
    float fSegStep, fSegH, fGlowR, fFillA;
    float4 fVu;     // VU needle L, R (0..1 of the scale), peak LED L, R (0..1)
    float4 fVuBox;  // one meter's width, height, offset of the second meter x, y
    uint fSpecW, fSpecHead, fSpecTex, fSpecPad;  // bars per row, newest row, rows in the texture
    // FX (2.1): Glow strength 0..1 and radius px, Bloom strength 0..1 and
    // radius px; texel size of the quarter-res bloom target and of the scene.
    float fFxGlow, fFxGlowR, fFxBloom, fFxBloomR;
    float4 fFxTexel;
    // Outline and Shadow (FX, 2.1): outline colour (straight alpha), shadow
    // colour (alpha = strength), then outline width, shadow offset x, y and
    // softness, all px.
    float4 fFxLineColor;
    float4 fFxShadowColor;
    float fFxLineW, fFxShadowX, fFxShadowY, fFxShadowSoft;
}
TT_CBUFFER_END

TT_CBUFFER(PassCB, b1) {
    uint pPass, pCount, pPad0, pPad1;
}
TT_CBUFFER_END

TT_CBUFFER(CsCB, b2) {
    uint cN, cHalf, cLog2Half, cNumBands;
    uint cNumBars, cTierCount, cTierList, cDetector;  // tier list: 2 bits per dispatched group;
                                                     // tier count: bits 8-10 = tiers in use
    float cFloorDb, cRangeDb, cSensDb, cDt;
    float cAttackMs, cReleaseMs, cReleaseDbPerSec, cAutoGainMaxDb;
    uint cReleaseLinear, cCurve, cShape, cAutoGain;
    float cPeakHoldMs, cGravity, cLinFall, cBarRangePx;
    uint cPeakGravity, cDomOn, cPad1, cPad2;
    float cTierRate0, cTierRate1, cTierRate2, cPeakCal;
    float cFmin, cFmax, cBreatheUp, cBreatheDown;  // breathe env taus, seconds
}
TT_CBUFFER_END
