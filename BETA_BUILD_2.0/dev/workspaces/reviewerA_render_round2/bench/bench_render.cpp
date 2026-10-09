// CPU-side cost of the render tick's hash + upload work, before (src_before_ultrareview)
// and after (wA). The loops are copied from Render() / VizBuildTextFrame(); a D3D11
// Map(WRITE_DISCARD) is stood in for by a write to a separate heap buffer of the same
// size (the driver call and the buffer rename it also costs are NOT included, so the
// "before" numbers are a lower bound). Build: clang++ -O2 -std=c++20 bench_render.cpp
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <random>
#include <string>
#include <vector>

#define swprintf_s(buf, ...) swprintf(buf, sizeof(buf) / sizeof(buf[0]), __VA_ARGS__)
static inline void Clobber() { asm volatile("" ::: "memory"); }

inline void Mix(uint64_t& h, uint64_t v) { h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2); }
inline void MixF(uint64_t& h, float v, float q) { Mix(h, (uint64_t)(int64_t)llroundf(v * q)); }
// After: rounds with a plain conversion instead of the llroundf library call.
inline void MixQ(uint64_t& h, float v, float q) {
    float r = v * q;
    Mix(h, (uint64_t)(int64_t)(r + (r >= 0.f ? 0.5f : -0.5f)));
}

template <class F>
static double MedianNs(F&& f, int iters) {
    std::vector<double> t;
    for (int r = 0; r < 51; r++) {
        auto a = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; i++) f();
        auto b = std::chrono::steady_clock::now();
        t.push_back(std::chrono::duration<double, std::nano>(b - a).count() / iters);
    }
    std::nth_element(t.begin(), t.begin() + 25, t.end());
    return t[25];
}

constexpr int kCells = 65536;
static std::vector<uint32_t> g_cells(kCells);
static std::vector<uint32_t> g_gpuCells(kCells);  // stands in for the mapped cellsDyn
static uint64_t g_sink;
static uint32_t g_termGridSerial = 1;

// Terminal grid, before: Map + memcpy every tick, then hash all 65,536 cells.
static void TermBefore() {
    memcpy(g_gpuCells.data(), g_cells.data(), kCells * 4);
    Clobber();
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < kCells; i++) Mix(h, g_cells[i]);
    g_sink += h;
}
// After, frame where the grid didn't change: the serial stands for every cell.
static uint32_t s_uploadedSerial = 0;
static uint32_t s_count = 0;
static void TermAfter() {
    uint64_t h = 1469598103934665603ull;
    Mix(h, (uint64_t)256 << 32 | 256);
    Mix(h, g_termGridSerial);
    if (s_uploadedSerial != g_termGridSerial) {  // changed: compact upload of glyph cells only
        uint32_t* out = g_gpuCells.data();
        uint32_t k = 0;
        for (int i = 0; i < kCells; i++) {
            uint32_t c = g_cells[i];
            out[k] = (c & 0xFFFFu) | ((uint32_t)i << 16);
            k += ((c & 127u) > 32u) ? 1u : 0u;
        }
        Clobber();
        s_count = k;
        s_uploadedSerial = g_termGridSerial;
    }
    g_sink += h + s_count;
}

// Bars: 256 bars with peak caps, per tick.
constexpr int kBars = 256;
static float g_peak[kBars], g_hold[kBars];
static float g_barsGpu[2 * 2048], g_globalsGpu[16];
static void BarsBefore() {  // uploads (bars + globals) on every tick, hashed while uploading
    uint64_t h = 1469598103934665603ull;
    const float range = 200.f;
    float* p = g_barsGpu;
    for (int i = 0; i < kBars; i++) {
        float lv = std::max(0.f, g_peak[i]), pk = g_hold[i];
        p[2 * i] = lv;
        p[2 * i + 1] = pk;
        MixF(h, lv * range, 4.f);
        MixF(h, pk * range, 4.f);
    }
    float gl[16] = {0.3f, 0, 0, 0, 1, 2, 3, 0};
    memcpy(g_globalsGpu, gl, sizeof(gl));
    Clobber();
    g_sink += h;
}
static void BarsHashOldRounding() {  // the after-loop, but with llroundf (isolates the rounding change)
    uint64_t h = 1469598103934665603ull;
    const float range = 200.f;
    Mix(h, (uint64_t)kBars);
    for (int i = 0; i < kBars; i++) {
        MixF(h, std::max(0.f, g_peak[i]) * range, 4.f);
        MixF(h, g_hold[i] * range, 4.f);
    }
    g_sink += h;
}
static void BarsAfterSkipped() {  // hash only; nothing uploaded when unchanged
    uint64_t h = 1469598103934665603ull;
    const float range = 200.f;
    Mix(h, (uint64_t)kBars);
    for (int i = 0; i < kBars; i++) {
        MixQ(h, std::max(0.f, g_peak[i]) * range, 4.f);
        MixQ(h, g_hold[i] * range, 4.f);
    }
    g_sink += h;
}

// Readout (LoudnessFull + frequency, "Both"-style worst case) and the text frame.
struct Meters { double momentary = -14.2, shortTerm = -15.1, integrated = -16.3, truePeak = -1.2, correlation = 0.63; };
static Meters g_m;
static float g_hz = 997.3f;
static std::wstring g_np = L"Daft Punk - Harder, Better, Faster, Stronger", g_title = L"Harder, Better, Faster, Stronger",
                    g_artist = L"Daft Punk";
