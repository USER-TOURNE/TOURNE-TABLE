// Linux harness for the pure parts of the NPU path and the pacing math,
// compiled from code extracted verbatim from the mod.
#include "extracted.h"
#include <cstdio>
#include <complex>
#include <random>

static void RefDft(const std::vector<float>& x, std::vector<double>& re, std::vector<double>& im) {
    int n = (int)x.size(); re.assign(n, 0); im.assign(n, 0);
    for (int k = 0; k < n / 2; k++) {
        double sr = 0, si = 0;
        for (int i = 0; i < n; i++) {
            double ang = -2.0 * M_PI * (double)(((long long)k * i) % n) / n;
            sr += x[i] * cos(ang); si += x[i] * sin(ang);
        }
        re[k] = sr; im[k] = si;
    }
}

// Executes the four-step graph in double precision straight from the same
// constant construction BuildDftModel uses (independent of protobuf).
int main(int argc, char** argv) {
    int sizes[] = {1024, 2048, 4096, 8192};
    for (int n : sizes) {
        int n1, n2;
        if (!ttnpu::SplitSize(n, &n1, &n2)) { printf("split fail\n"); return 1; }
        std::string model = ttnpu::BuildDftModel(n1, n2);
        char path[64]; snprintf(path, sizeof path, "dft_%d.onnx", n);
        FILE* f = fopen(path, "wb"); fwrite(model.data(), 1, model.size(), f); fclose(f);
        printf("n=%d n1=%d n2=%d model=%zu bytes, first bytes %02x %02x\n", n, n1, n2, model.size(),
               (unsigned char)model[0], (unsigned char)model[1]);
    }
    // UnpackBins check against a synthetic y laid out as the four-step output.
    {
        int n1 = 32, n2 = 64, n = n1 * n2;
        std::vector<float> y((size_t)n1 * 2 * n2), re(n), im(n);
        for (int k1 = 0; k1 < n1; k1++)
            for (int k2 = 0; k2 < n2; k2++) {
                int k = k1 + n1 * k2;
                y[k1 * 2 * n2 + k2] = (float)k / n;          // re = k after scaling
                y[k1 * 2 * n2 + n2 + k2] = -(float)k / n;    // im = -k
            }
        ttnpu::UnpackBins(y.data(), n1, n2, re, im);
        double worst = 0;
        for (int k = 0; k < n / 2; k++) worst = std::max(worst, fabs(re[k] - k) + fabs(im[k] + k));
        printf("UnpackBins index error: %g\n", worst);
    }
    // Pacing: simulate 10 s at 144 Hz with a 60 FPS target, 0-1.2 ms wake jitter,
    // and a vblank reference that is re-read with fresh values twice a second.
    {
        const LONGLONG freq = 10000000;  // 100 ns QPC
        const double hz = 143.98;
        const LONGLONG period = (LONGLONG)(freq / hz);
        LONGLONG trueVblank0 = 123456;
        std::mt19937 rng(1);
        std::uniform_real_distribution<double> jit(0.0, 0.0012);
        int div = VizVsyncSchedule::Divisor(60.0, period, freq);
        LONGLONG now = 5000000, due = 0, vb = trueVblank0, lastSync = 0;
        std::vector<long long> refreshIdx;
        double worstPhaseErr = 0;
        for (int iter = 0; iter < 200000 && now < 5000000 + 10 * freq; iter++) {
            if (now - lastSync > freq / 2) {  // re-read: last vblank before now
                long long k = (now - trueVblank0) / period;
                vb = trueVblank0 + k * period;
                lastSync = now;
            }
            if (due == 0) due = VizVsyncSchedule::AlignUp(now, vb, period, 0.5);
            if (now < due) { now = due + (LONGLONG)(jit(rng) * freq); continue; }
            // frame happens at `now`: which refresh interval does it land in, and at what phase?
            double ph = (double)((now - trueVblank0) % period) / period;
            worstPhaseErr = std::max(worstPhaseErr, fabs(ph - 0.5));
            refreshIdx.push_back((now - trueVblank0) / period);
            due = VizVsyncSchedule::Next(due, now, vb, period, div);
        }
        int bad = 0;
        for (size_t i = 1; i < refreshIdx.size(); i++)
            if (refreshIdx[i] - refreshIdx[i - 1] != div) bad++;
        printf("pacing: divisor=%d frames=%zu (%.2f fps), uneven gaps=%d, worst phase error=%.3f of a refresh\n",
               div, refreshIdx.size(), refreshIdx.size() / 10.0, bad, worstPhaseErr);
        // Divisor table
        for (double hzz : {60.0, 75.0, 120.0, 144.0, 165.0, 240.0})
            for (int fps : {30, 48, 60, 72, 90, 144, 240}) {
                LONGLONG p = (LONGLONG)(freq / hzz);
                int dv = VizVsyncSchedule::Divisor(fps, p, freq);
                if (fps == 60 || fps == 144 || fps == 30) printf("  %3.0f Hz, target %3d -> every %d refresh(es) = %.1f fps\n", hzz, fps, dv, hzz / dv);
            }
        // AlignUp edge cases
        LONGLONG P = 1000, V = 50;
        printf("AlignUp: %lld %lld %lld %lld\n", VizVsyncSchedule::AlignUp(0, V, P, 0.5),
               VizVsyncSchedule::AlignUp(550, V, P, 0.5), VizVsyncSchedule::AlignUp(551, V, P, 0.5),
               VizVsyncSchedule::AlignUp(-2000, V, P, 0.5));
    }
    return 0;
}
