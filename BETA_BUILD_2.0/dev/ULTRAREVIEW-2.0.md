# Tourne'Table 2.0: ultrareview findings and merge plan

Status as of 2026-10-09. A review of the 2.0 beta for performance and correctness, done by three independent reviewer agents, then a second round where each implemented fixes in an isolated workspace. **Nothing from either round is merged into `dev/src` or the published beta file yet.** This doc is the map for whoever merges it.

## How the review was run

1. Round 1: three reviewers read the build (no edits asked for): **render** (per-frame GPU/compositor/CPU cost), **engine** (capture, analysis, idle), **correctness** (the new 2.0 features). Findings below.
2. Shared measuring layer added to the baseline: `src/p2_perf.cpp` (counters + a 30 s `[Perf]` log line behind the new **Performance -> Performance Stats** setting). Baseline `dev/src` includes it and passes both syntax checks.
3. Round 2: each reviewer got a workspace and owned files (`ULTRAREVIEW_RULES.md`), scored every item with the rubric, implemented, measured, reported.
   - Reviewer A (render): **finished**, report in `workspaces/reviewerA_render_round2/REPORT.md`.
   - Reviewer B (engine) and C (features/fixes): the user stopped round 2 before they reported. A partial B edit exists (`workspaces/reviewerB_partial/ttdsp.h`, unreviewed).
   - Separately, during round 1 the reviewers also produced experimental implementations in scratch copies (`workspaces/round1_*`). They weren't asked to and nobody has reviewed them, but they contain real work and benchmarks (see below). Treat as drafts.

## Rating system

Priority = Impact x Reach x Confidence / Cost (max 25). Impact 1 trivial .. 5 critical (crash, or full-rate work while it should be idle). Reach 1 rare combo .. 5 every user every frame. Confidence 0.5 plausible, 0.75 confirmed in code, 1.0 confirmed by a test or measurement. Cost 1 small/safe .. 3 large/risky. Skip under 3 unless nearly free.

## All findings, rated (main session's scores from the round-1 reports)

### Render (R) - implemented in `workspaces/reviewerA_render_round2` unless noted

