// ---- Style Editor and saved styles (2.1) -------------------------------------------------
// A small window, opened from the right-click menu (My Styles > Style
// Editor...), that builds a look out of the pieces every style is made of:
// the base style, colours, bar size and spacing, corner radius, peak caps,
// Reflection, Glow and Bloom. Every change goes through the same quick
// settings the menu uses and applies at once, so the visualizer itself is the
// live preview. A look can be saved under a name and comes back in one click
// from My Styles.
//
// Saved styles are mod-owned values like the quick settings: "stylePresets"
// lists the names, "stylePreset:<name>" holds one look as key=value lines.

HWND g_styleEditorWnd = nullptr;
static bool g_styleEditorClassRegistered = false;
static const wchar_t kStyleEditorClass[] = L"TourneTableStyleEditor";

// The keys that make up a look, and their current effective values.
static const wchar_t* const kLookKeys[] = {L"shape",      L"colorMode", L"color",       L"grad1",    L"grad2",
                                           L"peakHold",   L"beatFlash", L"barWidth",    L"barGap",   L"barMaxSize",
                                           L"barRadius",  L"reflection", L"fxGlow",     L"fxGlowRadius",
                                           L"fxBloom",    L"fxBloomRadius"};

static std::wstring VizHexColor(BYTE a, BYTE r, BYTE g, BYTE b) {
    wchar_t buf[16];
    swprintf(buf, 16, L"#%02X%02X%02X%02X", a, r, g, b);
    return buf;
}

static std::wstring VizLookValue(const std::wstring& key) {
    const Settings& s = g_settings;
    if (key == L"shape" || key == L"colorMode") return CurrentValue(key);
    if (key == L"color") return VizHexColor(s.colorA, s.colorR, s.colorG, s.colorB);
    if (key == L"grad1") return VizHexColor(s.grad1A, s.grad1R, s.grad1G, s.grad1B);
    if (key == L"grad2") return VizHexColor(s.grad2A, s.grad2R, s.grad2G, s.grad2B);
    if (key == L"peakHold") return s.peakHoldEnabled ? L"1" : L"0";
    if (key == L"beatFlash") return s.beatFlashEnabled ? L"1" : L"0";
    if (key == L"barWidth") return std::to_wstring(s.barWidth);
    if (key == L"barGap") return std::to_wstring(s.barGap);
    if (key == L"barMaxSize") return std::to_wstring(s.barMaxSize);
    if (key == L"barRadius") return std::to_wstring(s.barRadiusTL);
    if (key == L"reflection") return std::to_wstring(s.reflection);
    if (key == L"fxGlow") return std::to_wstring(s.fxGlow);
    if (key == L"fxGlowRadius") return std::to_wstring(s.fxGlowRadius);
    if (key == L"fxBloom") return std::to_wstring(s.fxBloom);
    if (key == L"fxBloomRadius") return std::to_wstring(s.fxBloomRadius);
    return L"";
}

// ---- Saved styles ----
static std::wstring VizReadValue(const std::wstring& name) {
    WCHAR buf[4096] = {};
    if (!Wh_GetStringValue(name.c_str(), buf, ARRAYSIZE(buf))) return L"";
    return buf;
}

std::vector<std::wstring> VizStylePresetNames() {
    std::vector<std::wstring> out;
    std::wstring s = VizReadValue(L"stylePresets");
    size_t pos = 0;
    while (pos < s.size()) {
        size_t nl = s.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = s.size();
        if (nl > pos) out.push_back(s.substr(pos, nl - pos));
        pos = nl + 1;
    }
    return out;
}

static void VizWritePresetNames(const std::vector<std::wstring>& names) {
    std::wstring s;
    for (const auto& n : names) s += n + L"\n";
    Wh_SetStringValue(L"stylePresets", s.c_str());
}

// Letters, digits, spaces, - and _, at most 40: safe as part of a value name.
static std::wstring VizCleanPresetName(const std::wstring& in) {
    std::wstring out;
    for (wchar_t c : in) {
        if (out.size() >= 40) break;
        if (iswalnum(c) || c == L' ' || c == L'-' || c == L'_') out += c;
    }
    while (!out.empty() && out.back() == L' ') out.pop_back();
    while (!out.empty() && out.front() == L' ') out.erase(out.begin());
    return out;
}

static bool VizSaveStylePreset(const std::wstring& rawName) {
    std::wstring name = VizCleanPresetName(rawName);
    if (name.empty()) return false;
    std::wstring body;
    for (const wchar_t* k : kLookKeys) body += std::wstring(k) + L"=" + VizLookValue(k) + L"\n";
    Wh_SetStringValue((L"stylePreset:" + name).c_str(), body.c_str());
    std::vector<std::wstring> names = VizStylePresetNames();
    if (std::find(names.begin(), names.end(), name) == names.end()) {
        names.push_back(name);
        VizWritePresetNames(names);
    }
    return true;
}

