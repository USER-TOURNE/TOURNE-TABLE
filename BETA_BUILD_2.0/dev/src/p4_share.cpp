// ---- Sharing setups (2.3) ----
// A setup is plain text anyone can read: a first line, its name and the parts it carries, then the
// settings in those parts that differ from the defaults, then quick settings as quick.<key>. It goes
// around as a file (.ttstyle), as a one-line code (TT2: and the text packed, for chats), or through
// the community list (a folder of setups on GitHub, read only when its button is pressed). The audio
// device, hotkeys, file paths, hardware and how it is moved or dragged never go in one.

enum : unsigned { kPartLook = 1, kPartMotion = 2, kPartExtras = 4, kPartPlace = 8, kPartsAll = 15 };
struct VizSetupKey {
    const wchar_t* name;
    const wchar_t* def;
    bool text;
    unsigned part;
};
@@SETUP_KEYS@@

struct VizQuickPart {
    const wchar_t* key;
    unsigned part;
};
static const VizQuickPart kQuickParts[] = {
    {L"shape", kPartLook},        {L"colorMode", kPartLook},    {L"color", kPartLook},       {L"grad1", kPartLook},
    {L"grad2", kPartLook},        {L"peakHold", kPartLook},     {L"beatFlash", kPartLook},   {L"barWidth", kPartLook},
    {L"barGap", kPartLook},       {L"barMaxSize", kPartLook},   {L"barRadius", kPartLook},   {L"reflection", kPartLook},
    {L"fxGlow", kPartLook},       {L"fxGlowRadius", kPartLook}, {L"fxBloom", kPartLook},     {L"fxBloomRadius", kPartLook},
    {L"termStyle", kPartLook},    {L"pixelText", kPartLook},    {L"engine", kPartMotion},    {L"bandLayout", kPartMotion},
    {L"weighting", kPartMotion},  {L"ballistics", kPartMotion}, {L"targetFps", kPartMotion}, {L"nowPlaying", kPartExtras},
    {L"progress", kPartExtras},   {L"mediaControls", kPartExtras}, {L"readout", kPartExtras}, {L"pixelSnap", kPartPlace},
};

static const wchar_t kCommunityFiles[] = L"https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/community/setups/";
static const wchar_t kCommunityFolder[] = L"https://github.com/USER-TOURNE/TOURNE-TABLE/tree/main/community/setups";
static const wchar_t kCommunityIssue[] = L"https://github.com/USER-TOURNE/TOURNE-TABLE/issues/new";

unsigned VizQuickPartOf(const std::wstring& key) {
    for (const auto& q : kQuickParts)
        if (key == q.key) return q.part;
    return 0;  // (audio source, hardware, nudging: the person's own)
}

const VizSetupKey* VizSetupKeyOf(const std::wstring& name) {
    for (const auto& k : kSetupKeys)
        if (name == k.name) return &k;
    return nullptr;
}

std::wstring VizPartsText(unsigned parts) {
    std::wstring s;
    const wchar_t* names[] = {L"look", L"motion", L"extras", L"place"};
    for (int i = 0; i < 4; i++)
        if (parts & (1u << i)) s += (s.empty() ? L"" : L",") + std::wstring(names[i]);
    return s;
}

unsigned VizPartsFrom(const std::wstring& s) {
    unsigned parts = 0;
    const wchar_t* names[] = {L"look", L"motion", L"extras", L"place"};
    for (int i = 0; i < 4; i++)
        if (s.find(names[i]) != std::wstring::npos) parts |= 1u << i;
    return parts;
}

std::wstring VizPartsWords(unsigned parts) {
    std::wstring s;
    const wchar_t* names[] = {L"Look", L"Motion", L"Extras", L"Place"};
    for (int i = 0; i < 4; i++)
        if (parts & (1u << i)) s += (s.empty() ? L"" : L", ") + std::wstring(names[i]);
    return s;
}

