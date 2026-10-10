"""Second stage of the 2.0 beta build: takes v2.cpp (from splice.py) and adds
the right-click menu, audio source selection, the media-widget options and
the Terminal shape, writing v2b.cpp. Same rule as splice.py: every edit
anchors on text that must occur exactly the stated number of times."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
S = os.path.dirname(HERE)
src = open(os.path.join(S, "v2.cpp"), encoding="utf-8").read()


def rep(old, new, count=1):
    global src
    n = src.count(old)
    if n != count:
        sys.exit(f"anchor count {n} != {count}:\n{old[:300]}")
    src = src.replace(old, new)


def before(anchor, text):
    rep(anchor, text + anchor)


def after(anchor, text):
    rep(anchor, anchor + text)


def read(name):
    return open(os.path.join(HERE, name), encoding="utf-8").read()


# ================================================================ header
rep("""// @description         BETA BUILD. Tourne'Table 2.0: a precision analysis core""",
    """// @description         BETA BUILD. Tourne'Table 2.0: a right-click menu for live settings and audio source (any output, input or virtual device), a Terminal shape, a media-widget layout, a precision analysis core""")
after("#include <cmath>\n", "#include <cwctype>\n")

# ================================================================ enums / settings
rep("enum class VizShape { Stereo, Mountain, Mirror, Wave, Breathe, Dots, Radial, Oscilloscope, Goniometer };",
    "enum class VizShape { Stereo, Mountain, Mirror, Wave, Breathe, Dots, Radial, Oscilloscope, Goniometer, Terminal };")
after("enum class VizReadout { Frequency, Loudness, LoudnessFull, Both };\n", """enum class VizTermStyle { Columns, Waterfall, Meters };
enum class VizNpLayout { OneLine, TwoLines };
enum class VizNpPlacement { Above, PanelTop, PanelBottom };
enum class VizProgressPlacement { Below, Above, PanelBottom };
enum class VizMediaAnchor { Screen, PanelTopLeft, PanelTopRight, PanelBottomLeft, PanelBottomRight };
enum class VizContextMenu { RightClick, CtrlRightClick, Off };
""")
rep("""    int layoutBandCount = 0;  // bars implied by an IEC / musical layout, 0 = use Bar Count
};""", """    int layoutBandCount = 0;  // bars implied by an IEC / musical layout, 0 = use Bar Count

    // Audio source (2.0): "" / default_output, default_input, id:<endpoint>, name:<text>.
    std::wstring audioSourceKey;

    // Media widget (2.0).
    VizNpLayout npLayout = VizNpLayout::OneLine;
    VizNpPlacement npPlacement = VizNpPlacement::Above;
    VizTextAlignH npAlign = VizTextAlignH::Center;
    BYTE npArtistA = 0xB3, npArtistR = 255, npArtistG = 255, npArtistB = 255;
    bool textPixel = false;
    bool progressEnabled = false;
    VizProgressPlacement progressPlacement = VizProgressPlacement::Below;
    int progressHeight = 2, progressGap = 6;
    BYTE progressA = 255, progressR = 255, progressG = 255, progressB = 255;
    BYTE progressTrackA = 0x40, progressTrackR = 255, progressTrackG = 255, progressTrackB = 255;
    VizMediaAnchor mediaAnchor = VizMediaAnchor::Screen;
    int mediaAnchorOffsetX = 8, mediaAnchorOffsetY = 8;
    VizContextMenu contextMenu = VizContextMenu::RightClick;

    // Terminal shape (2.0).
    VizTermStyle termStyle = VizTermStyle::Columns;
    std::wstring termFont = L"Consolas";
    int termFontSize = 14;
    int termRows = 16;
    int termMeterColumns = 40;
    int termHotThreshold = 75;
    wchar_t termColumnGlyph = L'#';
    wchar_t termPeakGlyph = L'-';
    std::wstring termRamp = L" .:-=+*#%@";
    int termScrollRate = 20;
    BYTE termDimA = 255, termDimR = 0x1E, termDimG = 0x6B, termDimB = 0x34;
    BYTE termLowA = 255, termLowR = 0x33, termLowG = 0xFF, termLowB = 0x66;
    BYTE termHighA = 255, termHighR = 0xFF, termHighG = 0x3B, termHighB = 0x3B;
    BYTE termLabelA = 255, termLabelR = 0xB8, termLabelG = 0xFF, termLabelB = 0xB8;
};""")

rep("#define WM_APP_REBUILD_DEVICE (WM_APP + 8)   // wParam: 1 = device was lost\n",
    """#define WM_APP_REBUILD_DEVICE (WM_APP + 8)   // wParam: 1 = device was lost
#define WM_APP_CONTEXT_MENU (WM_APP + 9)     // wParam, lParam: screen x, y
""")

# ================================================================ globals: hidden / menu state for the mouse hook
rep("bool g_autoHideBlanked = false;\n", """bool g_autoHideBlanked = false;
// For other threads (the right-click hook): true while Auto-Hide has the scene
// at zero alpha, so nothing is visible inside the still-valid draw rect. Set by
// the render thread every frame, from the same sceneAlpha the renderers use.
std::atomic<bool> g_vizSceneHidden{false};
// True while the right-click menu's TrackPopupMenuEx loop runs (message-window
// thread); the hook passes every click through while it is set.
std::atomic<bool> g_menuOpen{false};
""")
rep("""    // Direct3D 11 renderer (2.0). Falls through to the Direct2D path below if
""", """    g_vizSceneHidden.store(sceneAlpha <= 0.001f, std::memory_order_relaxed);

    // Direct3D 11 renderer (2.0). Falls through to the Direct2D path below if
""")

