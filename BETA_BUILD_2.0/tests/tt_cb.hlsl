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
