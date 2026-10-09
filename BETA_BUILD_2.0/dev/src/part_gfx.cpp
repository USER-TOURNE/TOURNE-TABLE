// ---- Direct3D 11 renderer (Renderer = Direct3D 11) -----------------------------------
//
// Why a second renderer, when Smooth Mode already batched the bars. Direct2D
// tessellates on the CPU and validates and saves Direct3D state around every
// batch; for a few hundred moving shapes that per-call CPU work, not the
// GPU, is what the 1.5 measurements kept pointing at. This renderer draws
// every bar, dot, cap, spoke and scope segment as an instanced quad from one
// tiny vertex shader that reads the bar heights straight from a buffer: the
// CPU writes about 16 KB per frame (two floats per bar) and issues a handful
// of draw calls, whatever the bar count. The pixel shader computes exact
// antialiased coverage analytically, so edges look like Direct2D's.
//
// Three further savings come from how the frame reaches the screen:
//
//   * Skip unchanged frames. A hash of everything that decides the picture
//     (bar heights to a quarter pixel, colours, the trace) is compared with
//     the last frame presented; if nothing moved, nothing is drawn or
//     presented, so DWM has nothing to recompose. During steady or quiet
//     passages and every idle state that is most frames.
//   * Two surfaces instead of one. The animated swap chain covers only the
//     panel, not the room reserved around it for text; the text lives on the
//     old full-size surface, which is redrawn only when the text or its fade
//     changes. DWM's per-frame work scales with the area that changes.
//   * Opaque panel. When the panel can't show anything behind it (Blur on, or
//     a fully opaque colour), its swap chain is created with alpha ignored and
//     the rounded corners come from a DirectComposition clip. DWM then copies
//     instead of blending, which Chromium measured as 3.12 W against 2.28 W.
//
// The same shaders also carry the GPU Workload: four compute passes run the
// whole precision analysis on the GPU and write the bar buffer the vertex
// shader reads, so nothing comes back to the CPU but an 8-byte stats block,
// read two frames late without waiting.
namespace ttgfx {

static const char kShaderSource[] =
/*@@SHADER_SOURCE@@*/;

#pragma pack(push, 4)
struct FrameCB {
    float viewport[2], block[2];
    float barW, barGap, maxSize, idleSize;
    float radii[4];
    float dotRadii[4];
    uint32_t barCount, shape, vertical, anchor;
    uint32_t colorMode, flags, maxDots, dotSlots;
    float c1[4], grad1[4], c2[4], peakColor[4], beatColor[4];
    float beatIntensity, rainbowBase, sceneAlpha, capThickness;
    float dotStep, dotR, innerR, strokeW;
    float center[2];
    float ampScale, sweepLen;
    float scopeCenter, sweepOrigin, wstep, gonioR;
    float plateRect[4];
    float scopeColor[4];
    float gonioDot, corrY, corrH, corr;
    float termColors[5][4];
    float termGeom[4];   // origin x, origin y, cell width, cell height
    float termAtlas[4];  // atlas width, atlas height, -, -
    uint32_t termCols, termRows, termAtlasCols, termPad;
};
struct PassCB {
    uint32_t pass, count, pad0, pad1;
};
struct CsCB {
    uint32_t n, half, log2Half, numBands;
    uint32_t numBars, tierCount, tierList, detector;
    float floorDb, rangeDb, sensDb, dt;
    float attackMs, releaseMs, releaseDbPerSec, autoGainMaxDb;
    uint32_t releaseLinear, curve, shape, autoGain;
    float peakHoldMs, gravity, linFall, barRangePx;
    uint32_t peakGravity, domOn, pad1, pad2;
    float tierRate0, tierRate1, tierRate2, peakCal;
    float fmin, fmax, breatheUp, breatheDown;
};
#pragma pack(pop)
static_assert(sizeof(FrameCB) == 26 * 16, "FrameCB must match tt_cb.hlsl");
static_assert(sizeof(PassCB) == 16, "PassCB must match tt_cb.hlsl");
static_assert(sizeof(CsCB) == 9 * 16, "CsCB must match tt_cb.hlsl");

enum Pass : uint32_t { kPlate = 0, kBars, kCaps, kDots, kRadial, kScope, kGonio, kCorr, kTerm, kPassCount };
constexpr int kMaxPoints = 8192;
constexpr int kGonioFrames = 6;  // persistence: this frame and the five before it

// D3DCompile, resolved from d3dcompiler_47.dll, which ships with Windows 10
// and later. Linking it would make the whole mod fail to load on a system
// without it; loaded like this, only this renderer is unavailable there.
typedef HRESULT(WINAPI* PFN_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
                                        LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

struct State {
    // Bytecode survives device rebuilds; it doesn't depend on the device.
    ComPtr<ID3DBlob> vsCode, psCode, csCode[4];
    bool compileTried = false, compileOk = false, csCompileTried = false, csCompileOk = false;
    std::wstring compileError;

    // Device objects.
    bool deviceReady = false;
    ComPtr<ID3D11DeviceContext> ctx;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11ComputeShader> cs[4];
    ComPtr<ID3D11Buffer> frameCB, passCB[kPassCount], csCB;
    ComPtr<ID3D11Buffer> barsDyn, globalsDyn, waveDyn, pointsDyn;
    ComPtr<ID3D11ShaderResourceView> barsDynSRV, globalsDynSRV, waveSRV, pointsSRV;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11BlendState> blend;
    ComPtr<ID3D11BlendState> blendOff;  // the plate: it is the first thing drawn, so it only overwrites
    ComPtr<ID3D11RasterizerState> raster;

    // What each dynamic buffer holds, so one whose contents haven't changed
    // isn't mapped again (WRITE_DISCARD is a driver call and a buffer rename
    // every time). Cleared whenever the buffers are (re)created.
    bool uploadsValid = false;
    uint64_t barsKey = 0, globalsKey = 0, waveKey = 0;
    uint32_t pointsSerial = 0;
    int pointsCount = 0;
    bool cellsValid = false;
    uint32_t cellsSerial = 0;
    uint64_t cellsShape = 0;
    UINT termCount = 0;  // non-blank cells uploaded (the Terminal draw's instance count)
    bool frameCBValid = false;
    FrameCB frameCBLast = {};

    // Panel surface.
    ComPtr<IDXGISwapChain1> sc;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<IDCompositionVisual> visual;
    ComPtr<IDCompositionRectangleClip> clip;
    UINT w = 0, h = 0;
    int offX = 0, offY = 0;  // in layout-local pixels
    bool opaque = false;
    bool visualAttached = false;
    // DirectComposition properties as last committed, so a still panel
    // costs no Commit (each one is a batch sent to DWM).
    bool dcValid = false;
    float dcOffX = 0.f, dcOffY = 0.f;
    bool dcClipOn = false;
    float dcClip[8] = {};  // left, top, right, bottom, radii TL TR BR BL

    // Baked background, the size of the panel surface.
    ComPtr<ID3D11Texture2D> plateTex;
    ComPtr<ID3D11ShaderResourceView> plateSRV;
    bool plateValid = false;
    uint64_t plateKey = 0;

    // GPU workload.
    bool gpuReady = false;
    int gpuSerial = -1, gpuN = 0, gpuBands = 0;
    ComPtr<ID3D11Buffer> tierIn, window, bandDesc, bandOff, shapeMod, power, bandState, barsGpu, barAux,
        globalsGpu, stats, staging[3];
    ComPtr<ID3D11ShaderResourceView> tierInSRV, windowSRV, bandDescSRV, bandOffSRV, shapeModSRV, barsGpuSRV,
        globalsGpuSRV;
    ComPtr<ID3D11UnorderedAccessView> powerUAV, bandStateUAV, barsGpuUAV, barAuxUAV, globalsGpuUAV, statsUAV;
    std::vector<float> tierScratch;
    unsigned frameNo = 0;
    float lastMaxDeltaPx = 1e9f;
    bool gpuWarned = false;

    // Present-on-change.
    uint64_t lastHash = 0;
    bool forcePresent = true;
    uint64_t textKey = 0;
    bool textForce = true;
    bool textDetached = false;  // the text surface is off its visual (nothing to show)
    bool blankPresented = false;

    // Terminal shape: the cell grid and the baked glyph atlas, created on
    // first use so nobody who never picks the shape pays for them.
    ComPtr<ID3D11Buffer> cellsDyn;
    ComPtr<ID3D11ShaderResourceView> cellsSRV;
    ComPtr<ID3D11Texture2D> glyphTex;
    ComPtr<ID3D11ShaderResourceView> glyphSRV;
    uint64_t glyphKey = 0;
    UINT glyphW = 0, glyphH = 0;

    // Goniometer persistence.
    std::vector<float> gonioHist[kGonioFrames];
    uint32_t gonioSerial = 0;
    int gonioHead = 0;

    bool warned = false;
};
State g;

// ---- Hashing (present-on-change) -------------------------------------------------------
inline void Mix(uint64_t& h, uint64_t v) {
    h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
}
// Quantised: v to the nearest 1/q. Rounded with a plain conversion rather than
// llroundf, a library call: about a third off hashing 256 bars and caps.
inline void MixF(uint64_t& h, float v, float q) {
    float r = v * q;
    Mix(h, (uint64_t)(int64_t)(r + (r >= 0.f ? 0.5f : -0.5f)));
}

// ---- Setup -------------------------------------------------------------------------------
bool Compile() {
    if (g.compileTried) return g.compileOk;
    g.compileTried = true;
    HMODULE lib = LoadLibraryExW(L"d3dcompiler_47.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto fn = lib ? (PFN_D3DCompile)(void*)GetProcAddress(lib, "D3DCompile") : nullptr;
    if (!fn) {
        g.compileError = L"d3dcompiler_47.dll isn't available, so the Direct3D 11 renderer can't build its shaders.";
        return false;
    }
    auto one = [&](const char* entry, const char* target, ComPtr<ID3DBlob>& out) -> bool {
        ComPtr<ID3DBlob> err;
        // Optimization level 3 (1 << 15).
        HRESULT hr = fn(kShaderSource, sizeof(kShaderSource) - 1, "tourne-table.hlsl", nullptr, nullptr, entry, target,
                        1u << 15, 0, &out, &err);
        if (FAILED(hr)) {
            std::wstring msg = L"Shader " + std::wstring(entry, entry + strlen(entry)) + L" failed to compile";
            if (err && err->GetBufferPointer()) {
                const char* s = (const char*)err->GetBufferPointer();
                msg += L": ";
                msg += std::wstring(s, s + strnlen(s, 600));
            }
            g.compileError = msg;
            Wh_Log(L"[D3D11] %s", msg.c_str());
            return false;
        }
        return true;
    };
    g.compileOk = one("VSMain", "vs_5_0", g.vsCode) && one("PSMain", "ps_5_0", g.psCode);
    // The compute passes compile on first use of Workload = GPU.
    g.csCompileTried = false;
    return g.compileOk;
}

bool CompileCompute() {
    if (g.csCompileTried) return g.csCompileOk;
    g.csCompileTried = true;
    HMODULE lib = LoadLibraryExW(L"d3dcompiler_47.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto fn = lib ? (PFN_D3DCompile)(void*)GetProcAddress(lib, "D3DCompile") : nullptr;
    if (!fn) return g.csCompileOk = false;
    const char* entries[4] = {"CsFft", "CsBands", "CsReduce", "CsShape"};
    for (int i = 0; i < 4; i++) {
        ComPtr<ID3DBlob> err;
        if (FAILED(fn(kShaderSource, sizeof(kShaderSource) - 1, "tourne-table.hlsl", nullptr, nullptr, entries[i],
                      "cs_5_0", 1u << 15, 0, &g.csCode[i], &err))) {
            if (err) Wh_Log(L"[D3D11] %S failed: %S", entries[i], (const char*)err->GetBufferPointer());
            return g.csCompileOk = false;
        }
    }
    return g.csCompileOk = true;
}

HRESULT MakeStructured(UINT stride, UINT count, bool dynamic, bool uav, const void* init, ComPtr<ID3D11Buffer>& buf,
                       ComPtr<ID3D11ShaderResourceView>* srv, ComPtr<ID3D11UnorderedAccessView>* uavOut) {
    D3D11_BUFFER_DESC d = {};
    d.ByteWidth = stride * count;
    d.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_SHADER_RESOURCE | (uav ? D3D11_BIND_UNORDERED_ACCESS : 0);
    d.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    d.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    d.StructureByteStride = stride;
    D3D11_SUBRESOURCE_DATA sd = {init, 0, 0};
    HRESULT hr = g_d3dDevice->CreateBuffer(&d, init ? &sd : nullptr, &buf);
    if (FAILED(hr)) return hr;
    if (srv) {
        hr = g_d3dDevice->CreateShaderResourceView(buf.Get(), nullptr, srv->ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;
    }
    if (uavOut) {
        hr = g_d3dDevice->CreateUnorderedAccessView(buf.Get(), nullptr, uavOut->ReleaseAndGetAddressOf());
        if (FAILED(hr)) return hr;
    }
    return S_OK;
}

HRESULT MakeCB(UINT size, bool dynamic, const void* init, ComPtr<ID3D11Buffer>& buf) {
    D3D11_BUFFER_DESC d = {};
    d.ByteWidth = size;
    d.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    d.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    D3D11_SUBRESOURCE_DATA sd = {init, 0, 0};
    return g_d3dDevice->CreateBuffer(&d, init ? &sd : nullptr, &buf);
}

bool EnsureDevice() {
    if (g.deviceReady) return true;
    if (!g_d3dDevice) return false;
    if (g_d3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0) {
        g.compileError = L"This GPU's Direct3D feature level is below 11.0, which the Direct3D 11 renderer needs.";
        return false;
    }
    if (!Compile()) return false;
    g_d3dDevice->GetImmediateContext(&g.ctx);
    if (FAILED(g_d3dDevice->CreateVertexShader(g.vsCode->GetBufferPointer(), g.vsCode->GetBufferSize(), nullptr, &g.vs)) ||
        FAILED(g_d3dDevice->CreatePixelShader(g.psCode->GetBufferPointer(), g.psCode->GetBufferSize(), nullptr, &g.ps)))
        return false;
    if (FAILED(MakeCB(sizeof(FrameCB), true, nullptr, g.frameCB))) return false;
    for (uint32_t p = 0; p < kPassCount; p++) {
        PassCB pc = {p, 0, 0, 0};
        if (FAILED(MakeCB(sizeof(PassCB), false, &pc, g.passCB[p]))) return false;
    }
    if (FAILED(MakeStructured(8, VIZ_BARS_MAX, true, false, nullptr, g.barsDyn, &g.barsDynSRV, nullptr)) ||
        FAILED(MakeStructured(16, 4, true, false, nullptr, g.globalsDyn, &g.globalsDynSRV, nullptr)) ||
        FAILED(MakeStructured(4, VIZ_WAVE_SAMPLES, true, false, nullptr, g.waveDyn, &g.waveSRV, nullptr)) ||
        FAILED(MakeStructured(16, kMaxPoints, true, false, nullptr, g.pointsDyn, &g.pointsSRV, nullptr)))
        return false;

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(g_d3dDevice->CreateSamplerState(&sd, &g.sampler))) return false;

    // Premultiplied alpha, like every other surface in this mod.
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(g_d3dDevice->CreateBlendState(&bd, &g.blend))) return false;
    // The plate is drawn first, over a cleared target: premultiplied "over"
    // onto zero is the source itself, so blending only adds a read of the
    // whole target. Without it the plate just writes.
    bd.RenderTarget[0].BlendEnable = FALSE;
    if (FAILED(g_d3dDevice->CreateBlendState(&bd, &g.blendOff))) return false;
    g.uploadsValid = false;
    g.cellsValid = false;
    g.frameCBValid = false;

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    if (FAILED(g_d3dDevice->CreateRasterizerState(&rd, &g.raster))) return false;

    g.deviceReady = true;
    Wh_Log(L"[D3D11] renderer ready (feature level %X)", (unsigned)g_d3dDevice->GetFeatureLevel());
    return true;
}

void ReleaseGpuAnalysis() {
    g.gpuReady = false;
    g.gpuSerial = -1;
    g.tierIn.Reset(); g.window.Reset(); g.bandDesc.Reset(); g.bandOff.Reset(); g.shapeMod.Reset();
    g.power.Reset(); g.bandState.Reset(); g.barsGpu.Reset(); g.barAux.Reset(); g.globalsGpu.Reset();
    g.stats.Reset();
    for (auto& s : g.staging) s.Reset();
    g.tierInSRV.Reset(); g.windowSRV.Reset(); g.bandDescSRV.Reset(); g.bandOffSRV.Reset(); g.shapeModSRV.Reset();
    g.barsGpuSRV.Reset(); g.globalsGpuSRV.Reset();
    g.powerUAV.Reset(); g.bandStateUAV.Reset(); g.barsGpuUAV.Reset(); g.barAuxUAV.Reset(); g.globalsGpuUAV.Reset();
    g.statsUAV.Reset();
}

// The text surface (g_compositionVisual: the Direct2D swap chain, the size of
// the whole widget box) sits over the panel. Empty, DWM would still read and
// blend it in every frame it composes there, so while there is no text it is
// taken off its visual, and put back right after the first frame drawn on it.
void ShowTextSurface(bool show) {
    if (show != g.textDetached) return;  // already so
    if (!g_compositionVisual || !g_compositionDevice || (show && !g_swapChain)) {
        g.textDetached = false;
        return;
    }
    if (FAILED(g_compositionVisual->SetContent(show ? (IUnknown*)g_swapChain.Get() : nullptr))) return;
    g_compositionDevice->Commit();
    VizPerf(kPerfCommits);
    g.textDetached = !show;
}

// The panel surface and its visual. Called before the composition device goes.
void ReleaseSurface() {
    ShowTextSurface(true);  // the Direct2D path draws everything on the text surface
    if (!g.sc && !g.visual && !g.plateTex) return;  // nothing to release (called every Direct2D frame)
    if (g.visual && g.visualAttached && g_rootVisual) {
        g_rootVisual->RemoveVisual(g.visual.Get());
        if (g_compositionDevice) {
            g_compositionDevice->Commit();
            VizPerf(kPerfCommits);
        }
    }
    g.visualAttached = false;
    g.dcValid = false;
    g.dcClipOn = false;
    g.rtv.Reset();
    g.sc.Reset();
    g.clip.Reset();
    g.visual.Reset();
    g.plateTex.Reset();
    g.plateSRV.Reset();
    g.plateValid = false;
    g.w = g.h = 0;
    g.forcePresent = true;
    g.textForce = true;
}

// Everything that belongs to the D3D device (device lost, Drawing Device
// changed, unload). The compiled bytecode is kept.
void ReleaseDevice() {
    ReleaseSurface();
    ReleaseGpuAnalysis();
    g.vs.Reset(); g.ps.Reset();
    for (auto& c : g.cs) c.Reset();
    g.frameCB.Reset(); g.csCB.Reset();
    for (auto& p : g.passCB) p.Reset();
    g.barsDyn.Reset(); g.globalsDyn.Reset(); g.waveDyn.Reset(); g.pointsDyn.Reset();
    g.barsDynSRV.Reset(); g.globalsDynSRV.Reset(); g.waveSRV.Reset(); g.pointsSRV.Reset();
    g.cellsDyn.Reset(); g.cellsSRV.Reset(); g.glyphTex.Reset(); g.glyphSRV.Reset();
    g.glyphKey = 0;
    g.sampler.Reset(); g.blend.Reset(); g.blendOff.Reset(); g.raster.Reset();
    g.uploadsValid = g.cellsValid = g.frameCBValid = false;
    if (g.ctx) g.ctx->ClearState();
    g.ctx.Reset();
    g.deviceReady = false;
}

void OnSettingsChanged() {
    g.plateValid = false;
    g.forcePresent = true;
    g.textForce = true;
    g.warned = false;
    g.gpuWarned = false;
}

// Is this renderer the one drawing? Falls back to Direct2D, once, with a
// warning, if it can't be set up.
bool Active() {
    if (g_settings.renderer != VizRenderer::D3D11) return false;
    if (EnsureDevice()) return true;
    if (!g.warned) {
        g.warned = true;
        ReportSettingWarning(L"Hardware", L"Renderer",
                             (g.compileError.empty() ? std::wstring(L"The Direct3D 11 renderer couldn't start")
                                                     : g.compileError) +
                                 L" Drawing with Direct2D instead.");
        FlushSettingsIssues();
    }
    return false;
}

// ---- Panel surface ------------------------------------------------------------------------
bool EnsureSurface(int offX, int offY, UINT w, UINT h, bool opaque, const D2D1_RECT_F& clipRect,
                   const float clipRadii[4], float layoutOriginX, float layoutOriginY) {
    if (!g_compositionDevice || !g_rootVisual) return false;
    w = std::max(1u, w);
    h = std::max(1u, h);
    bool recreate = !g.sc || opaque != g.opaque;
    bool dirty = false;  // something DirectComposition needs to be told
    if (recreate) {
        dirty = true;
        g.rtv.Reset();
        g.sc.Reset();
        DXGI_SWAP_CHAIN_DESC1 scd = {};
        scd.Width = w;
        scd.Height = h;
        scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        scd.SampleDesc.Count = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.BufferCount = 2;
        scd.Scaling = DXGI_SCALING_STRETCH;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scd.AlphaMode = opaque ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED;
        if (FAILED(g_dxgiFactory->CreateSwapChainForComposition(g_dxgiDevice.Get(), &scd, nullptr, &g.sc))) return false;
        g.w = w;
        g.h = h;
        g.opaque = opaque;
        if (!g.visual && FAILED(g_compositionDevice->CreateVisual(&g.visual))) return false;
        g.visual->SetContent(g.sc.Get());
        g.visual->SetBorderMode(DCOMPOSITION_BORDER_MODE_SOFT);
    } else if (w != g.w || h != g.h) {
        g.rtv.Reset();
        if (FAILED(g.sc->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0))) return false;
        g.w = w;
        g.h = h;
        dirty = true;
    }
    if (!g.rtv) {
        ComPtr<ID3D11Texture2D> back;
        if (FAILED(g.sc->GetBuffer(0, IID_PPV_ARGS(&back)))) return false;
        if (FAILED(g_d3dDevice->CreateRenderTargetView(back.Get(), nullptr, &g.rtv))) return false;
        g.forcePresent = true;
        g.plateValid = false;
    }
    if (!g.visualAttached) {
        // Below the text surface, so readouts placed over the bars stay on top.
        if (FAILED(g_rootVisual->AddVisual(g.visual.Get(), FALSE, g_compositionVisual.Get()))) return false;
        g.visualAttached = true;
        dirty = true;
    }
    // Offset and clip are set, and committed, only when they change: a
    // Commit sends a batch to DWM even when every value in it is the same,
    // and this runs on every render tick, including the ones the
    // skip-unchanged-frames test then doesn't draw.
    const float ox = layoutOriginX + (float)offX, oy = layoutOriginY + (float)offY;
    if (dirty || !g.dcValid || ox != g.dcOffX || oy != g.dcOffY) {
        g.visual->SetOffsetX(ox);
        g.visual->SetOffsetY(oy);
        g.dcOffX = ox;
        g.dcOffY = oy;
        dirty = true;
    }
    const float clipNow[8] = {clipRect.left, clipRect.top, clipRect.right, clipRect.bottom,
                              clipRadii[0],  clipRadii[1], clipRadii[2],   clipRadii[3]};
    if (opaque) {
        if (!g.clip && FAILED(g_compositionDevice->CreateRectangleClip(&g.clip))) return false;
        if (dirty || !g.dcValid || !g.dcClipOn || memcmp(clipNow, g.dcClip, sizeof(clipNow)) != 0) {
            g.clip->SetLeft(clipRect.left);
            g.clip->SetTop(clipRect.top);
            g.clip->SetRight(clipRect.right);
            g.clip->SetBottom(clipRect.bottom);
            g.clip->SetTopLeftRadiusX(clipRadii[0]);
            g.clip->SetTopLeftRadiusY(clipRadii[0]);
            g.clip->SetTopRightRadiusX(clipRadii[1]);
            g.clip->SetTopRightRadiusY(clipRadii[1]);
            g.clip->SetBottomRightRadiusX(clipRadii[2]);
            g.clip->SetBottomRightRadiusY(clipRadii[2]);
            g.clip->SetBottomLeftRadiusX(clipRadii[3]);
            g.clip->SetBottomLeftRadiusY(clipRadii[3]);
            g.visual->SetClip(g.clip.Get());
            memcpy(g.dcClip, clipNow, sizeof(clipNow));
            g.dcClipOn = true;
            dirty = true;
        }
    } else if (dirty || !g.dcValid || g.dcClipOn) {
        g.visual->SetClip((IDCompositionClip*)nullptr);
        g.dcClipOn = false;
        dirty = true;
    }
    if (offX != g.offX || offY != g.offY) g.forcePresent = true;
    g.offX = offX;
    g.offY = offY;
    if (dirty) {
        g_compositionDevice->Commit();
        VizPerf(kPerfCommits);
        g.dcValid = true;
    }
    return true;
}

// Presents one transparent frame on the panel surface (auto-hide, wallpaper
// capture). Cheap, and only ever done once per state change.
void PresentBlank() {
    if (!g.sc || !g.rtv || !g.ctx) return;
    const float zero[4] = {0, 0, 0, 0};
    g.ctx->ClearRenderTargetView(g.rtv.Get(), zero);
    HRESULT hr = g.sc->Present(0, 0);
    VizCheckDeviceLost(S_OK, hr);
    g.forcePresent = true;
}

// ---- Background plate ------------------------------------------------------------------
// Baked once into a texture the size of the panel surface: blurred wallpaper,
// panel fill and border, exactly as the Direct2D path composes them. For the
// opaque panel the fill is square and the border's outer edge is pushed a
// pixel out, because the DirectComposition clip draws the rounded edge.
bool BakePlate(const D2D1_RECT_F& bgRect, const float radii[4]) {
    if (!g_d2dDevice || !g_backgroundBrush || !g_d2dFactory) return false;
    if (!g.plateTex || [&] {
            D3D11_TEXTURE2D_DESC d;
            g.plateTex->GetDesc(&d);
            return d.Width != g.w || d.Height != g.h;
        }()) {
        g.plateTex.Reset();
        g.plateSRV.Reset();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = g.w;
        td.Height = g.h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        if (FAILED(g_d3dDevice->CreateTexture2D(&td, nullptr, &g.plateTex))) return false;
        if (FAILED(g_d3dDevice->CreateShaderResourceView(g.plateTex.Get(), nullptr, &g.plateSRV))) return false;
    }
    ComPtr<IDXGISurface> surf;
    if (FAILED(g.plateTex.As(&surf))) return false;
    ComPtr<ID2D1DeviceContext> bake;
    if (FAILED(g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &bake))) return false;
    D2D1_BITMAP_PROPERTIES1 bp = {};
    bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    ComPtr<ID2D1Bitmap1> target;
    if (FAILED(bake->CreateBitmapFromDxgiSurface(surf.Get(), bp, &target))) return false;

    ComPtr<ID2D1SolidColorBrush> fill, border;
    bake->CreateSolidColorBrush(g_backgroundBrush->GetColor(), &fill);
    if (g_borderBrush) bake->CreateSolidColorBrush(g_borderBrush->GetColor(), &border);

    // Panel geometry in this surface's coordinates.
    D2D1_RECT_F r = D2D1::RectF(bgRect.left - g.offX, bgRect.top - g.offY, bgRect.right - g.offX,
                                bgRect.bottom - g.offY);
    ComPtr<ID2D1PathGeometry> geo, ringOuter, ringInner;
    ComPtr<ID2D1GeometryGroup> ring;
    CreateRoundedRectPath(g_d2dFactory.Get(), r, radii[0], radii[1], radii[2], radii[3], &geo);
    if (border) {
        float bw = std::min((float)g_settings.bgBorderSize * g_dpiScale,
                            std::min(r.right - r.left, r.bottom - r.top) / 2.0f);
        float grow = g.opaque ? 1.0f : 0.0f;
        D2D1_RECT_F outer = D2D1::RectF(r.left - grow, r.top - grow, r.right + grow, r.bottom + grow);
        CreateRoundedRectPath(g_d2dFactory.Get(), outer, radii[0] + grow, radii[1] + grow, radii[2] + grow,
                              radii[3] + grow, &ringOuter);
        D2D1_RECT_F inner = D2D1::RectF(r.left + bw, r.top + bw, r.right - bw, r.bottom - bw);
        CreateRoundedRectPath(g_d2dFactory.Get(), inner, std::max(0.f, radii[0] - bw), std::max(0.f, radii[1] - bw),
                              std::max(0.f, radii[2] - bw), std::max(0.f, radii[3] - bw), &ringInner);
        if (ringOuter && ringInner) {
            ID2D1Geometry* geos[] = {ringOuter.Get(), ringInner.Get()};
            g_d2dFactory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, geos, 2, &ring);
        }
    }