# ================================================================ globals: now playing parts, timeline, brushes
rep("""std::wstring g_nowPlayingDisplay;
""", """std::wstring g_nowPlayingDisplay;
std::wstring g_nowPlayingTitle, g_nowPlayingArtist;  // the parts, for the two-line layout

// ---- track timeline: begin (src/tests_features/test_timeline.cpp compiles this block from v2b.cpp)
// Track timeline from the media session, for the progress bar: start, end and
// position in 100 ns units, and the tick at which the position was current.
// Between updates the bar extrapolates while playing, since most players only
// report the position on a seek or a state change.
//
// Writers are WinRT thread-pool callbacks (two can run at once), serialised by
// g_tlWriteMutex; each write is tagged with the session generation it was read
// for, so a late event from a session that has since been replaced is dropped.
// The render thread reads without locking, through the g_tlSeq sequence count
// (odd while a write is in progress), so it never sees a position paired with
// another update's tick.
std::atomic<bool> g_tlValid{false};
std::atomic<int64_t> g_tlStart{0}, g_tlEnd{0}, g_tlPos{0};
std::atomic<ULONGLONG> g_tlTick{0};
std::atomic<bool> g_tlRunning{false};  // extrapolating: the session reported Playing
std::atomic<uint32_t> g_tlSeq{0};
std::atomic<uint32_t> g_tlGen{0};
std::mutex g_tlWriteMutex;

static void VizTlWriteBegin() {
    g_tlSeq.store(g_tlSeq.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
}
static void VizTlWriteEnd() {
    g_tlSeq.store(g_tlSeq.load(std::memory_order_relaxed) + 1, std::memory_order_release);
}

// A new media session is being hooked up: forget the old timeline and return
// the generation the new session's events must carry.
uint32_t VizTimelineNewSource() {
    std::lock_guard<std::mutex> lock(g_tlWriteMutex);
    uint32_t gen = g_tlGen.load(std::memory_order_relaxed) + 1;
    VizTlWriteBegin();
    g_tlGen.store(gen, std::memory_order_relaxed);
    g_tlValid.store(false, std::memory_order_relaxed);
    g_tlRunning.store(false, std::memory_order_relaxed);
    g_tlPos.store(0, std::memory_order_relaxed);
    g_tlTick.store(GetTickCount64(), std::memory_order_relaxed);
    VizTlWriteEnd();
    return gen;
}

// Playback status. On a Playing <-> Paused change the position so far is
// folded in and the clock restarts from now, so the bar neither jumps back
// when pausing nor forward by the length of the pause when resuming (players
// often leave the timeline itself untouched across a pause).
void VizTimelineSetPlaying(uint32_t gen, bool playing) {
    std::lock_guard<std::mutex> lock(g_tlWriteMutex);
    if (gen != g_tlGen.load(std::memory_order_relaxed)) return;
    g_mediaIsPlaying.store(playing, std::memory_order_relaxed);
    bool running = g_tlRunning.load(std::memory_order_relaxed);
    if (running == playing) return;
    ULONGLONG now = GetTickCount64();
    int64_t pos = g_tlPos.load(std::memory_order_relaxed);
    if (running) pos += (int64_t)(now - g_tlTick.load(std::memory_order_relaxed)) * 10000;
    VizTlWriteBegin();
    g_tlPos.store(pos, std::memory_order_relaxed);
    g_tlTick.store(now, std::memory_order_relaxed);
    g_tlRunning.store(playing, std::memory_order_relaxed);
    VizTlWriteEnd();
}

// A fresh timeline (TimelinePropertiesChanged, or the first read of a
// session). ageMs is how old the position already is (from LastUpdatedTime);
// it only counts while playing, since a paused position doesn't move.
void VizTimelineSet(uint32_t gen, int64_t start, int64_t end, int64_t pos, int64_t ageMs) {
    std::lock_guard<std::mutex> lock(g_tlWriteMutex);
    if (gen != g_tlGen.load(std::memory_order_relaxed)) return;
    if (ageMs < 0 || ageMs > 6LL * 3600 * 1000) ageMs = 0;  // unset or nonsense
    if (!g_tlRunning.load(std::memory_order_relaxed)) ageMs = 0;
    VizTlWriteBegin();
    g_tlStart.store(start, std::memory_order_relaxed);
    g_tlEnd.store(end, std::memory_order_relaxed);
    g_tlPos.store(pos, std::memory_order_relaxed);
    g_tlTick.store(GetTickCount64() - (ULONGLONG)ageMs, std::memory_order_relaxed);
    g_tlValid.store(end > start, std::memory_order_relaxed);
    VizTlWriteEnd();
}

void VizTimelineInvalidate(uint32_t gen) {
    std::lock_guard<std::mutex> lock(g_tlWriteMutex);
    if (gen != g_tlGen.load(std::memory_order_relaxed)) return;
    VizTlWriteBegin();
    g_tlValid.store(false, std::memory_order_relaxed);
    VizTlWriteEnd();
}

// 0..1 along the track, or -1 for no bar. Render thread.
float VizTrackProgress() {
    static std::atomic<float> s_last{-1.f};  // if a writer is preempted mid-update
    bool valid = false, running = false;
    int64_t start = 0, end = 0, pos = 0;
    ULONGLONG tick = 0;
    for (int tries = 0;; tries++) {
        if (tries == 64) return s_last.load(std::memory_order_relaxed);
        uint32_t s1 = g_tlSeq.load(std::memory_order_acquire);
        if (s1 & 1u) continue;
        valid = g_tlValid.load(std::memory_order_relaxed);
        start = g_tlStart.load(std::memory_order_relaxed);
        end = g_tlEnd.load(std::memory_order_relaxed);
        pos = g_tlPos.load(std::memory_order_relaxed);
        tick = g_tlTick.load(std::memory_order_relaxed);
        running = g_tlRunning.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (g_tlSeq.load(std::memory_order_relaxed) == s1) break;
    }
    float result = -1.f;
    double dur = (double)(end - start);
    if (valid && dur > 0.0) {
        double p = (double)(pos - start);
        if (running) {
            // Modular difference, so a tick set back past boot by an age still works.
            int64_t elapsedMs = (int64_t)(GetTickCount64() - tick);
            if (elapsedMs > 0) p += (double)elapsedMs * 10000.0;
        }
        result = (float)std::clamp(p / dur, 0.0, 1.0);
    }
    s_last.store(result, std::memory_order_relaxed);
    return result;
}
// ---- track timeline: end
""")
rep("ComPtr<ID2D1SolidColorBrush> g_nowPlayingBrush;\n",
    """ComPtr<ID2D1SolidColorBrush> g_nowPlayingBrush;
ComPtr<ID2D1SolidColorBrush> g_npArtistBrush;
ComPtr<ID2D1SolidColorBrush> g_progressBrush;
""")
rep("""    g_nowPlayingBrush.Reset();
    g_textPanelBrush.Reset();
""", """    g_nowPlayingBrush.Reset();
    g_npArtistBrush.Reset();
    g_progressBrush.Reset();
    g_textPanelBrush.Reset();
""")
rep("""        g_dc->CreateSolidColorBrush(npColor, &g_nowPlayingBrush);
""", """        g_dc->CreateSolidColorBrush(npColor, &g_nowPlayingBrush);
        g_dc->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &g_npArtistBrush);
""")
rep("""    if (g_settings.nowPlayingEnabled || g_settings.peakFreqEnabled) {
        // One scratch brush shared by both text panels and both their borders.""",
    """    if (g_settings.progressEnabled) g_dc->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &g_progressBrush);

    if (g_settings.nowPlayingEnabled || g_settings.peakFreqEnabled) {
        // One scratch brush shared by both text panels and both their borders.""")

# ================================================================ GSMTC: title / artist parts, timeline
rep("""                if (!display.empty()) {
                    std::lock_guard<std::mutex> lock(g_nowPlayingMutex);
                    if (g_nowPlayingDisplay != display) {
                        g_nowPlayingDisplay = display;
                        g_nowPlayingChangedTick.store(GetTickCount64(), std::memory_order_relaxed);
                    }
                }""", """                if (!display.empty()) {
                    std::lock_guard<std::mutex> lock(g_nowPlayingMutex);
                    g_nowPlayingTitle = title;
                    g_nowPlayingArtist = artist;
                    if (g_nowPlayingDisplay != display) {
                        g_nowPlayingDisplay = display;
                        g_nowPlayingChangedTick.store(GetTickCount64(), std::memory_order_relaxed);
                    }
                }""")
rep("static winrt::event_token g_gsmtcPlaybackToken{};\n",
    "static winrt::event_token g_gsmtcPlaybackToken{};\nstatic winrt::event_token g_gsmtcTimelineToken{};\n")
# The session handlers run on WinRT thread-pool threads while
# CurrentSessionChanged (another pool thread) swaps g_gsmtcSession, so the
# handlers never read the global: each works on its `sender` and carries the
# generation it was hooked up for (see VizTimelineNewSource).
rep("""void RefreshMediaPlaybackStatus() {
    if (!g_gsmtcSession) return;
    try {
        auto info = g_gsmtcSession.GetPlaybackInfo();
        bool playing = info && info.PlaybackStatus() ==
            GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        g_mediaIsPlaying.store(playing, std::memory_order_relaxed);
    } catch (...) {}
    if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}

void SetupGsmtcSessionListener() {
    if (!g_gsmtcMgr) return;
    try {
        if (g_gsmtcSession) {
            try { g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken); } catch (...) {}
            try { g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken); } catch (...) {}
            g_gsmtcSession = nullptr;
        }
        g_gsmtcSession = g_gsmtcMgr.GetCurrentSession();
        if (!g_gsmtcSession) return;
        g_gsmtcMediaPropsToken = g_gsmtcSession.MediaPropertiesChanged(
            [](auto const&, auto const&) {
                if (g_settings.colorMode == VizColorMode::AlbumArt ||
                    g_settings.colorMode == VizColorMode::DynamicAlbum ||
                    g_settings.nowPlayingEnabled)
                    FetchAlbumArtColorAsync();
            });
        g_gsmtcPlaybackToken = g_gsmtcSession.PlaybackInfoChanged(
            [](auto const&, auto const&) { RefreshMediaPlaybackStatus(); });
        RefreshMediaPlaybackStatus();
    } catch (...) {}
}
""", """// g_gsmtcSession, g_gsmtcMgr and the session's event tokens are only touched
// under this lock: by SetupGsmtcSessionListener (the GSMTC thread at start, a
// WinRT thread-pool thread on every CurrentSessionChanged) and by shutdown.
// The session's own event handlers never read them; they get the session as
// `sender` and the generation it was hooked up with, and a handler still
// running for a replaced session has its result dropped by the timeline.
static std::mutex g_gsmtcSessionMutex;

void RefreshMediaPlaybackStatus(GlobalSystemMediaTransportControlsSession const& session, uint32_t gen) {
    if (!session) return;
    try {
        auto info = session.GetPlaybackInfo();
        bool playing = info && info.PlaybackStatus() ==
            GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        VizTimelineSetPlaying(gen, playing);  // also sets g_mediaIsPlaying
    } catch (...) {}
    if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}

// Reads the session's timeline for the progress bar. LastUpdatedTime says how
// old the position already is; a player that leaves it unset gets age 0.
void RefreshMediaTimeline(GlobalSystemMediaTransportControlsSession const& session, uint32_t gen) {
    if (!session) return;
    try {
        auto tl = session.GetTimelineProperties();
        if (!tl) {
            VizTimelineInvalidate(gen);
            return;
        }
        int64_t start = tl.StartTime().count(), end = tl.EndTime().count(), pos = tl.Position().count();
        int64_t ageMs = std::chrono::duration_cast<std::chrono::milliseconds>(winrt::clock::now() -
                                                                              tl.LastUpdatedTime())
                            .count();
        VizTimelineSet(gen, start, end, pos, ageMs);
    } catch (...) {
        VizTimelineInvalidate(gen);
    }
}

void SetupGsmtcSessionListener() {
    std::lock_guard<std::mutex> lock(g_gsmtcSessionMutex);
    if (!g_gsmtcMgr) return;
    try {
        if (g_gsmtcSession) {
            try { g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken); } catch (...) {}
            try { g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken); } catch (...) {}
            try { g_gsmtcSession.TimelinePropertiesChanged(g_gsmtcTimelineToken); } catch (...) {}
            g_gsmtcSession = nullptr;
        }
        // From here on, events from the old session are ignored.
        const uint32_t gen = VizTimelineNewSource();
        GlobalSystemMediaTransportControlsSession session = g_gsmtcMgr.GetCurrentSession();
        g_gsmtcSession = session;
        if (!session) return;
        g_gsmtcMediaPropsToken = session.MediaPropertiesChanged(
            [](auto const&, auto const&) {
                if (g_settings.colorMode == VizColorMode::AlbumArt ||
                    g_settings.colorMode == VizColorMode::DynamicAlbum ||
                    g_settings.nowPlayingEnabled)
                    FetchAlbumArtColorAsync();
            });
        // Pausing / resuming folds the position in (VizTimelineSetPlaying);
        // the timeline itself is only re-read when the player changes it.
        g_gsmtcPlaybackToken = session.PlaybackInfoChanged(
            [gen](GlobalSystemMediaTransportControlsSession const& sender, auto const&) {
                RefreshMediaPlaybackStatus(sender, gen);
            });
        g_gsmtcTimelineToken = session.TimelinePropertiesChanged(
            [gen](GlobalSystemMediaTransportControlsSession const& sender, auto const&) {
                RefreshMediaTimeline(sender, gen);
            });
        // Status first, so the first timeline read knows whether its age counts.
        RefreshMediaPlaybackStatus(session, gen);
        RefreshMediaTimeline(session, gen);
    } catch (...) {}
}
""")
# Shutdown: stop session swaps first (outside the lock, so an in-flight
# CurrentSessionChanged can finish), then unhook the session under the lock.
rep("""        try {
            if (g_gsmtcSession) {
                g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken);
                g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken);
            }
            if (g_gsmtcMgr) {
                g_gsmtcMgr.CurrentSessionChanged(g_gsmtcSessionToken);
            }
            g_gsmtcSession = nullptr;
            g_gsmtcMgr     = nullptr;
            winrt::uninit_apartment();
        } catch (...) {}""", """        try {
            if (g_gsmtcMgr) g_gsmtcMgr.CurrentSessionChanged(g_gsmtcSessionToken);
        } catch (...) {}
        {
            std::lock_guard<std::mutex> lock(g_gsmtcSessionMutex);
            if (g_gsmtcSession) {
                try { g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken); } catch (...) {}
                try { g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken); } catch (...) {}
                try { g_gsmtcSession.TimelinePropertiesChanged(g_gsmtcTimelineToken); } catch (...) {}
            }
            g_gsmtcSession = nullptr;
            g_gsmtcMgr     = nullptr;
        }
        try { winrt::uninit_apartment(); } catch (...) {}""")
