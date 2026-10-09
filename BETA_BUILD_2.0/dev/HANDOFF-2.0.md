# Tourne'Table 2.0: session handoff

Everything a new session needs to continue the 2.0 work, as of 2026-10-09. Read this first, then `ULTRAREVIEW-2.0.md` for the pending performance and bug work.

## Where things are

| What | Where | State |
|:--|:--|:--|
| Published mod | `ramensoftware/windhawk-mods`, `mods/tourne-table-desktop-audio-visualizer.wh.cpp` | **1.3.0** (id `tourne-table-desktop-audio-visualizer`) |
| 2.0 beta file | `USER-TOURNE/TOURNE-TABLE`, branch `beta-2.0-device-selection`, `BETA_BUILD_2.0/tourne-table-scope.wh.cpp` | commit `aca4fab`. Beta id `tourne-table-desktop-audio-visualizer-scope`, version 2.0.0. Compiled for the user in Windhawk (0 errors); runtime not yet confirmed on Windows |
| Dev sources | same branch family, `BETA_BUILD_2.0/dev/` (this folder; pushed on branch `ultrareview-wip`) | `dev/src` = the beta above **plus** the Performance Stats counters (`p2_perf.cpp`), syntax-checked clean, not yet in the beta file |
| Catalog PR branch | `USER-TOURNE/windhawk-mods-pr`, branch `tourne-table-2.0.0` | commit `19a915e`: the release build of `aca4fab` (published id, clean name/description). Validator: 0 warnings |
| The PR itself | not opened | GitHub returns 403 to the integration for PR creation; the user opens it: https://github.com/ramensoftware/windhawk-mods/compare/main...USER-TOURNE:windhawk-mods-pr:tourne-table-2.0.0?expand=1 with `PR_BODY.md` as the description |
| Ultrareview | `dev/ULTRAREVIEW-2.0.md`, `dev/workspaces/` | round 2 done for render only; nothing merged |

The PR review flow at windhawk-mods: comment `/ai-review` (AI review, repeatable), then `/ready-for-reviewer` (human). Tell the user not to send `/ready-for-reviewer` until 2.0 has run on their PC for a while. If the ultrareview work gets merged first, rebuild the release file and push it to the PR branch.

## How the build is made

The mod is generated, not hand-edited:

```
dev/base15.cpp            1.5.0 beta source (LF), the base
dev/src/splice.py         stage 1 -> v2.cpp  (precision DSP, D3D11 renderer, engine thread, YAML, readme)
dev/src/splice2.py        stage 2 -> v2b.cpp (right-click menu, audio source, Terminal, media widget,
                                              Pixel Snap, subpixel nudges, perf counters, review fixes)
dev/src/release.py        v2b.cpp -> release.cpp (catalog id/name/description, readme without "beta")
```

Run from `dev/`: `python3 src/splice.py && python3 src/splice2.py && python3 src/release.py`. Every edit anchors on text that must match an exact number of times, so a stale anchor fails loudly. Beta file = `v2b.cpp`; catalog file = `release.cpp`.

Main parts (in `dev/src`): `ttdsp.h` (DSP core), `part_engine.cpp` (WASAPI capture + VizEngine), `part_thread.cpp` (engine loop, pacing, idle ladder), `part_bars.cpp` (bar frame, engine config), `part_gfx.cpp` (ttgfx D3D11 renderer), `part_text.cpp` (text overlays, D3D frame), `tt_*.hlsl` (shaders, embedded at `/*@@SHADER_SOURCE@@*/`), `p2_*.cpp` (2.0 features), `yaml_*.txt` / `readme_*.md` / `part_loadsettings*.cpp` (settings and docs). `DESIGN-2.0.md` explains the architecture.

## Verification (Linux, no Windows available)

