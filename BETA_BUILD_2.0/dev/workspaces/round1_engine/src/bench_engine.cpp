// Engine cost benchmark: the engine thread's per-packet and per-frame work,
// before (the pre-change ttdsp.h, included into namespace oldv) and after.
// 48 kHz stereo float, 10 ms packets (480 frames), frames at 144 FPS,
// interleaved in time order the way RenderThreadProc runs them. The
// per-frame code mirrors VizEngine::PrecisionFrame / ConvertPacket (old and
// new); the WASAPI calls themselves are not included.
//
//   clang++ -std=c++20 -O2 -DTTDSP_OLD='"<old>/ttdsp.h"' -o bench_engine bench_engine.cpp
//
// Prints microseconds of CPU per second of audio (= % of one core x 1e4).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "ttdsp.h"
namespace oldv {
#include TTDSP_OLD
}

static const int FS = 48000, CH = 2, PKT = 480;
static const double FPS = 144.0;

// ~music: a few partials with vibrato, a kick every 0.5 s, noise, and a
// 2-second swell between -30 and -6 dBFS, so loudness and true peak see
// dynamics. Stereo with partial correlation.
static std::vector<float> Music(int seconds) {
    std::vector<float> x((size_t)FS * seconds * CH);
    std::mt19937 rng(11);
    std::normal_distribution<float> nd;
    for (int i = 0; i < FS * seconds; i++) {
        double t = (double)i / FS;
        double env = pow(10.0, (-18.0 + 12.0 * sin(2 * M_PI * 0.5 * t)) / 20.0);
        double tone = 0.3 * sin(2 * M_PI * 220 * t + 0.3 * sin(2 * M_PI * 5 * t)) + 0.2 * sin(2 * M_PI * 660 * t) +
                      0.1 * sin(2 * M_PI * 3300 * t);
        double kt = fmod(t, 0.5);
        double kick = exp(-kt * 30) * sin(2 * M_PI * 55 * kt);
        float n = 0.05f * nd(rng);
        x[2 * (size_t)i] = (float)(env * (tone + kick) + n);
        x[2 * (size_t)i + 1] = (float)(env * (0.8 * tone + kick) - 0.5 * n);
    }
    return x;
}

struct Cfg {
    int fft = 2048, bars = 256, maxTier = 2;
    bool dominant = false;
};

// ---- Old engine (2.0 as shipped) ----------------------------------------------------
struct OldEngine {
    oldv::ttdsp::SpectrumEngine spec;
    oldv::ttdsp::Ballistics ball = oldv::ttdsp::BallisticsPreset(0);
    oldv::ttdsp::DisplayMap disp;
    std::vector<float> level, mono;
    bool dominant = false;
    void Init(const Cfg& c) {
        oldv::ttdsp::SpectrumEngine::Config sc;
        sc.fftSize = c.fft;
        sc.bars = c.bars;
        sc.maxTier = c.maxTier;
        spec.Configure(sc);
        level.assign(spec.NumBands(), 0.f);
        dominant = c.dominant;
    }
    void Packet(const float* x, int n) {
        mono.resize(n);
        for (int f = 0; f < n; f++) mono[f] = 0.5f * (x[2 * f] + x[2 * f + 1]);
        spec.Push(mono.data(), n);
    }
    void Frame(double dt) {
        bool fresh = spec.Analyze(false);
        const float* db = spec.LevelsDb();
        float range = disp.ceilDb - disp.floorDb;
        for (int b = 0; b < spec.NumBands(); b++)
            level[b] = oldv::ttdsp::BallisticsStep(level[b], disp.ToNorm(db[b]), dt, ball, range);
        if (dominant && fresh) volatile double hz = spec.DominantHz(-70.0);
    }
};

