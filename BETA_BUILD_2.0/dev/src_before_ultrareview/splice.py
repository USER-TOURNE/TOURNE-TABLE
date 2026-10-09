"""Builds the 2.0.0 beta source from the 1.5.0 one. Every edit anchors on text
that must occur exactly once (or an exact stated number of times)."""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
S = os.path.dirname(HERE)
src = open(os.path.join(S, "base15.cpp"), encoding="utf-8").read()


def rep(old, new, count=1):
    global src
    n = src.count(old)
    if n != count:
        sys.exit(f"anchor count {n} != {count}:\n{old[:300]}")
    src = src.replace(old, new)


def between(start, end, new, keep_end=True):
    """Replace from `start` (inclusive) up to `end` (exclusive if keep_end)."""
    global src
    a = src.find(start)
    assert a >= 0 and src.count(start) == 1, start[:120]
    b = src.find(end, a)
    assert b >= 0, end[:120]
    if not keep_end:
        b += len(end)
    src = src[:a] + new + src[b:]


def read(name):
    return open(os.path.join(HERE, name), encoding="utf-8").read()


# ---------------------------------------------------------------- header
rep("// @id                  tourne-table-beta-build",
    "// @id                  tourne-table-desktop-audio-visualizer-scope")
rep("// @name                Tourne'Table [Audio Visualizer] (Scope Test)",
    "// @name                Tourne'Table [Audio Visualizer] (Beta 2.0)")
rep("""// @description         TEST BUILD. Tourne'Table with the Oscilloscope time base decoupled from FFT Size, plus a Hardware section: pick the GPU, CPU or NPU, and a Smooth Mode built for integrated GPUs. Installs alongside the release build. Not for publishing.""",
    """// @description         BETA BUILD. Tourne'Table 2.0: a precision analysis core (every bar its own frequency band, IEC 61260 / musical bands, A/C weighting, real ballistics, LUFS and true peak), a choice of where the work runs (GPU, CPU or Hybrid), and a Direct3D 11 renderer that skips unchanged frames and idles to nothing. Installs alongside the release build. Not for publishing.""")
rep("// @version             1.5.0", "// @version             2.0.0")

# ---------------------------------------------------------------- settings YAML
rep("""        - radial: Radial
        - oscilloscope: Oscilloscope
    - orientation: horizontal""", """        - radial: Radial
        - oscilloscope: Oscilloscope
        - goniometer: Goniometer (stereo field)
    - orientation: horizontal""")
rep("""    - fftSize: '1024'
      $name: FFT Size
      $description: Higher values give finer frequency detail at a small extra CPU cost""",
    """    - fftSize: '2048'
      $name: FFT Size
      $description: Higher values give finer frequency detail at a small extra CPU cost. With the Precision engine this is the size of each of its three resolution tiers, so bass detail comes from the tiers rather than from raising this; 2048 is the sweet spot. Workload GPU uses at most 4096""")
rep("""        - log: Log (natural, matches previous behavior)
        - linear: Linear (Hz-even spacing)
        - mel: Mel (perceptual pitch spacing)""", """        - log: Log (natural, matches previous behavior)
        - linear: Linear (Hz-even spacing)
        - mel: Mel (perceptual pitch spacing)
        - bark: Bark (critical bands, Traunmueller)
        - erb: ERB (auditory filter bandwidths, Glasberg and Moore)""")

ANALYSIS_YAML = read("yaml_analysis.txt")
rep("""  $name: Appearance
- position:""", "  $name: Appearance\n" + ANALYSIS_YAML + "- position:")

rep("""    - targetFps: 60
      $name: Target FPS
      $description: Caps how often the visualizer redraws itself. Match your monitor's refresh rate for the smoothest motion at the lowest overhead""",
    """    - targetFps: 60
      $name: Target FPS
      $description: Caps how often the visualizer redraws itself. 0 matches your display's refresh rate, whatever it is. With the Direct3D 11 renderer a frame where nothing moved is skipped entirely, so a high target costs far less than it used to during quiet or steady passages""")
rep("""    - pauseWhenSilentSeconds: 10
      $name: Pause When Silent (seconds)
      $description: 0 disables this behavior""", """    - pauseWhenSilentSeconds: 10
      $name: Pause When Silent (seconds)
      $description: After this long without audio, drawing drops to a trickle that presents nothing unless something actually changes. 0 disables this behavior, and Deep Idle with it
    - deepIdle: true
      $name: Deep Idle
      $description: Five seconds after Pause When Silent kicks in, stops the audio stream itself and watches Windows' own peak meter four times a second instead, restarting the moment anything plays. A running capture stream registers an audio power request, which can keep the PC from sleeping; a stopped one doesn't. Turn off only if the visualizer is slow to wake for a very quiet source""")

HW_YAML = read("yaml_hardware.txt")
between("- hardware:\n", "- validation:\n", HW_YAML)

# ---------------------------------------------------------------- includes

