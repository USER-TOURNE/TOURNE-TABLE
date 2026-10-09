// ---- Right-click menu: live settings ---------------------------------------------------
//
// Windhawk mods can read their settings but not write them, so anything
// changed from this menu is kept as a mod-owned value (Wh_SetStringValue, the
// same store the dragged position uses) and laid over the Windhawk settings
// every time they load. The menu's last item clears them all, handing control
// back to the settings page. Until then the settings page shows the values
// underneath, which is why the menu says how many quick settings are active.

std::vector<std::pair<std::wstring, std::wstring>> g_menuOverrides;
std::atomic<bool> g_userPaused{false};

void VizLoadMenuOverrides() {
    g_menuOverrides.clear();
    WCHAR buf[4096] = {};
    if (!Wh_GetStringValue(L"menuOverrides", buf, ARRAYSIZE(buf))) return;
    std::wstring s = buf;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t nl = s.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = s.size();
        std::wstring line = s.substr(pos, nl - pos);
        size_t eq = line.find(L'=');
        if (eq != std::wstring::npos && eq > 0) g_menuOverrides.emplace_back(line.substr(0, eq), line.substr(eq + 1));
        pos = nl + 1;
    }
}

void VizSaveMenuOverrides() {
    std::wstring s;
    for (const auto& kv : g_menuOverrides) s += kv.first + L"=" + kv.second + L"\n";
    Wh_SetStringValue(L"menuOverrides", s.c_str());
}

void VizSetMenuOverride(const std::wstring& key, const std::wstring& value) {
    for (auto& kv : g_menuOverrides)
        if (kv.first == key) {
            kv.second = value;
            VizSaveMenuOverrides();
            return;
        }
    g_menuOverrides.emplace_back(key, value);
    VizSaveMenuOverrides();
}

// Option tables shared by the menu (labels) and the overrides (values). The
// values are the same strings the settings YAML uses.
struct VizMenuOption {
    const wchar_t* value;
    const wchar_t* label;
};
static const VizMenuOption kShapes[] = {
    {L"stereo", L"Stereo"},     {L"mountain", L"Mountain"},         {L"mirror", L"Mirror"},
    {L"wave", L"Wave"},         {L"breathe", L"Breathe"},           {L"dots", L"Dots"},
    {L"radial", L"Radial"},     {L"oscilloscope", L"Oscilloscope"}, {L"goniometer", L"Goniometer"},
    {L"terminal", L"Terminal"}};
static const VizMenuOption kColorModes[] = {
    {L"solid", L"Solid"},          {L"gradient", L"Gradient"},           {L"reactive_gradient", L"Reactive Gradient"},
    {L"accent", L"Windows Accent"}, {L"album_art", L"Album Art"},         {L"dynamic_album", L"Dynamic Album"},
    {L"acrylic", L"Acrylic"},      {L"rainbow", L"Rainbow Cycle"},       {L"tourne", L"Tourne"}};
static const VizMenuOption kTermStyles[] = {
    {L"columns", L"Columns"}, {L"waterfall", L"Waterfall"}, {L"meters", L"Meters"}};
static const VizMenuOption kEngines[] = {{L"precision", L"Precision"}, {L"classic", L"Classic (1.4)"}};
static const VizMenuOption kLayouts[] = {
    {L"scale", L"Frequency Scale (Bar Count)"}, {L"iec:3", L"IEC 1/3 octave"},  {L"iec:6", L"IEC 1/6 octave"},
    {L"iec:12", L"IEC 1/12 octave"},            {L"iec:24", L"IEC 1/24 octave"}, {L"musical:12", L"Musical notes"},
    {L"musical:24", L"Musical quarter tones"}};
static const VizMenuOption kWeightings[] = {{L"z", L"Z (flat)"}, {L"a", L"A"}, {L"c", L"C"}};
static const VizMenuOption kBallistics[] = {
    {L"snappy", L"Snappy"}, {L"smooth", L"Smooth"},       {L"analyzer", L"Analyzer (20 dB/s)"},
    {L"vu", L"VU"},         {L"ppm_ebu", L"PPM, EBU"},    {L"ppm_din", L"PPM, DIN"}};
static const VizMenuOption kWorkloads[] = {
    {L"hybrid", L"Hybrid (CPU analysis, GPU drawing)"}, {L"gpu", L"GPU"}, {L"cpu", L"CPU"},
    {L"npu", L"NPU (experimental)"}};