std::string VizShareUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n > 0 ? n : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring VizShareWide(const std::string& s) {
    size_t skip = s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF ? 3 : 0;
    if (s.size() <= skip) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data() + skip, (int)(s.size() - skip), nullptr, 0);
    std::wstring w(n > 0 ? n : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.data() + skip, (int)(s.size() - skip), w.data(), n);
    return w;
}

std::wstring VizOneLine(std::wstring v) {
    for (auto& c : v)
        if (c == L'\r' || c == L'\n') c = L' ';
    return v.substr(0, 1024);
}

// What a setting is now (an imported setup's value, else the settings page's), as text.
std::wstring VizSettingNow(const VizSetupKey& k) {
    if (!k.text) return std::to_wstring(Wh_GetIntSetting(k.name));
    PCWSTR s = Wh_GetStringSetting(k.name);
    std::wstring v = s ? s : L"";
    Wh_FreeStringSetting(s);
    if (v.empty() && *k.def) {  // (a size saved as a whole number before 2.1)
        int n = Wh_GetIntSetting(k.name);
        if (n) v = std::to_wstring(n);
    }
    return v;
}

std::wstring VizExportSetup(unsigned parts, const std::wstring& name) {
    std::wstring t = L"Tourne'Table setup 1\n";
    if (!name.empty()) t += L"name=" + VizOneLine(name) + L"\n";
    t += L"parts=" + VizPartsText(parts) + L"\n";
    for (const auto& k : kSetupKeys) {
        if (!(k.part & parts)) continue;
        std::wstring v = VizSettingNow(k);
        bool same = k.text ? v == k.def : _wtoi(v.c_str()) == _wtoi(k.def);
        if (!same) t += std::wstring(k.name) + L"=" + VizOneLine(v) + L"\n";
    }
    for (const auto& kv : g_menuOverrides)
        if (VizQuickPartOf(kv.first) & parts) t += L"quick." + kv.first + L"=" + VizOneLine(kv.second) + L"\n";
    return t;
}

struct VizSetup {
    bool ok = false;
    std::wstring name, author;
    unsigned parts = 0;
    std::vector<std::pair<std::wstring, std::wstring>> values;
};

// Reads a setup out of text: a file, a code's contents, or a GitHub issue with it pasted in (lines
// before its first line, ``` fences and # notes are skipped).
VizSetup VizParseSetup(const std::wstring& text) {
    VizSetup s;
    bool started = false;
    for (size_t pos = 0; pos < text.size();) {
        size_t nl = text.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = text.size();
        std::wstring line = text.substr(pos, nl - pos);
        pos = nl + 1;
        while (!line.empty() && iswspace(line.back())) line.pop_back();
        while (!line.empty() && iswspace(line.front())) line.erase(line.begin());
        if (!started) {
            started = line.rfind(L"Tourne'Table setup", 0) == 0;
            s.ok = started;
            continue;
        }
        if (line.empty() || line[0] == L'#' || line.rfind(L"```", 0) == 0) continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos || eq == 0) continue;
        std::wstring key = line.substr(0, eq), value = line.substr(eq + 1, 1024);
        if (key == L"name") s.name = value.substr(0, 60);
        else if (key == L"author") s.author = value.substr(0, 40);
        else if (key == L"parts") s.parts = VizPartsFrom(value);
        else if (s.values.size() < 2000) s.values.emplace_back(key, value);
    }
    if (s.ok && !s.parts) s.parts = kPartLook;
    return s;
}

// Lays a setup's parts over the settings: each setting in them takes the setup's value or the default,
// and quick settings in them give way to the setup's own.
void VizApplySetup(const VizSetup& s, unsigned parts) {
    if (!g_setupLayerLoaded) VizSetupLoad();
    for (const auto& k : kSetupKeys)
        if (k.part & parts) g_setupLayer[k.name] = k.def;
    g_menuOverrides.erase(std::remove_if(g_menuOverrides.begin(), g_menuOverrides.end(),
                                         [&](const auto& kv) { return (VizQuickPartOf(kv.first) & parts) != 0; }),
                          g_menuOverrides.end());
    for (const auto& kv : s.values) {
        if (kv.first.rfind(L"quick.", 0) == 0) {
            std::wstring key = kv.first.substr(6);
            if (VizQuickPartOf(key) & parts) g_menuOverrides.emplace_back(key, kv.second);
        } else if (const VizSetupKey* k = VizSetupKeyOf(kv.first)) {
            if (k->part & parts) g_setupLayer[k->name] = kv.second;
        }
    }
    VizSetupSave();
    VizSaveMenuOverrides();
}

