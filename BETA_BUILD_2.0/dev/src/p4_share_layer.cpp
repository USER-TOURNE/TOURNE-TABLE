
// ---- Shared setups (2.3): the layer an imported setup sits in -------------------------------------
// Every setting is read through here. An imported setup's values (the mod value "sharedSetup", as
// key=value lines) come first, then the settings page; quick settings still go on top of both.
// Reset Quick Settings clears it, and the settings page's values come back.
std::map<std::wstring, std::wstring> g_setupLayer;
bool g_setupLayerLoaded = false;

void VizSetupLoad() {
    g_setupLayerLoaded = true;
    g_setupLayer.clear();
    std::vector<WCHAR> buf(32768);
    if (!Wh_GetStringValue(L"sharedSetup", buf.data(), buf.size())) return;
    std::wstring s = buf.data();
    for (size_t pos = 0; pos < s.size();) {
        size_t nl = s.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = s.size();
        std::wstring line = s.substr(pos, nl - pos);
        size_t eq = line.find(L'=');
        if (eq != std::wstring::npos && eq > 0) g_setupLayer[line.substr(0, eq)] = line.substr(eq + 1);
        pos = nl + 1;
    }
}

void VizSetupSave() {
    std::wstring s;
    for (const auto& kv : g_setupLayer) s += kv.first + L"=" + kv.second + L"\n";
    Wh_SetStringValue(L"sharedSetup", s.c_str());
}

const std::wstring* VizSetupFind(PCWSTR name) {
    if (!g_setupLayerLoaded) VizSetupLoad();
    auto it = g_setupLayer.find(name);
    return it == g_setupLayer.end() ? nullptr : &it->second;
}

int VizSettingInt(PCWSTR name) {
    if (const std::wstring* v = VizSetupFind(name)) return *v == L"true" ? 1 : *v == L"false" ? 0 : _wtoi(v->c_str());
    return Wh_GetIntSetting(name);
}

PCWSTR VizSettingString(PCWSTR name) {
    std::wstring v;
    if (const std::wstring* found = VizSetupFind(name)) {
        v = *found;
    } else if (PCWSTR s = Wh_GetStringSetting(name)) {
        v = s;
        Wh_FreeStringSetting(s);
    }
    WCHAR* out = new WCHAR[v.size() + 1];
    wcscpy_s(out, v.size() + 1, v.c_str());
    return out;
}

void VizSettingFree(PCWSTR s) { delete[] s; }

// From here on, settings come through the layer.
#define Wh_GetIntSetting VizSettingInt
#define Wh_GetStringSetting VizSettingString
#define Wh_FreeStringSetting VizSettingFree
