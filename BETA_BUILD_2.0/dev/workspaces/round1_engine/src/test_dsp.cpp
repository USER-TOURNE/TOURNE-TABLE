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
    if (mode == "fastlog") {  // fastlog -> max |FastDbFromPower - 10 log10| in dB over 1e-32 .. 1e6
        double worst = 0.0;
        for (int i = 0; i <= 2000000; i++) {
            double p = pow(10.0, -32.0 + 38.0 * i / 2000000.0);
            double ref = 10.0 * log10(std::max(p, 1e-30));
            worst = std::max(worst, fabs((double)FastDbFromPower(p) - ref));
        }
        printf("fastlog %.3e\n", worst);
        return 0;
    }
    if (mode == "tpgate") {
        // The gated true peak against the plain one (same taps, same float
        // arithmetic, every phase of every sample), on signals that exercise
        // the skip: quiet passages after a loud one, fades, silence, an
        // inter-sample peak arriving late. Prints the largest difference
        // (must be exactly 0) and the share of channel-frames interpolated.
        const int L = 4, T = LoudnessMeter::kTpTaps, taps = L * T;
        float tp[4][LoudnessMeter::kTpTaps];
        {
            double beta = 7.0;
            auto bessel0 = [](double x) {
                double s = 1.0, t = 1.0;
                for (int k = 1; k < 50; k++) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; }
                return s;
            };
            double den = bessel0(beta);
            for (int i = 0; i < taps; i++) {
                double n = i - (taps - 1) / 2.0, x = n / L;
                double h = (fabs(x) < 1e-12) ? 1.0 : sin(kPi * x) / (kPi * x);
                double r = n / ((taps - 1) / 2.0);
                h *= bessel0(beta * sqrt(std::max(0.0, 1.0 - r * r))) / den;
                tp[i % L][i / L] = (float)h;
            }
        }
        std::mt19937 rng(3);
        std::normal_distribution<float> nd;
        double worst = 0.0;
        long long evals = 0, frames = 0;
        for (int sig = 0; sig < 6; sig++) {
            const int fs = 48000, n = fs * 4;
            std::vector<float> x((size_t)n * 2);
            for (int i = 0; i < n; i++) {
                double t = (double)i / fs, l = 0, r = 0;
                switch (sig) {
                    case 0: l = 0.9 * sin(2 * kPi * 997 * t) * (t < 0.5 ? 1.0 : 0.05); r = 0.3 * nd(rng) * 0.1; break;
                    case 1: l = r = 0.5 * nd(rng) * exp(-t); break;                      // fade
                    case 2: l = (t > 1 && t < 3) ? 0.2 * nd(rng) : 0.0; r = 0; break;    // silence around noise
                    case 3: l = r = (t < 3.5 ? 0.3 : 1.0) * sin(2 * kPi * 12000 * t + kPi / 4); break;  // late ISP
                    case 4: l = 0.25 * sin(2 * kPi * 50 * t) + 0.05 * nd(rng); r = -l; break;
                    default: l = ((i % 4800) == 0) ? 0.8 : 0.0; r = ((i % 7000) == 1) ? -0.6 : 0.0; break;  // impulses
                }
                x[2 * (size_t)i] = (float)l;
                x[2 * (size_t)i + 1] = (float)r;
            }
            LoudnessMeter m;
            m.Configure(fs, 2, nullptr);
            float hist[2][LoudnessMeter::kTpTaps] = {};
            int pos = 0;
            float ref = 0.f;
            for (int f0 = 0; f0 < n; f0 += 480) {
                int nf = std::min(480, n - f0);
                m.Process(x.data() + (size_t)f0 * 2, nf, 2);
                for (int f = f0; f < f0 + nf; f++) {
                    for (int c = 0; c < 2; c++) hist[c][pos] = x[2 * (size_t)f + c];
                    for (int c = 0; c < 2; c++) {
                        ref = std::max(ref, fabsf(x[2 * (size_t)f + c]));
                        for (int p = 0; p < 4; p++) {
                            float acc = 0.f;
                            int idx = pos;
                            for (int j = 0; j < T; j++) {
                                acc += tp[p][j] * hist[c][idx];
                                idx = (idx == 0) ? T - 1 : idx - 1;
                            }
                            ref = std::max(ref, fabsf(acc));
                        }
                    }
                    pos = (pos + 1 == T) ? 0 : pos + 1;
                    // Compare at every packet boundary, not just the end.
                }
                double got = m.TruePeakDb(), want = 20.0 * log10(std::max(1e-10f, ref));
                worst = std::max(worst, fabs(got - want));
            }
            evals += m.TpEvaluations();
            frames += 2LL * n;
        }
        printf("tpgate %.9f %.4f\n", worst, (double)evals / (double)frames);
        return 0;
    }
    if (mode == "tpoff") {  // tpoff -> true peak with SetTruePeak(false), and loudness unchanged
        const int fs = 48000, n = fs * 5;
        std::vector<float> x((size_t)n * 2);
        for (int i = 0; i < n; i++) x[2 * (size_t)i] = x[2 * (size_t)i + 1] = (float)(0.1 * sin(2 * kPi * 1000.0 * i / fs));
        LoudnessMeter a, b;
        a.Configure(fs, 2, nullptr);
        b.Configure(fs, 2, nullptr);
        b.SetTruePeak(false);
        for (int f = 0; f < n; f += 480) {
            a.Process(x.data() + (size_t)f * 2, 480, 2);
            b.Process(x.data() + (size_t)f * 2, 480, 2);
        }
        printf("tpoff %.3f %lld %d\n", b.TruePeakDb(), b.TpEvaluations(),
               a.Momentary() == b.Momentary() && a.ShortTerm() == b.ShortTerm() ? 1 : 0);
        return 0;
    }
    if (mode == "ballcoef") {  // per-frame coefficients vs the per-band step: must be bit-identical
        int bad = 0;
        std::mt19937 rng(5);
        std::uniform_real_distribution<float> u(0.f, 1.f);
        for (int preset = 0; preset < 6; preset++) {
            Ballistics b = BallisticsPreset(preset);
            for (int it = 0; it < 20000; it++) {
                double dt = 0.0005 + u(rng) * 0.05;
                float y = u(rng), x = u(rng);
                float slow = 0.f;
                {  // the 2.0 formula, written out
                    if (x > y) slow = (float)(y + (x - y) * (1.0 - exp(-dt * 1000.0 / std::max(0.1, b.attackMs))));
                    else if (b.release == ReleaseKind::Linear)
                        slow = std::max(x, y - (float)(b.releaseDbPerSec * dt / std::max(1.f, 60.f)));
                    else slow = (float)(y + (x - y) * (1.0 - exp(-dt * 1000.0 / std::max(0.1, b.releaseMs))));
                }
                BallisticsCoef c = BallisticsCoefs(dt, b, 60.f);
                if (BallisticsApply(y, x, c) != slow) bad++;
            }
        }
        printf("ballcoef %d\n", bad);
        return 0;
    }
    fprintf(stderr, "unknown mode\n");
    return 1;
}