    bake->SetTarget(target.Get());
    bake->BeginDraw();
    bake->Clear(D2D1::ColorF(0, 0, 0, 0));
    if (g.opaque) {
        // Everything square: the clip rounds it. Blur first, the fill on top.
        if (g_blurredBitmap) {
            bake->SetTransform(D2D1::Matrix3x2F::Translation(-(float)g.offX, -(float)g.offY));
            bake->DrawBitmap(g_blurredBitmap.Get());
            bake->SetTransform(D2D1::Matrix3x2F::Identity());
        }
        bake->FillRectangle(D2D1::RectF(0, 0, (float)g.w, (float)g.h), fill.Get());
        if (ring && border) bake->FillGeometry(ring.Get(), border.Get());
    } else {
        if (g_blurredBitmap && geo) {
            bake->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), geo.Get()), nullptr);
            bake->SetTransform(D2D1::Matrix3x2F::Translation(-(float)g.offX, -(float)g.offY));
            bake->DrawBitmap(g_blurredBitmap.Get());
            bake->SetTransform(D2D1::Matrix3x2F::Identity());
            bake->PopLayer();
        }
        if (geo) bake->FillGeometry(geo.Get(), fill.Get());
        if (ring && border) bake->FillGeometry(ring.Get(), border.Get());
    }
    HRESULT hr = bake->EndDraw();
    bake->SetTarget(nullptr);
    if (FAILED(hr)) return false;
    g.plateValid = true;
    return true;
}

