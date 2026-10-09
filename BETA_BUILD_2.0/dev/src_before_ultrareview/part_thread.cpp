// Thread-scoped quality of service. While playing, the engine thread opts out
// of execution-speed throttling so frame pacing stays tight on a hybrid CPU;
// while idle it asks for EcoQoS, so the trickle and the meter reads run on
// efficiency cores at low clocks. Thread-scoped on purpose: nothing about the
// process changes. SetThreadInformation is resolved at runtime (Windows 8+;
// the power-throttling class needs Windows 10 1709, and older builds simply
// return an error, which is ignored).
void VizThreadEcoQoS(bool eco) {
    struct PowerThrottlingState {
        ULONG Version, ControlMask, StateMask;
    };
    typedef BOOL(WINAPI * SetThreadInformationFn)(HANDLE, int, LPVOID, DWORD);
    static SetThreadInformationFn fn =
        (SetThreadInformationFn)(void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadInformation");
    if (!fn) return;
    PowerThrottlingState st = {1, 0x1 /* EXECUTION_SPEED */, eco ? 0x1u : 0x0u};
    fn(GetCurrentThread(), 3 /* ThreadPowerThrottling */, &st, sizeof(st));
}

// The engine thread: frame pacing (as 1.5), and since 2.0 also audio capture
// and analysis (see "Audio engine" above). One wake per frame while playing,
// none at all in deep idle except four meter reads a second.
void RenderThreadProc() {
    HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    VizEngine engine;
    engine.ThreadInit();

    ULONGLONG lastSuccessfulPostTick = 0;
    int ecoState = -1;  // -1 unknown, 0 full speed, 1 EcoQoS
    auto setEco = [&](bool eco) {
        if (ecoState == (eco ? 1 : 0)) return;
        ecoState = eco ? 1 : 0;
        VizThreadEcoQoS(eco);
    };

    // Smooth Mode pacing state (see VizVsyncSchedule).
    LONGLONG vsPeriod = 0, vsVblank = 0, vsLastSync = 0, vsDue = 0;
    LONGLONG absDue = 0;

    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(&qpcFreq);
    LONGLONG lastRenderQpc = 0;
    LONGLONG lastEngineQpc = 0;  // for the analysis dt

    HANDLE hTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                           TIMER_ALL_ACCESS);
    if (!hTimer) hTimer = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);

    // Waits `ms`, but returns early if the wake event is set (settings change,
    // pause, unload), so none of those has to wait out a frame.
    auto preciseWait = [hTimer](double ms) {
        if (ms <= 0.0) return;
        if (hTimer) {
            LARGE_INTEGER due;
            due.QuadPart = -(LONGLONG)(ms * 10000.0);
            if (due.QuadPart == 0) due.QuadPart = -1;
            if (SetWaitableTimer(hTimer, &due, 0, nullptr, nullptr, FALSE)) {
                HANDLE hs[2] = {hTimer, g_engineWake};
                WaitForMultipleObjects(g_engineWake ? 2 : 1, hs, FALSE, INFINITE);
                return;
            }
        }
        Sleep((DWORD)std::ceil(ms));
    };

    auto postTick = [&](HWND overlayWnd, ULONGLONG now) {
        bool expected = false;
        if (g_renderTickPending.compare_exchange_strong(expected, true)) {
            PostMessage(overlayWnd, WM_APP_RENDER_TICK, 0, 0);
            lastSuccessfulPostTick = now;
        } else if (now - lastSuccessfulPostTick > 1000) {
            Wh_Log(L"[Viz] Render tick flag was stuck, forced a reset");
            g_renderTickPending.store(false, std::memory_order_relaxed);
            lastSuccessfulPostTick = now;
        }
    };
    auto engineFrame = [&]() {
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        double dt = lastEngineQpc ? (double)(q.QuadPart - lastEngineQpc) / (double)qpcFreq.QuadPart : 1.0 / 60.0;
        lastEngineQpc = q.QuadPart;
        engine.Frame(dt);
    };

    while (g_renderThreadRunning.load(std::memory_order_relaxed)) {
        HWND overlayWnd = g_overlayWnd.load(std::memory_order_relaxed);
        bool paused = g_fullscreenPaused.load(std::memory_order_relaxed);
        bool wanted = g_captureWanted.load(std::memory_order_relaxed);

        // Not visible, or capture not wanted: close the stream and sleep until
        // something changes. 1.5 woke every 150 ms here; this waits on the
        // wake event, with a slow timeout as a safety net.
        if (!overlayWnd || paused || !wanted) {
            if (engine.IsOpen() || g_audioOpen.load()) engine.Close();
            setEco(true);
            lastRenderQpc = lastEngineQpc = 0;
            vsDue = absDue = 0;
            g_idleState.store((int)VizIdleState::Playing);
            WaitForSingleObject(g_engineWake, overlayWnd ? 1000 : 250);
            continue;
        }

        engine.EnsureOpen();

        ULONGLONG now = GetTickCount64();
        int fps = g_settings.targetFps;
        double refreshHz = 0.0;
        if (vsPeriod > 0) refreshHz = (double)qpcFreq.QuadPart / (double)vsPeriod;
        if (fps <= 0) fps = (refreshHz > 0.0) ? (int)llround(refreshHz) : 60;  // 0 = match the display
        fps = std::max(1, fps);
        double intervalMs = 1000.0 / (double)fps;

        // ---- Idle ladder ----------------------------------------------------------
        ULONGLONG idleMs = now - g_lastAudibleTickMs.load(std::memory_order_relaxed);
        ULONGLONG silentAfter = (ULONGLONG)std::max(0, g_settings.pauseWhenSilentSeconds) * 1000ULL;
        bool silent = g_settings.pauseWhenSilentSeconds > 0 && idleMs > silentAfter;
        // An auto-hide fade in progress still wants full frames.
        bool fading = false;
        if (g_settings.autoHideEnabled) {
            ULONGLONG d = (ULONGLONG)std::max(0, g_settings.autoHideDelaySeconds) * 1000ULL;
            fading = idleMs > d && idleMs < d + 2000;
        }
        g_slowMode = silent && !fading;

        if (g_slowMode) {
            setEco(true);
            lastRenderQpc = 0;
            vsDue = absDue = 0;
            bool deep = g_settings.deepIdle && engine.IsOpen() && idleMs > silentAfter + 5000;
            if (deep) {
                if (g_idleState.exchange((int)VizIdleState::Deep) != (int)VizIdleState::Deep) {
                    // One last tick so the renderer can settle on its final frame.
                    postTick(overlayWnd, now);
                }
                engine.StopStream();  // idempotent; also covers a reopen after a device change
                WaitForSingleObject(g_engineWake, 250);
                if (engine.MeterPeak() > 0.0001f) {
                    engine.StartStream();
                    g_lastAudibleTickMs.store(GetTickCount64(), std::memory_order_relaxed);
                    g_idleState.store((int)VizIdleState::Playing);
                    lastEngineQpc = 0;
                }
                continue;
            }
            if (engine.IsStopped()) engine.StartStream();
            g_idleState.store((int)VizIdleState::Trickle);
            // Wake on audio, the wake event, or every 250 ms.
            HANDLE hs[2];
            DWORD nh = 0;
            if (engine.AudioEvent()) hs[nh++] = engine.AudioEvent();
            if (g_engineWake) hs[nh++] = g_engineWake;
            if (nh) WaitForMultipleObjects(nh, hs, FALSE, 250);
            else Sleep(250);
            if (!g_renderThreadRunning.load(std::memory_order_relaxed) || g_unloading.load()) break;
            engineFrame();
            postTick(overlayWnd, GetTickCount64());
            continue;
        }
        if (engine.IsStopped()) engine.StartStream();
        g_idleState.store((int)VizIdleState::Playing);
        setEco(false);

        LARGE_INTEGER nowQpc;
        QueryPerformanceCounter(&nowQpc);

        // ---- Pacing (1.5) ---------------------------------------------------------
        bool scheduled = false;
        if (g_smoothActive.load(std::memory_order_relaxed)) {
            if (vsPeriod == 0 || nowQpc.QuadPart - vsLastSync > qpcFreq.QuadPart / 2) {
                DWM_TIMING_INFO ti = {};
                ti.cbSize = sizeof(ti);
                if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &ti)) &&
                    ti.qpcRefreshPeriod >= (UINT64)(qpcFreq.QuadPart / 1000) &&
                    ti.qpcRefreshPeriod <= (UINT64)(qpcFreq.QuadPart / 20) && ti.qpcVBlank > 0) {
                    vsPeriod = (LONGLONG)ti.qpcRefreshPeriod;
                    vsVblank = (LONGLONG)ti.qpcVBlank;
                } else {
                    vsPeriod = 0;
                }
                vsLastSync = nowQpc.QuadPart;
            }
            if (vsPeriod > 0) {
                if (vsDue == 0) vsDue = VizVsyncSchedule::AlignUp(nowQpc.QuadPart, vsVblank, vsPeriod, 0.5);
                if (nowQpc.QuadPart < vsDue) {
                    preciseWait((double)(vsDue - nowQpc.QuadPart) * 1000.0 / (double)qpcFreq.QuadPart);
                    continue;
                }
                int divisor = VizVsyncSchedule::Divisor((double)fps, vsPeriod, qpcFreq.QuadPart);
                vsDue = VizVsyncSchedule::Next(vsDue, nowQpc.QuadPart, vsVblank, vsPeriod, divisor);
                absDue = 0;
            } else {
                LONGLONG interval = (LONGLONG)((double)qpcFreq.QuadPart * intervalMs / 1000.0);
                if (absDue == 0) absDue = nowQpc.QuadPart;
                if (nowQpc.QuadPart < absDue) {
                    preciseWait((double)(absDue - nowQpc.QuadPart) * 1000.0 / (double)qpcFreq.QuadPart);
                    continue;
                }
                absDue += interval;
                if (absDue <= nowQpc.QuadPart) absDue = nowQpc.QuadPart + interval;
                vsDue = 0;
            }
            lastRenderQpc = nowQpc.QuadPart;
            scheduled = true;
        } else {
            vsDue = absDue = 0;
            // Target FPS 0 needs the refresh rate even without Smooth Mode.
            if (g_settings.targetFps <= 0 && (vsPeriod == 0 || nowQpc.QuadPart - vsLastSync > qpcFreq.QuadPart * 2)) {
                DWM_TIMING_INFO ti = {};
                ti.cbSize = sizeof(ti);
                if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &ti)) && ti.qpcRefreshPeriod > 0)
                    vsPeriod = (LONGLONG)ti.qpcRefreshPeriod;
                vsLastSync = nowQpc.QuadPart;
            }
        }

        if (!scheduled) {
            double elapsedMs = (lastRenderQpc == 0)
                                   ? intervalMs
                                   : (double)(nowQpc.QuadPart - lastRenderQpc) * 1000.0 / (double)qpcFreq.QuadPart;
            if (elapsedMs < intervalMs) {
                preciseWait(intervalMs - elapsedMs);
                continue;
            }
            lastRenderQpc = nowQpc.QuadPart;
        }

        if (!g_renderThreadRunning.load(std::memory_order_relaxed) || g_unloading.load()) break;

        engineFrame();
        postTick(overlayWnd, GetTickCount64());
    }

    engine.ThreadExit();
    if (hTimer) CloseHandle(hTimer);
    if (SUCCEEDED(comHr)) CoUninitialize();
}

void StartRenderThread() {
    if (g_renderThread) return;
    if (!g_engineWake) g_engineWake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!g_engineClosed) g_engineClosed = CreateEvent(nullptr, TRUE, TRUE, nullptr);
    g_renderThreadRunning.store(true, std::memory_order_relaxed);
    g_renderThread = new std::thread(RenderThreadProc);
    HANDLE hRenderThread = g_renderThread->native_handle();
    SetThreadDescription(hRenderThread, L"TourneTable-Engine");
}

void StopRenderThread() {
    g_renderThreadRunning.store(false, std::memory_order_relaxed);
    if (g_engineWake) SetEvent(g_engineWake);
    if (g_renderThread) {
        if (g_renderThread->joinable()) g_renderThread->join();
        delete g_renderThread;
        g_renderThread = nullptr;
    }
    g_renderTickPending.store(false, std::memory_order_relaxed);
}