// ---- New engine ------------------------------------------------------------------------
struct NewEngine {
    ttdsp::SpectrumEngine spec;
    ttdsp::Ballistics ball = ttdsp::BallisticsPreset(0);
    ttdsp::DisplayMap disp;
    std::vector<float> level, mono;
    bool dominant = false;
    long long zeroRun = 0, silentNeeded = 0;
    bool silentFinal = false, settled = false, wake = false;
    long long ffts = 0;
    void Init(const Cfg& c) {
        ttdsp::SpectrumEngine::Config sc;
        sc.fftSize = c.fft;
        sc.bars = c.bars;
        sc.maxTier = c.maxTier;
        spec.Configure(sc);
        level.assign(spec.NumBands(), 0.f);
        dominant = c.dominant;
        int deepest = spec.TierUsed(2) ? 2 : spec.TierUsed(1) ? 1 : 0;
        silentNeeded = ((long long)2 * c.fft << (2 * deepest)) + 1024;
    }
    void Packet(const float* x, int n) {
        mono.resize(n);
        int lastNZ = -1;
        for (int f = 0; f < n; f++) {
            float v = 0.5f * (x[2 * f] + x[2 * f + 1]);
            mono[f] = v;
            if (v != 0.f) lastNZ = f;
        }
        if (lastNZ >= 0) {
            if (zeroRun >= silentNeeded) wake = true;
            zeroRun = n - 1 - lastNZ;
        } else {
            if (zeroRun >= silentNeeded) {
                zeroRun += n;
                return;  // nothing to push: the analysis already holds only zeros
            }
            zeroRun += n;
        }
        spec.Push(mono.data(), n);
    }
    bool Frame(double dt) {
        bool silent = zeroRun >= silentNeeded, force = wake;
        wake = false;
        if (!silent) silentFinal = settled = false;
        if (silent && settled) return false;
        bool fresh = false;
        if (!silentFinal) {
            fresh = spec.Analyze(force || silent);
            if (silent) silentFinal = true;
        }
        const float* db = spec.LevelsDb();
        float range = disp.ceilDb - disp.floorDb;
        ttdsp::BallisticsCoef bc = ttdsp::BallisticsCoefs(dt, ball, range);
        float mx = 0.f;
        for (int b = 0; b < spec.NumBands(); b++) {
            level[b] = ttdsp::BallisticsApply(level[b], disp.ToNorm(db[b]), bc);
            mx = std::max(mx, level[b]);
        }
        if (silent && silentFinal && mx < 1e-4f) {
            std::fill(level.begin(), level.end(), 0.f);
            settled = true;
        }
        if (dominant && fresh) volatile double hz = spec.DominantHz(-70.0);
        return true;
    }
};

