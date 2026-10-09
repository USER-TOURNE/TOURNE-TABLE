// Executes the mod's HLSL (through hlsl_shim.h) on the CPU:
//   raster <cfg> <out.f32>      rasterizes the draw passes like the GPU would
//   compute <cfg> <in.f32> <out> runs CsFft -> CsBands -> CsReduce -> CsShape
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <thread>

#include "hlsl_shim.h"
#include "tt_cb.hlsl"

struct Ctx : FrameCB, PassCB, CsCB {
    std::barrier<>* tt_barrier = nullptr;
#include "tt_body.hlsl"
};

#include "ttdsp.h"

static std::map<std::string, std::vector<double>> ReadCfg(const char* path) {
    std::map<std::string, std::vector<double>> m;
    FILE* f = fopen(path, "r");
    char key[128];
    while (fscanf(f, "%127s", key) == 1) {
        std::vector<double> v;
        double d;
        while (fscanf(f, "%lf", &d) == 1) v.push_back(d);
        m[key] = v;
        int c = fgetc(f);  // ';' separates entries
        (void)c;
    }
    fclose(f);
    return m;
}

static void WriteF32(const char* p, const std::vector<float>& v) {
    FILE* f = fopen(p, "wb");
    fwrite(v.data(), 4, v.size(), f);
    fclose(f);
}

static float4 F4(const std::vector<double>& v) { return {(float)v[0], (float)v[1], (float)v[2], (float)v[3]}; }