static const VizMenuOption kRenderers[] = {{L"d3d11", L"Direct3D 11"}, {L"direct2d", L"Direct2D (1.5)"}};
static const VizMenuOption kFps[] = {{L"0", L"Match display"}, {L"30", L"30"}, {L"60", L"60"},
                                     {L"120", L"120"},         {L"144", L"144"}, {L"240", L"240"}};
static const VizMenuOption kReadouts[] = {
    {L"off", L"Off"},           {L"frequency", L"Peak frequency"}, {L"loudness", L"Loudness"},
    {L"loudness_full", L"Loudness, true peak, PLR, correlation"}, {L"both", L"Peak frequency and loudness"}};

// Applied at the end of LoadSettings, before anything derived from settings.
void VizApplyMenuOverrides() {
    for (const auto& kv : g_menuOverrides) {
        const std::wstring& k = kv.first;
        PCWSTR v = kv.second.c_str();
        auto is = [&](PCWSTR s) { return wcscmp(v, s) == 0; };
        if (k == L"shape") {
            g_settings.shape = is(L"mountain") ? VizShape::Mountain : is(L"mirror") ? VizShape::Mirror
                             : is(L"wave") ? VizShape::Wave : is(L"breathe") ? VizShape::Breathe
                             : is(L"dots") ? VizShape::Dots : is(L"radial") ? VizShape::Radial
                             : is(L"oscilloscope") ? VizShape::Oscilloscope : is(L"goniometer") ? VizShape::Goniometer
                             : is(L"terminal") ? VizShape::Terminal : VizShape::Stereo;
        } else if (k == L"colorMode") {
            g_settings.colorMode = is(L"gradient") ? VizColorMode::Gradient
                                 : is(L"reactive_gradient") ? VizColorMode::ReactiveGradient
                                 : is(L"accent") ? VizColorMode::Accent : is(L"album_art") ? VizColorMode::AlbumArt
                                 : is(L"dynamic_album") ? VizColorMode::DynamicAlbum
                                 : is(L"acrylic") ? VizColorMode::Acrylic : is(L"rainbow") ? VizColorMode::RainbowCycle
                                 : is(L"tourne") ? VizColorMode::Tourne : VizColorMode::Solid;
        } else if (k == L"termStyle") {
            g_settings.termStyle = is(L"waterfall") ? VizTermStyle::Waterfall
                                 : is(L"meters") ? VizTermStyle::Meters : VizTermStyle::Columns;
        } else if (k == L"engine") {
            g_settings.engine = is(L"classic") ? VizEngineKind::Classic : VizEngineKind::Precision;
        } else if (k == L"bandLayout") {
            std::wstring s = v;
            size_t c = s.find(L':');
            std::wstring kind = s.substr(0, c);
            int frac = (c == std::wstring::npos) ? 0 : _wtoi(s.c_str() + c + 1);
            g_settings.bandLayout = (kind == L"iec") ? VizBandLayout::Iec
                                  : (kind == L"musical") ? VizBandLayout::Musical : VizBandLayout::Scale;
            if (frac == 1 || frac == 3 || frac == 6 || frac == 12 || frac == 24) g_settings.octaveFraction = frac;
        } else if (k == L"weighting") {
            g_settings.weighting = is(L"a") ? VizWeighting::A : is(L"c") ? VizWeighting::C : VizWeighting::Z;
        } else if (k == L"ballistics") {
            g_settings.ballistics = is(L"smooth") ? VizBallisticsPreset::Smooth
                                  : is(L"analyzer") ? VizBallisticsPreset::Analyzer
                                  : is(L"vu") ? VizBallisticsPreset::Vu : is(L"ppm_ebu") ? VizBallisticsPreset::PpmEbu
                                  : is(L"ppm_din") ? VizBallisticsPreset::PpmDin : VizBallisticsPreset::Snappy;
        } else if (k == L"workload") {
            g_settings.workload = is(L"gpu") ? VizWorkload::Gpu : is(L"cpu") ? VizWorkload::Cpu
                                : is(L"npu") ? VizWorkload::Npu : VizWorkload::Hybrid;
        } else if (k == L"renderer") {
            g_settings.renderer = is(L"direct2d") ? VizRenderer::Direct2D : VizRenderer::D3D11;
        } else if (k == L"targetFps") {
            g_settings.targetFps = std::clamp(_wtoi(v), 0, 1000);
        } else if (k == L"readout") {
            g_settings.peakFreqEnabled = !is(L"off");
            if (!is(L"off"))
                g_settings.readout = is(L"loudness") ? VizReadout::Loudness
                                   : is(L"loudness_full") ? VizReadout::LoudnessFull
                                   : is(L"both") ? VizReadout::Both : VizReadout::Frequency;
        } else if (k == L"audioSource") {
            g_settings.audioSourceKey = v;
        } else if (k == L"peakHold") {
            g_settings.peakHoldEnabled = is(L"1");
        } else if (k == L"beatFlash") {
            g_settings.beatFlashEnabled = is(L"1");
        } else if (k == L"nowPlaying") {
            g_settings.nowPlayingEnabled = is(L"1");
        } else if (k == L"progress") {
            g_settings.progressEnabled = is(L"1");
        } else if (k == L"mediaControls") {
            g_settings.mediaControlsEnabled = is(L"1");
        } else if (k == L"pixelText") {
            g_settings.textPixel = is(L"1");
        } else if (k == L"pixelSnap") {
            g_settings.pixelSnap = is(L"1");
        } else if (k == L"fineNudge") {
            g_settings.keyMoveFine = is(L"1");
        }
    }
}

