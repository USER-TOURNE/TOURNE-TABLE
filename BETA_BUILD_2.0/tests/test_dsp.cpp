// Test driver for ttdsp. Modes write raw float32 / text results that
// test_dsp.py compares against numpy / scipy and the standards' test cases.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "ttdsp.h"

using namespace ttdsp;

static std::vector<float> ReadF32(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<float> v(sz / 4);
    if (fread(v.data(), 4, v.size(), f) != v.size()) exit(3);
    fclose(f);
    return v;
}
static void WriteF32(const char* path, const std::vector<float>& v) {
    FILE* f = fopen(path, "wb");
    fwrite(v.data(), 4, v.size(), f);
    fclose(f);
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "fft") {  // fft <n> <in.f32> <out.f32>  (interleaved re,im for 0..n/2)
        int n = atoi(argv[2]);
        auto x = ReadF32(argv[3]);
        RealFft f;
        if (!f.Init(n)) return 4;
        std::vector<float> re(n / 2 + 1), im(n / 2 + 1), out;
        f.Forward(x.data(), re.data(), im.data());
        for (int k = 0; k <= n / 2; k++) { out.push_back(re[k]); out.push_back(im[k]); }
        WriteF32(argv[4], out);
        return 0;
    }
    if (mode == "window") {  // window <kind> <n> <out>
        int kind = atoi(argv[2]), n = atoi(argv[3]);
        std::vector<float> w(n);
        BuildWindow((WindowKind)kind, n, w.data());
        WriteF32(argv[4], w);
        return 0;
    }
    if (mode == "halfband") {  // halfband <in> <out>   one stage
        auto x = ReadF32(argv[2]);
        HalfBand hb;
        std::vector<float> out;
        for (float v : x) { float o; if (hb.Push(v, &o)) out.push_back(o); }
        WriteF32(argv[3], out);
        std::vector<float> c;
        for (int i = 0; i < HalfBand::kTaps; i++) c.push_back((float)hb.Coef(i));
        WriteF32((std::string(argv[3]) + ".coef").c_str(), c);
        return 0;
    }
    if (mode == "bands") {  // bands <layout> <scale> <frac> <bars>  -> prints f1 fc f2
        SpectrumEngine::Config c;
        c.layout = (BandLayout)atoi(argv[2]);
        c.scale = (FreqScale)atoi(argv[3]);
        c.octaveFraction = atoi(argv[4]);
        c.bars = atoi(argv[5]);
        SpectrumEngine e;
        e.Configure(c);
        for (size_t i = 0; i < e.Bands().size(); i++) {
            const auto& b = e.Bands()[i];
            const auto& m = e.Maps()[i];
            printf("%.6f %.6f %.6f %d %d %d %.4f %.4f\n", b.f1, b.fc, b.f2, m.tier, m.k0, m.k1, m.w0, m.w1);
        }
        return 0;
    }
    if (mode == "weight") {  // weight -> f A C
        double fs[] = {10, 12.5, 16, 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630,
                       800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000};
        for (double f : fs) printf("%g %.2f %.2f\n", f, AWeightDb(f), CWeightDb(f));
        return 0;
    }
    if (mode == "kweight") {
        Biquad s, h;
        KWeightingFilters(48000.0, s, h);
        printf("%.14f %.14f %.14f %.14f %.14f\n", s.b0, s.b1, s.b2, s.a1, s.a2);
        printf("%.14f %.14f %.14f %.14f %.14f\n", h.b0, h.b1, h.b2, h.a1, h.a2);
        return 0;
    }
    if (mode == "engine") {
        // engine <fs> <n> <layout> <scale> <frac> <bars> <det> <ref> <window> <in.f32> <out.f32>
        // Pushes the whole input, forces one analysis, writes band dB levels.
        SpectrumEngine::Config c;
        c.sampleRate = atoi(argv[2]);
        c.fftSize = atoi(argv[3]);
        c.layout = (BandLayout)atoi(argv[4]);
        c.scale = (FreqScale)atoi(argv[5]);
        c.octaveFraction = atoi(argv[6]);
        c.bars = atoi(argv[7]);
        c.detector = (Detector)atoi(argv[8]);
        c.levelRef = (LevelRef)atoi(argv[9]);
        c.window = (WindowKind)atoi(argv[10]);
        SpectrumEngine e;
        e.Configure(c);
        auto x = ReadF32(argv[11]);
        // Feed in chunks, analysing as a frame loop would, then force at the end.
        size_t chunk = 333;
        bool avg = getenv("TT_AVG") != nullptr;
        std::vector<double> acc(e.NumBands(), 0.0);
        int nAcc = 0;
        for (size_t i = 0; i < x.size(); i += chunk) {
            e.Push(x.data() + i, (int)std::min(chunk, x.size() - i));
            e.Analyze(false);
            if (avg && i > (size_t)c.fftSize * 40) {  // after every tier has filled
                for (int b = 0; b < e.NumBands(); b++) acc[b] += pow(10.0, e.LevelsDb()[b] / 10.0);
                nAcc++;
            }
        }
        e.Analyze(true);
        std::vector<float> out(e.LevelsDb(), e.LevelsDb() + e.NumBands());
        if (avg && nAcc) for (int b = 0; b < e.NumBands(); b++) out[b] = (float)(10.0 * log10(acc[b] / nAcc));
        WriteF32(argv[12], out);
        printf("dominant %.3f\n", e.DominantHz(-80.0));
        return 0;
    }
    if (mode == "loudness") {  // loudness <fs> <ch> <in.f32 interleaved>  -> M S I TP blocks
        int fs = atoi(argv[2]), ch = atoi(argv[3]);
        auto x = ReadF32(argv[4]);
        LoudnessMeter m;
        m.Configure(fs, ch, nullptr);
        int frames = (int)(x.size() / ch);
        double maxM = -1e9, maxS = -1e9;
        for (int f = 0; f < frames; f += 480) {
            int nf = std::min(480, frames - f);
            m.Process(x.data() + (size_t)f * ch, nf, ch);
            maxM = std::max(maxM, m.Momentary());
            maxS = std::max(maxS, m.ShortTerm());
        }
        printf("%.3f %.3f %.3f %.3f %lld %.3f %.3f\n", m.Momentary(), m.ShortTerm(), m.Integrated(),
               m.TruePeakDb(), m.Blocks(), maxM, maxS);
        return 0;
    }
    if (mode == "ballistics") {  // ballistics <fps>  -> prints t y at 10 ms marks for a step up then down
        double fps = atof(argv[2]);
        Ballistics b = BallisticsPreset(atoi(argv[3]));
        double dt = 1.0 / fps, t = 0;
        float y = 0;
        // Every update prints (t, y); the Python side interpolates.
        while (t < 1.5) {
            float x = (t < 0.5 - 1e-9) ? 1.f : 0.f;
            y = BallisticsStep(y, x, dt, b, 60.f);
            t += dt;
            printf("%.6f %.6f\n", t, y);
        }
        return 0;
    }
    if (mode == "correlation") {
        Correlation c;
        c.Configure(48000, 300);
        std::mt19937 rng(1);
        std::normal_distribution<float> nd;
        for (int i = 0; i < 48000; i++) { float v = nd(rng); c.Push(v, v); }
        printf("mono %.4f\n", c.Value());
        c.Reset();
        for (int i = 0; i < 48000; i++) { float v = nd(rng); c.Push(v, -v); }
        printf("anti %.4f\n", c.Value());
        c.Reset();
        for (int i = 0; i < 96000; i++) c.Push(nd(rng), nd(rng));
        printf("uncorrelated %.4f\n", c.Value());
        return 0;
    }
    fprintf(stderr, "unknown mode\n");
    return 1;
}
