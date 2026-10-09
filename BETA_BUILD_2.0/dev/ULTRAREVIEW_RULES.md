# Ultrareview round 2: implement, measure, report

## Rating system (every item you touch gets scored)

Priority = Impact x Reach x Confidence / Cost   (max 25)

| Factor | Scale |
|:--|:--|
| Impact | 1 trivial, 2 small, 3 noticeable (>=0.5% of a core, or a visible bug), 4 large (breaks the 2% CPU budget or the GPU's idle state), 5 critical (crash, or full-rate work while it should be idle) |
| Reach | 1 rare setting combination, 2 one opt-in feature, 3 a common option, 4 most users some of the time, 5 every user every frame |
| Confidence | 0.5 plausible, 0.75 confirmed in code, 1.0 confirmed by a measurement or test |
| Cost | 1 small and safe, 2 moderate change or some risk, 3 large or risky |

Do the highest-priority items in your slice first. Skip anything under 3 unless it is nearly free.

## Ownership (edit ONLY your files; everything else in your workspace is read-only)

| Reviewer | Workspace | Owns |
|:--|:--|:--|
| A, render | `wA/` | `src/part_gfx.cpp`, `src/part_text.cpp`, `src/tt_*.hlsl`, `src/test_gfx.*` |
| B, engine | `wB/` | `src/part_engine.cpp`, `src/part_thread.cpp`, `src/part_bars.cpp`, `src/ttdsp.h`, `src/test_dsp.*`, `src/part_loadsettings.cpp`, `src/yaml_hardware.txt`, `src/readme_hardware.md`, `src/readme_efficiency.md` |
| C, features and fixes | `wC/` | `src/splice2.py`, `src/p2_menu.cpp`, `src/p2_term.cpp`, `src/p2_device.cpp`, `src/part_loadsettings2.cpp`, `src/yaml_*2.txt`, `src/readme_widget.md` |

New helper files are fine if they're only used by your own files (put them in your `src/`, prefix `a_`, `b_` or `c_`). Benchmarks go in `<workspace>/bench/`.

## Shared contracts

* **Performance counters** (`src/p2_perf.cpp`, spliced before the audio engine; don't edit it): `VizPerf(kPerfXxx, n)` and `VizPerfScope s(g_perfEngineTicks / g_perfRenderTicks)`. A wires `kPerfRenderTicks` + `g_perfRenderTicks` (whole RenderVisualizerD3D), `kPerfPresents`, `kPerfSkipped`, `kPerfTextPresents`, `kPerfCommits`, `kPerfMaps`. B wires `kPerfEngineWakes` + `g_perfEngineTicks` (engine loop body), `kPerfAnalyses`, `kPerfFfts` (count in part_engine.cpp, keep ttdsp.h free of mod globals), `kPerfIdlePlaying/Trickle/Deep`, and calls `VizPerfMaybeLog()` once per engine loop.
* **Terminal grid serial**: C adds `uint32_t g_termGridSerial` to p2_term.cpp, incremented by `VizBuildTermGrid` only when the cells actually changed. A uses `Mix(hash, g_termGridSerial)` (plus grid size) instead of hashing every cell, and skips the cells upload when the serial is unchanged since the last upload.

## Build and check (inside your workspace, never in the shared scratchpad)

```
cd <workspace> && python3 src/splice.py && python3 src/splice2.py
bash ../tools/syntax_check_w.sh $PWD x86_64 && bash ../tools/syntax_check_w.sh $PWD i686
python3 src/fxc_lint.py v2b.cpp
```
Both syntax checks must be clean (no errors, no new warnings). A also runs DXC on the embedded shader (`../dxc/bin/dxc -T vs_6_0 -E VSMain ...` for VSMain, PSMain, CsFft, CsBands, CsReduce, CsShape) and `src/test_gfx`. B runs `src/test_dsp`. Shader rules from fxc: no `pass`/`line`/`point`/`sample`/... identifiers, no `#define F()`.

## Metrics (required; this is how the work is judged)

Nothing here runs on Windows, so measure what can be measured honestly:

1. **Microbenchmarks** for any CPU-side change: compile the before and after code natively (`clang++ -O2`, from the `src_before_ultrareview/` copy vs. yours) and report ns or us per call, median of many runs, with the inputs (bars, FFT size, cells).
2. **Call-rate model** for Windows API work you can't run (Commit, Map, Present, wakes): count calls per second, before and after, for these four scenarios, and show the arithmetic:
   * S1 playing music, 144 FPS, 256 bars, D3D11, panel on, Now Playing on;
   * S2 a quiet passage where bars have settled (nothing visibly moving);
   * S3 30 s of silence (through Pause When Silent at 10 s and Deep Idle at 15 s);
   * S4 visualizer hidden (fullscreen game).
3. Say plainly which numbers are measured and which are modelled.

## Deliverable

`<workspace>/REPORT.md`, under 900 words: a table of items (ID, title, Impact/Reach/Confidence/Cost, Priority, done or skipped and why), the before/after metrics, what changed per file, and any risk the merger should know about. Also return a short summary as your final message.