static void VizDeleteStylePreset(const std::wstring& name) {
    std::vector<std::wstring> names = VizStylePresetNames();
    names.erase(std::remove(names.begin(), names.end(), name), names.end());
    VizWritePresetNames(names);
    Wh_SetStringValue((L"stylePreset:" + name).c_str(), L"");
}

// Lays a saved look over the current settings, as quick settings.
bool VizApplyStylePreset(const std::wstring& name) {
    std::wstring body = VizReadValue(L"stylePreset:" + name);
    if (body.empty()) return false;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t nl = body.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = body.size();
        std::wstring line = body.substr(pos, nl - pos);
        size_t eq = line.find(L'=');
        if (eq != std::wstring::npos && eq > 0) {
            std::wstring k = line.substr(0, eq);
            for (const wchar_t* lk : kLookKeys)
                if (k == lk) VizSetMenuOverride(k, line.substr(eq + 1));
        }
        pos = nl + 1;
    }
    return true;
}

// ---- The editor window ----
namespace {
enum : int {
    kIdPreset = 100, kIdLoad, kIdSave, kIdDelete, kIdShape, kIdColorMode, kIdColor, kIdGrad1, kIdGrad2,
    kIdPeak, kIdBeat, kIdSliderBase = 200,  // + slider index; value labels at + 100
};
constexpr UINT_PTR kApplyTimer = 1;

struct EditorSlider {
    const wchar_t* key;
    const wchar_t* label;
    int lo, hi;
};
const EditorSlider kSliders[] = {
    {L"barWidth", L"Bar width", 1, 40},       {L"barGap", L"Bar gap", 0, 30},
    {L"barMaxSize", L"Height", 10, 400},      {L"barRadius", L"Corner radius", 0, 20},
    {L"reflection", L"Reflection", 0, 100},   {L"fxGlow", L"Glow", 0, 100},
    {L"fxGlowRadius", L"Glow radius", 1, 32}, {L"fxBloom", L"Bloom", 0, 100},
    {L"fxBloomRadius", L"Bloom radius", 4, 64},
};
HFONT s_editorFont = nullptr;
COLORREF s_customColors[16] = {};

// Settings changed from the editor: stored at once, applied at most every
// 80 ms while a slider is dragged (a full settings apply per mouse move
// would be wasted work), and at once on release.
void EditorApplySoon(HWND hWnd) { SetTimer(hWnd, kApplyTimer, 80, nullptr); }
void EditorApplyNow(HWND hWnd) {
    KillTimer(hWnd, kApplyTimer);
    ApplySettingsChanged();
}

void EditorFillPresets(HWND hWnd, const std::wstring& select) {
    HWND cb = GetDlgItem(hWnd, kIdPreset);
    SendMessageW(cb, CB_RESETCONTENT, 0, 0);
    for (const auto& n : VizStylePresetNames()) SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)n.c_str());
    SetWindowTextW(cb, select.c_str());
}

// Puts the current effective values into every control.
void EditorSync(HWND hWnd) {
    auto selectValue = [&](int id, const VizMenuOption* opts, size_t n, const std::wstring& v) {
        for (size_t i = 0; i < n; i++)
            if (v == opts[i].value) SendDlgItemMessageW(hWnd, id, CB_SETCURSEL, i, 0);
    };
    selectValue(kIdShape, kShapes, ARRAYSIZE(kShapes), VizLookValue(L"shape"));
    selectValue(kIdColorMode, kColorModes, ARRAYSIZE(kColorModes), VizLookValue(L"colorMode"));
    CheckDlgButton(hWnd, kIdPeak, g_settings.peakHoldEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hWnd, kIdBeat, g_settings.beatFlashEnabled ? BST_CHECKED : BST_UNCHECKED);
    for (int i = 0; i < (int)ARRAYSIZE(kSliders); i++) {
        int v = _wtoi(VizLookValue(kSliders[i].key).c_str());
        SendDlgItemMessageW(hWnd, kIdSliderBase + i, TBM_SETPOS, TRUE, v);
        SetDlgItemInt(hWnd, kIdSliderBase + 100 + i, v, FALSE);
    }
}

void EditorPickColor(HWND hWnd, const wchar_t* key) {
    BYTE a = 255, r = 255, g = 255, b = 255;
    ParseColorHex(VizLookValue(key).c_str(), &a, &r, &g, &b);
    CHOOSECOLORW cc = {sizeof(cc)};
    cc.hwndOwner = hWnd;
    cc.rgbResult = RGB(r, g, b);
    cc.lpCustColors = s_customColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (!ChooseColorW(&cc)) return;
    VizSetMenuOverride(key, VizHexColor(a, GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult)));
    EditorApplyNow(hWnd);
}