// ---- GPU workload --------------------------------------------------------------------------
// 1 ready, 0 not yet (the engine hasn't published a band layout; drawing
// shows idle bars until it does), -1 failed on this device.
int EnsureGpuAnalysis() {
    if (!CompileCompute()) return -1;
    for (int i = 0; i < 4; i++) {
        if (!g.cs[i] && FAILED(g_d3dDevice->CreateComputeShader(g.csCode[i]->GetBufferPointer(),
                                                                 g.csCode[i]->GetBufferSize(), nullptr, &g.cs[i])))
            return -1;
    }
    if (!g.csCB && FAILED(MakeCB(sizeof(CsCB), true, nullptr, g.csCB))) return -1;

    std::lock_guard<std::mutex> lock(g_gpuFeed.m);
    if (g_gpuFeed.n <= 0 || g_gpuFeed.maps.empty()) return 0;
    if (g.gpuReady && g.gpuSerial == g_gpuFeed.layoutSerial) return 1;

    ReleaseGpuAnalysis();
    const int n = g_gpuFeed.n, half = n / 2, nb = (int)g_gpuFeed.maps.size();
    std::vector<float> desc((size_t)nb * 4), off((size_t)nb * 4);
    for (int b = 0; b < nb; b++) {
        const auto& m = g_gpuFeed.maps[b];
        double fc = g_gpuFeed.bands[b].fc;
        int zone = (fc < 300.0) ? 0 : (fc < 2500.0) ? 1 : 2;
        desc[b * 4 + 0] = (float)(m.tier + 4 * zone);
        desc[b * 4 + 1] = (float)m.k0;
        desc[b * 4 + 2] = (float)m.k1;
        desc[b * 4 + 3] = (float)fc;
        off[b * 4 + 0] = m.offsetDb;
        off[b * 4 + 1] = m.peakOffsetDb;
        off[b * 4 + 2] = m.w0;
        off[b * 4 + 3] = m.w1;
    }
    std::vector<float> zerosA((size_t)std::max(nb, VIZ_BARS_MAX) * 4, 0.f);
    bool ok = SUCCEEDED(MakeStructured(4, 3 * n, false, false, g_gpuFeed.blocks.data(), g.tierIn, &g.tierInSRV, nullptr)) &&
              SUCCEEDED(MakeStructured(4, n, false, false, g_gpuFeed.window.data(), g.window, &g.windowSRV, nullptr)) &&
              SUCCEEDED(MakeStructured(16, nb, false, false, desc.data(), g.bandDesc, &g.bandDescSRV, nullptr)) &&
              SUCCEEDED(MakeStructured(16, nb, false, false, off.data(), g.bandOff, &g.bandOffSRV, nullptr)) &&
              SUCCEEDED(MakeStructured(4, VIZ_BARS_MAX, true, false, nullptr, g.shapeMod, &g.shapeModSRV, nullptr)) &&
              SUCCEEDED(MakeStructured(4, 3 * (half + 1), false, true, nullptr, g.power, nullptr, &g.powerUAV)) &&
              SUCCEEDED(MakeStructured(16, nb, false, true, zerosA.data(), g.bandState, nullptr, &g.bandStateUAV)) &&
              SUCCEEDED(MakeStructured(8, VIZ_BARS_MAX, false, true, zerosA.data(), g.barsGpu, &g.barsGpuSRV, &g.barsGpuUAV)) &&
              SUCCEEDED(MakeStructured(16, VIZ_BARS_MAX, false, true, zerosA.data(), g.barAux, nullptr, &g.barAuxUAV)) &&
              SUCCEEDED(MakeStructured(16, 4, false, true, zerosA.data(), g.globalsGpu, &g.globalsGpuSRV, &g.globalsGpuUAV)) &&
              SUCCEEDED(MakeStructured(4, 4, false, true, zerosA.data(), g.stats, nullptr, &g.statsUAV));
    if (ok) {
        // Same size and structured flags as the stats buffer, so CopyResource
        // between them is valid on every driver.
        for (auto& s : g.staging) {
            D3D11_BUFFER_DESC d = {};
            d.ByteWidth = 16;
            d.Usage = D3D11_USAGE_STAGING;
            d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            d.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            d.StructureByteStride = 4;
            if (FAILED(g_d3dDevice->CreateBuffer(&d, nullptr, &s))) ok = false;
        }
    }
    if (!ok) {
        ReleaseGpuAnalysis();
        return -1;
    }
    g.gpuN = n;
    g.gpuBands = nb;
    g.gpuSerial = g_gpuFeed.layoutSerial;
    g.tierScratch.assign((size_t)3 * n, 0.f);
    g_gpuFeed.dirty = 7;
    g.gpuReady = true;
    g.lastMaxDeltaPx = 1e9f;
    Wh_Log(L"[GPU] analysis on the GPU: %d bands, FFT %d", nb, n);
    return 1;
}