# ---------------------------------------------------------------- enums + settings
rep("enum class VizShape { Stereo, Mountain, Mirror, Wave, Breathe, Dots, Radial, Oscilloscope };",
    "enum class VizShape { Stereo, Mountain, Mirror, Wave, Breathe, Dots, Radial, Oscilloscope, Goniometer };")
rep("enum class VizFreqScale { Log, Linear, Mel };", "enum class VizFreqScale { Log, Linear, Mel, Bark, Erb };")
rep("""enum class VizRenderDevice { Auto, Integrated, Discrete, Cpu };
enum class VizAnalysisDevice { Cpu, Npu };
enum class VizSmoothMode { Auto, On, Off };""", """enum class VizRenderDevice { Auto, Integrated, Discrete, Cpu };
enum class VizSmoothMode { Auto, On, Off };
// 2.0. Where the work runs; which analysis core; which renderer.
enum class VizWorkload { Hybrid, Gpu, Cpu, Npu };
enum class VizRenderer { D3D11, Direct2D };
enum class VizOpaquePanel { Auto, Off };
enum class VizEngineKind { Precision, Classic };
enum class VizBandLayout { Scale, Iec, Musical };
enum class VizWeighting { Z, A, C };
enum class VizDetector { Rms, Peak };
enum class VizLevelRef { ThirdOctave, Band };
enum class VizWindowKind { Hann, Hamming, BlackmanHarris, FlatTop };
enum class VizChannel { Mix, Left, Right, Mid, Side };
enum class VizBallisticsPreset { Snappy, Smooth, Analyzer, Vu, PpmEbu, PpmDin, Custom };
enum class VizPeakFall { Gravity, Linear };
enum class VizReadout { Frequency, Loudness, LoudnessFull, Both };""")
rep("""    VizRenderDevice renderDevice = VizRenderDevice::Auto;
    VizAnalysisDevice analysisDevice = VizAnalysisDevice::Cpu;
    std::wstring npuRuntimePath;
    VizSmoothMode smoothMode = VizSmoothMode::Auto;
};""", """    VizRenderDevice renderDevice = VizRenderDevice::Auto;
    std::wstring npuRuntimePath;
    VizSmoothMode smoothMode = VizSmoothMode::Auto;
    VizWorkload workload = VizWorkload::Hybrid;
    VizRenderer renderer = VizRenderer::D3D11;
    VizOpaquePanel opaquePanel = VizOpaquePanel::Auto;
    bool deepIdle = true;

    // Analysis (2.0).
    VizEngineKind engine = VizEngineKind::Precision;
    VizBandLayout bandLayout = VizBandLayout::Scale;
    int octaveFraction = 6;
    int minFreq = 20, maxFreq = 20000;
    float tuningA4 = 440.f;
    VizWeighting weighting = VizWeighting::Z;
    float tiltDbPerOct = 1.5f;
    VizDetector detector = VizDetector::Rms;
    VizLevelRef levelRef = VizLevelRef::ThirdOctave;
    VizWindowKind window = VizWindowKind::Hann;
    int bassDetail = 2;
    VizChannel channel = VizChannel::Mix;
    int dbFloor = -72, dbCeiling = -12;
    VizBallisticsPreset ballistics = VizBallisticsPreset::Snappy;
    int attackMs = 10, releaseDbPerSec = 20;
    int peakHoldMs = 500;
    VizPeakFall peakFall = VizPeakFall::Gravity;
    VizReadout readout = VizReadout::Frequency;
    bool loudnessResetOnTrack = true;
    int layoutBandCount = 0;  // bars implied by an IEC / musical layout, 0 = use Bar Count
};""")

# Composition root (2.0: two surfaces share it).
rep("""ComPtr<IDCompositionVisual> g_compositionVisual;""", """ComPtr<IDCompositionVisual> g_compositionVisual;
// Since 2.0 the target's root is an empty container with the text surface
// (g_compositionVisual) as a child, so the Direct3D 11 renderer can slide its
// panel surface in underneath it.
ComPtr<IDCompositionVisual> g_rootVisual;""")

# ---------------------------------------------------------------- ttdsp
rep("""// ---- NPU audio analysis (Audio Analysis Device = NPU) -------------------------""",
    read("ttdsp.h") + "\n" + """// ---- NPU audio analysis (Workload = NPU) ----------------------------------------""")

# ---------------------------------------------------------------- engine (replaces the capture thread)
between("bool VizInitAudioClient(IMMDeviceEnumerator* pEnum, ComPtr<IAudioClient>& pClient,",
        "void UpdateVisualizerTargets() {", read("part_engine.cpp") + "\n")

# UpdateVisualizerTargets (classic): effective bar count, Bark / ERB warps.
rep("""void UpdateVisualizerTargets() {
    const int vizBars = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));""",
    """int VizEffectiveBarCount();

void UpdateVisualizerTargets() {
    const int vizBars = VizEffectiveBarCount();""")