- **Syntax check** against mingw-w64 headers with clang 18: `tools/syntax_check.sh <file> x86_64|i686` (or `syntax_check_w.sh <workdir> <arch>`). `tools/make_test_tu.py` stubs out the WinRT/GSMTC region (FetchAlbumArtColorAsync .. g_mediaIconPixels), so **that region is never compiled here; review it by eye**. Needs: mingw-w64 headers, LLVM 18 libc++ headers, `tools/sysroot_cxx` (`__config_site`, `__assertion_handler`), `tools/windhawk_api.h`, `tools/windhawk_utils.h` (empty stub). The scripts hard-code the old scratchpad path `S=...`; edit it. The user's original `verify-kit.zip` had this kit.
- **Shaders**: DXC (Linux release v1.8.2505) for VSMain/PSMain (`vs_6_0`/`ps_6_0`) and CsFft/CsBands/CsReduce/CsShape (`cs_6_0`). **DXC is more lenient than Windows' fxc**: always also run `python3 src/fxc_lint.py v2b.cpp` (reserved words like `pass`, `line`, `point`, `sample`; no `#define F()`; ASCII only). This is what broke the first 2.0 build on the user's PC.
- **Tests**: `clang++ -std=c++20 -O2 -o test_dsp test_dsp.cpp && python3 test_dsp.py` (FFT, windows, decimators, IEC bands, calibration, EBU 3341, true peak, ballistics); `clang++ -std=c++20 -O2 -pthread -o test_gfx test_gfx.cpp && python3 test_gfx.py` (shader logic run as C++ via `hlsl_shim.h`, rasterisation vs a supersampled reference, compute passes vs ttdsp).
- **Catalog validator**: `ramensoftware/windhawk-mods` `.github/pr_validation.py`. It fetches from raw.githubusercontent; offline, monkeypatch the fetch helpers (see the main session's approach: stub author data, licenses, existing metadata/versions, readme images).
- **Settings cross-check**: every `Wh_Get*Setting` key must exist in the YAML block.
- Before release, diff settings against the published version: no key removed or retyped (2.0 vs 1.3.0: none; only FFT Size default 1024 -> 2048).

## Things learned the hard way

- Windhawk editor: paste into an **empty** file (Ctrl+A, Delete first). A doubled paste gives "Redefinition of VizShape ..." at line 13,218 + n.
- Mods can read settings but not write them; mod-owned values (`Wh_SetStringValue`) hold drag position, nudges and the right-click menu's quick settings (`menuOverrides`).
- Windhawk 1.x has run tool mods as 32-bit; OpenVINO is 64-bit only, so the NPU path reports that and stays on the CPU. Unconfirmed whether the user's Windhawk is 64-bit.
- OpenVINO search no longer looks in `C:\Intel` or `C:\openvino*` (user-creatable folders = DLL planting).
- GitHub from the session: pushing to the user's repos works (add_repo with push); creating PRs (`create_pull_request`) and `merge-upstream` are refused; give the user compare links.
- Low-level hooks: never write storage or do anything slow inside them; use `RequestPositionOverrideSave()` (deferred).

## User preferences

Short dashes only, never long dashes. Conversational but polished. Explain the reasoning behind structural choices. Keep alternatives as options or comments rather than deleting them. Project goal: 144 FPS at maxed visuals for 0-2% CPU and GPU, no boost clocks, no lag.

## Open threads with the user

1. **Testing 2.0 on Windows**: right-click menu, device switching, Terminal shape, media widget, progress bar, Pixel Snap. Ask for screenshots and the `[Perf]` log lines (Performance Stats, once merged).
2. **NPU**: the user was installing Python to get OpenVINO. `py`/`python` weren't on PATH (only the Store alias). Next step given: `winget install -e --id Python.Python.3.12`, new terminal, `py -m pip install openvino`, then `py -c "import openvino as ov; print(ov.Core().available_devices)"` should list `NPU`; leave NPU Runtime Folder blank if `sys.prefix` is under `%LOCALAPPDATA%\Programs\Python`.
3. **Issue #1** (DavidHiFi, device selection): fixed in 2.0. Draft reply, not posted: "Thanks David, this is in the 2.0 beta (`BETA_BUILD_2.0` on the `beta-2.0-device-selection` branch). Under Audio Source you can pick the default output, the default input, or a device by name. Type part of it, like `VB-Audio Matrix`, and outputs are captured by loopback while inputs are recorded directly. You can also right-click the visualizer, open Audio Source and pick from every active device, virtual ones included, and it switches live. If the device isn't connected it falls back to the default output and switches back when it reappears. Let me know how it does with your Ableton routing."
4. **Separate project, TourneDesktop** (the user's own desktop app, not this mod): Defender flagged `TourneDesktop.exe` as Trojan:Win32/Bearfoos.A!ml, a heuristic false positive driven by Winlogon `Shell` replacement + Run key + global hooks + unsigned exe in AppData. Advice given: allow + folder exclusion for dev, check `HKCU\...\Winlogon\Shell` before rebooting, make shell mode opt-in with an Explorer fallback, sign the exe, use `RegisterHotKey` instead of a keyboard hook, install to Program Files, submit to Microsoft as a false positive.