void EditorBuild(HWND hWnd) {
    const UINT dpi = GetDpiForWindow(hWnd);
    auto px = [&](int v) { return MulDiv(v, dpi ? dpi : 96, 96); };
    NONCLIENTMETRICSW ncm = {sizeof(ncm)};
    if (!s_editorFont && SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        ncm.lfMessageFont.lfHeight = -px(12);
        s_editorFont = CreateFontIndirectW(&ncm.lfMessageFont);
    }
    HINSTANCE inst = GetCurrentModuleHandle();
    int y = px(12);
    const int L = px(12), labelW = px(96), ctlX = L + labelW, ctlW = px(260), rowH = px(30);
    auto add = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int yy, int w, int h, int id) {
        HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, yy, w, h, hWnd,
                                 (HMENU)(INT_PTR)id, inst, nullptr);
        if (c && s_editorFont) SendMessageW(c, WM_SETFONT, (WPARAM)s_editorFont, FALSE);
        return c;
    };
    auto label = [&](const wchar_t* text) { add(L"STATIC", text, SS_LEFT, L, y + px(5), labelW, px(20), -1); };

    label(L"Saved style");
    add(L"COMBOBOX", L"", CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP, ctlX, y, px(118), px(200), kIdPreset);
    add(L"BUTTON", L"Load", BS_PUSHBUTTON | WS_TABSTOP, ctlX + px(122), y, px(44), px(24), kIdLoad);
    add(L"BUTTON", L"Save", BS_PUSHBUTTON | WS_TABSTOP, ctlX + px(170), y, px(44), px(24), kIdSave);
    add(L"BUTTON", L"Delete", BS_PUSHBUTTON | WS_TABSTOP, ctlX + px(218), y, px(50), px(24), kIdDelete);
    y += rowH + px(6);
    label(L"Base style");
    HWND shape = add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ctlX, y, ctlW, px(320), kIdShape);
    for (const auto& o : kShapes) SendMessageW(shape, CB_ADDSTRING, 0, (LPARAM)o.label);
    y += rowH;
    label(L"Color mode");
    HWND cm = add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ctlX, y, ctlW, px(320), kIdColorMode);
    for (const auto& o : kColorModes) SendMessageW(cm, CB_ADDSTRING, 0, (LPARAM)o.label);
    y += rowH;
    label(L"Colors");
    add(L"BUTTON", L"Color", BS_PUSHBUTTON | WS_TABSTOP, ctlX, y, px(80), px(24), kIdColor);
    add(L"BUTTON", L"Gradient start", BS_PUSHBUTTON | WS_TABSTOP, ctlX + px(86), y, px(90), px(24), kIdGrad1);
    add(L"BUTTON", L"Gradient end", BS_PUSHBUTTON | WS_TABSTOP, ctlX + px(182), y, px(78), px(24), kIdGrad2);
    y += rowH;
    add(L"BUTTON", L"Peak caps", BS_AUTOCHECKBOX | WS_TABSTOP, ctlX, y, px(110), px(22), kIdPeak);
    add(L"BUTTON", L"Beat flash", BS_AUTOCHECKBOX | WS_TABSTOP, ctlX + px(120), y, px(110), px(22), kIdBeat);
    y += rowH;
    for (int i = 0; i < (int)ARRAYSIZE(kSliders); i++) {
        label(kSliders[i].label);
        HWND tb = add(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_NOTICKS | WS_TABSTOP, ctlX - px(4), y, ctlW - px(36), px(26),
                      kIdSliderBase + i);
        SendMessageW(tb, TBM_SETRANGE, FALSE, MAKELPARAM(kSliders[i].lo, kSliders[i].hi));
        add(L"STATIC", L"", SS_RIGHT, ctlX + ctlW - px(36), y + px(5), px(36), px(20), kIdSliderBase + 100 + i);
        y += px(28);
    }
    y += px(4);
    add(L"STATIC",
        L"Changes apply at once. Reset Quick Settings in the right-click menu returns to the settings page. "
        L"Reflection, Glow and Bloom need the Direct3D 11 renderer.",
        SS_LEFT, L, y, labelW + ctlW, px(48), -1);
    y += px(56);
    RECT rc = {0, 0, labelW + ctlW + L * 2, y};
    AdjustWindowRectExForDpi(&rc, GetWindowLongW(hWnd, GWL_STYLE), FALSE, GetWindowLongW(hWnd, GWL_EXSTYLE), dpi);
    SetWindowPos(hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    EditorFillPresets(hWnd, L"");
    EditorSync(hWnd);
}