rep("""            case VizFreqScale::Mel: {
                auto melOf = [](float f) { return 2595.f * log10f(1.f + f / 700.f); };
                float melMin = melOf(20.f), melMax = melOf(20000.f);
                float mel = melMin + pos * (melMax - melMin);
                float hz = 700.f * (powf(10.f, mel / 2595.f) - 1.f);
                return HzToBandPos(hz) / (float)(VIZ_NUM_BANDS - 1);
            }""", """            case VizFreqScale::Mel: {
                auto melOf = [](float f) { return 2595.f * log10f(1.f + f / 700.f); };
                float melMin = melOf(20.f), melMax = melOf(20000.f);
                float mel = melMin + pos * (melMax - melMin);
                float hz = 700.f * (powf(10.f, mel / 2595.f) - 1.f);
                return HzToBandPos(hz) / (float)(VIZ_NUM_BANDS - 1);
            }
            case VizFreqScale::Bark:
            case VizFreqScale::Erb: {
                ttdsp::FreqScale s = (g_settings.freqScale == VizFreqScale::Bark) ? ttdsp::FreqScale::Bark
                                                                                   : ttdsp::FreqScale::Erb;
                double u0 = ttdsp::ScaleFwd(s, 20.0), u1 = ttdsp::ScaleFwd(s, 20000.0);
                float hz = (float)ttdsp::ScaleInv(s, u0 + pos * (u1 - u0));
                return HzToBandPos(hz) / (float)(VIZ_NUM_BANDS - 1);
            }""")
rep("""            case VizShape::Oscilloscope:
                target = 0.f;  // drawn directly from the raw waveform buffer, not per-bar targets
                break;""", """            case VizShape::Oscilloscope:
            case VizShape::Goniometer:
                target = 0.f;  // drawn directly from the raw waveform buffer, not per-bar targets
                break;""")

rep("""struct RGBA { BYTE a, r, g, b; };""", read("part_bars.cpp") + "\nstruct RGBA { BYTE a, r, g, b; };")

# ---------------------------------------------------------------- device lifetime hooks
rep("""void ReleaseSwapChainResources();
bool CreateSwapChainResources();
void RenderVisualizer();""", """void ReleaseSwapChainResources();
bool CreateSwapChainResources();
void RenderVisualizer();
namespace ttgfx {
void ReleaseDevice();
void ReleaseSurface();
void OnSettingsChanged();
void PresentBlank();
}  // namespace ttgfx""")
rep("""    ReleaseSwapChainResources();
    g_roundCapStrokeStyle.Reset();
    g_d2dDevice.Reset();
    g_d2dFactory.Reset();
    g_dxgiDevice.Reset();
    g_d3dDevice.Reset();
    g_dxgiFactory.Reset();

    if (!InitDirectX()) {""", """    ReleaseSwapChainResources();
    ttgfx::ReleaseDevice();
    g_roundCapStrokeStyle.Reset();
    g_d2dDevice.Reset();
    g_d2dFactory.Reset();
    g_dxgiDevice.Reset();
    g_d3dDevice.Reset();
    g_dxgiFactory.Reset();

    if (!InitDirectX()) {""")
rep("""void UninitDirectX() {
    g_dwriteTextFormat.Reset();""", """void UninitDirectX() {
    ttgfx::ReleaseDevice();
    g_dwriteTextFormat.Reset();""")

# Workload CPU draws through WARP, whatever Drawing Device says.
rep("""VizAdapterChoice VizPickRenderAdapter(IDXGIFactory1* factory, VizRenderDevice want,
                                      HMONITOR monitor) {
    VizAdapterChoice choice;
    if (want == VizRenderDevice::Cpu) {""", """VizAdapterChoice VizPickRenderAdapter(IDXGIFactory1* factory, VizRenderDevice want,
                                      HMONITOR monitor) {
    VizAdapterChoice choice;
    if (want == VizRenderDevice::Cpu || g_settings.workload == VizWorkload::Cpu) {""")

# ---------------------------------------------------------------- layout: bar count, square shapes, wide readout
rep("""bool ComputeVizLayout(VizLayout* out) {
    if (!out) return false;

    int barCount  = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));""", """bool ComputeVizLayout(VizLayout* out) {
    if (!out) return false;

    int barCount  = VizEffectiveBarCount();""")
rep("""    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }

    HMONITOR monitor = g_cachedMonitor;""", """    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial || g_settings.shape == VizShape::Goniometer) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }

    HMONITOR monitor = g_cachedMonitor;""")
rep("""        textAnchorSide = std::max(textAnchorSide, 60.0f * g_dpiScale);
        extraSide = std::max(extraSide, pfOffX);""", """        textAnchorSide = std::max(textAnchorSide, 60.0f * g_dpiScale);
        // A loudness readout is a whole line of figures: reserve enough either
        // side for it to sit centred on the bars without being clipped.
        float wide = VizReadoutWidthEstimate();
        if (wide > 0.f) textAnchorSide = std::max(textAnchorSide, (wide - totalWidth) * 0.5f + 8.0f * g_dpiScale);
        extraSide = std::max(extraSide, pfOffX);""")