| ID | Finding | I/R/C/Co | Pri | State |
|:--|:--|:--|--:|:--|
| R1 | DComp setters + `Commit()` every tick, even when nothing presents | 3/5/0.75/1 | 11.3 | A1 done |
| R3 | 3-5 dynamic buffers mapped before the "nothing changed" check | 2/5/1/1 | 10.0 | A2 done |
| R6 | Plate blended onto a just-cleared target; clear redundant when opaque | 2/4/1/1 | 8.0 | A4 done |
| R2 | Readouts (Hz, correlation) redraw the text surface ~every frame | 3/2/1/1 | 6.0 | A6 done |
| R8 | Text frame strings rebuilt every tick | 1/5/1/1 | 5.0 | A9 done |
| R10 | `ComputeVizLayout` twice per tick | 1/5/1/1 | 5.0 | skipped (base code, ~1 us) |
| R5 | Empty text swap chain still blended by DWM over the whole box | 3/4/0.75/2 | 4.5 | A7 done |
| R9 | Dots draws bars x slots instances | 2/2/1/2 | 2.0 | skipped |
| R4 | Terminal grid 65,536 cells: rebuild + 256 KB upload + per-cell hash + 65k VS instances | 4/1/1/3 | 1.3 | A8 done (with C's serial still to write) |
| R7 | GPU workload runs compute passes while idle | 3/1/0.75/2 | 1.1 | skipped (risk: frozen caps) |

### Engine (E) - not implemented in round 2; drafts in `workspaces/round1_engine`

| ID | Finding | I/R/C/Co | Pri |
|:--|:--|:--|--:|
| E6 | FFTs + band rebuild keep running on pure silence for 10 s (forever with Pause When Silent 0) | 3/4/1/1 | 12.0 |
| E1 | Deep Idle wake loop: meter wakes at -80 dBFS into Playing, engine counts audible only above -70 dBFS + 2% bar, input gain not applied; failed meter read returns 1.0 | 5/2/1/1 | 10.0 |
| E3 | `exp()` per bar per frame (ballistics) and `log10()` per bar per FFT | 2/5/1/1 | 10.0 |
| E7 | Trickle waits on the audio event; a silent stream (any app, or an input source) wakes it 100x/s | 3/3/1/1 | 9.0 |
| E2 | Engine thread opts out of power throttling while playing (P-cores, boost) | 3/5/0.5/1 | 7.5 (measure first) |
| E9 | `GetTickCount64` per sample; stereo/correlation work when not shown; gonio erase-from-front | 1/5/1/1 | 5.0 |
| E4 | 4x true peak runs in Loudness mode though only LoudnessFull shows it | 2/2/1/1 | 4.0 |
| E5 | Oscilloscope/Goniometer still run the full 3-tier, up-to-2048-band analysis | 2/2/1/1 | 4.0 |
| E8 | Bass tiers re-transform every n/16 samples (~117 extra FFTs/s at 2048) | 1/5/0.75/1 | 3.8 (visual trade-off) |
| E10 | No backoff when no device can open (full lookup every 500 ms forever) | 1/1/1/1 | 1.0 |

`workspaces/round1_engine/src/bench_out.txt` (draft, unverified) reports, in us of CPU per second of audio:
analysis 2048/2048 bars 15,109 -> 10,254; ballistics at 2048 bands 3,218 -> 589; Loudness (M/S/I) 5,514 -> 558; scope/gonio shapes 11,862 -> 2,331; silence steady state 5,173 -> 96 (256 bars), 11,817 -> 67 (2048 bars). It also claims "analysis + peak-frequency readout" 554,683 -> 5,477 (55% of a core -> 0.5%). **Verify that one first**: if real, `SpectrumEngine::DominantHz` (a `log10` per bin across all three tiers, every frame) is the single biggest cost in the mod with the readout on; if it's a bench artefact, discard.

### Correctness (B) - not implemented in round 2; drafts in `workspaces/round1_features` and `round1_engine`

| ID | Bug | I/R/C/Co | Pri |
|:--|:--|:--|--:|
| B1 | GSMTC handlers read the global `g_gsmtcSession` unlocked while another thread replaces it -> use-after-release crash (fix: use the event sender) | 5/3/0.75/1 | 11.3 |
| B2 | Input-device source: capture event fires every ~10 ms in silence -> Trickle at 100 wakes/s | 3/2/1/1 | 6.0 |
| B4 | Chosen device present but `Initialize`/`Start` fails (exclusive mode) -> no audio, no notice, retries forever | 3/2/1/1 | 6.0 |
| B5 | Auto-Hide faded: right-click on the empty desktop still opens our menu | 3/2/1/1 | 6.0 |
| B6 | Right-click while the menu is open: swallowed, nested TrackPopupMenuEx fails | 2/3/0.75/1 | 4.5 |
| B9 | Progress bar snaps back on pause and jumps on resume (extrapolation bookkeeping) | 2/3/0.75/1 | 4.5 |
| B3 | Deep Idle with an input device may never wake (capture meter reads 0 once our stream stops); needs a hardware test | 4/2/0.5/1 | 4.0 |
| B7 | Progress "inside panel bottom" and two-line NP inside-bottom can be clipped (no layout reservation) | 2/2/1/1 | 4.0 |
| B8 | Progress "Above" runs through Now Playing "Above" text | 2/2/1/1 | 4.0 | (done by A as A11) |
| B11 | Waterfall scroll: `&v[steps*cols]` one past end when steps == rows; stale rows when steps > 1 | 3/1/1/1 | 3.0 |
| B10 | Menu shows "Peak frequency" ticked for readout = both, no Both option; strip right-click ignores Ctrl mode | 1/2/1/1 | 2.0 |

`workspaces/round1_features/tests_features/` has ASan/UBSan/TSan harnesses (timeline, waterfall, layout, GSMTC mock) and `run.sh <new v2b> <old v2b>`.

## Merge status (2026-10-09)

Merged on branch `ultrareview-merge`, all checks clean (x64 + x86 syntax check with the WinRT region stubbed, fxc_lint, DXC on all six entries, test_dsp, test_gfx, the features harness incl. TSan on the GSMTC mock, settings-key cross-check):

- Steps 1 and 2: A's render files; `g_termGridSerial` in `p2_term.cpp` (bumped by a memcmp against the previous grid); B11.
- Step 3: the `round1_engine` drafts (E1, E3, E4, E5, E6, E7+B2, B3, B4, E9, E10) after review, with two changes: trickle still posts a render tick every 250 ms (the draft skipped it when the engine reported no change, which could freeze a peak cap mid-fall), and **E2 is not applied** (QoS unchanged; the alternative is left as a comment in `VizThreadEcoQoS` until `[Perf]` / HWiNFO numbers decide it). Engine counters wired (wakes, engine time, analyses, FFTs, idle split, `VizPerfMaybeLog`).
- Step 4: the `round1_features` drafts (B1, B5, B6, B7, B9, B10, Copy Quick Settings), with the perf-counter hunks of `splice2.py` kept.
- Not done: step 6 (measured `[Perf]` lines from Windows), the E8 visual trade-off, R7, R9, R10. The "55% of a core" DominantHz figure is not reproduced; the fix (search on power) is merged either way, since it is exact.

## Merge plan (for the next session)

Ownership keeps merges conflict-free; follow it.

1. Start from `dev/src` (baseline with perf counters). Copy in A's four files from `workspaces/reviewerA_render_round2/src/` (`part_gfx.cpp`, `part_text.cpp`, `tt_body.hlsl`, `test_gfx.py`). Don't copy `bench/inject_serial.py` into the build.
2. Implement the terminal serial in `p2_term.cpp`: `uint32_t g_termGridSerial`, bumped by `VizBuildTermGrid` only when cells change (Waterfall scrolls included). Fix B11 there at the same time.
3. Engine: do E1, E6, E7+B2, B3, E3, E4, E5, E9, B4, E10 in `part_engine.cpp`, `part_thread.cpp`, `part_bars.cpp`, `ttdsp.h`. Use `workspaces/round1_engine` and `reviewerB_partial` as drafts: diff them against `src_before_ultrareview`, keep what's correct, re-run `test_dsp` (extend it: identical ballistics output, fast-log error bound). Wire `kPerfEngineWakes` + `g_perfEngineTicks`, `kPerfAnalyses`, `kPerfFfts`, idle states, and call `VizPerfMaybeLog()` once per engine loop. Decide E2 only with measurements.
4. Features/fixes: B1, B5, B6, B7, B9, B10 in `splice2.py` / `p2_menu.cpp` (drafts in `workspaces/round1_features`; `features.patch` is their whole diff). The WinRT code is stubbed out of the syntax check, so review cppwinrt types by eye.
5. Rebuild, check, then publish: `python3 src/splice.py && python3 src/splice2.py && python3 src/release.py`; both syntax checks; `fxc_lint.py`; DXC all six entries; `test_gfx`, `test_dsp`, the features harness; the settings-key cross-check.
6. Ask the user to run with **Performance Stats** on and send the `[Perf]` lines for S1-S4 before and after. That replaces the modelled numbers with measured ones.

## Combined expected effect (modelled, to be confirmed with `[Perf]` on Windows)

| Scenario | Today | After all merges |
|:--|:--|:--|
| S1 playing 144 FPS, 256 bars | 144 commits/s, 432 maps/s, analysis every frame, engine on P-cores | ~0 commits/s, 144 maps/s, cheaper ballistics, no empty text layer for DWM to blend |
| S2 quiet, bars settled | 144 commits/s, 288 maps/s | 0 / 0 |
| S3 30 s silence | 48.7 commits/s, 102 maps/s, FFTs on zeros for 10 s, possible deep-idle wake loop | 0 commits, ~5 maps/s, analysis stops after ~1.4 s of zeros, no wake loop |
| S4 hidden | already ~1 wake/s | unchanged |
| Readout on | text surface up to 144 redraws/s; dominant-Hz scan possibly 55% of a core (unverified) | <=10 redraws/s; scan fix if confirmed |