std::wstring VizBase64(const std::string& in) {
    static const char* abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::wstring out;
    for (size_t i = 0; i < in.size(); i += 3) {
        unsigned v = (unsigned char)in[i] << 16;
        if (i + 1 < in.size()) v |= (unsigned char)in[i + 1] << 8;
        if (i + 2 < in.size()) v |= (unsigned char)in[i + 2];
        out += abc[(v >> 18) & 63];
        out += abc[(v >> 12) & 63];
        if (i + 1 < in.size()) out += abc[(v >> 6) & 63];
        if (i + 2 < in.size()) out += abc[v & 63];
    }
    return out;
}

bool VizUnbase64(const std::wstring& in, std::string* out) {
    unsigned v = 0;
    int bits = 0;
    for (wchar_t c : in) {
        int d = c >= L'A' && c <= L'Z' ? c - L'A' : c >= L'a' && c <= L'z' ? c - L'a' + 26 : c >= L'0' && c <= L'9' ? c - L'0' + 52
              : c == L'-' || c == L'+' ? 62 : c == L'_' || c == L'/' ? 63 : -1;
        if (d < 0) {
            if (iswspace(c) || c == L'=') continue;
            return false;
        }
        v = (v << 6) | d;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out->push_back((char)((v >> bits) & 0xFF));
        }
    }
    return true;
}

// Windows' own compression (XPRESS with Huffman): a setup's text packs to about a third.
bool VizPack(const std::string& in, std::string* out) {
    COMPRESSOR_HANDLE c = nullptr;
    if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &c)) return false;
    SIZE_T need = 0;
    Compress(c, (PVOID)in.data(), in.size(), nullptr, 0, &need);  // (PVOID: MinGW's header isn't const)
    out->resize(need);
    bool ok = need && Compress(c, (PVOID)in.data(), in.size(), out->data(), need, &need);
    if (ok) out->resize(need);
    CloseCompressor(c);
    return ok;
}

bool VizUnpack(const std::string& in, std::string* out) {
    DECOMPRESSOR_HANDLE d = nullptr;
    if (!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &d)) return false;
    SIZE_T need = 0;
    Decompress(d, (PVOID)in.data(), in.size(), nullptr, 0, &need);
    bool ok = need && need <= (1 << 20);
    if (ok) {
        out->resize(need);
        ok = Decompress(d, (PVOID)in.data(), in.size(), out->data(), need, &need);
        if (ok) out->resize(need);
    }
    CloseDecompressor(d);
    return ok;
}

std::wstring VizShareCode(const std::wstring& text) {
    std::string raw = VizShareUtf8(text), packed;
    if (VizPack(raw, &packed)) return L"TT2:" + VizBase64(packed);
    return L"TT2T:" + VizBase64(raw);  // (unpacked: longer, the same setup)
}

// A code back to its text; anything else is taken as the text itself.
std::wstring VizSetupText(std::wstring given) {
    while (!given.empty() && iswspace(given.front())) given.erase(given.begin());
    while (!given.empty() && iswspace(given.back())) given.pop_back();
    std::string bytes, text;
    if (given.rfind(L"TT2:", 0) == 0 && VizUnbase64(given.substr(4), &bytes) && VizUnpack(bytes, &text)) return VizShareWide(text);
    if (given.rfind(L"TT2T:", 0) == 0 && VizUnbase64(given.substr(5), &bytes)) return VizShareWide(bytes);
    return given;
}