rep("""    int barCount = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
    float barW = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    float barGap = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;
    bool horizontal = (g_settings.orientation == VizOrientation::Horizontal);
    float barsThickness = barCount * barW + (barCount - 1) * barGap;
    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {""", """    int barCount = VizEffectiveBarCount();
    float barW = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    float barGap = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;
    bool horizontal = (g_settings.orientation == VizOrientation::Horizontal);
    float barsThickness = barCount * barW + (barCount - 1) * barGap;
    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial || g_settings.shape == VizShape::Goniometer) {""")

# ComputeVizLayout reads VizReadoutWidthEstimate, defined with the text helpers later.
rep("""// Geometry of the visualizer, resolved once and used both to size the swap
// chain and to draw into it.""", """float VizReadoutWidthEstimate();

// Geometry of the visualizer, resolved once and used both to size the swap
// chain and to draw into it.""")

# ---------------------------------------------------------------- composition root
rep("""    hr = g_compositionDevice->CreateVisual(&g_compositionVisual);
    if (FAILED(hr)) return false;""", """    hr = g_compositionDevice->CreateVisual(&g_rootVisual);
    if (FAILED(hr)) return false;

    hr = g_compositionDevice->CreateVisual(&g_compositionVisual);
    if (FAILED(hr)) return false;

    hr = g_rootVisual->AddVisual(g_compositionVisual.Get(), TRUE, nullptr);
    if (FAILED(hr)) return false;""")
rep("""    hr = g_compositionTarget->SetRoot(g_compositionVisual.Get());""",
    """    hr = g_compositionTarget->SetRoot(g_rootVisual.Get());""")
rep("""void ReleaseSwapChainResources() {
    ReleaseVisualResources();
    g_spriteBatch.Reset();
    g_dc3.Reset();
    g_compositionVisual.Reset();""", """void ReleaseSwapChainResources() {
    ReleaseVisualResources();
    ttgfx::ReleaseSurface();
    g_spriteBatch.Reset();
    g_dc3.Reset();
    g_compositionVisual.Reset();
    g_rootVisual.Reset();""")

# Wallpaper capture clears the panel surface too.
rep("""    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    g_dc->EndDraw();
    g_swapChain->Present(1, 0);
    DwmFlush();""", """    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    g_dc->EndDraw();
    g_swapChain->Present(1, 0);
    ttgfx::PresentBlank();
    DwmFlush();""")

# Settings / wallpaper changes invalidate the renderer's baked plate.
rep("""    g_npLayoutCache.Reset();
    g_pfLayoutCache.Reset();
}""", """    g_npLayoutCache.Reset();
    g_pfLayoutCache.Reset();
    ttgfx::OnSettingsChanged();
}""")

# ---------------------------------------------------------------- DrawOverlayText: no-wrap option
rep("""void DrawOverlayText(PCWSTR text, UINT32 len, const D2D1_RECT_F& box, ID2D1Brush* textBrush,
                     const TextPanelStyle& style, float fadeAlpha,
                     VizTextLayoutCache* cache = nullptr) {""", """void DrawOverlayText(PCWSTR text, UINT32 len, const D2D1_RECT_F& box, ID2D1Brush* textBrush,
                     const TextPanelStyle& style, float fadeAlpha,
                     VizTextLayoutCache* cache = nullptr, bool noWrap = false) {""")
rep("""            if (SUCCEEDED(g_dwriteFactory->CreateTextLayout(text, len, g_dwriteTextFormat.Get(), boxW,
                                                            boxH, &cache->layout)) &&
                cache->layout) {
                cache->text.assign(text, len);""", """            if (SUCCEEDED(g_dwriteFactory->CreateTextLayout(text, len, g_dwriteTextFormat.Get(), boxW,
                                                            boxH, &cache->layout)) &&
                cache->layout) {
                // A readout line is never broken over two lines: the box is
                // only one line tall, so a wrapped second line would be lost.
                if (noWrap) cache->layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                cache->text.assign(text, len);""")

# ---------------------------------------------------------------- renderer + shared helpers, before RenderVisualizer
SHADER = read("tt_prelude.hlsl") + read("tt_cb.hlsl") + read("tt_body.hlsl")
assert ")HLSL\"" not in SHADER
gfx = read("part_gfx.cpp").replace("/*@@SHADER_SOURCE@@*/", 'R"HLSL(' + SHADER + ')HLSL"')
rep("""void RenderVisualizer() {
    if (g_unloading || !g_dc || !g_swapChain) return;""", gfx + "\n" + read("part_text.cpp") + """
void RenderVisualizer() {
    if (g_unloading || !g_dc || !g_swapChain) return;""")

