// ---- Performance Stats (2.0) -----------------------------------------------------------
//
// Cheap counters for the claims this mod makes about its own cost, so they can
// be checked on any PC rather than taken on trust. Every counter is one relaxed
// atomic add; the timers are two QueryPerformanceCounter reads. With
// Performance -> Performance Stats on, the engine thread writes one line to the
// Windhawk log every 30 seconds:
//
//   [Perf] 30.0 s: engine 144.0 wakes/s, 0.11 ms avg | analyses 100.0/s, FFTs 117.3/s |
//          ticks 144.0/s, render 0.06 ms avg | presents 98.2/s, skipped 45.8/s, text 0.3/s |
//          commits 0.0/s, maps 98.2/s | idle: playing 100% trickle 0% deep 0%
//
// The figures are per second of wall time, so they compare directly across
// settings, renderers and machines.

enum VizPerfCounter : int {
    kPerfEngineWakes,   // engine loop iterations
    kPerfAnalyses,      // frames that ran the spectrum analysis
    kPerfFfts,          // FFTs computed (all tiers)
    kPerfRenderTicks,   // render ticks handled on the UI thread
    kPerfPresents,      // panel surface presents (Direct3D 11) or frames drawn (Direct2D)
    kPerfSkipped,       // frames skipped because nothing changed
    kPerfTextPresents,  // text surface presents
    kPerfCommits,       // DirectComposition commits from the render path
    kPerfMaps,          // dynamic buffer uploads (Map / Unmap pairs)
    kPerfIdlePlaying,   // engine wakes spent in each idle state
    kPerfIdleTrickle,
    kPerfIdleDeep,
    kPerfCount
};
std::atomic<uint32_t> g_perfCount[kPerfCount];
std::atomic<uint64_t> g_perfEngineTicks{0}, g_perfRenderTicks{0};  // QPC ticks
std::atomic<bool> g_perfStatsEnabled{false};

inline void VizPerf(VizPerfCounter c, uint32_t n = 1) { g_perfCount[c].fetch_add(n, std::memory_order_relaxed); }

// Adds the time between construction and destruction to an accumulator.
struct VizPerfScope {
    std::atomic<uint64_t>& acc;
    LARGE_INTEGER t0;
    explicit VizPerfScope(std::atomic<uint64_t>& a) : acc(a) { QueryPerformanceCounter(&t0); }
    ~VizPerfScope() {
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        acc.fetch_add((uint64_t)(t1.QuadPart - t0.QuadPart), std::memory_order_relaxed);
    }
};

// Called by the engine thread once per loop. Logs and resets every 30 s.
void VizPerfMaybeLog() {
    static LARGE_INTEGER s_start = {}, s_freq = {};
    if (!s_freq.QuadPart) QueryPerformanceFrequency(&s_freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (!s_start.QuadPart) {
        s_start = now;
        return;
    }
    double secs = (double)(now.QuadPart - s_start.QuadPart) / (double)s_freq.QuadPart;
    if (secs < 30.0) return;
    s_start = now;
    uint32_t c[kPerfCount];
    for (int i = 0; i < kPerfCount; i++) c[i] = g_perfCount[i].exchange(0, std::memory_order_relaxed);
    uint64_t eng = g_perfEngineTicks.exchange(0, std::memory_order_relaxed);
    uint64_t ren = g_perfRenderTicks.exchange(0, std::memory_order_relaxed);
    if (!g_perfStatsEnabled.load(std::memory_order_relaxed)) return;
    auto rate = [&](int i) { return c[i] / secs; };
    auto avgMs = [&](uint64_t t, uint32_t n) { return n ? (double)t * 1000.0 / (double)s_freq.QuadPart / n : 0.0; };
    double idleTotal = (double)std::max<uint32_t>(1, c[kPerfIdlePlaying] + c[kPerfIdleTrickle] + c[kPerfIdleDeep]);
    Wh_Log(L"[Perf] %.1f s: engine %.1f wakes/s, %.3f ms avg | analyses %.1f/s, FFTs %.1f/s | ticks %.1f/s, "
           L"render %.3f ms avg | presents %.1f/s, skipped %.1f/s, text %.1f/s | commits %.1f/s, maps %.1f/s | "
           L"idle: playing %.0f%% trickle %.0f%% deep %.0f%%",
           secs, rate(kPerfEngineWakes), avgMs(eng, c[kPerfEngineWakes]), rate(kPerfAnalyses), rate(kPerfFfts),
           rate(kPerfRenderTicks), avgMs(ren, c[kPerfRenderTicks]), rate(kPerfPresents), rate(kPerfSkipped),
           rate(kPerfTextPresents), rate(kPerfCommits), rate(kPerfMaps), 100.0 * c[kPerfIdlePlaying] / idleTotal,
           100.0 * c[kPerfIdleTrickle] / idleTotal, 100.0 * c[kPerfIdleDeep] / idleTotal);
}