# ================================================================ media strip: anchoring, right-click
rep("""    int x = mi.rcWork.left + (int)std::lround((workWidth - width) * (hPercent / 100.0f));
    int y = mi.rcWork.top + (int)std::lround((workHeight - height) * (vPercent / 100.0f));
""", """    int x = mi.rcWork.left + (int)std::lround((workWidth - width) * (hPercent / 100.0f));
    int y = mi.rcWork.top + (int)std::lround((workHeight - height) * (vPercent / 100.0f));

    // Anchored to the visualizer's panel (2.0): a corner of the panel as last
    // drawn, inset by the anchor offsets, replacing the position above. The
    // panel publishes its rect every frame and re-posts this when it moves.
    if (g_settings.mediaAnchor != VizMediaAnchor::Screen && g_drawRectValid.load(std::memory_order_relaxed)) {
        LONG pl = g_drawRectL.load(std::memory_order_relaxed), pt = g_drawRectT.load(std::memory_order_relaxed);
        LONG pr = g_drawRectR.load(std::memory_order_relaxed), pb = g_drawRectB.load(std::memory_order_relaxed);
        int ox = (int)std::lround(g_settings.mediaAnchorOffsetX * dpiScale);
        int oy = (int)std::lround(g_settings.mediaAnchorOffsetY * dpiScale);
        bool right = g_settings.mediaAnchor == VizMediaAnchor::PanelTopRight ||
                     g_settings.mediaAnchor == VizMediaAnchor::PanelBottomRight;
        bool bottom = g_settings.mediaAnchor == VizMediaAnchor::PanelBottomLeft ||
                      g_settings.mediaAnchor == VizMediaAnchor::PanelBottomRight;
        x = right ? (int)pr - ox - width : (int)pl + ox;
        y = bottom ? (int)pb - oy - height : (int)pt + oy;
    }
""")
rep("""        case WM_APP_MEDIA_REPAINT:
            RepositionAndRepaintMediaControls();
            return 0;
""", """        case WM_APP_MEDIA_REPAINT:
            RepositionAndRepaintMediaControls();
            return 0;

        // Right-click on the strip opens the same quick-settings menu as the
        // visualizer (handy when the visualizer is paused from it), with the
        // same Ctrl requirement when Right-Click Menu is Ctrl + Right-Click.
        case WM_RBUTTONUP:
            if (g_settings.contextMenu == VizContextMenu::CtrlRightClick && !(wParam & MK_CONTROL)) break;
            if (g_messageWnd && g_settings.contextMenu != VizContextMenu::Off) {
                POINT pt;
                GetCursorPos(&pt);
                PostMessage(g_messageWnd, WM_APP_CONTEXT_MENU, (WPARAM)pt.x, (LPARAM)pt.y);
            }
            return 0;
""")

# ================================================================ audio source (device helpers before the notification client)
before("class VizEndpointNotificationClient : public IMMNotificationClient {", read("p2_perf.cpp") + "\n" + read("p2_device.cpp") + "\n")
rep("""        if (flow == eRender) g_deviceChanged.store(true, std::memory_order_relaxed);""",
    """        if ((flow == eRender || flow == eCapture) && VizSourceFollowsDefault(flow))
            g_deviceChanged.store(true, std::memory_order_relaxed);""")
rep("""                    ReportSettingWarning(L"Hardware", L"Audio Analysis Device", *msg);""",
    """                    if (wParam == 1) ReportSettingWarning(L"Audio", L"Audio Source", *msg);
                    else ReportSettingWarning(L"Hardware", L"Audio Analysis Device", *msg);""")

# ================================================================ classic targets: Terminal draws like Stereo
rep("""        switch (g_settings.shape) {
            case VizShape::Stereo:
                target = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));""",
    """        switch (g_settings.shape) {
            case VizShape::Stereo:
            case VizShape::Terminal:
                target = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));""")

# ================================================================ Terminal shape code, then layout sizing
before("""// Hit-tests against the bounds of the last drawn frame (already tracked for
// the occlusion check)""", read("p2_term.cpp") + "\n")
rep("""    if (g_settings.shape == VizShape::Radial || g_settings.shape == VizShape::Goniometer) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {""", """    if (g_settings.shape == VizShape::Terminal) {
        VizTermBox(&totalWidth, &totalHeight);
    } else if (g_settings.shape == VizShape::Radial || g_settings.shape == VizShape::Goniometer) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {""", count=3)

# Layout room for the Now Playing layouts and the progress bar.
rep("""    if (g_settings.nowPlayingEnabled) {
        float npOffX = reserveFor(std::abs(EffectiveNowPlayingOffsetX()) + npPanel);
        float npOffY = reserveFor(std::abs(EffectiveNowPlayingOffsetY()) + npPanel);
        textTop    = std::max(textTop, fontPx * 1.6f + 8.0f * g_dpiScale + npOffY);
        textBottom = std::max(textBottom, npOffY);
        textAnchorSide = std::max(textAnchorSide, 100.0f * g_dpiScale);
        extraSide  = std::max(extraSide, npOffX);
    }""", """    if (g_settings.nowPlayingEnabled) {
        float npOffX = reserveFor(std::abs(EffectiveNowPlayingOffsetX()) + npPanel);
        float npOffY = reserveFor(std::abs(EffectiveNowPlayingOffsetY()) + npPanel);
        if (g_settings.npPlacement == VizNpPlacement::Above) {
            float lines = (g_settings.npLayout == VizNpLayout::TwoLines) ? 2.7f : 1.6f;
            textTop = std::max(textTop, fontPx * lines + 8.0f * g_dpiScale + npOffY);
            textAnchorSide = std::max(textAnchorSide, 100.0f * g_dpiScale);
        } else {
            // Inside the panel, in the padding band (VizDrawTextOverlays): Panel
            // Bottom starts at the bars' bottom edge and Panel Top at the panel's
            // top edge, both npHeight tall. Padding smaller than the text (or
            // the background off, padding 0) lets it hang past the panel's
            // bottom: reserve that overflow, plus the offset room as before.
            float npHeight = fontPx * ((g_settings.npLayout == VizNpLayout::TwoLines) ? 2.7f : 1.6f);
            float overflow = (g_settings.npPlacement == VizNpPlacement::PanelBottom)
                                 ? npHeight - padB
                                 : npHeight - padT - totalHeight - padB;
            textTop = std::max(textTop, npOffY);
            textBottom = std::max(textBottom, std::max(0.f, overflow) + npOffY);
        }
        textBottom = std::max(textBottom, npOffY);
        extraSide  = std::max(extraSide, npOffX);
    }
    if (g_settings.progressEnabled) {
        float need = (float)(std::max(1, g_settings.progressHeight) + g_settings.progressGap) * g_dpiScale +
                     2.0f * g_dpiScale;
        if (g_settings.progressPlacement == VizProgressPlacement::Above) textTop += need;
        else if (g_settings.progressPlacement == VizProgressPlacement::Below) textBottom = std::max(textBottom, need);
        // Panel Bottom: drawn at the bars' bottom edge + gap (VizProgressRect),
        // so whatever of gap + height the bottom padding doesn't cover.
        else textBottom = std::max(textBottom, std::max(0.f, need - padB));
    }""")