# Frame dt alongside the frame scale.
rep("""        g_frameScale = 1.0f;
        if (smooth && s_prevFrameQpc) {
            double dtMs = (double)(q.QuadPart - s_prevFrameQpc) * 1000.0 / (double)VizQpcFreq();
            double refMs = 1000.0 / (double)std::max(1, g_settings.targetFps);
            g_frameScale = (float)std::clamp(dtMs / refMs, 0.25, 4.0);
        }
        s_prevFrameQpc = q.QuadPart;""", """        g_frameScale = 1.0f;
        double dtMs = s_prevFrameQpc ? (double)(q.QuadPart - s_prevFrameQpc) * 1000.0 / (double)VizQpcFreq()
                                     : 1000.0 / 60.0;
        if (smooth && s_prevFrameQpc) {
            double refMs = 1000.0 / (double)std::max(1, g_settings.targetFps > 0 ? g_settings.targetFps : 60);
            g_frameScale = (float)std::clamp(dtMs / refMs, 0.25, 4.0);
        }
        // Seconds, for everything that runs on elapsed time (precision peak
        // caps, the breathe envelope, GPU ballistics). Long gaps (idle,
        // pause) are clamped so nothing jumps when drawing resumes.
        g_frameDt = (float)std::clamp(dtMs / 1000.0, 0.0005, 0.25);
        s_prevFrameQpc = q.QuadPart;""")

# Renderer branch, after the auto-hide alpha is known.
rep("""    if (sceneAlpha <= 0.001f) {
        // Fully faded out. Present one blank frame to clear whatever was last
        // shown, then skip the render path entirely until audio returns --""", """    // Direct3D 11 renderer (2.0). Falls through to the Direct2D path below if
    // it isn't selected, or can't run on this device.
    if (ttgfx::Active()) {
        if (RenderVisualizerD3D(sceneAlpha)) {
            g_autoHideBlanked = sceneAlpha <= 0.001f;
            return;
        }
    } else {
        ttgfx::ReleaseSurface();
    }

    if (sceneAlpha <= 0.001f) {
        // Fully faded out. Present one blank frame to clear whatever was last
        // shown, then skip the render path entirely until audio returns --""")

# Direct2D path: bar levels come from VizComputeBarFrame now.
rep("""    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));

    UpdateVisualizerTargets();

    int barCount = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));""", """    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));

    // Every bar's height and peak cap for this frame, for either engine (see
    // VizComputeBarFrame). The loops below only draw them.
    if (!g_dragRenderPauseActive.load(std::memory_order_relaxed)) VizComputeBarFrame();

    int barCount = VizEffectiveBarCount();""")
rep("""    float attack = 0.55f, decay = 0.18f;
    switch (g_settings.shape) {
        case VizShape::Stereo:  attack = 0.72f; decay = 0.22f; break;
        case VizShape::Mirror:  attack = 0.52f; decay = 0.20f; break;
        case VizShape::Wave:    attack = 0.34f; decay = 0.17f; break;
        case VizShape::Breathe: attack = 0.20f; decay = 0.11f; break;
        default: break;
    }
    if (g_settings.smoothing > 0) {
        // Scales both toward 0 (slower catch-up to the target level) without
        // ever fully freezing the bar, even at max smoothing.
        float smoothFactor = 1.0f - (g_settings.smoothing / 100.0f) * 0.9f;
        attack *= smoothFactor;
        decay *= smoothFactor;
    }
    // Identity unless Smooth Mode is on and this frame was early or late.
    attack = VizEaseForFrame(attack);
    decay = VizEaseForFrame(decay);

""", "")
rep("""    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? groupThickness : groupExtent;
        totalHeight = horizontal ? groupExtent    : groupThickness;
    }""", """    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial || g_settings.shape == VizShape::Goniometer) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? groupThickness : groupExtent;
        totalHeight = horizontal ? groupExtent    : groupThickness;
    }""")
# Dots loop
rep("""            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float nxt = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(nxt - cur) > 0.0005f) ? nxt : tgt;
                float fac = std::max(0.f, g_vizPeak[i]);""", """            for (int i = 0; i < barCount; i++) {
                float fac = std::max(0.f, g_vizPeak[i]);""")
# Radial loop
rep("""            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float next = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(next - cur) > 0.0005f) ? next : tgt;
                float fac = std::max(0.f, g_vizPeak[i]);
                float len = idleSize + fac * std::max(0.f, maxSize - idleSize);""", """            for (int i = 0; i < barCount; i++) {
                float fac = std::max(0.f, g_vizPeak[i]);
                float len = idleSize + fac * std::max(0.f, maxSize - idleSize);""")
# Bars loop
rep("""            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float next = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(next - cur) > 0.0005f) ? next : tgt;

                float fac = std::max(0.f, g_vizPeak[i]);""", """            for (int i = 0; i < barCount; i++) {
                float fac = std::max(0.f, g_vizPeak[i]);""")
