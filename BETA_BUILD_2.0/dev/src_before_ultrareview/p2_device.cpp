// ---- Audio source ---------------------------------------------------------------------
//
// Up to 2.0 the mod always listened to the default playback device. Anyone who
// routes audio through virtual devices (VB-Audio Matrix / Voicemeeter / VAIO
// cables, an Ableton return on its own device, ...) had nothing to show, since
// the default device was silent. Now any active endpoint can be the source:
//
//   * an output device, captured the way the default one always was
//     (WASAPI loopback: what is being played to it), or
//   * an input device: a microphone, a line-in, or the capture side of a
//     virtual cable, read as an ordinary recording stream.
//
// The source is a short key string:
//   ""  / "default_output"   the default playback device (the old behaviour)
//   "default_input"          the default recording device
//   "id:<endpoint id>"       one exact device (what the right-click menu stores)
//   "name:<text>"            the first device whose name contains <text>
//                            (what the Windhawk setting stores, since a person
//                            can type a name but not an endpoint id)

static const PROPERTYKEY kPKEY_DeviceFriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

std::wstring VizDeviceName(IMMDevice* d) {
    std::wstring out;
    ComPtr<IPropertyStore> ps;
    if (!d || FAILED(d->OpenPropertyStore(STGM_READ, &ps)) || !ps) return out;
    PROPVARIANT v;
    PropVariantInit(&v);
    if (SUCCEEDED(ps->GetValue(kPKEY_DeviceFriendlyName, &v)) && v.vt == VT_LPWSTR && v.pwszVal) out = v.pwszVal;
    PropVariantClear(&v);
    return out;
}

std::wstring VizDeviceId(IMMDevice* d) {
    std::wstring out;
    LPWSTR id = nullptr;
    if (d && SUCCEEDED(d->GetId(&id)) && id) {
        out = id;
        CoTaskMemFree(id);
    }
    return out;
}

bool VizDeviceIsRender(IMMDevice* d) {
    ComPtr<IMMEndpoint> ep;
    EDataFlow flow = eRender;
    if (d && SUCCEEDED(d->QueryInterface(__uuidof(IMMEndpoint), (void**)ep.GetAddressOf())) && ep)
        ep->GetDataFlow(&flow);
    return flow == eRender;
}

static bool ContainsNoCase(const std::wstring& hay, const std::wstring& needle) {
    if (needle.empty()) return false;
    auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
                          [](wchar_t a, wchar_t b) { return towlower(a) == towlower(b); });
    return it != hay.end();
}

struct VizAudioEndpoint {
    std::wstring id, name;
    bool render = true;
};

// Every active endpoint, outputs first. Used by the right-click menu.
std::vector<VizAudioEndpoint> VizListAudioEndpoints(IMMDeviceEnumerator* e) {
    std::vector<VizAudioEndpoint> out;
    if (!e) return out;
    for (EDataFlow flow : {eRender, eCapture}) {
        ComPtr<IMMDeviceCollection> col;
        if (FAILED(e->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &col)) || !col) continue;
        UINT n = 0;
        col->GetCount(&n);
        for (UINT i = 0; i < n; i++) {
            ComPtr<IMMDevice> d;
            if (FAILED(col->Item(i, &d)) || !d) continue;
            VizAudioEndpoint ep;
            ep.id = VizDeviceId(d.Get());
            ep.name = VizDeviceName(d.Get());
            ep.render = (flow == eRender);
            if (ep.name.empty()) ep.name = ep.id;
            out.push_back(ep);
        }
    }
    return out;
}

// Resolves a source key to a device. Falls back to the default output (and
// says so through *fellBack) when the asked-for device isn't there, so a
// device that was unplugged or renamed leaves the visualizer working.
ComPtr<IMMDevice> VizResolveAudioDevice(IMMDeviceEnumerator* e, const std::wstring& key, bool* loopback,
                                        bool* fellBack) {
    ComPtr<IMMDevice> d;
    *fellBack = false;
    *loopback = true;
    if (!e) return d;
    if (key == L"default_input") {
        if (SUCCEEDED(e->GetDefaultAudioEndpoint(eCapture, eConsole, &d)) && d) {
            *loopback = false;
            return d;
        }
        *fellBack = true;
    } else if (key.rfind(L"id:", 0) == 0) {
        if (SUCCEEDED(e->GetDevice(key.c_str() + 3, &d)) && d) {
            DWORD state = 0;
            if (SUCCEEDED(d->GetState(&state)) && state == DEVICE_STATE_ACTIVE) {
                *loopback = VizDeviceIsRender(d.Get());
                return d;
            }
        }
        d.Reset();
        *fellBack = true;
    } else if (key.rfind(L"name:", 0) == 0) {
        std::wstring want = key.substr(5);
        for (const auto& ep : VizListAudioEndpoints(e)) {
            if (ContainsNoCase(ep.name, want) && SUCCEEDED(e->GetDevice(ep.id.c_str(), &d)) && d) {
                *loopback = ep.render;
                return d;
            }
        }
        d.Reset();
        *fellBack = true;
    }
    if (SUCCEEDED(e->GetDefaultAudioEndpoint(eRender, eConsole, &d))) *loopback = true;
    return d;
}

// The source in effect, as the engine thread reads it. Written on the UI
// thread by VizPublishAudioSource (from LoadSettings); a change flags a
// reopen the same way a device change does.
std::mutex g_audioSourceMutex;
std::wstring g_audioSourceKeyShared;
std::atomic<bool> g_audioOnFallback{false};

std::wstring VizAudioSourceKey() {
    std::lock_guard<std::mutex> lock(g_audioSourceMutex);
    return g_audioSourceKeyShared;
}

// Whether a change of Windows' default device for `flow` affects the stream:
// only when the source is that default, or the chosen device is missing and
// the default output is standing in for it.
bool VizSourceFollowsDefault(EDataFlow flow) {
    std::wstring key = VizAudioSourceKey();
    if (flow == eCapture) return key == L"default_input";
    return key.empty() || key == L"default_output" || g_audioOnFallback.load(std::memory_order_relaxed);
}

// A readable name for a source key, for messages.
std::wstring VizAudioSourceLabel(const std::wstring& key) {
    if (key.empty() || key == L"default_output") return L"default output";
    if (key == L"default_input") return L"default input";
    if (key.rfind(L"name:", 0) == 0) return L"\"" + key.substr(5) + L"\"";
    return L"the device picked from the right-click menu";
}

// Reported through the message window as an Audio settings warning (the
// receiver frees the string).
void VizPostAudioNotice(const std::wstring& msg) {
    Wh_Log(L"[Audio] %s", msg.c_str());
    HWND wnd = g_messageWnd;
    if (!wnd) return;
    auto* heap = new std::wstring(msg);
    if (!PostMessage(wnd, WM_APP_HW_NOTICE, 1, (LPARAM)heap)) delete heap;
}
