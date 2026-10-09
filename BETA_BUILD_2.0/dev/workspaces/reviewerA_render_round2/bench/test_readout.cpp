// Checks the readout's frequency hold (same rule as VizFormatReadout in part_text.cpp):
// what is shown never differs from the live value by a full display step or more.
#include <cmath>
#include <cstdio>
#include <random>
int main() {
    std::mt19937 rng(3);
    std::normal_distribution<float> N(0.f, 1.f);
    float shown = 0.f, hz = 60.f, worst = 0.f;
    int changes = 0, n = 2000000;
    for (int i = 0; i < n; i++) {
        hz = std::fmax(20.f, std::fmin(20000.f, hz * std::exp(0.01f * N(rng))));
        float step = (shown >= 1000.f) ? 100.f : 1.f;
        float before = shown;
        if (shown <= 0.f || (hz >= 1000.f) != (shown >= 1000.f) || std::fabs(hz - shown) >= step) shown = hz;
        if (shown != before) changes++;
        float st = (shown >= 1000.f) ? 100.f : 1.f;
        worst = std::fmax(worst, std::fabs(hz - shown) / st);
        if ((hz >= 1000.f) != (shown >= 1000.f)) { printf("FAIL range\n"); return 1; }
    }
    printf("%s: worst |live - shown| = %.3f display steps, %d updates in %d samples\n",
           worst < 1.f ? "PASS" : "FAIL", worst, changes, n);
    return worst < 1.f ? 0 : 1;
}