rep("""                float holdSize = idleSize;
                if (g_settings.peakHoldEnabled) {
                    g_vizPeakHold[i] = (fac >= g_vizPeakHold[i])
                        ? fac : std::max(0.f, g_vizPeakHold[i] - 0.012f * g_frameScale);
                    holdSize = idleSize + g_vizPeakHold[i] * std::max(0.f, maxSize - idleSize);
                } else {
                    g_vizPeakHold[i] = 0.f;
                }""", """                float holdSize = idleSize;
                if (g_settings.peakHoldEnabled)
                    holdSize = idleSize + g_vizPeakHold[i] * std::max(0.f, maxSize - idleSize);""")
# Scope: trace + multiband through the shared helpers.
rep("""                float bandsSnap[VIZ_NUM_BANDS];
                ReadBands(bandsSnap);
                float lowE = 0.f, midE = 0.f, highE = 0.f;
                for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                    float e = bandsSnap[b];
                    if (VIZ_BAND_EQ_ZONE[b] == 0) lowE += e;
                    else if (VIZ_BAND_EQ_ZONE[b] == 1) midE += e;
                    else highE += e;
                }""", """                float zones[3];
                VizZoneEnergies(zones);
                float lowE = zones[0], midE = zones[1], highE = zones[2];""")
rep("""            float strokeW = std::max(1.0f, barW * 0.5f);
            float waveSnap[VIZ_WAVE_SAMPLES];
            ReadWaveform(waveSnap);
""", """            float strokeW = std::max(1.0f, barW * 0.5f);
""")
rep("""            static float s_scopeDisp[VIZ_WAVE_SAMPLES] = {};
            float scopeEase = (g_settings.oscilloscopeDamping > 0)
                                  ? 1.0f - (g_settings.oscilloscopeDamping / 100.0f) * 0.95f
                                  : 1.0f;
            // Undamped (1.0) stays 1.0; damping becomes frame-time aware in
            // Smooth Mode like the bars' attack and decay.
            if (scopeEase < 1.0f) scopeEase = VizEaseForFrame(scopeEase);
            for (int w = 0; w < VIZ_WAVE_SAMPLES; w++) {
                s_scopeDisp[w] += (waveSnap[w] - s_scopeDisp[w]) * scopeEase;
            }
""", """            VizUpdateScopeTrace();
            const float* s_scopeDisp = g_scopeDisp;
""")
# Goniometer, Direct2D fallback.
rep("""        else {
            // Smooth Mode: bars go out as one sprite batch (see""", """        else if (g_settings.shape == VizShape::Goniometer) {
            // The Direct2D fallback for the goniometer: the same points the
            // Direct3D 11 renderer draws, as small squares, and the
            // correlation bar. Fine for a fallback; the Direct3D 11 renderer
            // is the one to use for this shape.
            static std::vector<float> s_pts(ttgfx::kMaxPoints * 4);
            int n = ttgfx::BuildGonioPoints(s_pts.data(), ttgfx::kMaxPoints);
            RGBA col = c1;
            if (g_settings.colorMode == VizColorMode::Gradient || g_settings.colorMode == VizColorMode::Tourne)
                col = LerpColor(cGrad1, c2, 0.5f);
            float cx = blockX + totalWidth * 0.5f, cy = blockY + totalHeight * 0.5f;
            float R = maxSize * 0.85f, d = std::max(0.75f, barW * 0.2f);
            for (int k = 0; k < n; k++) {
                float x = cx + std::clamp(s_pts[k * 4], -1.f, 1.f) * R;
                float y = cy - std::clamp(s_pts[k * 4 + 1], -1.f, 1.f) * R;
                g_barBrush->SetColor(D2D1::ColorF(col.r / 255.f, col.g / 255.f, col.b / 255.f,
                                                  col.a / 255.f * s_pts[k * 4 + 2]));
                g_dc->FillRectangle(D2D1::RectF(x - d, y - d, x + d, y + d), g_barBrush.Get());
            }
            float corr;
            {
                std::lock_guard<std::mutex> lock(g_meterMutex);
                corr = (float)g_meters.correlation;
            }
            float ch = std::max(2.0f, 3.0f * g_dpiScale), cyBar = cy + maxSize * 0.92f - ch;
            g_barBrush->SetColor(D2D1::ColorF(col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a / 255.f * 0.25f));
            g_dc->FillRectangle(D2D1::RectF(cx - R, cyBar, cx + R, cyBar + ch), g_barBrush.Get());
            g_barBrush->SetColor(D2D1::ColorF(col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a / 255.f));
            float mx = cx + std::clamp(corr, -1.f, 1.f) * R;
            g_dc->FillRectangle(D2D1::RectF(mx - 1.5f, cyBar - 1.f, mx + 1.5f, cyBar + ch + 1.f), g_barBrush.Get());
        }
        else {
            // Smooth Mode: bars go out as one sprite batch (see""")
