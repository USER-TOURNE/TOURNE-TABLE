// ---- Media Card (2.1): output switching and volume -------------------------------------
// Windows has no public call to change the default output; IPolicyConfig is
// the interface the Sound control panel itself uses, unchanged since Windows
// 7 (only SetDefaultEndpoint is called; the slots before it keep the vtable
// order).
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8")
IVizPolicyConfig : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX*, WAVEFORMATEX*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR, ERole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};
static const CLSID kCLSID_VizPolicyConfigClient = {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
static const IID kIID_VizPolicyConfig = {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};

namespace {
// The default output's volume control, kept between calls and fetched again
// after a switch, a failure, or every ten seconds (so a default changed
// elsewhere is picked up). Media window thread only.
ComPtr<IAudioEndpointVolume> s_cardVol;
ULONGLONG s_cardVolTick = 0;

// COM on the calling thread for the duration of a call, if it isn't already.
struct VizComScope {
    bool owned;
    VizComScope() : owned(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {}
    ~VizComScope() {
        if (owned) CoUninitialize();
    }
};

IAudioEndpointVolume* CardVolume() {
    ULONGLONG now = GetTickCount64();
    if (s_cardVol && now - s_cardVolTick < 10000) return s_cardVol.Get();
    s_cardVol.Reset();
    s_cardVolTick = now;
    ComPtr<IMMDeviceEnumerator> e;
    ComPtr<IMMDevice> d;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&e))) ||
        FAILED(e->GetDefaultAudioEndpoint(eRender, eMultimedia, &d)) ||
        FAILED(d->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)s_cardVol.GetAddressOf())))
        s_cardVol.Reset();
    return s_cardVol.Get();
}
}  // namespace

float VizCardGetVolume() {
    VizComScope com;
    IAudioEndpointVolume* v = CardVolume();
    float level = -1.f;
    if (v && FAILED(v->GetMasterVolumeLevelScalar(&level))) {
        s_cardVol.Reset();
        level = -1.f;
    }
    return level;
}

void VizCardSetVolume(float level) {
    VizComScope com;
    IAudioEndpointVolume* v = CardVolume();
    if (v) v->SetMasterVolumeLevelScalar(std::clamp(level, 0.f, 1.f), nullptr);
}

// One click: the outputs, the current default ticked; picking one makes it
// the default for every role, as the Sound settings page does.
void VizCardOutputMenu(HWND hWnd, POINT pt) {
    VizComScope com;
    ComPtr<IMMDeviceEnumerator> e;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&e)))) return;
    std::wstring current;
    {
        ComPtr<IMMDevice> d;
        if (SUCCEEDED(e->GetDefaultAudioEndpoint(eRender, eMultimedia, &d))) current = VizDeviceId(d.Get());
    }
    std::vector<VizAudioEndpoint> outs;
    for (auto& ep : VizListAudioEndpoints(e.Get()))
        if (ep.render) outs.push_back(ep);
    if (outs.empty()) return;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    for (size_t i = 0; i < outs.size(); i++)
        AppendMenuW(menu, MF_STRING | (outs[i].id == current ? MF_CHECKED : 0), 1 + i, outs[i].name.c_str());
    SetForegroundWindow(hWnd);  // so a click elsewhere closes the menu
    UINT cmd = (UINT)TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN, pt.x, pt.y, hWnd, nullptr);
    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (cmd < 1 || cmd > outs.size() || outs[cmd - 1].id == current) return;
    ComPtr<IVizPolicyConfig> pc;
    if (FAILED(CoCreateInstance(kCLSID_VizPolicyConfigClient, nullptr, CLSCTX_ALL, kIID_VizPolicyConfig,
                                (void**)pc.GetAddressOf())) || !pc) {
        Wh_Log(L"[Media] output switch unavailable");
        return;
    }
    for (ERole role : {eConsole, eMultimedia, eCommunications}) pc->SetDefaultEndpoint(outs[cmd - 1].id.c_str(), role);
    s_cardVol.Reset();  // the volume now belongs to the new output
}
