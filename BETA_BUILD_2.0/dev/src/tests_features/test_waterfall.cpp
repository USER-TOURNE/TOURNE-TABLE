// Waterfall history scroll: old code (verbatim from the original p2_term.cpp,
// VizBuildTermGrid) vs VizTermScrollHistory cut from v2b.cpp (gen/waterfall_new.inc).
// Build with -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "gen/waterfall_new.inc"

static void OldScroll(std::vector<float>& g_termHistory, int rows, int cols, int steps, const float* g_vizPeak) {
    struct { int rows, cols; } g{rows, cols};
    if (steps > 0) {
        memmove(&g_termHistory[(size_t)steps * g.cols], &g_termHistory[0],
                sizeof(float) * (size_t)(g.rows - steps) * g.cols);
    }
    for (int c = 0; c < g.cols; c++) g_termHistory[c] = std::max(0.f, g_vizPeak[c]);
}

static void Dump(const char* tag, const std::vector<float>& h, int rows, int cols) {
    printf("  %s:", tag);
    for (int r = 0; r < rows; r++) {
        printf(" [");
        for (int c = 0; c < cols; c++) printf("%s%.2f", c ? " " : "", h[(size_t)r * cols + c]);
        printf("]");
    }
    printf("\n");
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    bool runOld = argc > 1 && strcmp(argv[1], "old") == 0;
    const int rows = 4, cols = 3;
    int fails = 0;
    // History rows hold 0.1*row+1 ("old moments"); newest frame = 0.9.
    auto fresh = [&] {
        std::vector<float> h((size_t)rows * cols);
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++) h[(size_t)r * cols + c] = 0.1f * (r + 1);
        return h;
    };
    const float newest[cols] = {0.9f, 0.9f, -0.2f};  // negative clamps to 0
    for (int steps : {1, 2, 3, rows, rows + 5}) {
        printf("steps=%d (rows=%d)\n", steps, rows);
        std::vector<float> h = fresh();
        Dump("before", h, rows, cols);
        if (runOld) {
            OldScroll(h, rows, cols, std::min(steps, rows), newest);  // caller clamps to rows, as the old code did
            Dump("old   ", h, rows, cols);
            continue;
        }
        VizTermScrollHistory(h.data(), rows, cols, std::min(steps, rows), newest);
        Dump("new   ", h, rows, cols);
        // Checks: row 0 = clamped newest; rows below `steps` = old rows shifted;
        // rows 1..steps-1 lie between newest and the previous newest (no stale data).
        int s = std::min(steps, rows);
        for (int c = 0; c < cols; c++) {
            float nv = std::max(0.f, newest[c]);
            if (h[c] != nv) fails++;
            for (int r = s; r < rows; r++)
                if (h[(size_t)r * cols + c] != 0.1f * (r - s + 1)) fails++;
            for (int r = 1; r < s; r++) {
                float v = h[(size_t)r * cols + c];
                float lo = std::min(nv, 0.1f), hi = std::max(nv, 0.1f);
                if (s >= rows) { if (v != nv) fails++; }
                else if (v < lo - 1e-6f || v > hi + 1e-6f) fails++;
            }
        }
    }
    if (!runOld) printf(fails ? "FAIL (%d)\n" : "PASS\n", fails);
    return fails ? 1 : 0;
}