# Text overlays through the shared helper.
between("""        if (g_settings.nowPlayingEnabled && g_dwriteTextFormat && g_nowPlayingBrush) {
            ULONGLONG changedAt = g_nowPlayingChangedTick.load(std::memory_order_relaxed);
            ULONGLONG npElapsed = GetTickCount64() - changedAt;""", """        if (useFadeLayer) {
            g_dc->PopLayer();
        }""", """        {
            VizTextFrame tf;
            VizBuildTextFrame(tf);
            VizDrawTextOverlays(tf, layout, smooth);
        }

""")
# Publish rect through the shared helper.
between("""        // Publish the bounds of what is actually VISIBLE, for the occlusion
        // check -- not the full render surface.""", """        if (g_backgroundBrush) {
            float padL = (float)g_settings.bgPaddingL * g_dpiScale;""", """        VizPublishDrawRect(layout);

""")

# ---------------------------------------------------------------- engine thread
between("void RenderThreadProc() {\n", "void PauseForFullscreen() {", read("part_thread.cpp") + "\n")

# ---------------------------------------------------------------- LoadSettings
rep("""    PCWSTR analysisDevice = Wh_GetStringSetting(L"hardware.analysisDevice");
    g_settings.analysisDevice = (wcscmp(analysisDevice, L"npu") == 0) ? VizAnalysisDevice::Npu
                                                                      : VizAnalysisDevice::Cpu;
    Wh_FreeStringSetting(analysisDevice);
""", read("part_loadsettings.cpp"))
rep("""        if (!path.empty() && g_settings.analysisDevice == VizAnalysisDevice::Npu) {""",
    """        if (!path.empty() && g_settings.workload == VizWorkload::Npu) {""")
rep("""    // The NPU worker reads only its own copies, taken here.
    ttnpu::Configure(g_settings.analysisDevice == VizAnalysisDevice::Npu, g_settings.npuRuntimePath);""",
    """    // The NPU worker reads only its own copies, taken here.
    ttnpu::Configure(g_settings.workload == VizWorkload::Npu, g_settings.npuRuntimePath);
    VizPublishEngineConfig();""")
rep("""    PCWSTR shape = Wh_GetStringSetting(L"appearance.shape");
    g_settings.shape = (wcscmp(shape, L"mountain") == 0)     ? VizShape::Mountain""",
    """    PCWSTR shape = Wh_GetStringSetting(L"appearance.shape");
    g_settings.shape = (wcscmp(shape, L"goniometer") == 0)   ? VizShape::Goniometer
                       : (wcscmp(shape, L"mountain") == 0)   ? VizShape::Mountain""")
rep("""    g_settings.freqScale = (wcscmp(freqScale, L"linear") == 0) ? VizFreqScale::Linear
                          : (wcscmp(freqScale, L"mel") == 0)   ? VizFreqScale::Mel
                                                                : VizFreqScale::Log;""",
    """    g_settings.freqScale = (wcscmp(freqScale, L"linear") == 0) ? VizFreqScale::Linear
                          : (wcscmp(freqScale, L"mel") == 0)   ? VizFreqScale::Mel
                          : (wcscmp(freqScale, L"bark") == 0)  ? VizFreqScale::Bark
                          : (wcscmp(freqScale, L"erb") == 0)   ? VizFreqScale::Erb
                                                                : VizFreqScale::Log;""")
rep("""    g_settings.fftSize = (fftSize == 1024 || fftSize == 2048 || fftSize == 4096 || fftSize == 8192)
                              ? fftSize : 1024;""", """    g_settings.fftSize = (fftSize == 1024 || fftSize == 2048 || fftSize == 4096 || fftSize == 8192)
                              ? fftSize : 2048;""")
rep("""    g_settings.pauseWhenSilentSeconds = std::max(0, Wh_GetIntSetting(L"performance.pauseWhenSilentSeconds"));""",
    """    g_settings.pauseWhenSilentSeconds = std::max(0, Wh_GetIntSetting(L"performance.pauseWhenSilentSeconds"));
    g_settings.deepIdle = Wh_GetIntSetting(L"performance.deepIdle") != 0;""")
rep("""    g_settings.targetFps = std::max(1, Wh_GetIntSetting(L"performance.targetFps"));""",
    """    // 0 = match the display (the engine thread reads the refresh rate).
    g_settings.targetFps = std::max(0, Wh_GetIntSetting(L"performance.targetFps"));""")

# ---------------------------------------------------------------- uninit: events
rep("""    StopVizCaptureThread();
    // After the capture thread, which hands its NPU engine back on the way out.
    ttnpu::Shutdown();""", """    StopVizCaptureThread();
    // After the engine thread, which hands its NPU engine back on the way out.
    ttnpu::Shutdown();
    if (g_engineWake) {
        CloseHandle(g_engineWake);
        g_engineWake = nullptr;
    }
    if (g_engineClosed) {
        CloseHandle(g_engineClosed);
        g_engineClosed = nullptr;
    }""")