# ================================================================ Direct2D path: Terminal
rep("""    if (!g_dragRenderPauseActive.load(std::memory_order_relaxed)) VizComputeBarFrame();
""", """    if (!g_dragRenderPauseActive.load(std::memory_order_relaxed)) {
        VizComputeBarFrame();
        if (g_settings.shape == VizShape::Terminal) VizBuildTermGrid();
    }
""")
rep("""        else {
            // Smooth Mode: bars go out as one sprite batch (see""", """        else if (g_settings.shape == VizShape::Terminal) {
            // Direct2D fallback: the grid as runs of text. The Direct3D 11
            // renderer draws it from a glyph atlas in one call instead.
            VizDrawTermGridD2D(floorf(blockX + 0.5f), floorf(blockY + 0.5f));
        }
        else {
            // Smooth Mode: bars go out as one sprite batch (see""")

# ================================================================ right-click hook
rep("""LRESULT CALLBACK DragMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && !g_unloading.load(std::memory_order_relaxed) &&
        g_settings.dragEnabled) {""", """// True when what is under `pt` is the desktop the visualizer lives on (its
// WorkerW / Progman layer), not an application window covering it. Window
// lookup and class names only, no messages: this runs inside a low-level
// mouse hook, where anything slow holds up all input.
bool VizDesktopUnderPoint(POINT pt) {
    HWND h = WindowFromPoint(pt);
    if (!h) return false;
    HWND root = GetAncestor(h, GA_ROOT);
    if (!root) root = h;
    WCHAR cls[32] = {};
    GetClassNameW(root, cls, ARRAYSIZE(cls));
    return wcscmp(cls, L"WorkerW") == 0 || wcscmp(cls, L"Progman") == 0;
}

std::atomic<bool> g_userPaused{false};  // Pause Visualizer, from the right-click menu
bool g_menuButtonDown = false;          // hook thread only

// Right-click over the visualizer opens the quick-settings menu (Interaction,
// Right-Click Menu). The press and release are both swallowed, so the
// desktop's own menu doesn't open as well; a drag bound to the right button
// with its modifier held keeps priority.
bool VizMenuHook(WPARAM wParam, const MSLLHOOKSTRUCT* info) {
    if (g_settings.contextMenu == VizContextMenu::Off) return false;
    // Our menu is up: everything goes through, so a click elsewhere (either
    // button) dismisses it the normal way. Only the release of a press this
    // hook already swallowed is swallowed too, so the desktop never gets an
    // unpaired button-up.
    if (g_menuOpen.load(std::memory_order_acquire)) {
        if (wParam == WM_RBUTTONUP && g_menuButtonDown) {
            g_menuButtonDown = false;
            return true;
        }
        return false;
    }
    if (wParam == WM_RBUTTONDOWN) {
        g_menuButtonDown = false;
        if (g_settings.dragEnabled && g_settings.dragButton == VizDragButton::Right && DragModifierHeld())
            return false;
        if (g_settings.contextMenu == VizContextMenu::CtrlRightClick && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            return false;
        // Hidden for a fullscreen app or a covering window: nothing to click.
        if (g_fullscreenPaused.load(std::memory_order_relaxed) && !g_userPaused.load(std::memory_order_relaxed))
            return false;
        // Faded out by Auto-Hide: the draw rect is still valid, but there is
        // nothing visible there, so the click belongs to the desktop. (Pause
        // Visualizer still catches it: that is how it gets unticked.)
        if (g_vizSceneHidden.load(std::memory_order_relaxed) && !g_userPaused.load(std::memory_order_relaxed))
            return false;
        if (!PointInVisualizerBounds(info->pt) || !VizDesktopUnderPoint(info->pt)) return false;
        g_menuButtonDown = true;
        return true;
    }
    if (wParam == WM_RBUTTONUP && g_menuButtonDown) {
        g_menuButtonDown = false;
        if (g_messageWnd) PostMessage(g_messageWnd, WM_APP_CONTEXT_MENU, (WPARAM)info->pt.x, (LPARAM)info->pt.y);
        return true;
    }
    return false;
}

LRESULT CALLBACK DragMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && !g_unloading.load(std::memory_order_relaxed) &&
        VizMenuHook(wParam, (const MSLLHOOKSTRUCT*)lParam))
        return 1;
    if (nCode == HC_ACTION && !g_unloading.load(std::memory_order_relaxed) &&
        g_settings.dragEnabled) {""")

# ================================================================ menu, message handling, pause
menu = read("p2_menu.cpp").replace("std::atomic<bool> g_userPaused{false};\n", "")
before("LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {", menu + "\n")
rep("""        case WM_APP_SETTINGS_CHANGED:
            ApplySettingsChanged();
            return 0;

        case WM_DESTROY:
            g_messageWnd = nullptr;""", """        case WM_APP_SETTINGS_CHANGED:
            ApplySettingsChanged();
            return 0;

        case WM_APP_CONTEXT_MENU:
            if (!g_unloading) VizShowContextMenu(POINT{(LONG)(int)wParam, (LONG)(int)lParam});
            return 0;

        case WM_DESTROY:
            g_messageWnd = nullptr;""")
rep("""                bool shouldPause = false;
""", """                bool shouldPause = g_userPaused.load(std::memory_order_relaxed);
""")

# ================================================================ LoadSettings
rep("""void LoadSettings() {
    g_settingsIssues.clear();""", """void LoadSettings() {
    g_settingsIssues.clear();
    VizLoadMenuOverrides();""")
rep("""                       : (wcscmp(shape, L"oscilloscope") == 0) ? VizShape::Oscilloscope
                                                               : VizShape::Stereo;""",
    """                       : (wcscmp(shape, L"oscilloscope") == 0) ? VizShape::Oscilloscope
                       : (wcscmp(shape, L"terminal") == 0)   ? VizShape::Terminal
                                                               : VizShape::Stereo;""")
rep("""        // The IEC and musical layouts decide the bar count. Worked out here at""",
    read("part_loadsettings2.cpp") + """
        // Quick settings from the right-click menu sit on top of all of the
        // above (and are cleared from the same menu).
        VizApplyMenuOverrides();
        if (g_settings.workload == VizWorkload::Gpu && g_settings.shape == VizShape::Terminal) {
            ReportSettingWarning(L"Hardware", L"Workload",
                                 L"The Terminal shape is built on the CPU from the bar levels, so with it the "
                                 L"analysis runs on the CPU (Hybrid).");
        }

        // The IEC and musical layouts decide the bar count. Worked out here at""")

# ================================================================ YAML
rep("""        - goniometer: Goniometer (stereo field)
    - orientation: horizontal""", """        - goniometer: Goniometer (stereo field)
        - terminal: Terminal (text characters, see the Terminal section)
    - orientation: horizontal""")
rep("""      $name: Now Playing Display Seconds
      $description: How long the text stays visible after a track changes, before fading out""",
    """      $name: Now Playing Display Seconds
      $description: How long the text stays visible after a track changes, before fading out. 0 keeps it on screen all the time, like a media widget""")
rep("""      $description: 'Color of the readout panel''s outline. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
  $name: Appearance""", """      $description: 'Color of the readout panel''s outline. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
""" + read("yaml_appearance2.txt") + """  $name: Appearance
""" + read("yaml_groups2.txt").rstrip("\n"))
rep("""        - right: Right Click
  $name: Interaction""", """        - right: Right Click
    - contextMenu: right_click
      $name: Right-Click Menu
      $description: Right-click the visualizer for a menu of quick settings (shape, colours, analysis, audio source, overlays, pause). It only opens where the visualizer is actually showing on the desktop, never through a window covering it. Ctrl + Right-Click leaves a plain right-click to the desktop
      $options:
        - right_click: Right-Click
        - ctrl_right_click: Ctrl + Right-Click
        - 'off': 'Off'
  $name: Interaction""")
