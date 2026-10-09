// Progress-bar extrapolation across pause / resume, old logic vs the shipped
// block (gen/timeline_new.inc is cut from v2b.cpp between the "track timeline"
// markers; gen/timeline_old.inc is the old VizTrackProgress cut from the
// original v2b.cpp, and OldRefreshTimeline mirrors the old RefreshMediaTimeline).
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

typedef unsigned long long ULONGLONG;
static ULONGLONG g_fakeNow = 1000000;  // ms
static ULONGLONG GetTickCount64() { return g_fakeNow; }
std::atomic<bool> g_mediaIsPlaying{false};

namespace nw {
#include "gen/timeline_new.inc"
}
namespace old {
#include "gen/timeline_old.inc"
// Old RefreshMediaTimeline, minus the WinRT calls (v2b 3075-3099).
void OldRefreshTimeline(int64_t start, int64_t end, int64_t pos, int64_t ageMs) {
    if (ageMs < 0 || ageMs > 6LL * 3600 * 1000) ageMs = 0;
    g_tlStart.store(start); g_tlEnd.store(end); g_tlPos.store(pos);
    g_tlTick.store(GetTickCount64() - (ULONGLONG)ageMs);
    g_tlValid.store(end > start);
}
}

// A player: 200 s track. Plays 0-30 s, paused 30-50 s, plays from 50 s.
// It reports its timeline once at the start and, if `reportsOnPause`, again
// when pausing (LastUpdatedTime = that moment); never on resume.
struct Player {
    bool reportsOnPause;
    int64_t tlPosMs = 0;
    ULONGLONG tlUpdated = 0;  // fake ms
};
static const double kDurMs = 200000.0;
static double TruePosMs(double tSec) {
    if (tSec <= 30) return tSec * 1000;
    if (tSec <= 50) return 30000;
    return 30000 + (tSec - 50) * 1000;
}

static double Run(bool useNew, bool reportsOnPause, bool verbose) {
    const ULONGLONG t0 = 1000000;
    g_fakeNow = t0;
    Player pl{reportsOnPause};
    uint32_t gen = 0;
    auto statusEvent = [&](bool playing) {
        g_mediaIsPlaying.store(playing);
        if (useNew) nw::VizTimelineSetPlaying(gen, playing);
    };
    auto timelineEvent = [&]() {
        int64_t age = (int64_t)(g_fakeNow - pl.tlUpdated);
        if (useNew) nw::VizTimelineSet(gen, 0, (int64_t)kDurMs * 10000, pl.tlPosMs * 10000, age);
        else old::OldRefreshTimeline(0, (int64_t)kDurMs * 10000, pl.tlPosMs * 10000, age);
    };
    // Session hookup at t=0 (initial status then initial timeline).
    if (useNew) gen = nw::VizTimelineNewSource();
    pl.tlPosMs = 0; pl.tlUpdated = t0;
    statusEvent(true);
    timelineEvent();
    double worst = 0;
    for (int ms = 0; ms <= 70000; ms += 250) {
        g_fakeNow = t0 + ms;
        double t = ms / 1000.0;
        if (ms == 30000) {  // pause
            if (pl.reportsOnPause) { pl.tlPosMs = 30000; pl.tlUpdated = g_fakeNow; }
            statusEvent(false);
            if (!useNew) timelineEvent();             // old: PlaybackInfoChanged re-read the timeline
            else if (pl.reportsOnPause) timelineEvent();  // new: only on TimelinePropertiesChanged
        }
        if (ms == 50000) {  // resume, timeline untouched
            statusEvent(true);
            if (!useNew) timelineEvent();
        }
        float p = useNew ? nw::VizTrackProgress() : old::VizTrackProgress();
        double errMs = std::fabs(p * kDurMs - TruePosMs(t));
        worst = std::max(worst, errMs);
        if (verbose && (ms == 29000 || ms == 31000 || ms == 49000 || ms == 51000 || ms == 70000))
            printf("    t=%5.1fs  true=%6.1fs  bar=%6.1fs  err=%6.2fs\n", t, TruePosMs(t) / 1000, p * kDurMs / 1000,
                   errMs / 1000);
    }
    return worst / 1000;
}

int main() {
    int fails = 0;
    for (int rop = 0; rop < 2; rop++) {
        printf("player %s its timeline on pause:\n", rop ? "updates" : "does NOT update");
        printf("  OLD\n");
        double o = Run(false, rop, true);
        printf("  NEW\n");
        double n = Run(true, rop, true);
        printf("  worst error over 0-70 s: old %.2f s, new %.2f s\n", o, n);
        if (n > 0.3) fails++;
    }
    // Stale-generation writes are dropped.
    {
        g_fakeNow = 5000000;
        uint32_t g1 = nw::VizTimelineNewSource();
        nw::VizTimelineSetPlaying(g1, false);
        nw::VizTimelineSet(g1, 0, 1000000000, 500000000, 0);  // 50%
        uint32_t g2 = nw::VizTimelineNewSource();
        nw::VizTimelineSet(g2, 0, 1000000000, 100000000, 0);  // 10%
        nw::VizTimelineSet(g1, 0, 1000000000, 900000000, 0);  // late event from the old session
        nw::VizTimelineSetPlaying(g1, true);
        float p = nw::VizTrackProgress();
        printf("stale-session write after swap: progress %.3f (expect 0.100), playing=%d (expect 0)\n", p,
               (int)g_mediaIsPlaying.load());
        if (std::fabs(p - 0.1f) > 1e-4 || g_mediaIsPlaying.load()) fails++;
    }
    // Concurrent writers + reader (TSan run): consistency of pos/tick pairs.
    {
        uint32_t g = nw::VizTimelineNewSource();
        nw::VizTimelineSet(g, 0, 1000000000, 0, 0);
        std::atomic<bool> stop{false};
        std::thread w1([&] { for (int i = 0; i < 20000; i++) nw::VizTimelineSetPlaying(g, i & 1); });
        std::thread w2([&] { for (int i = 0; i < 20000; i++) nw::VizTimelineSet(g, 0, 1000000000, 300000000, 0); });
        int bad = 0;
        std::thread r([&] { while (!stop.load()) { float p = nw::VizTrackProgress(); if (p < 0.f || p > 1.f) bad++; } });
        w1.join(); w2.join(); stop = true; r.join();
        printf("concurrent writers/reader: %d out-of-range reads\n", bad);
        if (bad) fails++;
    }
    printf(fails ? "FAIL\n" : "PASS\n");
    return fails ? 1 : 0;
}