// Runs the four analysis passes. Leaves barsGpu / globalsGpu holding this
// frame's bars for the draw.
void RunGpuAnalysis(int bars, float barRangePx) {
    unsigned dirty = 0;
    int n = g.gpuN;
    double rates[3];
    double peakCal;
    {
        std::lock_guard<std::mutex> lock(g_gpuFeed.m);
        // The layout can change between EnsureGpuAnalysis and here; the
        // blocks then have a different size, so take nothing this frame.
        if (g_gpuFeed.layoutSerial == g.gpuSerial && g_gpuFeed.blocks.size() >= (size_t)3 * n) {
            dirty = g_gpuFeed.dirty;
            g_gpuFeed.dirty = 0;
        }
        if (dirty) memcpy(g.tierScratch.data(), g_gpuFeed.blocks.data(), sizeof(float) * 3 * (size_t)n);
        for (int t = 0; t < 3; t++) rates[t] = g_gpuFeed.rate[t];
        peakCal = g_gpuFeed.peakCal;
    }
    uint32_t tierList = 0, tierCount = 0, used = 0;
    for (int t = 0; t < 3; t++) {
        if (!(dirty & (1u << t))) continue;
        D3D11_BOX box = {(UINT)(t * n * 4), 0, 0, (UINT)((t + 1) * n * 4), 1, 1};
        g.ctx->UpdateSubresource(g.tierIn.Get(), 0, &box, &g.tierScratch[(size_t)t * n], 0, 0);
        tierList |= (uint32_t)t << (2 * tierCount);
        tierCount++;
    }
    {
        std::lock_guard<std::mutex> lock(g_gpuFeed.m);
        for (int t = 0; t < 3; t++)
            if (g_gpuFeed.used[t]) used |= 1u << t;
    }

    VizEngineConfig cfg;
    {
        std::lock_guard<std::mutex> lock(g_engineCfgMutex);
        cfg = g_engineCfg;
    }
    D3D11_MAPPED_SUBRESOURCE ms;
    if (SUCCEEDED(g.ctx->Map(g.csCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        CsCB c = {};
        c.n = (uint32_t)n;
        c.half = (uint32_t)n / 2;
        c.log2Half = (uint32_t)ttdsp::Log2i(n / 2);
        c.numBands = (uint32_t)g.gpuBands;
        c.numBars = (uint32_t)bars;
        c.tierCount = tierCount | (used << 8);
        c.tierList = tierList;
        c.detector = cfg.spec.detector == ttdsp::Detector::Peak ? 1u : 0u;
        c.floorDb = cfg.disp.floorDb;
        c.rangeDb = std::max(1.f, cfg.disp.ceilDb - cfg.disp.floorDb);
        c.sensDb = cfg.sensDb;
        c.dt = std::clamp(g_frameDt, 0.0005f, 0.25f);
        c.attackMs = (float)cfg.ball.attackMs;
        c.releaseMs = (float)cfg.ball.releaseMs;
        c.releaseDbPerSec = (float)cfg.ball.releaseDbPerSec;
        c.autoGainMaxDb = cfg.autoGainMaxDb;
        c.releaseLinear = cfg.ball.release == ttdsp::ReleaseKind::Linear ? 1u : 0u;
        c.curve = (uint32_t)cfg.disp.curve;
        c.shape = (uint32_t)g_settings.shape;
        c.autoGain = cfg.autoGain ? 1u : 0u;
        c.peakHoldMs = (float)g_settings.peakHoldMs;
        c.gravity = 2.0f;
        c.linFall = 0.72f;
        c.barRangePx = barRangePx;
        c.peakGravity = g_settings.peakFall == VizPeakFall::Gravity ? 1u : 0u;
        c.domOn = cfg.wantDominant ? 1u : 0u;
        c.tierRate0 = (float)rates[0];
        c.tierRate1 = (float)rates[1];
        c.tierRate2 = (float)rates[2];
        c.peakCal = (float)peakCal;
        c.fmin = (float)cfg.spec.fmin;
        c.fmax = (float)cfg.spec.fmax;
        c.breatheUp = 0.408f;
        c.breatheDown = 1.103f;
        memcpy(ms.pData, &c, sizeof(c));
        g.ctx->Unmap(g.csCB.Get(), 0);
    }
    if (g_settings.shape == VizShape::Wave || g_settings.shape == VizShape::Breathe) {
        if (SUCCEEDED(g.ctx->Map(g.shapeMod.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            float* m = (float*)ms.pData;
            double clock = VizClockSeconds();
            for (int i = 0; i < bars; i++) m[i] = VizShapeMod(i, bars, clock);
            g.ctx->Unmap(g.shapeMod.Get(), 0);
        }
    }

    ID3D11ShaderResourceView* srvs[5] = {g.tierInSRV.Get(), g.windowSRV.Get(), g.bandDescSRV.Get(),
                                         g.bandOffSRV.Get(), g.shapeModSRV.Get()};
    ID3D11UnorderedAccessView* uavs[6] = {g.powerUAV.Get(), g.bandStateUAV.Get(), g.barsGpuUAV.Get(),
                                          g.barAuxUAV.Get(), g.globalsGpuUAV.Get(), g.statsUAV.Get()};
    ID3D11Buffer* cbs[1] = {g.csCB.Get()};
    g.ctx->CSSetShaderResources(5, 5, srvs);
    g.ctx->CSSetUnorderedAccessViews(0, 6, uavs, nullptr);
    g.ctx->CSSetConstantBuffers(2, 1, cbs);
    if (tierCount) {
        g.ctx->CSSetShader(g.cs[0].Get(), nullptr, 0);
        g.ctx->Dispatch(tierCount, 1, 1);
    }
    g.ctx->CSSetShader(g.cs[1].Get(), nullptr, 0);
    g.ctx->Dispatch((UINT)(g.gpuBands + 63) / 64, 1, 1);
    g.ctx->CSSetShader(g.cs[2].Get(), nullptr, 0);
    g.ctx->Dispatch(1, 1, 1);
    g.ctx->CSSetShader(g.cs[3].Get(), nullptr, 0);
    g.ctx->Dispatch((UINT)(bars + 63) / 64, 1, 1);
    ID3D11UnorderedAccessView* nullU[6] = {};
    ID3D11ShaderResourceView* nullS[5] = {};
    g.ctx->CSSetUnorderedAccessViews(0, 6, nullU, nullptr);
    g.ctx->CSSetShaderResources(5, 5, nullS);
    g.ctx->CSSetShader(nullptr, nullptr, 0);

    // Stats: copy this frame's, read the one from two frames ago if it's
    // ready, never wait for it.
    unsigned slot = g.frameNo % 3;
    g.ctx->CopyResource(g.staging[slot].Get(), g.stats.Get());
    if (g.frameNo >= 2) {
        unsigned old = (g.frameNo + 1) % 3;
        if (SUCCEEDED(g.ctx->Map(g.staging[old].Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &ms))) {
            uint32_t v[2];
            memcpy(v, ms.pData, 8);
            g.ctx->Unmap(g.staging[old].Get(), 0);
            float hz, delta;
            memcpy(&hz, &v[0], 4);
            memcpy(&delta, &v[1], 4);
            if (hz > 0.f) g_dominantFreqHz.store(hz, std::memory_order_relaxed);
            g.lastMaxDeltaPx = delta;
        }
    }
    g.frameNo++;
}

// ---- Goniometer points (shared with the Direct2D fallback) --------------------------------
// Newest samples as this frame's points, plus the previous five frames' at
// falling alpha: persistence without a second render target.
int BuildGonioPoints(float* out4, int maxPoints) {
    uint32_t serial = g_gonioSerial.load(std::memory_order_acquire);
    if (serial != g.gonioSerial) {
        g.gonioSerial = serial;
        g.gonioHead = (g.gonioHead + 1) % kGonioFrames;
        std::lock_guard<std::mutex> lock(g_gonioMutex);
        // At most 512 points per frame, evenly strided.
        size_t n = g_gonioXY.size() / 2;
        size_t step = std::max<size_t>(1, n / 512);
        auto& h = g.gonioHist[g.gonioHead];
        h.clear();
        for (size_t i = 0; i < n; i += step) {
            h.push_back(g_gonioXY[2 * i]);
            h.push_back(g_gonioXY[2 * i + 1]);
        }
    }
    int count = 0;
    float alpha = 1.0f;
    for (int age = 0; age < kGonioFrames; age++) {
        const auto& h = g.gonioHist[(g.gonioHead - age + kGonioFrames) % kGonioFrames];
        // Scaled by sqrt(2) so a full-scale mono signal reaches the top.
        for (size_t i = 0; i + 1 < h.size() && count < maxPoints; i += 2) {
            out4[count * 4 + 0] = h[i] * 1.41421356f;
            out4[count * 4 + 1] = h[i + 1] * 1.41421356f;
            out4[count * 4 + 2] = alpha * 0.85f;
            out4[count * 4 + 3] = 0.f;
            count++;
        }
        alpha *= 0.55f;
    }
    return count;
}

// ---- Terminal shape ----------------------------------------------------------------------
//
// Printable ASCII (32-126) baked once, white, into a 16 x 6 atlas of cells
// exactly the size of a grid cell, so the pixel shader samples it 1:1 with
// point filtering: a pixel font stays pixel-exact, and the whole grid is one
// instanced draw. Rebaked only when the font, its size or the text rendering
// mode changes.
constexpr UINT kAtlasCols = 16, kAtlasRows = 6;

bool EnsureTermResources() {
    if (!g.cellsDyn) {
        if (FAILED(MakeStructured(4, VIZ_TERM_MAX_CELLS, true, false, nullptr, g.cellsDyn, &g.cellsSRV, nullptr)))
            return false;
        g.cellsValid = false;
    }
    if (!VizTermEnsureFormat() || !g_d2dDevice) return false;
    uint64_t key = 1469598103934665603ull;
    for (wchar_t c : g_termFormatFont) Mix(key, (uint64_t)c);
    MixF(key, g_termFormatPx, 64.f);
    Mix(key, (uint64_t)g_termCellW * 4096u + (uint64_t)g_termCellH);
    Mix(key, g_settings.textPixel ? 1u : 0u);
    if (g.glyphSRV && key == g.glyphKey) return true;
    g.glyphSRV.Reset();
    g.glyphTex.Reset();
    UINT w = kAtlasCols * (UINT)g_termCellW, h = kAtlasRows * (UINT)g_termCellH;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    if (FAILED(g_d3dDevice->CreateTexture2D(&td, nullptr, &g.glyphTex))) return false;
    ComPtr<IDXGISurface> surf;
    ComPtr<ID2D1DeviceContext> dc;
    ComPtr<ID2D1Bitmap1> target;
    ComPtr<ID2D1SolidColorBrush> white;
    if (FAILED(g.glyphTex.As(&surf)) ||
        FAILED(g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc)))
        return false;
    D2D1_BITMAP_PROPERTIES1 bp = {};
    bp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
    bp.dpiX = bp.dpiY = 96.f;
    bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    if (FAILED(dc->CreateBitmapFromDxgiSurface(surf.Get(), &bp, &target)) ||
        FAILED(dc->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &white)))
        return false;
    dc->SetTarget(target.Get());
    dc->SetTextAntialiasMode(g_settings.textPixel ? D2D1_TEXT_ANTIALIAS_MODE_ALIASED
                                                  : D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    dc->BeginDraw();
    dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    for (wchar_t c = 33; c < 127; c++) {
        UINT i = (UINT)(c - 32);
        // Cell column and row in the atlas (whole numbers on purpose), then pixels.
        UINT col = i % kAtlasCols, row = i / kAtlasCols;
        float x = (float)(col * (UINT)g_termCellW), y = (float)(row * (UINT)g_termCellH);
        dc->DrawText(&c, 1, g_termFormat.Get(), D2D1::RectF(x, y, x + g_termCellW, y + g_termCellH), white.Get(),
                     D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    if (FAILED(dc->EndDraw())) {
        g.glyphTex.Reset();
        return false;
    }
    dc->SetTarget(nullptr);
    if (FAILED(g_d3dDevice->CreateShaderResourceView(g.glyphTex.Get(), nullptr, &g.glyphSRV))) {
        g.glyphTex.Reset();
        return false;
    }
    g.glyphKey = key;
    g.glyphW = w;
    g.glyphH = h;
    g.forcePresent = true;
    return true;
}

// ---- The frame -------------------------------------------------------------------------------
struct FrameInputs {
    const VizLayout* layout;
    D2D1_RECT_F bgRect;       // layout-local; valid when hasPanel
    bool hasPanel;
    float bgRadii[4];
    float sceneAlpha;
    bool dragPause;
    RGBA c1, cGrad1, c2;
    float rainbowBase;
    float scopeDisp[VIZ_WAVE_SAMPLES];
    float zones[3];
    float correlation;
};

// Returns false if nothing could be drawn this way (the caller then uses the
// Direct2D path for this frame).
bool Render(const FrameInputs& in) {
    if (!g.ctx) return false;
    const VizLayout& L = *in.layout;
    const bool horizontal = g_settings.orientation == VizOrientation::Horizontal;
    const VizShape shape = g_settings.shape;
    const int bars = VizEffectiveBarCount();
    const float barW = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    const float barGap = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    const float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;
    const float idleSize = (float)std::max(0, g_settings.barIdleSize) * g_dpiScale;
    const bool roundShape = shape == VizShape::Radial || shape == VizShape::Goniometer;
    const bool term = shape == VizShape::Terminal;

    // ---- Surface: the panel, or the bars plus their bleed without one -----
    float margin = std::max(4.0f * g_dpiScale, barW);
    if (shape == VizShape::Radial) margin += maxSize * 0.15f;
    D2D1_RECT_F content = D2D1::RectF(L.blockX - margin, L.blockY - margin, L.blockX + L.totalWidth + margin,
                                      L.blockY + L.totalHeight + margin);
    D2D1_RECT_F want = content;
    bool opaque = false;
    if (in.hasPanel) {
        const D2D1_RECT_F& b = in.bgRect;
        bool plateOpaque = (g_settings.bgBlur > 0 && g_blurredBitmap) || g_settings.bgA == 255;
        // Everything drawn has to sit inside the panel for the clip not to cut
        // it: bars and their antialiased edge, stroked shapes' half stroke.
        float bleed = roundShape ? margin : (shape == VizShape::Oscilloscope ? barW : 1.0f);
        bool inside = b.left <= L.blockX - bleed && b.top <= L.blockY - bleed &&
                      b.right >= L.blockX + L.totalWidth + bleed && b.bottom >= L.blockY + L.totalHeight + bleed;
        opaque = g_settings.opaquePanel == VizOpaquePanel::Auto && plateOpaque && inside &&
                 !g_settings.autoHideEnabled;
        want = opaque ? b
                      : D2D1::RectF(std::min(b.left - 2.f, content.left), std::min(b.top - 2.f, content.top),
                                    std::max(b.right + 2.f, content.right), std::max(b.bottom + 2.f, content.bottom));
    }
    int offX = (int)floorf(want.left), offY = (int)floorf(want.top);
    UINT w = (UINT)std::max(1.f, ceilf(want.right) - (float)offX);
    UINT h = (UINT)std::max(1.f, ceilf(want.bottom) - (float)offY);
    D2D1_RECT_F clipRect = D2D1::RectF(in.bgRect.left - offX, in.bgRect.top - offY, in.bgRect.right - offX,
                                       in.bgRect.bottom - offY);
    if (!EnsureSurface(offX, offY, w, h, opaque, clipRect, in.bgRadii, L.originX, L.originY)) return false;

    // ---- Auto-hide fully faded: one blank frame, then nothing --------------
    if (in.sceneAlpha <= 0.001f) {
        if (!g.blankPresented) {
            PresentBlank();
            g.blankPresented = true;
            VizPerf(kPerfPresents);
        } else {
            VizPerf(kPerfSkipped);
        }
        return true;
    }
    g.blankPresented = false;

    // ---- Plate -----------------------------------------------------------------
    if (in.hasPanel) {
        uint64_t key = 1469598103934665603ull;
        MixF(key, in.bgRect.left, 64.f); MixF(key, in.bgRect.top, 64.f);
        MixF(key, in.bgRect.right, 64.f); MixF(key, in.bgRect.bottom, 64.f);
        for (int i = 0; i < 4; i++) MixF(key, in.bgRadii[i], 64.f);
        Mix(key, (uint64_t)(uintptr_t)g_blurredBitmap.Get());
        Mix(key, g.opaque);
        Mix(key, g.w * 65536u + g.h);
        if (!g.plateValid || key != g.plateKey) {
            g.plateKey = key;
            if (!BakePlate(in.bgRect, in.bgRadii)) g.plateValid = false;
            g.forcePresent = true;
        }
    }

    // ---- Bar state ---------------------------------------------------------------
    // The Terminal grid is built on the CPU from the bar levels, so that
    // shape keeps the analysis there (Hybrid).
    const bool gpuWork = g_settings.workload == VizWorkload::Gpu && g_settings.engine == VizEngineKind::Precision &&
                         !in.dragPause && !term;
    bool gpuOk = false;
    if (gpuWork) {
        int st = EnsureGpuAnalysis();
        gpuOk = st > 0;
        if (st < 0 && !g.gpuWarned) {
            g.gpuWarned = true;
            ReportSettingWarning(L"Hardware", L"Workload",
                                 L"The GPU analysis passes couldn't start on this device, so analysis runs on the "
                                 L"CPU (Hybrid) instead.");
            FlushSettingsIssues();
        }
        if (gpuOk) RunGpuAnalysis(bars, std::max(0.f, maxSize - idleSize));
    }

    // ---- Frame constants -----------------------------------------------------------
    FrameCB f = {};
    f.viewport[0] = (float)g.w;
    f.viewport[1] = (float)g.h;
    f.block[0] = L.blockX - (float)g.offX;
    f.block[1] = L.blockY - (float)g.offY;
    f.barW = barW;
    f.barGap = barGap;
    f.maxSize = maxSize;
    f.idleSize = idleSize;
    float rr[4] = {g_settings.barRadiusTL * g_dpiScale, g_settings.barRadiusTR * g_dpiScale,
                   g_settings.barRadiusBR * g_dpiScale, g_settings.barRadiusBL * g_dpiScale};
    memcpy(f.radii, rr, sizeof(rr));
    memcpy(f.dotRadii, rr, sizeof(rr));
    f.barCount = (uint32_t)bars;
    f.shape = (uint32_t)shape;
    f.vertical = horizontal ? 0u : 1u;
    f.anchor = g_settings.verticalAnchor == VizAnchor::Top ? 0u : g_settings.verticalAnchor == VizAnchor::Middle ? 1u : 2u;
    f.colorMode = (uint32_t)g_settings.colorMode;
    f.flags = (g_settings.peakHoldEnabled ? 1u : 0u) | (g_settings.beatFlashEnabled ? 2u : 0u) |
              (g_settings.oscilloscopeMultibandEnabled ? 4u : 0u);
    auto setCol = [](float* d, RGBA c) {
        d[0] = c.r / 255.f;
        d[1] = c.g / 255.f;
        d[2] = c.b / 255.f;
        d[3] = c.a / 255.f;
    };
    setCol(f.c1, in.c1);
    setCol(f.grad1, in.cGrad1);
    setCol(f.c2, in.c2);
    setCol(f.peakColor, RGBA{g_settings.peakHoldA, g_settings.peakHoldR, g_settings.peakHoldG, g_settings.peakHoldB});
    setCol(f.beatColor, RGBA{g_settings.beatFlashA, g_settings.beatFlashR, g_settings.beatFlashG, g_settings.beatFlashB});
    f.beatIntensity = g_settings.beatFlashIntensity / 100.0f;
    // Only Rainbow Cycle reads the hue clock; leaving it at 0 otherwise keeps
    // the frame constants unchanged (and un-uploaded) from frame to frame.
    f.rainbowBase = g_settings.colorMode == VizColorMode::RainbowCycle ? in.rainbowBase : 0.f;
    f.sceneAlpha = in.sceneAlpha;
    f.capThickness = std::max(1.5f, 2.0f * g_dpiScale);
    f.dotStep = barW + barGap;
    f.dotR = barW * 0.5f;
    f.maxDots = (uint32_t)ceilf(maxSize / std::max(0.5f, f.dotStep)) + 1u;
    f.dotSlots = (f.anchor == 1u) ? f.maxDots * 2u + 1u : f.maxDots;
    f.innerR = maxSize * 0.15f;
    f.strokeW = std::max(1.0f, barW);
    f.center[0] = f.block[0] + L.totalWidth * 0.5f;
    f.center[1] = f.block[1] + L.totalHeight * 0.5f;
    f.ampScale = maxSize * 0.5f;
    f.sweepLen = horizontal ? L.totalWidth : L.totalHeight;
    f.scopeCenter = horizontal ? f.block[1] + L.totalHeight * 0.5f : f.block[0] + L.totalWidth * 0.5f;
    f.sweepOrigin = horizontal ? f.block[0] : f.block[1];
    f.wstep = f.sweepLen / (float)(VIZ_WAVE_SAMPLES - 1);
    f.gonioR = maxSize * 0.85f;
    f.gonioDot = std::max(0.75f, barW * 0.2f);
    f.corrH = std::max(2.0f, 3.0f * g_dpiScale);
    f.corrY = f.center[1] + maxSize * 0.92f - f.corrH;
    f.corr = shape == VizShape::Goniometer ? in.correlation : 0.f;
    // Scope colour: the Colour Mode rules for a single line, as in 1.5.
    {
        RGBA col = in.c1;
        if (g_settings.colorMode == VizColorMode::Gradient || g_settings.colorMode == VizColorMode::Tourne)
            col = LerpColor(in.cGrad1, in.c2, 0.5f);
        else if (g_settings.colorMode == VizColorMode::RainbowCycle)
            col = HSVtoRGB(fmodf(in.rainbowBase, 360.f), 0.85f, 1.0f, in.c1.a);
        setCol(f.scopeColor, col);
    }
    // Terminal: palette, grid origin on a whole pixel (glyphs land 1:1), cell
    // size, atlas size.
    bool termReady = false;
    if (term && !in.dragPause) {
        termReady = EnsureTermResources() && !g_termGrid.cells.empty();
        VizTermPalette(f.termColors);
        f.termGeom[0] = floorf(f.block[0] + 0.5f);
        f.termGeom[1] = floorf(f.block[1] + 0.5f);
        f.termGeom[2] = (float)g_termCellW;
        f.termGeom[3] = (float)g_termCellH;
        f.termAtlas[0] = (float)g.glyphW;
        f.termAtlas[1] = (float)g.glyphH;
        f.termCols = (uint32_t)g_termGrid.cols;
        f.termRows = (uint32_t)g_termGrid.rows;
        f.termAtlasCols = kAtlasCols;
    }
    f.plateRect[0] = 0.f;
    f.plateRect[1] = 0.f;
    f.plateRect[2] = (float)g.w;
    f.plateRect[3] = (float)g.h;

    // ---- Did anything change? ---------------------------------------------------------
    // Hashed from the CPU-side inputs first; the dynamic buffers are mapped
    // only when the frame will actually be drawn, and then only the ones
    // whose contents changed since their last upload.
    const float rangePx = std::max(0.f, maxSize - idleSize);
    float pulse = g_beatPulse.load(std::memory_order_relaxed);
    uint64_t hash = 1469598103934665603ull;
    Mix(hash, g.w * 65536u + g.h);
    Mix(hash, (uint64_t)g.offX * 65536u + (uint64_t)(uint32_t)g.offY);
    Mix(hash, g.plateKey);
    Mix(hash, g.plateValid);
    MixF(hash, in.sceneAlpha, 255.f);
    MixF(hash, f.block[0], 64.f);
    MixF(hash, f.block[1], 64.f);
    Mix(hash, (uint64_t)shape * 131u + (uint64_t)g_settings.colorMode);
    for (int k = 0; k < 4; k++) {
        MixF(hash, f.c1[k], 255.f);
        MixF(hash, f.grad1[k], 255.f);
        MixF(hash, f.c2[k], 255.f);
    }
    if (g_settings.colorMode == VizColorMode::RainbowCycle) MixF(hash, in.rainbowBase, 2.f);
    if (g_settings.beatFlashEnabled) MixF(hash, pulse, 128.f);
    Mix(hash, in.dragPause);

    const bool drawBars = !in.dragPause;
    const bool cpuBars = !gpuOk && drawBars;
    // Bars: what is drawn, to a quarter pixel. The same key decides whether
    // barsDyn needs new contents: a change smaller than that can't be seen.
    uint64_t barsKey = 0, globalsKey = 0;
    // Globals hold only what the shader reads: the beat pulse when Beat Flash
    // is on, the zone energies when the multiband colour is (both are gated
    // by fFlags there). Anything else would change, and re-upload, every frame.
    const float glPulse = g_settings.beatFlashEnabled ? pulse : 0.f;
    const bool glZones = g_settings.oscilloscopeMultibandEnabled;
    const float glZ[3] = {glZones ? in.zones[0] : 0.f, glZones ? in.zones[1] : 0.f, glZones ? in.zones[2] : 0.f};
    if (cpuBars) {
        barsKey = 1469598103934665603ull;
        Mix(barsKey, (uint64_t)bars);
        const bool caps = g_settings.peakHoldEnabled;
        for (int i = 0; i < bars; i++) {
            MixF(barsKey, std::max(0.f, g_vizPeak[i]) * rangePx, 4.f);
            if (caps) MixF(barsKey, g_vizPeakHold[i] * rangePx, 4.f);
        }
        Mix(hash, barsKey);
        globalsKey = 1469598103934665603ull;
        uint32_t bits[4];
        memcpy(&bits[0], &glPulse, 4);
        memcpy(&bits[1], glZ, 12);
        for (uint32_t b : bits) Mix(globalsKey, b);
        if (g_settings.oscilloscopeMultibandEnabled)
            for (int z = 0; z < 3; z++) MixF(hash, in.zones[z], 64.f);
    } else if (gpuOk) {
        // GPU bars aren't visible from here; the stats block says how far the
        // last frames moved, two frames late.
        Mix(hash, g.lastMaxDeltaPx >= 0.25f ? (uint64_t)g.frameNo : 0ull);
    }
    uint64_t waveKey = 0;
    if (shape == VizShape::Oscilloscope && drawBars) {
        waveKey = 1469598103934665603ull;
        for (int i = 0; i < VIZ_WAVE_SAMPLES; i++) MixF(waveKey, in.scopeDisp[i] * f.ampScale, 4.f);
        Mix(hash, waveKey);
    }
    // Terminal: VizBuildTermGrid bumps g_termGridSerial only when a cell
    // actually changed, so one number stands for all 65,536 of them.
    const uint64_t cellsShape = ((uint64_t)(uint32_t)g_termGrid.cols << 32) | (uint32_t)g_termGrid.rows;
    if (termReady) {
        Mix(hash, g.glyphKey);
        Mix(hash, cellsShape);
        Mix(hash, (uint64_t)g_termGridSerial);
        for (int c = 0; c < 5; c++)
            for (int k = 0; k < 4; k++) MixF(hash, f.termColors[c][k], 255.f);
    }
    const bool gonio = shape == VizShape::Goniometer && drawBars;
    const uint32_t gonioSerial = g_gonioSerial.load(std::memory_order_acquire);
    if (gonio) {
        Mix(hash, gonioSerial);
        MixF(hash, in.correlation, 256.f);
    }

    if (!g.forcePresent && hash == g.lastHash) {  // nothing changed: no upload, no draw, no present
        VizPerf(kPerfSkipped);
        return true;
    }
    g.lastHash = hash;
    g.forcePresent = false;

    // ---- Uploads (CPU paths), only what changed ---------------------------------------
    D3D11_MAPPED_SUBRESOURCE ms;
    if (cpuBars) {
        if (!g.uploadsValid || barsKey != g.barsKey) {
            if (SUCCEEDED(g.ctx->Map(g.barsDyn.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
                float* p = (float*)ms.pData;
                for (int i = 0; i < bars; i++) {
                    p[2 * i] = std::max(0.f, g_vizPeak[i]);
                    p[2 * i + 1] = g_vizPeakHold[i];
                }
                g.ctx->Unmap(g.barsDyn.Get(), 0);
                VizPerf(kPerfMaps);
                g.barsKey = barsKey;
            }
        }
        if (!g.uploadsValid || globalsKey != g.globalsKey) {
            if (SUCCEEDED(g.ctx->Map(g.globalsDyn.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
                float gl[16] = {glPulse, 0, 0, 0, glZ[0], glZ[1], glZ[2], 0};
                memcpy(ms.pData, gl, sizeof(gl));
                g.ctx->Unmap(g.globalsDyn.Get(), 0);
                VizPerf(kPerfMaps);
                g.globalsKey = globalsKey;
            }
        }
    }
    if (waveKey && (!g.uploadsValid || waveKey != g.waveKey)) {
        if (SUCCEEDED(g.ctx->Map(g.waveDyn.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            memcpy(ms.pData, in.scopeDisp, sizeof(float) * VIZ_WAVE_SAMPLES);
            g.ctx->Unmap(g.waveDyn.Get(), 0);
            VizPerf(kPerfMaps);
            g.waveKey = waveKey;
        }
    }
    if (termReady && (!g.cellsValid || g.cellsSerial != g_termGridSerial || g.cellsShape != cellsShape)) {
        // Only the cells with a glyph, each as char | colour << 8 | index << 16:
        // blanks cost neither upload nor a vertex-shader instance.
        const size_t n = std::min(g_termGrid.cells.size(), (size_t)VIZ_TERM_MAX_CELLS);
        if (SUCCEEDED(g.ctx->Map(g.cellsDyn.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            uint32_t* out = (uint32_t*)ms.pData;
            const uint32_t* src = g_termGrid.cells.data();
            UINT k = 0;
            for (size_t i = 0; i < n; i++) {  // branch-free: k <= i, always in bounds
                uint32_t cell = src[i];
                out[k] = (cell & 0xFFFFu) | ((uint32_t)i << 16);
                k += ((cell & 127u) > 32u) ? 1u : 0u;
            }
            g.ctx->Unmap(g.cellsDyn.Get(), 0);
            VizPerf(kPerfMaps);
            g.termCount = k;
            g.cellsSerial = g_termGridSerial;
            g.cellsShape = cellsShape;
            g.cellsValid = true;
        }
    }
    int points = g.pointsCount;
    if (gonio && (!g.uploadsValid || gonioSerial != g.pointsSerial)) {
        // The persistence history only moves when the engine publishes a new
        // block, so the points only need rebuilding then.
        if (SUCCEEDED(g.ctx->Map(g.pointsDyn.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            points = BuildGonioPoints((float*)ms.pData, kMaxPoints);
            g.ctx->Unmap(g.pointsDyn.Get(), 0);
            VizPerf(kPerfMaps);
            g.pointsCount = points;
            g.pointsSerial = gonioSerial;
        }
    }
    g.uploadsValid = true;

    if (!g.frameCBValid || memcmp(&f, &g.frameCBLast, sizeof(f)) != 0) {
        if (SUCCEEDED(g.ctx->Map(g.frameCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            memcpy(ms.pData, &f, sizeof(f));
            g.ctx->Unmap(g.frameCB.Get(), 0);
            VizPerf(kPerfMaps);
            memcpy(&g.frameCBLast, &f, sizeof(f));  // bytes, padding included, for the memcmp above
            g.frameCBValid = true;
        }
    }

    // ---- Draw -----------------------------------------------------------------------
    // The plate is the size of the surface and drawn first, 1:1, without
    // blending, so it writes every pixel (transparent outside the panel):
    // a clear before it would only be overwritten.
    const bool drawPlate = in.hasPanel && g.plateValid;
    ID3D11RenderTargetView* rtv = g.rtv.Get();
    g.ctx->OMSetRenderTargets(1, &rtv, nullptr);
    if (!drawPlate) {
        const float zero[4] = {0, 0, 0, 0};
        g.ctx->ClearRenderTargetView(rtv, zero);
    }
    D3D11_VIEWPORT vp = {0, 0, (float)g.w, (float)g.h, 0, 1};
    g.ctx->RSSetViewports(1, &vp);
    g.ctx->RSSetState(g.raster.Get());
    g.ctx->OMSetDepthStencilState(nullptr, 0);
    g.ctx->IASetInputLayout(nullptr);
    g.ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g.ctx->VSSetShader(g.vs.Get(), nullptr, 0);
    g.ctx->PSSetShader(g.ps.Get(), nullptr, 0);
    ID3D11ShaderResourceView* vsSrv[4] = {gpuOk ? g.barsGpuSRV.Get() : g.barsDynSRV.Get(),
                                          gpuOk ? g.globalsGpuSRV.Get() : g.globalsDynSRV.Get(), g.waveSRV.Get(),
                                          g.pointsSRV.Get()};
    g.ctx->VSSetShaderResources(0, 4, vsSrv);
    ID3D11ShaderResourceView* psSrv[5] = {nullptr, nullptr, nullptr, nullptr, g.plateSRV.Get()};
    g.ctx->PSSetShaderResources(0, 5, psSrv);
    if (termReady) {
        ID3D11ShaderResourceView* cells = g.cellsSRV.Get();
        ID3D11ShaderResourceView* glyphs = g.glyphSRV.Get();
        g.ctx->VSSetShaderResources(10, 1, &cells);
        g.ctx->PSSetShaderResources(11, 1, &glyphs);
    }
    ID3D11SamplerState* smp = g.sampler.Get();
    g.ctx->PSSetSamplers(0, 1, &smp);
    ID3D11Buffer* fcb = g.frameCB.Get();
    g.ctx->VSSetConstantBuffers(0, 1, &fcb);
    g.ctx->PSSetConstantBuffers(0, 1, &fcb);
    auto draw = [&](Pass p, UINT count) {
        if (!count) return;
        ID3D11Buffer* pcb = g.passCB[p].Get();
        g.ctx->VSSetConstantBuffers(1, 1, &pcb);
        g.ctx->DrawInstanced(4, count, 0, 0);
    };
    if (drawPlate) {
        g.ctx->OMSetBlendState(g.blendOff.Get(), nullptr, 0xffffffff);
        draw(kPlate, 1);
    }
    g.ctx->OMSetBlendState(g.blend.Get(), nullptr, 0xffffffff);
    if (drawBars) {
        switch (shape) {
            case VizShape::Dots: draw(kDots, (UINT)bars * f.dotSlots); break;
            case VizShape::Radial: draw(kRadial, (UINT)bars); break;
            case VizShape::Terminal:
                if (termReady) draw(kTerm, g.termCount);
                break;
            case VizShape::Oscilloscope: draw(kScope, VIZ_WAVE_SAMPLES - 1); break;
            case VizShape::Goniometer:
                draw(kGonio, (UINT)points);
                draw(kCorr, 2);
                break;
            default:
                draw(kBars, (UINT)bars);
                if (g_settings.peakHoldEnabled) draw(kCaps, (UINT)bars);
                break;
        }
    }
    ID3D11ShaderResourceView* nullSrv[5] = {};
    g.ctx->VSSetShaderResources(0, 4, nullSrv);
    g.ctx->PSSetShaderResources(0, 5, nullSrv);
    if (termReady) {
        g.ctx->VSSetShaderResources(10, 1, nullSrv);
        g.ctx->PSSetShaderResources(11, 1, nullSrv);
    }

    HRESULT hr = g.sc->Present(0, 0);
    VizCheckDeviceLost(S_OK, hr);
    VizPerf(kPerfPresents);
    return true;
}

}  // namespace ttgfx