struct TextFrame { std::wstring np, npTitle, npArtist; float npAlpha = 0; float progress = -1; std::wstring pf; bool pfWide = false; };
static void AppendLufs(std::wstring& s, const wchar_t* label, double v) {
    wchar_t b[32];
    if (std::isfinite(v) && v > -70.0) swprintf_s(b, L"%ls %.1f", label, v);
    else swprintf_s(b, L"%ls --", label);
    if (!s.empty()) s += L"  ";
    s += b;
}
static std::wstring FormatReadout(float hzv) {
    wchar_t fb[32] = L"";
    if (hzv > 0.f) {
        if (hzv >= 1000.f) swprintf_s(fb, L"%.1f kHz", hzv / 1000.f);
        else swprintf_s(fb, L"%.0f Hz", hzv);
    }
    std::wstring s = fb;
    AppendLufs(s, L"M", g_m.momentary);
    AppendLufs(s, L"S", g_m.shortTerm);
    AppendLufs(s, L"I", g_m.integrated);
    s += L" LUFS";
    wchar_t b[96];
    swprintf_s(b, L"  TP %.1f dBTP", g_m.truePeak);
    s += b;
    swprintf_s(b, L"  PLR %.1f", g_m.truePeak - g_m.integrated);
    s += b;
    swprintf_s(b, L"  r %+.2f", g_m.correlation);
    s += b;
    return s;
}
static uint64_t TextKey(const TextFrame& tf) {
    uint64_t key = 1469598103934665603ull;
    for (wchar_t c : tf.np) Mix(key, (uint64_t)c);
    MixF(key, tf.npAlpha, 255.f);
    for (wchar_t c : tf.pf) Mix(key, (uint64_t)c);
    for (wchar_t c : tf.npArtist) Mix(key, (uint64_t)c);
    return key;
}
static void TextBefore() {  // fresh frame, formatted every tick
    TextFrame t;
    t.npAlpha = 1.f;
    t.np = g_np;
    t.npTitle = g_title;
    t.npArtist = g_artist;
    t.pf = FormatReadout(g_hz);
    t.pfWide = true;
    g_sink += TextKey(t);
}
static std::wstring s_cache;
static int s_tick = 0;
static void TextAfter() {  // persistent frame; readout rebuilt every 100 ms = 1 tick in 14.4 at 144 Hz
    static TextFrame t;
    t.np.clear(); t.npTitle.clear(); t.npArtist.clear(); t.pf.clear();
    t.npAlpha = 1.f;
    t.np = g_np;
    t.npTitle = g_title;
    t.npArtist = g_artist;
    if (s_cache.empty() || (++s_tick % 14) == 0) s_cache = FormatReadout(g_hz);
    t.pf = s_cache;
    t.pfWide = true;
    g_sink += TextKey(t);
}

int main() {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> U(0.f, 1.f);
    for (int i = 0; i < kBars; i++) { g_peak[i] = U(rng); g_hold[i] = std::min(1.f, g_peak[i] + 0.1f); }
    printf("cells=%d, bars=%d (peak caps on), readout=LoudnessFull+Hz, NP one line (44 chars)\n", kCells, kBars);
    for (double fill : {0.3, 0.6, 1.0, -1.0}) {
        if (fill >= 0) {
            for (int i = 0; i < kCells; i++) g_cells[i] = (U(rng) < fill) ? ('|' | (1u << 8)) : 32u;
        } else {  // Columns style, 256 x 256: each column lit from the bottom to a random height
            std::fill(g_cells.begin(), g_cells.end(), 32u);
            int lit = 0;
            for (int c = 0; c < 256; c++) {
                int hgt = (int)(U(rng) * 256);
                for (int k = 0; k < hgt; k++) g_cells[(255 - k) * 256 + c] = '|' | (1u << 8), lit++;
            }
            fill = lit / (double)kCells;
        }
        double b = MedianNs(TermBefore, 200);
        s_uploadedSerial = g_termGridSerial;  // unchanged frame
        double aSame = MedianNs(TermAfter, 2000);
        double aChanged = MedianNs([] { g_termGridSerial++; TermAfter(); }, 200);
        printf("terminal %3.0f%% glyphs: before %8.1f us/tick | after unchanged %6.3f us, changed %7.1f us (%u of %d cells uploaded)\n",
               fill * 100, b / 1000, aSame / 1000, aChanged / 1000, s_count, kCells);
    }
    printf("bars 256: before (hash + 2 uploads, every tick) %.0f ns | after, skipped tick (hash only) %.0f ns\n",
           MedianNs(BarsBefore, 20000), MedianNs(BarsAfterSkipped, 20000));
    printf("bars 256 hash only: llroundf %.0f ns | conversion %.0f ns\n", MedianNs(BarsHashOldRounding, 20000),
           MedianNs(BarsAfterSkipped, 20000));
    printf("text frame + readout: before %.0f ns/tick | after %.0f ns/tick\n", MedianNs(TextBefore, 20000),
           MedianNs(TextAfter, 20000));
    printf("(sink %llu)\n", (unsigned long long)(g_sink & 1));
}