rep("""      $description: 0-100, percentage down the monitor's work area. Decimals allowed. Same caveat as Horizontal Position -- a saved keyboard position takes priority over this field until it is cleared
  $name: Media Controls""", """      $description: 0-100, percentage down the monitor's work area. Decimals allowed. Same caveat as Horizontal Position -- a saved keyboard position takes priority over this field until it is cleared
    - anchor: screen
      $name: Anchor
      $description: Screen places the strip with the two Position percentages above. A panel corner pins it inside that corner of the visualizer's background panel instead, so it moves with the visualizer (drags included) and the Position settings are ignored. Give the panel enough padding on that side to hold it
      $options:
        - screen: Screen (Position settings)
        - panel_top_left: Panel, top left
        - panel_top_right: Panel, top right
        - panel_bottom_left: Panel, bottom left
        - panel_bottom_right: Panel, bottom right
    - anchorOffsetX: 8
      $name: Anchor Inset X
      $description: Distance in from the panel's left or right edge, in pixels. Only used with a panel anchor
    - anchorOffsetY: 8
      $name: Anchor Inset Y
      $description: Distance in from the panel's top or bottom edge, in pixels. Only used with a panel anchor
  $name: Media Controls""")

# ================================================================ readme
before("# ✦ EVERY SETTING, EXPLAINED\n", read("readme_widget.md"))
rep("""**Target FPS 0** matches your display's refresh rate, whatever it is.
""", """**Target FPS 0** matches your display's refresh rate, whatever it is.

**Right-click for quick settings.** Shape, colours, analysis, readout, overlays and **which audio device it listens to**, any output, input or virtual device (VB-Audio, Voicemeeter, an Ableton return), all live, without opening Windhawk. Plus a **Terminal** shape (text characters: columns, a waterfall or text meters) and a **media widget** layout: title and artist inside the panel, pixel-sharp text, a track progress bar and media controls pinned to the panel.
""")

# ================================================================ pixel snap / subpixel placement
# Why it went soft at some sizes: every size was DPI-scaled and every position
# was a percentage of the work area, both kept as exact fractions. At 125% a
# 2 px bar is 2.5 px; a Position of 50% lands the block on x.37; bars at a
# fractional x get an antialiased edge on both sides. Pixel Snap rounds the
# block origin and every scaled size to whole pixels (bar heights stay
# fractional, which is what keeps motion smooth). Off keeps the exact
# fractions, for subpixel placement.
rep("float g_dpiScale = 1.0f;\n", """float g_dpiScale = 1.0f;

// A size in settings pixels, DPI-scaled, and with Pixel Snap on rounded to a
// whole device pixel so edges land on the pixel grid.
inline float VizPx(float v) {
    float p = v * g_dpiScale;
    return g_settings.pixelSnap ? roundf(p) : p;
}
""")
rep("(float)std::max(1, g_settings.barWidth) * g_dpiScale", "std::max(1.f, VizPx((float)std::max(1, g_settings.barWidth)))", count=4)
rep("(float)std::max(0, g_settings.barGap) * g_dpiScale", "VizPx((float)std::max(0, g_settings.barGap))", count=4)
rep("(float)std::max(2, g_settings.barMaxSize) * g_dpiScale", "std::max(2.f, VizPx((float)std::max(2, g_settings.barMaxSize)))", count=4)
rep("(float)std::max(0, g_settings.barIdleSize) * g_dpiScale", "VizPx((float)std::max(0, g_settings.barIdleSize))", count=2)
for side in "LRTB":
    rep(f"(float)g_settings.bgPadding{side} * g_dpiScale", f"VizPx((float)g_settings.bgPadding{side})",
        count=5 if side in "TB" else 4)
rep("""    float blockX = waLeft + (workWidth  - totalWidth)  * (hPercent / 100.0f);
    float blockY = waTop  + (workHeight - totalHeight) * (vPercent / 100.0f);
""", """    float blockX = waLeft + (workWidth  - totalWidth)  * (hPercent / 100.0f);
    float blockY = waTop  + (workHeight - totalHeight) * (vPercent / 100.0f);
    // Pixel Snap: the block on a whole pixel. With the sizes above already
    // whole, every bar edge and the panel then sit exactly on the grid.
    if (g_settings.pixelSnap) {
        blockX = roundf(blockX);
        blockY = roundf(blockY);
    }
""")
rep("""    int keyMoveFastStep = 10;
    unsigned keyMoveFastKey = VIZ_MOD_SHIFT;
""", """    int keyMoveFastStep = 10;
    unsigned keyMoveFastKey = VIZ_MOD_SHIFT;
    float keyMoveFineStep = 0.25f;           // px, for subpixel nudges
    unsigned keyMoveFineKey = VIZ_MOD_NONE;  // held for a fine step
    bool keyMoveFine = false;                // every nudge fine (right-click menu)
    bool pixelSnap = true;
""")
rep("""    Wh_FreeStringSetting(keyMoveFastKey);
""", """    Wh_FreeStringSetting(keyMoveFastKey);
    {
        PCWSTR v = Wh_GetStringSetting(L"interaction.keyMoveFineKey");
        g_settings.keyMoveFineKey = (wcscmp(v, L"shift") == 0) ? (unsigned)VIZ_MOD_SHIFT
                                  : (wcscmp(v, L"ctrl") == 0)  ? (unsigned)VIZ_MOD_CTRL
                                  : (wcscmp(v, L"alt") == 0)   ? (unsigned)VIZ_MOD_ALT
                                  : (wcscmp(v, L"win") == 0)   ? (unsigned)VIZ_MOD_WIN
                                                               : (unsigned)VIZ_MOD_NONE;
        Wh_FreeStringSetting(v);
        v = Wh_GetStringSetting(L"interaction.keyMoveFineStep");
        float f = v ? (float)_wtof(v) : 0.f;
        g_settings.keyMoveFineStep = (f > 0.f) ? std::clamp(f, 1.0f / 64.0f, 1.0f) : 0.25f;
        Wh_FreeStringSetting(v);
        g_settings.keyMoveFine = false;  // a quick setting only
        g_settings.pixelSnap = Wh_GetIntSetting(L"position.pixelSnap") != 0;
    }
""")
rep("""                    int step = fast ? g_settings.keyMoveFastStep : g_settings.keyMoveStep;
                    if (g_keyMoveTarget == VizMoveTarget::Visualizer) {
                        NudgeVisualizerPx(dx * step, dy * step);""", """                    int step = fast ? g_settings.keyMoveFastStep : g_settings.keyMoveStep;
                    // Fine: a fraction of a pixel. Only the visualizer can sit
                    // between pixels; the strip and the text move whole pixels.
                    bool fine = !fast && (g_settings.keyMoveFine ||
                                          (g_settings.keyMoveFineKey != VIZ_MOD_NONE &&
                                           ModKeysHeld(g_settings.keyMoveFineKey)));
                    float vstep = fine ? g_settings.keyMoveFineStep : (float)step;
                    if (g_keyMoveTarget == VizMoveTarget::Visualizer) {
                        NudgeVisualizerPx(dx * vstep, dy * vstep);""")
rep("void NudgeVisualizerPx(int dxPx, int dyPx) {", "void NudgeVisualizerPx(float dxPx, float dyPx) {")
rep("""    if (dxPx && travelX > 1.0f) h = std::clamp(h + (dxPx / travelX) * 100.0f, 0.0f, 100.0f);
    if (dyPx && travelY > 1.0f) v = std::clamp(v + (dyPx / travelY) * 100.0f, 0.0f, 100.0f);

    g_dragOverrideH.store(h, std::memory_order_relaxed);""", """    if (dxPx != 0.f && travelX > 1.0f) h = std::clamp(h + (dxPx / travelX) * 100.0f, 0.0f, 100.0f);
    if (dyPx != 0.f && travelY > 1.0f) v = std::clamp(v + (dyPx / travelY) * 100.0f, 0.0f, 100.0f);

    g_dragOverrideH.store(h, std::memory_order_relaxed);""")
rep("""    - keyMoveFastKey: shift""", """    - keyMoveFineStep: '0.25'
      $name: Keyboard Move Fine Step
      $description: A subpixel step for the visualizer, used while the Fine Key is held or with Subpixel Nudges ticked in the right-click menu. Only shows as movement with Pixel Snap off (Position); with it on, the position still accumulates and the picture moves a whole pixel at a time
      $options:
        - '0.5': 1/2 pixel
        - '0.25': 1/4 pixel
        - '0.125': 1/8 pixel
        - '0.0625': 1/16 pixel
    - keyMoveFineKey: none
      $name: Keyboard Move Fine Key
      $description: Held alongside the modifier for the fine step. Must not be one of the modifier's keys or the Fast Key. None leaves fine steps to the right-click menu's Subpixel Nudges
      $options:
        - none: None
        - shift: Shift
        - ctrl: Ctrl
        - alt: Alt
        - win: Win
    - keyMoveFastKey: shift""")
