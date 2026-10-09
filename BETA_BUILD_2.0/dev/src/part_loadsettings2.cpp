        // ---- 2.0: audio source, media widget, Terminal ------------------------------
        str(L"audio.source", [](PCWSTR v) {
            g_settings.audioSourceKey = (wcscmp(v, L"default_input") == 0) ? L"default_input"
                                      : (wcscmp(v, L"named") == 0)         ? L"named"
                                                                           : L"";
        });
        if (g_settings.audioSourceKey == L"named") {
            PCWSTR raw = Wh_GetStringSetting(L"audio.deviceName");
            std::wstring name = raw ? raw : L"";
            Wh_FreeStringSetting(raw);
            while (!name.empty() && iswspace(name.front())) name.erase(name.begin());
            while (!name.empty() && iswspace(name.back())) name.pop_back();
            if (name.empty()) {
                ReportSettingIssue(L"Audio", L"Device Name", L"",
                                   L"part of a device's name, as Windows' Sound settings show it",
                                   L"the default output");
                g_settings.audioSourceKey.clear();
            } else {
                g_settings.audioSourceKey = L"name:" + name;
            }
        }

        str(L"appearance.nowPlayingLayout", [](PCWSTR v) {
            g_settings.npLayout = (wcscmp(v, L"two_lines") == 0) ? VizNpLayout::TwoLines : VizNpLayout::OneLine;
        });
        str(L"appearance.nowPlayingPlacement", [](PCWSTR v) {
            g_settings.npPlacement = (wcscmp(v, L"panel_top") == 0)      ? VizNpPlacement::PanelTop
                                   : (wcscmp(v, L"panel_bottom") == 0)   ? VizNpPlacement::PanelBottom
                                                                         : VizNpPlacement::Above;
        });
        str(L"appearance.nowPlayingAlign", [](PCWSTR v) {
            g_settings.npAlign = (wcscmp(v, L"left") == 0)    ? VizTextAlignH::Left
                               : (wcscmp(v, L"right") == 0)   ? VizTextAlignH::Right
                                                              : VizTextAlignH::Center;
        });
        ReadColorSetting(L"appearance.nowPlayingArtistColor", L"Appearance", L"Now Playing Artist Color", 0xB3, 255,
                         255, 255, &g_settings.npArtistA, &g_settings.npArtistR, &g_settings.npArtistG,
                         &g_settings.npArtistB);
        str(L"appearance.textRendering", [](PCWSTR v) { g_settings.textPixel = wcscmp(v, L"pixel") == 0; });

        g_settings.progressEnabled = Wh_GetIntSetting(L"progress.enabled") != 0;
        str(L"progress.placement", [](PCWSTR v) {
            g_settings.progressPlacement = (wcscmp(v, L"above") == 0)          ? VizProgressPlacement::Above
                                         : (wcscmp(v, L"panel_bottom") == 0)   ? VizProgressPlacement::PanelBottom
                                                                               : VizProgressPlacement::Below;
        });
        g_settings.progressHeight = std::clamp(Wh_GetIntSetting(L"progress.height"), 1, 40);
        g_settings.progressGap = std::clamp(Wh_GetIntSetting(L"progress.gap"), 0, 200);
        ReadColorSetting(L"progress.color", L"Track Progress", L"Color", 255, 255, 255, 255, &g_settings.progressA,
                         &g_settings.progressR, &g_settings.progressG, &g_settings.progressB);
        ReadColorSetting(L"progress.trackColor", L"Track Progress", L"Track Color", 0x40, 255, 255, 255,
                         &g_settings.progressTrackA, &g_settings.progressTrackR, &g_settings.progressTrackG,
                         &g_settings.progressTrackB);

        str(L"media_controls.anchor", [](PCWSTR v) {
            g_settings.mediaAnchor = (wcscmp(v, L"panel_top_left") == 0)       ? VizMediaAnchor::PanelTopLeft
                                   : (wcscmp(v, L"panel_top_right") == 0)      ? VizMediaAnchor::PanelTopRight
                                   : (wcscmp(v, L"panel_bottom_left") == 0)    ? VizMediaAnchor::PanelBottomLeft
                                   : (wcscmp(v, L"panel_bottom_right") == 0)   ? VizMediaAnchor::PanelBottomRight
                                                                               : VizMediaAnchor::Screen;
        });
        g_settings.mediaAnchorOffsetX = std::clamp(Wh_GetIntSetting(L"media_controls.anchorOffsetX"), -2000, 2000);
        g_settings.mediaAnchorOffsetY = std::clamp(Wh_GetIntSetting(L"media_controls.anchorOffsetY"), -2000, 2000);
        str(L"interaction.contextMenu", [](PCWSTR v) {
            g_settings.contextMenu = (wcscmp(v, L"ctrl_right_click") == 0) ? VizContextMenu::CtrlRightClick
                                   : (wcscmp(v, L"off") == 0)              ? VizContextMenu::Off
                                                                           : VizContextMenu::RightClick;
        });

        str(L"terminal.style", [](PCWSTR v) {
            g_settings.termStyle = (wcscmp(v, L"waterfall") == 0) ? VizTermStyle::Waterfall
                                 : (wcscmp(v, L"meters") == 0)    ? VizTermStyle::Meters
                                                                  : VizTermStyle::Columns;
        });
        str(L"terminal.font", [](PCWSTR v) { g_settings.termFont = *v ? v : L"Consolas"; });
        g_settings.termFontSize = std::clamp(Wh_GetIntSetting(L"terminal.fontSize"), 6, 96);
        g_settings.termRows = std::clamp(Wh_GetIntSetting(L"terminal.rows"), 2, 128);
        g_settings.termMeterColumns = std::clamp(Wh_GetIntSetting(L"terminal.meterColumns"), 20, 200);
        g_settings.termHotThreshold = std::clamp(Wh_GetIntSetting(L"terminal.hotThreshold"), 1, 100);
        g_settings.termScrollRate = std::clamp(Wh_GetIntSetting(L"terminal.scrollRate"), 1, 120);
        // One printable ASCII character each; the atlas holds 32-126.
        auto glyph = [](PCWSTR key, PCWSTR name, wchar_t def) {
            PCWSTR v = Wh_GetStringSetting(key);
            wchar_t c = (v && v[0]) ? v[0] : def;
            if (c < 33 || c > 126) {
                WCHAR d[2] = {def, 0};
                ReportSettingIssue(L"Terminal", name, v ? v : L"", L"one printable ASCII character", d);
                c = def;
            }
            Wh_FreeStringSetting(v);
            return c;
        };
        g_settings.termColumnGlyph = glyph(L"terminal.columnGlyph", L"Column Glyph", L'#');
        g_settings.termPeakGlyph = glyph(L"terminal.peakGlyph", L"Peak Glyph", L'-');
        str(L"terminal.ramp", [](PCWSTR v) {
            std::wstring r;
            for (const wchar_t* p = v; *p; p++)
                if (*p >= 32 && *p < 127) r.push_back(*p);
            g_settings.termRamp = r.size() >= 2 ? r : L" .:-=+*#%@";
        });
        ReadColorSetting(L"terminal.dimColor", L"Terminal", L"Dim Color", 255, 0x1E, 0x6B, 0x34, &g_settings.termDimA,
                         &g_settings.termDimR, &g_settings.termDimG, &g_settings.termDimB);
        ReadColorSetting(L"terminal.lowColor", L"Terminal", L"Color", 255, 0x33, 0xFF, 0x66, &g_settings.termLowA,
                         &g_settings.termLowR, &g_settings.termLowG, &g_settings.termLowB);
        ReadColorSetting(L"terminal.highColor", L"Terminal", L"Hot Color", 255, 0xFF, 0x3B, 0x3B, &g_settings.termHighA,
                         &g_settings.termHighR, &g_settings.termHighG, &g_settings.termHighB);
        ReadColorSetting(L"terminal.labelColor", L"Terminal", L"Label Color", 255, 0xB8, 0xFF, 0xB8,
                         &g_settings.termLabelA, &g_settings.termLabelR, &g_settings.termLabelG,
                         &g_settings.termLabelB);