LRESULT CALLBACK StyleEditorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            EditorBuild(hWnd);
            return 0;
        case WM_HSCROLL: {
            HWND tb = (HWND)lParam;
            int i = GetDlgCtrlID(tb) - kIdSliderBase;
            if (i < 0 || i >= (int)ARRAYSIZE(kSliders)) break;
            int v = (int)SendMessageW(tb, TBM_GETPOS, 0, 0);
            SetDlgItemInt(hWnd, kIdSliderBase + 100 + i, v, FALSE);
            VizSetMenuOverride(kSliders[i].key, std::to_wstring(v));
            if (LOWORD(wParam) == TB_THUMBTRACK) EditorApplySoon(hWnd);
            else EditorApplyNow(hWnd);
            return 0;
        }
        case WM_TIMER:
            if (wParam == kApplyTimer) EditorApplyNow(hWnd);
            return 0;
        case WM_COMMAND: {
            int id = LOWORD(wParam), code = HIWORD(wParam);
            if ((id == kIdShape || id == kIdColorMode) && code == CBN_SELCHANGE) {
                int sel = (int)SendDlgItemMessageW(hWnd, id, CB_GETCURSEL, 0, 0);
                const VizMenuOption* opts = id == kIdShape ? kShapes : kColorModes;
                size_t n = id == kIdShape ? ARRAYSIZE(kShapes) : ARRAYSIZE(kColorModes);
                if (sel >= 0 && (size_t)sel < n) {
                    VizSetMenuOverride(id == kIdShape ? L"shape" : L"colorMode", opts[sel].value);
                    EditorApplyNow(hWnd);
                }
            } else if ((id == kIdPeak || id == kIdBeat) && code == BN_CLICKED) {
                VizSetMenuOverride(id == kIdPeak ? L"peakHold" : L"beatFlash",
                                   IsDlgButtonChecked(hWnd, id) == BST_CHECKED ? L"1" : L"0");
                EditorApplyNow(hWnd);
            } else if (id == kIdColor || id == kIdGrad1 || id == kIdGrad2) {
                EditorPickColor(hWnd, id == kIdColor ? L"color" : id == kIdGrad1 ? L"grad1" : L"grad2");
            } else if (id == kIdLoad || id == kIdSave || id == kIdDelete) {
                WCHAR name[64] = {};
                GetDlgItemTextW(hWnd, kIdPreset, name, ARRAYSIZE(name));
                std::wstring n = VizCleanPresetName(name);
                if (n.empty()) {
                    MessageBeep(MB_ICONWARNING);
                    return 0;
                }
                if (id == kIdSave) {
                    VizSaveStylePreset(n);
                    EditorFillPresets(hWnd, n);
                } else if (id == kIdDelete) {
                    VizDeleteStylePreset(n);
                    EditorFillPresets(hWnd, L"");
                } else if (VizApplyStylePreset(n)) {
                    EditorApplyNow(hWnd);
                    EditorSync(hWnd);
                }
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            KillTimer(hWnd, kApplyTimer);
            g_styleEditorWnd = nullptr;
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
}  // namespace

// Opens the editor near the visualizer, or brings it forward if it is open.
// Runs on the message window's thread, whose loop then serves it.
void VizOpenStyleEditor() {
    if (g_styleEditorWnd) {
        SetForegroundWindow(g_styleEditorWnd);
        return;
    }
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);
    if (!g_styleEditorClassRegistered) {
        WNDCLASSEXW wc = {sizeof(wc)};
        wc.lpfnWndProc = StyleEditorProc;
        wc.hInstance = GetCurrentModuleHandle();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = kStyleEditorClass;
        if (!RegisterClassExW(&wc)) return;
        g_styleEditorClassRegistered = true;
    }
    POINT at = {CW_USEDEFAULT, CW_USEDEFAULT};
    if (g_drawRectValid.load(std::memory_order_relaxed)) {
        at.x = g_drawRectL.load(std::memory_order_relaxed);
        at.y = std::max<LONG>(0, g_drawRectT.load(std::memory_order_relaxed) - 600);
    }
    g_styleEditorWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kStyleEditorClass, L"Tourne'Table Style Editor",
                                       WS_CAPTION | WS_SYSMENU | WS_POPUP, at.x, at.y, 400, 600, nullptr, nullptr,
                                       GetCurrentModuleHandle(), nullptr);
    if (!g_styleEditorWnd) return;
    ShowWindow(g_styleEditorWnd, SW_SHOW);
    SetForegroundWindow(g_styleEditorWnd);
}

void UnregisterStyleEditorClass() {
    if (s_editorFont) {
        DeleteObject(s_editorFont);
        s_editorFont = nullptr;
    }
    if (g_styleEditorClassRegistered) {
        UnregisterClassW(kStyleEditorClass, GetCurrentModuleHandle());
        g_styleEditorClassRegistered = false;
    }
}