rep("""    - monitor: 1
      $name: Monitor
      $description: 1-based monitor index
  $name: Position""", """    - monitor: 1
      $name: Monitor
      $description: 1-based monitor index
    - pixelSnap: true
      $name: Pixel Snap
      $description: Puts the panel, every bar, the text and the grid on whole pixels, so edges stay razor sharp at any size and any display scaling (at 125% or 150%, sizes otherwise land on half pixels and blur). Turn off for subpixel placement, which moves the picture by fractions of a pixel at the cost of antialiased, slightly soft edges. Bar heights move smoothly either way
  $name: Position""")
# Window DPI awareness per thread too: the process call can fail when the
# host already declared one, and a window created without it is stretched
# (and blurred) by Windows on any monitor not at the system scale.
rep("""DWORD WINAPI UiThreadProc(LPVOID) {
""", """DWORD WINAPI UiThreadProc(LPVOID) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
""")
rep("""DWORD WINAPI InputHookThreadProc(LPVOID) {
""", """DWORD WINAPI InputHookThreadProc(LPVOID) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
""")

rep("""    } else if (!g_settings.pauseOnFullscreen) {
        ResumeFromFullscreen();
    }""", """    } else if (!g_settings.pauseOnFullscreen && !g_userPaused.load()) {
        ResumeFromFullscreen();
    }""")

# ================================================================ carried-over review items (1.3.0 PR)
# EndDrag was the last synchronous storage write from a low-level hook.
rep("""    if (g_dragMoved) {
        PersistOverrideState();
    }
}""", """    // Deferred like the other hook paths: a WH_MOUSE_LL callback holds up all
    // input until it returns, so no storage write happens in here.
    if (g_dragMoved) {
        RequestPositionOverrideSave();
    }
}""")
# The album-art thread pointer is handed between callers on several threads.
rep("static std::thread* g_albumArtThread = nullptr;\n",
    """static std::thread* g_albumArtThread = nullptr;
// Guards the pointer above: callers arrive from WinRT callbacks, the GSMTC
// thread and the UI thread, and the worker can clear the pending flag before
// the caller has stored its new thread.
static std::mutex g_albumArtThreadMutex;
""")
rep("""    if (!g_albumArtFetchPending.compare_exchange_strong(expected, true))
        return;

    if (g_albumArtThread) {""", """    if (!g_albumArtFetchPending.compare_exchange_strong(expected, true))
        return;

    std::lock_guard<std::mutex> threadLock(g_albumArtThreadMutex);
    if (g_albumArtThread) {""")
rep("""    if (g_albumArtThread) {
        if (g_albumArtThread->joinable()) {
            HANDLE hThread = g_albumArtThread->native_handle();""", """    std::lock_guard<std::mutex> albumThreadLock(g_albumArtThreadMutex);
    if (g_albumArtThread) {
        if (g_albumArtThread->joinable()) {
            HANDLE hThread = g_albumArtThread->native_handle();""")
# OpenVINO search: no folders any user can create (C:\Intel, C:\openvino*),
# since a DLL loaded from there would run inside Windhawk.
rep("""    for (const auto& d : GlobDirs(L"C:\\\\Intel", L"openvino*")) addRoot(d);
    for (const auto& d : GlobDirs(L"C:", L"openvino*")) addRoot(d);
""", """    // Not C:\\Intel or C:\\openvino*: any user can create folders at the root
    // of C:, and a DLL loaded from one would run inside Windhawk. An archive
    // extracted there still works through NPU Runtime Folder.
""")

rep("Program Files\\Intel\\openvino*, C:\\Intel\\openvino*, a pip-installed", "Program Files\\Intel\\openvino*, a pip-installed")

# ================================================================ Performance Stats setting
rep("""    - autoHideEnabled: false
""", """    - perfStats: false
      $name: Performance Stats
      $description: Every 30 seconds, writes one line to the Windhawk log (turn on Enable logging) with what the visualizer actually cost - engine wakes and milliseconds, analyses and FFTs, render ticks, frames presented versus skipped, text redraws, compositor commits and buffer uploads, and time spent playing / trickling / in deep idle. All per second, so settings and PCs compare directly. Counting is always on and costs a few atomic adds per frame; this only switches the log line
    - autoHideEnabled: false
""")
rep("""    g_settings.deepIdle = Wh_GetIntSetting(L"performance.deepIdle") != 0;
""", """    g_settings.deepIdle = Wh_GetIntSetting(L"performance.deepIdle") != 0;
    g_perfStatsEnabled.store(Wh_GetIntSetting(L"performance.perfStats") != 0, std::memory_order_relaxed);
""")
# ================================================================ styles (2.1)
# Eight new styles chosen from the Shape list, plus Reflection. See
# p3_styles.cpp for what each one is and how it rides on an existing shape.
after("enum class VizContextMenu { RightClick, CtrlRightClick, Off };\n",
      "enum class VizStyle { None, Led, Line, Bloom, Spectrogram, Vu, SplitLR, Particles };\n")
after("    std::wstring audioSourceKey;\n", """
    // Styles (2.1): picked from the Shape list on top of an internal shape.
    VizStyle style = VizStyle::None;
    int reflection = 0;  // %, of Bar Max Size
""")
after("std::atomic<uint32_t> g_gonioSerial{0};\n",
      "std::atomic<uint32_t> g_vizStereoRate{48000};  // sample rate of g_gonioXY, for Stereo Field\n")
rep("""        gonioPending_.clear();
        g_gonioSerial.fetch_add(1, std::memory_order_release);""", """        gonioPending_.clear();
        g_vizStereoRate.store(sampleRate_, std::memory_order_relaxed);
        g_gonioSerial.fetch_add(1, std::memory_order_release);""")
# The stereo feed also drives VU Needles and Stereo Field; beats throw the sparks.
rep("    c.wantGonio = g_settings.shape == VizShape::Goniometer;",
    "    c.wantGonio = g_settings.shape == VizShape::Goniometer || g_settings.style == VizStyle::Vu ||\n"
    "                  g_settings.style == VizStyle::SplitLR;")
rep("    c.beat = g_settings.beatFlashEnabled;",
    "    c.beat = g_settings.beatFlashEnabled || g_settings.style == VizStyle::Particles;")
before("bool ComputeVizLayout(VizLayout* out) {", read("p3_styles.cpp") + "\n")
rep("""        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }
""", """        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }
    VizStyleBox(&totalWidth, &totalHeight, maxSize, horizontal);
""", 2)
rep("""        totalWidth  = horizontal ? groupThickness : groupExtent;
        totalHeight = horizontal ? groupExtent    : groupThickness;
    }
""", """        totalWidth  = horizontal ? groupThickness : groupExtent;
        totalHeight = horizontal ? groupExtent    : groupThickness;
    }
    VizStyleBox(&totalWidth, &totalHeight, maxSize, horizontal);
""")
rep("if (g_settings.shape == VizShape::Terminal) VizBuildTermGrid();\n",
    "if (g_settings.shape == VizShape::Terminal) VizBuildTermGrid();\n        VizStylesFrame();\n", 2)
rep("""        if (g_settings.shape == VizShape::Dots) {""", """        if (VizDrawStyleD2D(blockX, blockY, totalWidth, totalHeight, barCount, barW, barGap, maxSize, idleSize,
                            horizontal, c1, cGrad1, c2, rainbowBase)) {
            // drawn by the style
        } else if (g_settings.shape == VizShape::Dots) {""")
rep("""    PCWSTR shape = Wh_GetStringSetting(L"appearance.shape");
    g_settings.shape = (wcscmp(shape, L"goniometer") == 0)   ? VizShape::Goniometer
                       : (wcscmp(shape, L"mountain") == 0)   ? VizShape::Mountain
                       : (wcscmp(shape, L"mirror") == 0)     ? VizShape::Mirror
                       : (wcscmp(shape, L"wave") == 0)       ? VizShape::Wave
                       : (wcscmp(shape, L"breathe") == 0)    ? VizShape::Breathe
                       : (wcscmp(shape, L"dots") == 0)       ? VizShape::Dots
                       : (wcscmp(shape, L"radial") == 0)     ? VizShape::Radial
                       : (wcscmp(shape, L"oscilloscope") == 0) ? VizShape::Oscilloscope
                       : (wcscmp(shape, L"terminal") == 0)   ? VizShape::Terminal
                                                               : VizShape::Stereo;
    Wh_FreeStringSetting(shape);""", """    PCWSTR shape = Wh_GetStringSetting(L"appearance.shape");
    VizParseShape(shape, &g_settings.shape, &g_settings.style);
    Wh_FreeStringSetting(shape);
    g_settings.reflection = std::clamp(Wh_GetIntSetting(L"appearance.reflection"), 0, 100);""")