# Fullscreen pause blanks the panel surface too.
rep("""    StopVizCaptureThread();
    if (g_dc && g_swapChain) {
        g_dc->BeginDraw();
        g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
        g_dc->EndDraw();
        g_swapChain->Present(1, 0);
    }""", """    StopVizCaptureThread();
    if (g_dc && g_swapChain) {
        g_dc->BeginDraw();
        g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
        g_dc->EndDraw();
        g_swapChain->Present(1, 0);
        ttgfx::PresentBlank();
    }""")

# ---------------------------------------------------------------- readme
rep("""## ◈ PERFORMANCE AT A GLANCE""", read("readme_new20.md") + """## ◈ PERFORMANCE AT A GLANCE""")
rep("""| **Oscilloscope** | A single line tracing the actual sound wave. |""",
    """| **Oscilloscope** | A single line tracing the actual sound wave. |
| **Goniometer** | The stereo field: every sample plotted with mono straight up and width sideways, with a correlation bar underneath. *(2.0)* |""")
rep("""### 8 Shapes""", """### 9 Shapes""")
rep("""**Frequency Scale**: How the frequency range spreads across the bars. This matters more than it sounds.""",
    """**Frequency Scale**: How the frequency range spreads across the bars (with Band Layout = Frequency Scale). This matters more than it sounds.""")
rep("""- **Mel**: uses the *mel scale*, built from research on how people actually perceive pitch. Like Log, tuned to human hearing.""",
    """- **Mel**: uses the *mel scale*, built from research on how people actually perceive pitch. Like Log, tuned to human hearing.
- **Bark**: the ear's critical bands (Traunmueller's formula): more room for the midrange, where hearing is most finely tuned.
- **ERB**: equivalent rectangular bandwidths (Glasberg and Moore), the widths of the ear's own auditory filters. Between Log and Bark.""")
rep("""**Peak Frequency Readout**: Shows the loudest note as a live number, e.g. `1.2 kHz`.""",
    """**Peak Frequency Readout**: Shows the loudest note as a live number, e.g. `1.2 kHz`, or loudness figures instead (see **Readout Content** under Analysis).""")
rep("""`1024` fastest, plenty for most · `2048` / `4096` noticeably crisper · `8192` maximum detail.""",
    """`1024` fastest, plenty for most · `2048` / `4096` noticeably crisper · `8192` maximum detail.

> **With the Precision engine (2.0)** this is the size of each of the three resolution tiers, and bass detail comes from the tiers rather than from raising this. `2048` *(the new default)* is the sweet spot. Sensitivity no longer has to be retuned per FFT Size either: levels are calibrated in dBFS, so a change of FFT Size changes resolution, not height.""")
rep("""## Position

**Horizontal Position**""", read("readme_analysis.md") + """## Position

**Horizontal Position**""")
rep("""**Target FPS**: How many times per second it redraws. Higher = smoother, more CPU. Little point exceeding your monitor's refresh rate.""",
    """**Target FPS**: How many times per second it redraws. Higher = smoother, more CPU. Little point exceeding your monitor's refresh rate. **`0` matches the refresh rate**, whatever it is. With the Direct3D 11 renderer a frame where nothing moved is skipped entirely, so a high target costs far less than it used to in quiet or steady passages.""")
rep("""**Pause When Silent (seconds)**: After this long without audio, drops to a trickle instead of full speed. `0` disables.""",
    """**Pause When Silent (seconds)**: After this long without audio, drops to a trickle instead of full speed: it wakes only when audio arrives or four times a second, and with Direct3D 11 presents nothing unless something changes. `0` disables (and Deep Idle with it).

**Deep Idle** *(2.0)*: Five seconds after Pause When Silent kicks in, stops the audio stream itself and reads Windows' own peak meter four times a second instead, restarting the moment anything plays. A running capture stream registers an audio power request, which can keep the PC from sleeping (`powercfg /requests` shows it); a stopped one doesn't. On by default.""")
between("""## Hardware

There are two jobs in this mod""", """![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/4.gif)""",
        read("readme_hardware.md") + "---\n\n")
between("""# ◇ RUNNING THE ANALYSIS ON AN NPU""", """# ▲ WHERE THE EFFICIENCY COMES FROM""", read("readme_npu.md"))
rep("""**Auto-Hide** now stops rendering completely once faded: presents one blank frame, then exits the render path entirely. **Pause When Covered** stops rendering and capture while hidden, checked once per second rather than per frame.
""", """**Auto-Hide** now stops rendering completely once faded: presents one blank frame, then exits the render path entirely. **Pause When Covered** stops rendering and capture while hidden, checked once per second rather than per frame.

""" + read("readme_efficiency.md"))

out = os.path.join(S, "v2.cpp")
open(out, "w", encoding="utf-8", newline="\n").write(src)
print("wrote", out, src.count("\n"), "lines")