// Drives `packet` and `frame` over `audio` in time order; returns us per audio second.
template <typename P, typename F>
static double Drive(const std::vector<float>& audio, P&& packet, F&& frame, double warmSec = 0.0) {
    const int total = (int)(audio.size() / CH);
    const double framePeriod = FS / FPS;
    double nextFrame = framePeriod;
    int warm = (int)(warmSec * FS);
    auto t0 = std::chrono::steady_clock::now();
    for (int s = 0; s < total; s += PKT) {
        if (s == warm && warm > 0) t0 = std::chrono::steady_clock::now();
        packet(&audio[(size_t)s * CH], std::min(PKT, total - s));
        while (nextFrame <= s + PKT) {
            frame(1.0 / FPS);
            nextFrame += framePeriod;
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    double sec = (double)(total - warm) / FS;
    return std::chrono::duration<double, std::micro>(t1 - t0).count() / sec;
}

template <typename Fn>
static double Best(Fn&& fn, int reps = 5) {
    double b = 1e30;
    for (int i = 0; i < reps; i++) b = std::min(b, fn());
    return b;
}

static void Row(const char* name, double before, double after, const char* note = "") {
    printf("%-46s %9.1f %9.1f  %6.3f%% -> %6.3f%%  x%.1f %s\n", name, before, after, before / 1e4, after / 1e4,
           before / std::max(after, 1e-9), note);
}

int main() {
    const int SECS = 6;
    auto music = Music(SECS);
    std::vector<float> zeros((size_t)FS * 10 * CH, 0.f);
    printf("%-46s %9s %9s  (us of CPU per second of audio; %% of one core)\n", "scenario", "before", "after");

    auto analysis = [&](Cfg c, const std::vector<float>& a, double warm) {
        double o = Best([&] {
            OldEngine e;
            e.Init(c);
            return Drive(a, [&](const float* x, int n) { e.Packet(x, n); }, [&](double dt) { e.Frame(dt); }, warm);
        });
        double n = Best([&] {
            NewEngine e;
            e.Init(c);
            return Drive(a, [&](const float* x, int n) { e.Packet(x, n); }, [&](double dt) { e.Frame(dt); }, warm);
        });
        return std::make_pair(o, n);
    };
    Cfg c256;
    auto r = analysis(c256, music, 0);
    Row("analysis FFT 2048 / 256 bars", r.first, r.second);
    Cfg c2048;
    c2048.bars = 2048;
    r = analysis(c2048, music, 0);
    Row("analysis FFT 2048 / 2048 bars", r.first, r.second);
    Cfg cdom = c256;
    cdom.dominant = true;
    r = analysis(cdom, music, 0);
    Row("analysis 2048/256 + peak-frequency readout", r.first, r.second);

    // Ballistics alone, 2048 bands, 144 FPS: one second = 144 frames.
    {
        std::vector<float> lv(2048, 0.f), x(2048);
        std::mt19937 rng(2);
        std::uniform_real_distribution<float> u(0, 1);
        for (auto& v : x) v = u(rng);
        oldv::ttdsp::Ballistics ob = oldv::ttdsp::BallisticsPreset(0);
        ttdsp::Ballistics nb = ttdsp::BallisticsPreset(0);
        auto time = [&](auto&& body) {
            return Best([&] {
                auto t0 = std::chrono::steady_clock::now();
                for (int rep = 0; rep < 20; rep++)
                    for (int f = 0; f < 144; f++) {
                        double dt = 1.0 / 144 + (f & 1) * 1e-5;
                        body(dt, f);
                    }
                auto t1 = std::chrono::steady_clock::now();
                return std::chrono::duration<double, std::micro>(t1 - t0).count() / 20;
            });
        };
        double o = time([&](double dt, int f) {
            for (int b = 0; b < 2048; b++) lv[b] = oldv::ttdsp::BallisticsStep(lv[b], (f & 8) ? x[b] : 0.f, dt, ob, 60.f);
        });
        double n = time([&](double dt, int f) {
            auto c = ttdsp::BallisticsCoefs(dt, nb, 60.f);
            for (int b = 0; b < 2048; b++) lv[b] = ttdsp::BallisticsApply(lv[b], (f & 8) ? x[b] : 0.f, c);
        });
        Row("ballistics only, 2048 bands @144 FPS", o, n);
    }

    // Loudness. Old: K-weighting + true peak always, correlation always.
    // New: Loudness readout = no true peak, no correlation; LoudnessFull =
    // both, with the exact skip.
    {
        auto run = [&](auto& meter, oldv::ttdsp::Correlation* oc, ttdsp::Correlation* nc) {
            return Drive(
                music,
                [&](const float* x, int n) {
                    meter.Process(x, n, 2);
                    for (int f = 0; f < n; f++) {
                        if (oc) oc->Push(x[2 * f], x[2 * f + 1]);
                        if (nc) nc->Push(x[2 * f], x[2 * f + 1]);
                    }
                },
                [&](double) {});
        };
        double o = Best([&] {
            oldv::ttdsp::LoudnessMeter m;
            m.Configure(FS, 2, nullptr);
            oldv::ttdsp::Correlation c;
            c.Configure(FS, 300);
            return run(m, &c, nullptr);
        });
        double nLoud = Best([&] {
            ttdsp::LoudnessMeter m;
            m.Configure(FS, 2, nullptr);
            m.SetTruePeak(false);
            return run(m, nullptr, nullptr);
        });
        long long evals = 0;
        double nFull = Best([&] {
            ttdsp::LoudnessMeter m;
            m.Configure(FS, 2, nullptr);
            ttdsp::Correlation c;
            c.Configure(FS, 300);
            double v = run(m, nullptr, &c);
            evals = m.TpEvaluations();
            return v;
        });
        Row("loudness readout (Loudness: M/S/I)", o, nLoud, "(TP + corr skipped)");
        char note[96];
        snprintf(note, sizeof note, "(TP interpolated %.1f%% of ch-frames)", 100.0 * evals / (2.0 * FS * SECS));
        Row("loudness readout (Full: + TP + corr)", o, nFull, note);
        // Worst case for the skip: stationary full-band noise at a constant
        // level, where new sample peaks keep arriving near the running max.
        std::vector<float> noise((size_t)FS * SECS * CH);
        std::mt19937 rng(9);
        std::normal_distribution<float> nd;
        for (auto& v : noise) v = std::clamp(0.25f * nd(rng), -1.f, 1.f);
        std::swap(noise, music);
        double oN = Best([&] {
            oldv::ttdsp::LoudnessMeter m;
            m.Configure(FS, 2, nullptr);
            oldv::ttdsp::Correlation c;
            c.Configure(FS, 300);
            return run(m, &c, nullptr);
        });
        double nN = Best([&] {
            ttdsp::LoudnessMeter m;
            m.Configure(FS, 2, nullptr);
            ttdsp::Correlation c;
            c.Configure(FS, 300);
            double v = run(m, nullptr, &c);
            evals = m.TpEvaluations();
            return v;
        });
        std::swap(noise, music);
        snprintf(note, sizeof note, "(TP interpolated %.1f%% of ch-frames)", 100.0 * evals / (2.0 * FS * SECS));
        Row("loudness Full, stationary noise (worst case)", oN, nN, note);
    }

    // Scope / Goniometer: full config (256 bars, 3 tiers) vs 24 bands, tier 0.
    {
        Cfg full = c256;
        Cfg scope;
        scope.bars = 24;
        scope.maxTier = 0;
        double o = Best([&] {
            OldEngine e;
            e.Init(full);
            return Drive(music, [&](const float* x, int n) { e.Packet(x, n); }, [&](double dt) { e.Frame(dt); });
        });
        double n = Best([&] {
            NewEngine e;
            e.Init(scope);
            return Drive(music, [&](const float* x, int n) { e.Packet(x, n); }, [&](double dt) { e.Frame(dt); });
        });
        Row("scope/gonio shape: 256 bars 3 tiers -> 24/tier0", o, n);
        Cfg full2048 = c2048;
        o = Best([&] {
            OldEngine e;
            e.Init(full2048);
            return Drive(music, [&](const float* x, int n) { e.Packet(x, n); }, [&](double dt) { e.Frame(dt); });
        });
        Row("scope/gonio shape: 2048 bars -> 24/tier0", o, n);
    }

    // Digital silence: 10 s of zeros, timed after 3 s (the skip has engaged
    // and the bars have settled).
    {
        r = analysis(c256, zeros, 3.0);
        Row("silence, 256 bars (steady state)", r.first, r.second);
        r = analysis(c2048, zeros, 3.0);
        Row("silence, 2048 bars (steady state)", r.first, r.second);
        // How long until the skip engages and the bars settle.
        NewEngine e;
        e.Init(c256);
        int frames = 0, settledAt = -1;
        double t = 0;
        for (int s = 0; s < FS * 10; s += PKT) {
            e.Packet(&zeros[(size_t)s * CH], PKT);
            for (; t <= s + PKT; t += FS / FPS) {
                frames++;
                if (!e.Frame(1.0 / FPS) && settledAt < 0) settledAt = frames;
            }
        }
        printf("silence: frames report 'no change' from %.2f s of zeros (skip after %lld samples)\n",
               settledAt / FPS, e.silentNeeded);
    }
    return 0;
}