rep("""        if (g_settings.workload == VizWorkload::Gpu && g_settings.shape == VizShape::Terminal) {""",
    """        if (g_settings.workload == VizWorkload::Gpu && VizStyleNeedsCpuBars()) {
            ReportSettingWarning(L"Hardware", L"Workload",
                                 L"Spectrogram, Stereo Field and Particles work from the bar levels on the CPU, "
                                 L"so with them the analysis runs on the CPU (Hybrid).");
        }
        if (g_settings.workload == VizWorkload::Gpu && g_settings.shape == VizShape::Terminal) {""")
rep("""        - terminal: Terminal (text characters, see the Terminal section)
    - orientation: horizontal""", """        - terminal: Terminal (text characters, see the Terminal section)
        - led: LED Meter (segmented, green / amber / red)
        - line: Line Spectrum (filled curve, glowing edge)
        - bloom: Polar Bloom (Radial as one filled shape)
        - spectrogram: Spectrogram (scrolling colour history)
        - vu: VU Needles (two analog meters, L and R)
        - stereo_field: Stereo Field (left above, right below)
        - particles: Particles (bars plus sparks on each beat)
    - reflection: 0
      $name: Reflection
      $description: 0-100. Mirrors the bars onto a floor beneath them, fading out over this percentage of Bar Max Size. Horizontal bars anchored to the bottom only, with the bar shapes, LED Meter, Line Spectrum and Particles. Both renderers (Direct2D draws it on the CPU, so it costs a little more there)
    - orientation: horizontal""")

# ================================================================ Media Card (2.1)
# Media Controls > Layout = Card: album art with controls on hover, a seek
# bar, one-click output switching and a volume slider. See p3_media.cpp.
after("#include <mmdeviceapi.h>\n", "#include <endpointvolume.h>\n")
after("    int mediaPlatePadding = 0;\n", "    bool mediaCard = false;  // Media Controls > Layout = Card (2.1)\n")
after("static std::thread* g_albumArtThread = nullptr;\n", """
// Media Card (2.1): the cover, box-filtered down to at most 160 px, straight
// alpha BGRA as WIC decodes it. Written by the album-art thread, read by the
// media window's paint.
std::mutex g_artTileMutex;
std::vector<BYTE> g_artTile;
int g_artTileW = 0, g_artTileH = 0;
std::atomic<int64_t> g_mediaSeekTicks{0};  // seek target for media command 3, 100 ns units

void VizStoreArtTile(const BYTE* px, int w, int h) {
    std::vector<BYTE> out;
    int ow = 0, oh = 0;
    if (px && w > 0 && h > 0) {
        int f = std::max(1, (std::max(w, h) + 159) / 160);
        ow = std::max(1, w / f);
        oh = std::max(1, h / f);
        out.resize((size_t)ow * oh * 4);
        for (int y = 0; y < oh; y++)
            for (int x = 0; x < ow; x++)
                for (int k = 0; k < 4; k++) {
                    unsigned sum = 0;
                    for (int yy = 0; yy < f; yy++)
                        for (int xx = 0; xx < f; xx++) sum += px[((size_t)(y * f + yy) * w + (x * f + xx)) * 4 + k];
                    out[((size_t)y * ow + x) * 4 + k] = (BYTE)(sum / (unsigned)(f * f));
                }
    }
    {
        std::lock_guard<std::mutex> lock(g_artTileMutex);
        if (out.empty() && g_artTile.empty()) return;
        g_artTile.swap(out);
        g_artTileW = ow;
        g_artTileH = oh;
    }
    if (g_mediaWnd && g_settings.mediaCard) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}
""")
rep("            if (!thumbRef) { winrt::uninit_apartment();",
    "            if (!thumbRef) { VizStoreArtTile(nullptr, 0, 0); winrt::uninit_apartment();")
rep("""            if (!pixels.empty()) {
                struct Bucket""", """            if (!pixels.empty()) VizStoreArtTile(pixels.data(), imgW, imgH);
            if (!pixels.empty()) {
                struct Bucket""")
rep("                    else if (cmd == 2) session.TrySkipNextAsync().get();",
    """                    else if (cmd == 2) session.TrySkipNextAsync().get();
                    else if (cmd == 3)
                        session.TryChangePlaybackPositionAsync(g_mediaSeekTicks.load(std::memory_order_relaxed)).get();""")
before("void PaintMediaControls(int x, int y, int width, int height) {", read("p3_media.cpp") + "\n")
rep("""    // Optional backing plate behind the whole strip, with an optional outline.""",
    """    if (VizCardActive()) {
        VizPaintCard(buf, stride, width, height);
    } else {
    // Optional backing plate behind the whole strip, with an optional outline.""")
rep("""    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, dib);

    POINT ptSrc = {0, 0};""", """    }  // strip

    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, dib);

    POINT ptSrc = {0, 0};""")
rep("""    if (!g_settings.mediaControlsEnabled) {
        ShowWindow(g_mediaWnd, SW_HIDE);
        return;
    }""", """    if (!g_settings.mediaControlsEnabled) {
        VizCardTimer(g_mediaWnd, false);
        ShowWindow(g_mediaWnd, SW_HIDE);
        return;
    }
    VizCardTimer(g_mediaWnd, VizCardActive());""")
rep("""    int width = size * 3 + spacing * 2 + pad * 2;
    int height = size + pad * 2;
""", """    int width = size * 3 + spacing * 2 + pad * 2;
    int height = size + pad * 2;
    if (VizCardActive()) VizCardSize(&width, &height);
""", 2)
rep("""LRESULT CALLBACK MediaWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {""", """LRESULT CALLBACK MediaWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (VizCardActive() && VizCardMessage(hWnd, uMsg, wParam, lParam)) return 0;
    switch (uMsg) {""")
before("// Resolves a source key to a device. Falls back", read("p3_media_dev.cpp") + "\n")
after("""    g_settings.mediaIconSize = std::clamp(Wh_GetIntSetting(L"media_controls.iconSize"), 8, 256);
""", """    {
        PCWSTR layout = Wh_GetStringSetting(L"media_controls.layout");
        g_settings.mediaCard = layout && wcscmp(layout, L"card") == 0;
        Wh_FreeStringSetting(layout);
    }
""")
rep("""      $description: Distance in from the panel's top or bottom edge, in pixels. Only used with a panel anchor
""", """      $description: Distance in from the panel's top or bottom edge, in pixels. Only used with a panel anchor
    - layout: strip
      $name: Layout
      $description: Strip is the three buttons. Card is a small media card in the same place, sized from Icon Size and Icon Spacing - the album art (hover it for previous / play / next), a seek bar (click or drag), a speaker button that switches the Windows default output in one click, and a volume slider (drag it, or scroll anywhere on the card)
      $options:
        - strip: Strip (three buttons)
        - card: Card (art, seek, output, volume)
""")

# ================================================================ Media Card theme (2.1)
after("    bool mediaCard = false;  // Media Controls > Layout = Card (2.1)\n", """    BYTE cardBgA = 158, cardBgR = 10, cardBgG = 10, cardBgB = 13;
    BYTE cardBorderA = 0, cardBorderR = 255, cardBorderG = 255, cardBorderB = 255;
    int cardBorderSize = 0, cardRadius = 12, cardArtSize = 0;
    int cardAccentSource = 0;  // 0 icon colour, 1 custom, 2 album art, 3 Windows accent
    BYTE cardAccentA = 255, cardAccentR = 255, cardAccentG = 255, cardAccentB = 255;
""")
after("std::atomic<int64_t> g_mediaSeekTicks{0};  // seek target for media command 3, 100 ns units\n",
      "inline bool VizCardWantsArt() { return g_settings.mediaControlsEnabled && g_settings.mediaCard; }\n")
# The card shows the cover, so it fetches it whatever the colour mode.
import re as _re
_n = len(_re.findall(r"g_settings\.nowPlayingEnabled\)\n(\s*)FetchAlbumArtColorAsync\(\);", src))
if _n != 4:
    sys.exit(f"album fetch anchors {_n} != 4")
src = _re.sub(r"g_settings\.nowPlayingEnabled\)\n(\s*)FetchAlbumArtColorAsync\(\);",
              r"g_settings.nowPlayingEnabled || VizCardWantsArt())\n\1FetchAlbumArtColorAsync();", src)
after("                    g_albumArtColorReady.store(true, std::memory_order_relaxed);\n",
      "                    if (g_mediaWnd && g_settings.mediaCard) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);\n")
