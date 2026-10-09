// Compiles the GSMTC region cut from v2b.cpp (the part the Windows syntax check
// stubs out) against a small mock of the C++/WinRT projection, and runs it
// under ThreadSanitizer: session swaps (CurrentSessionChanged) racing session
// events on other threads, as the WinRT thread pool delivers them.
//   -DUSE_OLD : the original region and timeline (expect TSan reports)
// The mock's delegates are built the way C++/WinRT builds them (any callable
// invocable as (TSender const&, TArgs const&)), and event handlers are invoked
// outside the source's lock, after copying the handler list, like WinRT does.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

typedef unsigned long long ULONGLONG;
static ULONGLONG GetTickCount64() {
    return (ULONGLONG)std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ---- mock Win32 bits the region uses
typedef struct MockEvent { std::atomic<bool> set{false}; } *HANDLE;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define INFINITE 0xFFFFFFFFu
static HANDLE CreateEvent(void*, BOOL, BOOL, void*) { return new MockEvent; }
static unsigned WaitForSingleObject(HANDLE h, unsigned) {
    while (!h->set.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 0;
}
static std::atomic<int> g_posts{0};
static int PostMessage(void*, unsigned, int, int) { g_posts++; return 1; }
static void* g_mediaWnd = (void*)1;
#define WM_APP_MEDIA_REPAINT 0x8005
enum class VizColorMode { Solid, AlbumArt, DynamicAlbum };
struct { VizColorMode colorMode = VizColorMode::Solid; bool nowPlayingEnabled = false; } g_settings;
static void FetchAlbumArtColorAsync() {}
static bool VizCardWantsArt() { return false; }  // Media Card (2.1)
std::atomic<bool> g_mediaIsPlaying{false};
static HANDLE g_gsmtcStopEvent = nullptr;
static std::optional<std::thread> g_gsmtcThread;

// ---- mock C++/WinRT
namespace winrt {
struct event_token { int64_t value = 0; };
enum class apartment_type { multi_threaded };
inline void init_apartment(apartment_type) {}
inline void uninit_apartment() {}
struct clock {
    using rep = int64_t;
    using period = std::ratio<1, 10000000>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<clock, duration>;
    static constexpr bool is_steady = false;
    static time_point now() {
        return time_point(std::chrono::duration_cast<duration>(std::chrono::system_clock::now().time_since_epoch()));
    }
};
namespace Windows::Foundation {
template <class S, class A>
struct TypedEventHandler {
    std::function<void(S const&, A const&)> f;
    template <class L>
    TypedEventHandler(L l) : f(std::move(l)) {}
    void operator()(S const& s, A const& a) const { f(s, a); }
};
using TimeSpan = clock::duration;
using DateTime = clock::time_point;
}  // namespace Windows::Foundation
namespace Windows::Media::Control {
using winrt::Windows::Foundation::TypedEventHandler;
enum class GlobalSystemMediaTransportControlsSessionPlaybackStatus { Closed, Opened, Changing, Stopped, Playing, Paused };
struct MediaPropertiesChangedEventArgs {};
struct PlaybackInfoChangedEventArgs {};
struct TimelinePropertiesChangedEventArgs {};
struct CurrentSessionChangedEventArgs {};
struct GlobalSystemMediaTransportControlsSessionPlaybackInfo {
    std::shared_ptr<GlobalSystemMediaTransportControlsSessionPlaybackStatus> s;
    explicit operator bool() const { return (bool)s; }
    GlobalSystemMediaTransportControlsSessionPlaybackStatus PlaybackStatus() const { return *s; }
};
struct GlobalSystemMediaTransportControlsSessionTimelineProperties {
    std::shared_ptr<int64_t[]> v;  // start, end, pos (100 ns), lastUpdated (clock ticks)
    explicit operator bool() const { return (bool)v; }
    Windows::Foundation::TimeSpan StartTime() const { return Windows::Foundation::TimeSpan(v[0]); }
    Windows::Foundation::TimeSpan EndTime() const { return Windows::Foundation::TimeSpan(v[1]); }
    Windows::Foundation::TimeSpan Position() const { return Windows::Foundation::TimeSpan(v[2]); }
    Windows::Foundation::DateTime LastUpdatedTime() const { return Windows::Foundation::DateTime(clock::duration(v[3])); }
};
class GlobalSystemMediaTransportControlsSession;
template <class A>
using SessionHandler = TypedEventHandler<GlobalSystemMediaTransportControlsSession, A>;
struct SessionImpl {
    std::mutex m;
    int64_t next = 1;
    std::vector<std::pair<int64_t, SessionHandler<MediaPropertiesChangedEventArgs>>> mp;
    std::vector<std::pair<int64_t, SessionHandler<PlaybackInfoChangedEventArgs>>> pb;
    std::vector<std::pair<int64_t, SessionHandler<TimelinePropertiesChangedEventArgs>>> tl;
    std::atomic<bool> playing{true};
    std::atomic<int64_t> pos{0};
};
template <class V, class H>
static event_token Add(SessionImpl* i, V& v, H const& h) {
    std::lock_guard<std::mutex> l(i->m);
    v.emplace_back(i->next, h);
    return event_token{i->next++};
}
template <class V>
static void Remove(SessionImpl* i, V& v, event_token const& t) {
    std::lock_guard<std::mutex> l(i->m);
    v.erase(std::remove_if(v.begin(), v.end(), [&](auto& p) { return p.first == t.value; }), v.end());
}
class GlobalSystemMediaTransportControlsSession {
  public:
    std::shared_ptr<SessionImpl> p;
    GlobalSystemMediaTransportControlsSession(std::nullptr_t = nullptr) {}
    explicit GlobalSystemMediaTransportControlsSession(std::shared_ptr<SessionImpl> i) : p(std::move(i)) {}
    explicit operator bool() const { return (bool)p; }
    GlobalSystemMediaTransportControlsSessionPlaybackInfo GetPlaybackInfo() const {
        return {std::make_shared<GlobalSystemMediaTransportControlsSessionPlaybackStatus>(
            p->playing ? GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing
                       : GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused)};
    }
    GlobalSystemMediaTransportControlsSessionTimelineProperties GetTimelineProperties() const {
        std::shared_ptr<int64_t[]> v(new int64_t[4]);
        v[0] = 0; v[1] = 2000000000; v[2] = p->pos.load(); v[3] = clock::now().time_since_epoch().count();
        return {v};
    }
    event_token MediaPropertiesChanged(SessionHandler<MediaPropertiesChangedEventArgs> const& h) const { return Add(p.get(), p->mp, h); }
    void MediaPropertiesChanged(event_token const& t) const { Remove(p.get(), p->mp, t); }
    event_token PlaybackInfoChanged(SessionHandler<PlaybackInfoChangedEventArgs> const& h) const { return Add(p.get(), p->pb, h); }
    void PlaybackInfoChanged(event_token const& t) const { Remove(p.get(), p->pb, t); }
    event_token TimelinePropertiesChanged(SessionHandler<TimelinePropertiesChangedEventArgs> const& h) const { return Add(p.get(), p->tl, h); }
    void TimelinePropertiesChanged(event_token const& t) const { Remove(p.get(), p->tl, t); }
    // Test side: raise events the way WinRT does (copy, then call unlocked).
    void FirePlayback() const { decltype(p->pb) c; { std::lock_guard<std::mutex> l(p->m); c = p->pb; } for (auto& h : c) h.second(*this, {}); }
    void FireTimeline() const { decltype(p->tl) c; { std::lock_guard<std::mutex> l(p->m); c = p->tl; } for (auto& h : c) h.second(*this, {}); }
    decltype(SessionImpl::tl) TimelineHandlers() const { std::lock_guard<std::mutex> l(p->m); return p->tl; }
};
class GlobalSystemMediaTransportControlsSessionManager;
using MgrHandler = TypedEventHandler<GlobalSystemMediaTransportControlsSessionManager, CurrentSessionChangedEventArgs>;
struct MgrImpl {
    std::mutex m;
    std::vector<std::pair<int64_t, MgrHandler>> cs;
    int64_t next = 1;
    GlobalSystemMediaTransportControlsSession sessions[2];
    std::atomic<int> current{0};
};
static std::shared_ptr<MgrImpl> g_mockMgr = std::make_shared<MgrImpl>();
class GlobalSystemMediaTransportControlsSessionManager {
  public:
    std::shared_ptr<MgrImpl> p;
    GlobalSystemMediaTransportControlsSessionManager(std::nullptr_t = nullptr) {}
    explicit operator bool() const { return (bool)p; }
    struct Op { GlobalSystemMediaTransportControlsSessionManager get() const { GlobalSystemMediaTransportControlsSessionManager r; r.p = g_mockMgr; return r; } };
    static Op RequestAsync() { return {}; }
    GlobalSystemMediaTransportControlsSession GetCurrentSession() const { return p->sessions[p->current.load()]; }
    event_token CurrentSessionChanged(MgrHandler const& h) const { std::lock_guard<std::mutex> l(p->m); p->cs.emplace_back(p->next, h); return {p->next++}; }
    void CurrentSessionChanged(event_token const& t) const {
        std::lock_guard<std::mutex> l(p->m);
        p->cs.erase(std::remove_if(p->cs.begin(), p->cs.end(), [&](auto& x) { return x.first == t.value; }), p->cs.end());
    }
    void FireChanged() const { decltype(p->cs) c; { std::lock_guard<std::mutex> l(p->m); c = p->cs; } for (auto& h : c) h.second(*this, {}); }
};
}  // namespace Windows::Media::Control
}  // namespace winrt
using namespace winrt::Windows::Media::Control;

#ifdef USE_OLD
#include "gen/timeline_old.inc"
#include "gen/gsmtc_old.inc"
#else
#include "gen/timeline_new.inc"
#include "gen/gsmtc_new.inc"
#endif

int main() {
    auto& M = *g_mockMgr;
    M.sessions[0] = GlobalSystemMediaTransportControlsSession(std::make_shared<SessionImpl>());
    M.sessions[1] = GlobalSystemMediaTransportControlsSession(std::make_shared<SessionImpl>());
    M.sessions[0].p->pos = 1800000000;  // 90 %
    M.sessions[1].p->pos = 200000000;   // 10 %
    InitGsmtcListener();
    while (true) {  // wait for the GSMTC thread's first hookup
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::lock_guard<std::mutex> l(M.sessions[0].p->m);
        if (!M.sessions[0].p->tl.empty()) break;
    }
    GlobalSystemMediaTransportControlsSessionManager mgr;
    mgr.p = g_mockMgr;
    int fails = 0;
#ifndef USE_OLD
    // Deterministic: a handler of session 0 still in flight when the current
    // session becomes 1 must not overwrite session 1's timeline.
    {
        auto stale = M.sessions[0].TimelineHandlers();
        M.current = 1;
        mgr.FireChanged();
        for (auto& h : stale) h.second(M.sessions[0], {});
        float p = VizTrackProgress();
        printf("stale in-flight handler after swap: progress %.3f (session 1 = 0.100)\n", p);
        if (p < 0.09f || p > 0.12f) fails++;
    }
#endif
    std::atomic<bool> stop{false};
    std::thread swapper([&] { for (int i = 0; i < 300; i++) { M.current = i & 1; mgr.FireChanged(); } });
    std::thread ev1([&] { while (!stop) { M.sessions[0].FirePlayback(); M.sessions[1].FireTimeline(); } });
    std::thread ev2([&] { while (!stop) { M.sessions[1].FirePlayback(); M.sessions[0].FireTimeline(); } });
    std::thread reader([&] { while (!stop) (void)VizTrackProgress(); });
    swapper.join();
    stop = true;
    ev1.join(); ev2.join(); reader.join();
    g_gsmtcStopEvent->set = true;
    g_gsmtcThread->join();
    printf("swaps done, %d repaint posts, %s\n", g_posts.load(), fails ? "FAIL" : "ok");
    return fails;
}