// What the menu shows as checked: the current effective value as a string.
static std::wstring CurrentValue(const std::wstring& key) {
    auto pick = [](const VizMenuOption* opts, size_t n, int index) -> std::wstring {
        return (index >= 0 && (size_t)index < n) ? opts[index].value : L"";
    };
    if (key == L"shape") return pick(kShapes, ARRAYSIZE(kShapes), (int)g_settings.shape);
    if (key == L"colorMode") return pick(kColorModes, ARRAYSIZE(kColorModes), (int)g_settings.colorMode);
    if (key == L"termStyle") return pick(kTermStyles, ARRAYSIZE(kTermStyles), (int)g_settings.termStyle);
    if (key == L"engine") return pick(kEngines, ARRAYSIZE(kEngines), (int)g_settings.engine);
    if (key == L"weighting") return pick(kWeightings, ARRAYSIZE(kWeightings), (int)g_settings.weighting);
    if (key == L"ballistics") return pick(kBallistics, ARRAYSIZE(kBallistics), (int)g_settings.ballistics);
    if (key == L"workload") return pick(kWorkloads, ARRAYSIZE(kWorkloads), (int)g_settings.workload);
    if (key == L"renderer") return pick(kRenderers, ARRAYSIZE(kRenderers), (int)g_settings.renderer);
    if (key == L"targetFps") return std::to_wstring(g_settings.targetFps);
    if (key == L"bandLayout") {
        if (g_settings.bandLayout == VizBandLayout::Scale) return L"scale";
        return std::wstring(g_settings.bandLayout == VizBandLayout::Iec ? L"iec:" : L"musical:") +
               std::to_wstring(g_settings.octaveFraction);
    }
    if (key == L"readout") {
        if (!g_settings.peakFreqEnabled) return L"off";
        return g_settings.readout == VizReadout::Loudness       ? L"loudness"
               : g_settings.readout == VizReadout::LoudnessFull ? L"loudness_full"
               : g_settings.readout == VizReadout::Both         ? L"both"
                                                                : L"frequency";
    }
    return L"";
}

enum : UINT {
    kMenuToggleBase = 100,  // + index into kToggles
    kMenuPause = 190,
    kMenuReset = 191,
    kMenuCopy = 192,
    kMenuDefaultOut = 200,
    kMenuDefaultIn = 201,
    kMenuDeviceBase = 300,   // + endpoint index
    kMenuChoiceBase = 1000,  // + group * 100 + option
};
struct VizMenuGroup {
    const wchar_t* key;
    const wchar_t* label;
    const VizMenuOption* opts;
    size_t n;
};
static const VizMenuGroup kGroups[] = {
    {L"shape", L"Shape", kShapes, ARRAYSIZE(kShapes)},
    {L"termStyle", L"Terminal Style", kTermStyles, ARRAYSIZE(kTermStyles)},
    {L"colorMode", L"Color Mode", kColorModes, ARRAYSIZE(kColorModes)},
    {L"engine", L"Analysis Engine", kEngines, ARRAYSIZE(kEngines)},
    {L"bandLayout", L"Band Layout", kLayouts, ARRAYSIZE(kLayouts)},
    {L"weighting", L"Weighting", kWeightings, ARRAYSIZE(kWeightings)},
    {L"ballistics", L"Ballistics", kBallistics, ARRAYSIZE(kBallistics)},
    {L"readout", L"Readout", kReadouts, ARRAYSIZE(kReadouts)},
    {L"workload", L"Workload", kWorkloads, ARRAYSIZE(kWorkloads)},
    {L"renderer", L"Renderer", kRenderers, ARRAYSIZE(kRenderers)},
    {L"targetFps", L"Target FPS", kFps, ARRAYSIZE(kFps)},
};
struct VizMenuToggle {
    const wchar_t* key;
    const wchar_t* label;
    bool* field;
};