after("""        g_settings.mediaCard = layout && wcscmp(layout, L"card") == 0;
        Wh_FreeStringSetting(layout);
    }
""", """    ReadColorSetting(L"media_controls.cardBackground", L"Media Controls", L"Card Background", 158, 10, 10, 13,
                     &g_settings.cardBgA, &g_settings.cardBgR, &g_settings.cardBgG, &g_settings.cardBgB);
    ReadColorSetting(L"media_controls.cardBorderColor", L"Media Controls", L"Card Border Color", 0, 255, 255, 255,
                     &g_settings.cardBorderA, &g_settings.cardBorderR, &g_settings.cardBorderG, &g_settings.cardBorderB);
    g_settings.cardBorderSize = std::clamp(Wh_GetIntSetting(L"media_controls.cardBorderSize"), 0, 20);
    g_settings.cardRadius = std::clamp(Wh_GetIntSetting(L"media_controls.cardCornerRadius"), 0, 64);
    g_settings.cardArtSize = std::clamp(Wh_GetIntSetting(L"media_controls.cardArtSize"), 0, 600);
    {
        PCWSTR acc = Wh_GetStringSetting(L"media_controls.cardAccent");
        g_settings.cardAccentSource = !acc ? 0 : wcscmp(acc, L"custom") == 0 ? 1 : wcscmp(acc, L"album") == 0 ? 2
                                    : wcscmp(acc, L"windows") == 0 ? 3 : 0;
        Wh_FreeStringSetting(acc);
    }
    ReadColorSetting(L"media_controls.cardAccentColor", L"Media Controls", L"Card Accent Color", 255, 255, 255, 255,
                     &g_settings.cardAccentA, &g_settings.cardAccentR, &g_settings.cardAccentG, &g_settings.cardAccentB);
""")
rep("""        - card: Card (art, seek, output, volume)
""", """        - card: Card (art, seek, output, volume)
    - cardBackground: '#9E0A0A0D'
      $name: Card Background
      $description: 'Card only. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b). Kept just above fully transparent at the least, so the card always takes clicks'
    - cardBorderColor: '#00FFFFFF'
      $name: Card Border Color
      $description: 'Card only. Same formats as Card Background'
    - cardBorderSize: 0
      $name: Card Border Size
      $description: Card only. Pixels, drawn inward from the edge
    - cardCornerRadius: 12
      $name: Card Corner Radius
      $description: Card only. Pixels. The album art's corners follow it
    - cardArtSize: 0
      $name: Card Art Size
      $description: Card only. Width of the album art in pixels, which sets the card's width. 0 sizes it from Icon Size and Icon Spacing
    - cardAccent: icon
      $name: Card Accent
      $description: Card only. Colour of the seek and volume fills and their knobs
      $options:
        - icon: Icon Color
        - custom: Card Accent Color
        - album: Album art
        - windows: Windows accent
    - cardAccentColor: '#FFFFFFFF'
      $name: Card Accent Color
      $description: 'Card only, with Card Accent = Card Accent Color. Same formats as Card Background'
""")

# ================================================================ FX: Glow and Bloom (2.1)
after("    int reflection = 0;  // %, of Bar Max Size\n",
      "    int fxGlow = 0, fxGlowRadius = 6, fxBloom = 0, fxBloomRadius = 16;  // FX (2.1)\n")
rep("""    g_settings.reflection = std::clamp(Wh_GetIntSetting(L"appearance.reflection"), 0, 100);""",
    """    g_settings.reflection = std::clamp(Wh_GetIntSetting(L"appearance.reflection"), 0, 100);
    g_settings.fxGlow = std::clamp(Wh_GetIntSetting(L"appearance.fxGlow"), 0, 100);
    g_settings.fxGlowRadius = std::clamp(Wh_GetIntSetting(L"appearance.fxGlowRadius"), 1, 32);
    g_settings.fxBloom = std::clamp(Wh_GetIntSetting(L"appearance.fxBloom"), 0, 100);
    g_settings.fxBloomRadius = std::clamp(Wh_GetIntSetting(L"appearance.fxBloomRadius"), 4, 64);""")
rep("""      $description: 0-100. Mirrors the bars onto a floor beneath them, fading out over this percentage of Bar Max Size. Horizontal bars anchored to the bottom only, with the bar shapes, LED Meter, Line Spectrum and Particles. Both renderers (Direct2D draws it on the CPU, so it costs a little more there)
""", """      $description: 0-100. Mirrors the bars onto a floor beneath them, fading out over this percentage of Bar Max Size. Horizontal bars anchored to the bottom only, with the bar shapes, LED Meter, Line Spectrum and Particles. Both renderers (Direct2D draws it on the CPU, so it costs a little more there)
    - fxGlow: 0
      $name: Glow
      $description: 0-100. A soft halo around each bar, dot, line and spark, worked out in the same shader pass that draws them, so it costs next to nothing. Direct3D 11 renderer only
    - fxGlowRadius: 6
      $name: Glow Radius
      $description: 1-32 pixels. How far the halo reaches
    - fxBloom: 0
      $name: Bloom
      $description: 0-100. Light bleeding out of the whole picture, like a camera lens, from a blurred copy at a quarter of the size added back on top. A little GPU work, and only on frames that change. Direct3D 11 renderer only
    - fxBloomRadius: 16
      $name: Bloom Radius
      $description: 4-64 pixels. How far the light spreads
""")

# ================================================================ Style Editor (2.1)
rep("-luuid -luser32 -ladvapi32", "-luuid -luser32 -ladvapi32 -lcomctl32 -lcomdlg32")
after("#include <windowsx.h>\n", "#include <commctrl.h>\n#include <commdlg.h>\n")
before("LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {", read("p3_editor.cpp") + "\n")
before("    if (g_messageWnd) SendMessage(g_messageWnd, WM_APP_CLEANUP, 0, 0);\n    if (g_mediaWnd)",
       "    if (g_styleEditorWnd) SendMessage(g_styleEditorWnd, WM_CLOSE, 0, 0);\n")
after("    UnregisterMediaWindowClass();\n", "    UnregisterStyleEditorClass();\n")

# ================================================================ README: 2.0 additions
rep("media controls pinned to the panel.\n",
    "media controls pinned to the panel.\n\n"
    "**Seven new styles.** LED Meter, Line Spectrum, Polar Bloom, Spectrogram, VU Needles (real IEC VU ballistics), Stereo Field and Particles, "
    "all in the Shape list and the right-click menu, plus **Reflection**, a fading mirror under the bars.\n\n"
    "**Glow and Bloom.** A soft halo around every bar, worked out in the same shader pass that draws it, and a lens-style bloom from a quarter-size blur "
    "that only runs on frames that change. Direct3D 11 renderer.\n\n"
    "**A Media Card.** Media Controls > Layout = Card: album art with previous / play / next on hover, a seek bar, one-click output switching and a volume slider. "
    "Theme its background, border, corner radius, art size and accent (icon colour, custom, album art or your Windows accent).\n\n"
    "**My Styles.** Right-click > My Styles > Style Editor: mix a base style, colours, bar sizes, reflection, glow and bloom while the visualizer previews it live, "
    "save it under a name, and pick it from the menu any time.\n")

# ================================================================ Input hooks yield to covering apps (2.1)
# A drag used to start whenever the combo was pressed inside the visualizer's
# rectangle, even with a game on top, and then swallowed every mouse move
# until release. It now needs the desktop itself under the cursor, like the
# right-click menu, and the visualizer to be showing.
rep("""            if (wParam == downMsg && DragModifierHeld() && PointInVisualizerBounds(info->pt)) {""",
    """            if (wParam == downMsg && DragModifierHeld() && PointInVisualizerBounds(info->pt) &&
                !g_fullscreenPaused.load(std::memory_order_relaxed) &&
                !g_vizSceneHidden.load(std::memory_order_relaxed) && VizDesktopUnderPoint(info->pt)) {""")
# The move keys go to whatever has focus while a fullscreen or covering app
# has the visualizer hidden.
rep("""        if ((isDown || isUp) && ModKeysHeld(g_settings.keyMoveModifier)) {""",
    """        if ((isDown || isUp) && !g_fullscreenPaused.load(std::memory_order_relaxed) &&
            ModKeysHeld(g_settings.keyMoveModifier)) {""")

# ================================================================ Reflection on Direct2D (2.1)
rep("""        if (VizDrawStyleD2D(blockX, blockY, totalWidth, totalHeight, barCount, barW, barGap, maxSize, idleSize,
                            horizontal, c1, cGrad1, c2, rainbowBase)) {""",
    """        const bool reflD2D = VizReflD2DBegin(useFadeLayer);
        if (VizDrawStyleD2D(blockX, blockY, totalWidth, totalHeight, barCount, barW, barGap, maxSize, idleSize,
                            horizontal, c1, cGrad1, c2, rainbowBase)) {""")
rep("""        }

        {
            VizTextFrame tf;
            VizBuildTextFrame(tf);
            VizDrawTextOverlays(tf, layout, smooth);""",
    """        }
        if (reflD2D) VizReflD2DEnd(blockY + maxSize, VizReflectionDepth(maxSize));

        {
            VizTextFrame tf;
            VizBuildTextFrame(tf);
            VizDrawTextOverlays(tf, layout, smooth);""")

out = os.path.join(S, "v2b.cpp")
open(out, "w", encoding="utf-8", newline="\n").write(src)
print("wrote", out, src.count("\n"), "lines")