int main(int argc, char** argv) {
    std::string mode = argv[1];
    auto cfg = ReadCfg(argv[2]);
    auto num = [&](const char* k, double def) { return cfg.count(k) ? cfg[k][0] : def; };
    Ctx* c = new Ctx();
    c->fViewport = {(float)num("vw", 400), (float)num("vh", 200)};
    c->fBlock = {(float)num("bx", 10), (float)num("by", 10)};
    c->fBarW = (float)num("barW", 6);
    c->fBarGap = (float)num("gap", 4);
    c->fMaxSize = (float)num("maxSize", 100);
    c->fIdleSize = (float)num("idle", 4);
    c->fRadii = cfg.count("radii") ? F4(cfg["radii"]) : float4(0, 0, 0, 0);
    c->fDotRadii = cfg.count("dotRadii") ? F4(cfg["dotRadii"]) : float4(0, 0, 0, 0);
    c->fBarCount = (uint)num("bars", 8);
    c->fShape = (uint)num("shape", 0);
    c->fVertical = (uint)num("vertical", 0);
    c->fAnchor = (uint)num("anchor", 2);
    c->fColorMode = (uint)num("colorMode", 0);
    c->fFlags = (uint)num("flags", 0);
    c->fC1 = cfg.count("c1") ? F4(cfg["c1"]) : float4(1, 1, 1, 1);
    c->fGrad1 = cfg.count("grad1") ? F4(cfg["grad1"]) : float4(0, 1, 0, 1);
    c->fC2 = cfg.count("c2") ? F4(cfg["c2"]) : float4(0, 0, 1, 1);
    c->fPeakColor = cfg.count("peakColor") ? F4(cfg["peakColor"]) : float4(1, 0, 0, 1);
    c->fBeatColor = float4(1, 1, 1, 1);
    c->fBeatIntensity = 0.8f;
    c->fRainbowBase = (float)num("rainbowBase", 0);
    c->fSceneAlpha = (float)num("sceneAlpha", 1);
    c->fCapThickness = (float)num("capT", 2);
    c->fDotStep = c->fBarW + c->fBarGap;
    c->fDotR = c->fBarW * 0.5f;
    c->fMaxDots = (uint)ceil(c->fMaxSize / std::max(0.5f, c->fDotStep)) + 1u;
    c->fDotSlots = (c->fAnchor == 1u) ? c->fMaxDots * 2u + 1u : c->fMaxDots;
    c->fInnerR = c->fMaxSize * 0.15f;
    c->fStrokeW = std::max(1.0f, c->fBarW);
    c->fCenter = {(float)num("cx", 100), (float)num("cy", 100)};
    c->fScopeColor = float4(1, 1, 1, 1);
    c->fGonioR = (float)num("gonioR", 50);
    c->fGonioDot = 1.0f;
    // Styles (2.1).
    c->fSegs = (uint)num("segs", 0);
    c->fSegStep = (float)num("segStep", 0);
    c->fSegH = (float)num("segH", 0);
    c->fSubdiv = (uint)num("subdiv", 4);
    c->fGlowR = (float)num("glowR", 0);
    c->fFillA = (float)num("fillA", 0.35);
    c->fVu = cfg.count("vu") ? F4(cfg["vu"]) : float4(0, 0, 0, 0);
    c->fVuBox = cfg.count("vuBox") ? F4(cfg["vuBox"]) : float4(0, 0, 0, 0);
    c->fReflBase = (float)num("reflBase", 0);
    c->fReflDir = 1.0f;
    c->fReflDepth = (float)num("reflDepth", 1);
    c->fReflAlpha = (float)num("reflAlpha", 0.4);
    c->fSpecW = (uint)num("specW", 0);
    c->fSpecRows = (uint)num("specRows", 1);
    c->fSpecTex = (uint)num("specTex", 1);
    c->fSpecHead = (uint)num("specHead", 0);

    std::vector<float2> bars(4096);
    std::vector<float4> globals(4);
    std::vector<float> wave(256, 0.f);
    std::vector<float4> points(16);
    if (cfg.count("levels"))
        for (size_t i = 0; i < cfg["levels"].size(); i++) bars[i].x = (float)cfg["levels"][i];
    if (cfg.count("peaks"))
        for (size_t i = 0; i < cfg["peaks"].size(); i++) bars[i].y = (float)cfg["peaks"][i];
    if (cfg.count("wave"))
        for (size_t i = 0; i < cfg["wave"].size(); i++) wave[i] = (float)cfg["wave"][i];
    // Terminal: synthetic atlas, alpha encodes glyph and texel position.
    std::vector<uint> cells(4096, 32u);
    std::vector<float4> atlas;
    if (cfg.count("termCols")) {
        int cw = (int)num("cellW", 7), chh = (int)num("cellH", 12), ac = 16, ar = 6;
        c->fTermCols = (uint)num("termCols", 8);
        c->fTermRows = (uint)num("termRows", 4);
        c->fTermAtlasCols = ac;
        c->fTermGeom = float4((float)num("tx", 3), (float)num("ty", 5), (float)cw, (float)chh);
        c->fTermAtlas = float4((float)(ac * cw), (float)(ar * chh), 0, 0);
        for (int k = 0; k < 5; k++) c->fTermColors[k] = float4(0.2f * (k + 1), 1.0f - 0.15f * k, 0.5f, 1.0f);
        atlas.resize((size_t)ac * cw * ar * chh);
        for (int y = 0; y < ar * chh; y++)
            for (int x = 0; x < ac * cw; x++) {
                int g = (y / chh) * ac + (x / cw);
                float a = (float)(((g * 7 + (x % cw) * 3 + (y % chh) * 5) % 11)) / 10.0f;
                atlas[(size_t)y * ac * cw + x] = float4(a, a, a, a);
            }
        c->gGlyphs.data = &atlas;
        c->gGlyphs.w = ac * cw;
        c->gGlyphs.h = ar * chh;
        if (cfg.count("cells"))
            for (size_t i = 0; i < cfg["cells"].size(); i++) cells[i] = (uint)cfg["cells"][i];
    }
    c->gCells.data = &cells;
    c->gBars.data = &bars;
    c->gGlobals.data = &globals;
    c->gWave.data = &wave;
    c->gPoints.data = &points;
    if (cfg.count("points")) {
        points.assign(cfg["points"].size() / 4 + 1, float4(0, 0, 0, 0));
        for (size_t i = 0; i + 3 < cfg["points"].size(); i += 4)
            points[i / 4] = float4((float)cfg["points"][i], (float)cfg["points"][i + 1], (float)cfg["points"][i + 2],
                                   (float)cfg["points"][i + 3]);
    }
    std::vector<float> spec(1, 0.f);
    if (cfg.count("spec")) {
        spec.assign(cfg["spec"].begin(), cfg["spec"].end());
        c->gSpec.data = &spec;
        c->gSpec.w = (int)c->fSpecW;
        c->gSpec.h = (int)c->fSpecTex;
    }

    if (mode == "raster") {
        int W = (int)c->fViewport.x, H = (int)c->fViewport.y;
        std::vector<float> img((size_t)W * H * 4, 0.f);
        std::vector<double>& passes = cfg["passes"];  // pairs: pass, count
        for (size_t pi = 0; pi + 1 < passes.size(); pi += 2) {
            c->pPass = (uint)passes[pi] % 100u;  // 100 + pass: the Reflection twin
            c->pPad0 = passes[pi] >= 100 ? 1u : 0u;
            uint count = (uint)passes[pi + 1];
            for (uint id = 0; id < count; id++) {
                Ctx::Prim pr = c->BuildPrim(c->pPass, id);
                if (c->pPad0 != 0u) pr = c->ReflectPrim(pr);
                Ctx::VsOut v0 = c->EmitVertex(pr, 0), v3 = c->EmitVertex(pr, 3);
                if (pr.kind == 3u) continue;
                float lx = v0.pix.x, ly = v0.pix.y, hx = v3.pix.x, hy = v3.pix.y;
                for (int py = std::max(0, (int)floor(ly)); py < std::min(H, (int)ceil(hy) + 1); py++) {
                    for (int px = std::max(0, (int)floor(lx)); px < std::min(W, (int)ceil(hx) + 1); px++) {
                        float sx = px + 0.5f, sy = py + 0.5f;
                        // Top-left fill rule on the quad.
                        if (!(sx >= lx && sx < hx && sy >= ly && sy < hy)) continue;
                        Ctx::VsOut in = v0;
                        in.pix = {sx, sy};
                        float4 s = c->PSMain(in);
                        float* d = &img[((size_t)py * W + px) * 4];
                        float k = 1.f - s.w;
                        d[0] = s.x + d[0] * k;
                        d[1] = s.y + d[1] * k;
                        d[2] = s.z + d[2] * k;
                        d[3] = s.w + d[3] * k;
                    }
                }
            }
        }
        WriteF32(argv[3], img);
        return 0;
    }

    if (mode == "compute") {
        // compute <cfg> <signal.f32> <out-prefix>
        // Builds the bands with ttdsp, feeds the same audio to both, runs the
        // GPU kernels on the CPU, writes both sides' results.
        FILE* f = fopen(argv[3], "rb");
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<float> x(sz / 4);
        if (fread(x.data(), 4, x.size(), f) != x.size()) return 3;
        fclose(f);

        ttdsp::SpectrumEngine::Config ec;
        ec.sampleRate = 48000;
        ec.fftSize = (int)num("n", 2048);
        ec.layout = (ttdsp::BandLayout)(int)num("layout", 1);
        ec.octaveFraction = (int)num("frac", 6);
        ec.bars = (int)num("ebars", 64);
        ec.detector = (ttdsp::Detector)(int)num("det", 0);
        ttdsp::SpectrumEngine e;
        e.Configure(ec);
        e.Push(x.data(), (int)x.size());
        e.Analyze(true);
        const int n = ec.fftSize, half = n / 2;
        const int nb = e.NumBands();

        std::vector<float> tierIn(3 * n, 0.f), window(e.Window(), e.Window() + n), power(3 * (half + 1), 0.f);
        std::vector<float4> desc(nb), off(nb), bandState(nb), barAux(nb), globalsOut(4);
        std::vector<float> shapeMod(nb, 1.f);
        std::vector<float2> barsOut(nb);
        std::vector<uint> stats(4, 0u);
        uint tierList = 0, tierCount = 0, used = 0;
        for (int t = 0; t < 3; t++) {
            if (!e.TierUsed(t)) continue;
            e.TierRing(t).Latest(&tierIn[t * n], n);
            tierList |= (uint)t << (2 * tierCount);
            tierCount++;
            used |= 1u << t;
        }
        for (int b = 0; b < nb; b++) {
            const auto& m = e.Maps()[b];
            double fc = e.Bands()[b].fc;
            int zone = (fc < 300.0) ? 0 : (fc < 2500.0) ? 1 : 2;
            desc[b] = float4((float)(m.tier + 4 * zone), (float)m.k0, (float)m.k1, (float)fc);
            off[b] = float4(m.offsetDb, m.peakOffsetDb, m.w0, m.w1);
        }
        StructuredBuffer<float> sTierIn{&tierIn}, sWindow{&window}, sMod{&shapeMod};
        c->gTierIn = sTierIn;
        c->gWindow = sWindow;
        c->gShapeMod = sMod;
        c->gBandDesc.data = &desc;
        c->gBandOff.data = &off;
        c->gPower.data = &power;
        c->gBandState.data = &bandState;
        c->gBarsOut.data = &barsOut;
        c->gBarAux.data = &barAux;
        c->gGlobalsOut.data = &globalsOut;
        c->gStats.data = &stats;
        c->cN = n;
        c->cHalf = half;
        c->cLog2Half = ttdsp::Log2i(half);
        c->cNumBands = nb;
        c->cNumBars = nb;
        c->cTierCount = tierCount | (used << 8);
        c->cTierList = tierList;
        c->cDetector = (uint)num("det", 0);
        c->cFloorDb = -72;
        c->cRangeDb = 60;
        c->cSensDb = 0;
        c->cDt = 1.0f;  // one long step: ballistics land (almost) on target
        c->cAttackMs = 0.001f;
        c->cReleaseMs = 0.001f;
        c->cReleaseLinear = 0;
        c->cCurve = 3;  // linear
        c->cShape = 0;
        c->cAutoGain = 0;
        c->cPeakHoldMs = 500;
        c->cGravity = 2;
        c->cLinFall = 0.7f;
        c->cBarRangePx = 100;
        c->cPeakGravity = 1;
        c->cDomOn = 1;
        c->cTierRate0 = (float)e.TierRate(0);
        c->cTierRate1 = (float)e.TierRate(1);
        c->cTierRate2 = (float)e.TierRate(2);
        c->cPeakCal = (float)e.PeakCal();
        c->cFmin = 20;
        c->cFmax = 20000;
        c->cBreatheUp = 0.408f;
        c->cBreatheDown = 1.103f;

        auto group = [&](int threads, auto&& fn) {
            std::barrier<> bar(threads);
            c->tt_barrier = &bar;
            std::vector<std::thread> th;
            for (int t = 0; t < threads; t++) th.emplace_back([&, t] { fn((uint)t); });
            for (auto& t : th) t.join();
        };
        for (uint g = 0; g < tierCount; g++)
            group(256, [&](uint t) { c->CsFft(uint3{g, 0, 0}, uint3{t, 0, 0}); });
        for (int b = 0; b < nb; b++) c->CsBands(uint3{(uint)b, 0, 0});
        group(256, [&](uint t) { c->CsReduce(uint3{t, 0, 0}); });
        for (int b = 0; b < nb; b++) c->CsShape(uint3{(uint)b, 0, 0});

        std::string pre = argv[4];
        WriteF32((pre + ".gpu_power").c_str(), power);
        std::vector<float> lv(nb), raw(nb), cpuDb(e.LevelsDb(), e.LevelsDb() + nb);
        for (int b = 0; b < nb; b++) { lv[b] = barsOut[b].x; raw[b] = bandState[b].y; }
        WriteF32((pre + ".gpu_level").c_str(), lv);
        WriteF32((pre + ".gpu_db").c_str(), raw);
        WriteF32((pre + ".cpu_db").c_str(), cpuDb);
        // CPU power spectra for the same blocks, via ttdsp's RealFft.
        std::vector<float> cpuPow(3 * (half + 1), 0.f), wbuf(n), re(half + 1), im(half + 1);
        ttdsp::RealFft fft;
        fft.Init(n);
        for (int t = 0; t < 3; t++) {
            if (!(used & (1u << t))) continue;
            for (int i = 0; i < n; i++) wbuf[i] = tierIn[t * n + i] * window[i];
            fft.Forward(wbuf.data(), re.data(), im.data());
            for (int k = 0; k <= half; k++) cpuPow[t * (half + 1) + k] = re[k] * re[k] + im[k] * im[k];
        }
        WriteF32((pre + ".cpu_power").c_str(), cpuPow);
        printf("dominant gpu %.3f cpu %.3f master %.4f\n", asfloat(stats[0]), e.DominantHz(-80.0), globalsOut[0].y);
        return 0;
    }
    return 1;
}