void VizClipboardSet(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (mem) {
        memcpy(GlobalLock(mem), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(mem);
        if (!SetClipboardData(CF_UNICODETEXT, mem)) GlobalFree(mem);
    }
    CloseClipboard();
}

std::wstring VizClipboardGet(HWND owner) {
    std::wstring out;
    if (!OpenClipboard(owner)) return out;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
        if (const wchar_t* p = (const wchar_t*)GlobalLock(h)) {
            out.assign(p, wcsnlen(p, 1 << 20));
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return out;
}

// Imports a setup (a code, a file's text, a community one) into the parts both it and the ticked boxes
// have, shows it, and keeps it only if asked to; otherwise everything goes back as it was.
bool VizImportSetup(HWND owner, const std::wstring& given, unsigned boxes) {
    VizSetup s = VizParseSetup(VizSetupText(given));
    if (!s.ok) {
        MessageBoxW(owner, L"That isn't a Tourne'Table setup. Copy a TT2: code, or open a .ttstyle file.", L"Tourne'Table",
                    MB_OK | MB_ICONWARNING);
        return false;
    }
    unsigned parts = s.parts & boxes;
    if (!parts) {
        std::wstring why = L"This setup carries " + VizPartsWords(s.parts) + L", and none of those boxes is ticked.";
        MessageBoxW(owner, why.c_str(), L"Tourne'Table", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    if (!g_setupLayerLoaded) VizSetupLoad();
    auto layer = g_setupLayer;
    auto quick = g_menuOverrides;
    VizApplySetup(s, parts);
    ApplySettingsChanged();
    std::wstring ask = L"Keep " + (s.name.empty() ? std::wstring(L"this setup") : L"\"" + s.name + L"\"") +
                       (s.author.empty() ? L"" : L" by " + s.author) + L"?\n\nIt changed " + VizPartsWords(parts) +
                       L". No puts yours back.";
    if (MessageBoxW(owner, ask.c_str(), L"Tourne'Table", MB_YESNO | MB_ICONQUESTION) == IDYES) return true;
    g_setupLayer = layer;
    g_menuOverrides = quick;
    VizSetupSave();
    VizSaveMenuOverrides();
    ApplySettingsChanged();
    return false;
}

bool VizSaveSetupFile(HWND owner, const std::wstring& text, const std::wstring& name) {
    WCHAR path[MAX_PATH] = {};
    wcsncpy_s(path, (name.empty() ? std::wstring(L"My setup") : name).c_str(), _TRUNCATE);
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Tourne'Table setups (*.ttstyle)\0*.ttstyle\0All files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"ttstyle";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return false;
    std::string bytes = VizShareUtf8(text);
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD put = 0;
    bool ok = WriteFile(f, bytes.data(), (DWORD)bytes.size(), &put, nullptr) && put == bytes.size();
    CloseHandle(f);
    return ok;
}

bool VizOpenSetupFile(HWND owner, std::wstring* text) {
    WCHAR path[MAX_PATH] = {};
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Tourne'Table setups (*.ttstyle)\0*.ttstyle\0Text\0*.txt\0All files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return false;
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    std::string bytes(256 * 1024, '\0');
    DWORD got = 0;
    bool ok = ReadFile(f, bytes.data(), (DWORD)bytes.size(), &got, nullptr);
    CloseHandle(f);
    bytes.resize(got);
    *text = VizShareWide(bytes);
    return ok;
}

// ---- The community list: setups in a folder of the TOURNE-TABLE repository on GitHub ----
// index.txt there has a line per setup, "file|name|author|about". Nothing is fetched until the
// Community button is pressed, and then only those small text files.
struct VizCommunityItem {
    std::wstring file, name, author;
};
constexpr UINT WM_APP_SHARE_LIST = WM_APP + 60;   // wParam: reached; lParam: std::vector<VizCommunityItem>*
constexpr UINT WM_APP_SHARE_SETUP = WM_APP + 61;  // lParam: std::wstring* (empty: not reached)

bool VizFetchText(const std::wstring& url, std::wstring* out) {
    IStream* stream = nullptr;
    if (url.rfind(L"https://", 0) != 0 || FAILED(URLOpenBlockingStreamW(nullptr, url.c_str(), &stream, 0, nullptr)) || !stream)
        return false;
    std::string data;
    char buf[8192];
    for (;;) {
        ULONG got = 0;
        HRESULT hr = stream->Read(buf, sizeof(buf), &got);
        if (FAILED(hr) || !got) break;
        data.append(buf, got);
        if (data.size() > 256 * 1024) break;
    }
    stream->Release();
    if (data.empty() || data.size() > 256 * 1024) return false;
    *out = VizShareWide(data);
    return true;
}

bool VizCommunityFileName(const std::wstring& f) {
    if (f.size() < 9 || f.size() > 80 || f.compare(f.size() - 8, 8, L".ttstyle") != 0) return false;
    for (wchar_t c : f)
        if (!(iswalnum(c) || c == L'-' || c == L'_' || c == L'.')) return false;
    return f.find(L"..") == std::wstring::npos;
}

void VizFetchCommunityList(HWND notify) {
    std::thread([notify] {
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        auto* items = new std::vector<VizCommunityItem>;
        std::wstring text;
        bool ok = VizFetchText(std::wstring(kCommunityFiles) + L"index.txt?t=" + std::to_wstring(GetTickCount64()), &text);
        for (size_t pos = 0; ok && pos < text.size() && items->size() < 200;) {
            size_t nl = text.find(L'\n', pos);
            if (nl == std::wstring::npos) nl = text.size();
            std::wstring line = text.substr(pos, nl - pos);
            pos = nl + 1;
            while (!line.empty() && iswspace(line.back())) line.pop_back();
            if (line.empty() || line[0] == L'#') continue;
            std::wstring parts[4];
            size_t at = 0;
            for (int i = 0; i < 4; i++) {
                size_t bar = i < 3 ? line.find(L'|', at) : std::wstring::npos;
                parts[i] = line.substr(at, bar == std::wstring::npos ? std::wstring::npos : bar - at);
                if (bar == std::wstring::npos) break;
                at = bar + 1;
            }
            if (VizCommunityFileName(parts[0])) items->push_back({parts[0], parts[1].substr(0, 60), parts[2].substr(0, 40)});
        }
        if (SUCCEEDED(com)) CoUninitialize();
        if (!PostMessageW(notify, WM_APP_SHARE_LIST, ok, (LPARAM)items)) delete items;
    }).detach();
}

void VizFetchCommunitySetup(HWND notify, const std::wstring& file) {
    std::thread([notify, file] {
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        auto* text = new std::wstring;
        if (!VizFetchText(std::wstring(kCommunityFiles) + file, text)) text->clear();
        if (SUCCEEDED(com)) CoUninitialize();
        if (!PostMessageW(notify, WM_APP_SHARE_SETUP, 0, (LPARAM)text)) delete text;
    }).detach();
}

std::wstring VizUrlEncode(const std::wstring& w) {
    std::string s = VizShareUtf8(w);
    std::wstring out;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (wchar_t)c;
        } else {
            wchar_t b[4];
            swprintf(b, 4, L"%%%02X", c);
            out += b;
        }
    }
    return out;
}

// Opens a new GitHub issue with the setup in it, for the person to send; Tourne adds the good ones
// to the community folder.
void VizShareToCommunity(HWND owner, unsigned parts, const std::wstring& name) {
    std::wstring text = VizExportSetup(parts, name);
    std::wstring head = std::wstring(kCommunityIssue) + L"?title=" + VizUrlEncode(L"Setup: " + name) + L"&body=";
    std::wstring url = head + VizUrlEncode(L"Made in Tourne'Table. A screenshot helps people pick it.\n\n```\n" + text + L"```\n");
    if (url.size() > 7000) {  // (too long for a link: it goes on the clipboard instead)
        VizClipboardSet(owner, text);
        url = head + VizUrlEncode(L"Made in Tourne'Table. Paste the setup here (it's on your clipboard):\n\n```\n\n```\n");
    }
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