// Windhawk has no way for a mod to open or fill in its settings page, so this
// puts the active quick settings on the clipboard as "key = value" lines to
// carry over by hand. Keys and values are written as the menu shows them
// ("Peak Hold Caps = On", "Audio Source = <device name>"), close to the
// settings page's own labels, not as the internal override strings.
static void VizCopyMenuOverrides(const VizMenuToggle* toggles, size_t nToggles,
                                 const std::vector<VizAudioEndpoint>& eps) {
    std::wstring s;
    for (const auto& kv : g_menuOverrides) {
        std::wstring key = kv.first, value = kv.second;
        for (const VizMenuGroup& grp : kGroups) {
            if (kv.first != grp.key) continue;
            key = grp.label;
            for (size_t i = 0; i < grp.n; i++)
                if (kv.second == grp.opts[i].value) value = grp.opts[i].label;
        }
        for (size_t i = 0; i < nToggles; i++)
            if (kv.first == toggles[i].key) {
                key = toggles[i].label;
                value = (kv.second == L"1") ? L"On" : L"Off";
            }
        if (kv.first == L"audioSource") {
            key = L"Audio Source";
            if (kv.second == L"default_output") value = L"Default output";
            else if (kv.second == L"default_input") value = L"Default input";
            else
                for (const auto& ep : eps)
                    if (kv.second == L"id:" + ep.id) value = ep.name;
        }
        s += key + L" = " + value + L"\r\n";
    }
    if (s.empty() || !OpenClipboard(g_messageWnd)) return;
    EmptyClipboard();
    const size_t bytes = (s.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool owned = false;  // true once the clipboard owns `mem`
    if (mem) {
        if (void* p = GlobalLock(mem)) {
            memcpy(p, s.c_str(), bytes);
            GlobalUnlock(mem);
            owned = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
        }
        if (!owned) GlobalFree(mem);
    }
    CloseClipboard();
}

void VizShowContextMenu(POINT pt) {
    // One menu at a time: a second request (e.g. the media strip, posted while
    // this menu's modal loop is dispatching messages) would nest and fail.
    if (!g_messageWnd || g_menuOpen.load(std::memory_order_acquire)) return;
    const VizMenuToggle toggles[] = {
        {L"peakHold", L"Peak Hold Caps", &g_settings.peakHoldEnabled},
        {L"beatFlash", L"Beat Flash", &g_settings.beatFlashEnabled},
        {L"nowPlaying", L"Now Playing Text", &g_settings.nowPlayingEnabled},
        {L"progress", L"Track Progress Bar", &g_settings.progressEnabled},
        {L"mediaControls", L"Media Controls", &g_settings.mediaControlsEnabled},
        {L"pixelText", L"Pixel-Sharp Text", &g_settings.textPixel},
        {L"pixelSnap", L"Pixel Snap (sharp edges)", &g_settings.pixelSnap},
        {L"fineNudge", L"Subpixel Nudges (keyboard)", &g_settings.keyMoveFine},
    };

    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, L"Tourne'Table");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Audio source.
    ComPtr<IMMDeviceEnumerator> en;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                     (void**)en.GetAddressOf());
    std::vector<VizAudioEndpoint> eps = VizListAudioEndpoints(en.Get());
    const std::wstring& src = g_settings.audioSourceKey;
    HMENU dev = CreatePopupMenu();
    AppendMenuW(dev, MF_STRING | ((src.empty() || src == L"default_output") ? MF_CHECKED : 0), kMenuDefaultOut,
                L"Default output (what you hear)");
    AppendMenuW(dev, MF_STRING | (src == L"default_input" ? MF_CHECKED : 0), kMenuDefaultIn, L"Default input");
    bool any[2] = {false, false};
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < eps.size() && i < 600; i++) {
            if (eps[i].render != (pass == 0)) continue;
            if (!any[pass]) {
                AppendMenuW(dev, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(dev, MF_STRING | MF_GRAYED, 0,
                            pass == 0 ? L"Outputs (captured by loopback)" : L"Inputs (mics, line-in, virtual cables)");
                any[pass] = true;
            }
            bool on = src == L"id:" + eps[i].id;
            AppendMenuW(dev, MF_STRING | (on ? MF_CHECKED : 0), kMenuDeviceBase + (UINT)i, eps[i].name.c_str());
        }
    }
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)dev, L"Audio Source");

    for (size_t gi = 0; gi < ARRAYSIZE(kGroups); gi++) {
        const VizMenuGroup& grp = kGroups[gi];
        if (wcscmp(grp.key, L"termStyle") == 0 && g_settings.shape != VizShape::Terminal) continue;
        std::wstring cur = CurrentValue(grp.key);
        HMENU sub = CreatePopupMenu();
        for (size_t oi = 0; oi < grp.n; oi++)
            AppendMenuW(sub, MF_STRING | (cur == grp.opts[oi].value ? MF_CHECKED : 0),
                        kMenuChoiceBase + (UINT)(gi * 100 + oi), grp.opts[oi].label);
        AppendMenuW(menu, MF_POPUP, (UINT_PTR)sub, grp.label);
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    for (size_t ti = 0; ti < ARRAYSIZE(toggles); ti++)
        AppendMenuW(menu, MF_STRING | (*toggles[ti].field ? MF_CHECKED : 0), kMenuToggleBase + (UINT)ti,
                    toggles[ti].label);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (g_userPaused.load() ? MF_CHECKED : 0), kMenuPause, L"Pause Visualizer");
    std::wstring reset = L"Reset Quick Settings";
    if (!g_menuOverrides.empty()) reset += L" (" + std::to_wstring(g_menuOverrides.size()) + L" active)";
    AppendMenuW(menu, MF_STRING | (g_menuOverrides.empty() ? MF_GRAYED : 0), kMenuCopy, L"Copy Quick Settings");
    AppendMenuW(menu, MF_STRING | (g_menuOverrides.empty() ? MF_GRAYED : 0), kMenuReset, reset.c_str());

    // A menu only closes on an outside click when its owner is foreground.
    // The right-click went to the desktop (the hook swallowed it), so borrow
    // the foreground thread's input state long enough to take foreground.
    HWND fg = GetForegroundWindow();
    DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
    DWORD me = GetCurrentThreadId();
    bool attached = fgThread && fgThread != me && AttachThreadInput(me, fgThread, TRUE);
    SetForegroundWindow(g_messageWnd);
    if (attached) AttachThreadInput(me, fgThread, FALSE);

    // While the menu is up the mouse hook passes every click through, so a
    // right-click elsewhere closes it instead of being swallowed.
    g_menuOpen.store(true, std::memory_order_release);
    UINT cmd = (UINT)TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, g_messageWnd,
                                      nullptr);
    g_menuOpen.store(false, std::memory_order_release);
    PostMessage(g_messageWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);  // destroys the submenus with it
    if (!cmd) return;

    if (cmd == kMenuCopy) {
        VizCopyMenuOverrides(toggles, ARRAYSIZE(toggles), eps);
        return;
    } else if (cmd == kMenuReset) {
        g_menuOverrides.clear();
        VizSaveMenuOverrides();
    } else if (cmd == kMenuPause) {
        g_userPaused.store(!g_userPaused.load());
        if (g_userPaused.load()) PauseForFullscreen();
        else ResumeFromFullscreen();
        return;
    } else if (cmd == kMenuDefaultOut) {
        VizSetMenuOverride(L"audioSource", L"default_output");
    } else if (cmd == kMenuDefaultIn) {
        VizSetMenuOverride(L"audioSource", L"default_input");
    } else if (cmd >= kMenuDeviceBase && cmd < kMenuDeviceBase + eps.size()) {
        VizSetMenuOverride(L"audioSource", L"id:" + eps[cmd - kMenuDeviceBase].id);
    } else if (cmd >= kMenuToggleBase && cmd < kMenuToggleBase + ARRAYSIZE(toggles)) {
        const VizMenuToggle& t = toggles[cmd - kMenuToggleBase];
        VizSetMenuOverride(t.key, *t.field ? L"0" : L"1");
    } else if (cmd >= kMenuChoiceBase) {
        UINT gi = (cmd - kMenuChoiceBase) / 100, oi = (cmd - kMenuChoiceBase) % 100;
        if (gi >= ARRAYSIZE(kGroups) || oi >= kGroups[gi].n) return;
        VizSetMenuOverride(kGroups[gi].key, kGroups[gi].opts[oi].value);
    } else {
        return;
    }
    ApplySettingsChanged();
}
