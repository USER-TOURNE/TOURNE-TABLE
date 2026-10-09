# Reviewer A (render), round 2 report

Written by the render reviewer agent; saved verbatim-in-substance by the main session because the agent couldn't write report files. Not yet merged into `dev/src`.

Checks the agent ran in its workspace: both syntax checks (x86_64, i686) clean, `fxc_lint` clean, DXC compiles all six entries (`bench/dxc_all.sh`), `test_gfx` ALL PASSED including a new sparse-terminal check.

Priority = Impact x Reach x Confidence / Cost (see `../../ULTRAREVIEW_RULES.md`).

| ID | Item | I/R/C/Co | Pri | Status |
|:--|:--|:--|--:|:--|
| A1 | DirectComposition set + `Commit` only on change (`EnsureSurface`) | 5/5/0.75/1 | 18.8 | done |
| A2 | Check for changes before uploading; map only buffers whose contents changed | 4/5/0.75/1 | 15.0 | done |
| A3 | Faster rounding in the hash (`MixF` without `llroundf`) | 2/5/1.0/1 | 10.0 | done |
| A4 | Plate drawn without blending, no clear when the plate is drawn | 3/4/0.75/1 | 9.0 | done |
| A5 | Frame constants and globals uploaded only on change | 2/5/0.75/1 | 7.5 | done |
| A6 | Readout updated at 10 Hz, frequency held within one display step | 4/2/0.75/1 | 6.0 | done |
| A7 | Text surface removed from its visual while empty | 3/5/0.75/2 | 5.6 | done |
| A8 | Terminal: `g_termGridSerial` replaces the per-cell hash; compact upload of glyph cells | 4/2/1.0/2 | 4.0 | done |
| A9 | Text frame reused between ticks, no per-tick string reallocation | 1/3/1.0/1 | 3.0 | done |
| A10 | Perf counters wired (ticks, timing, presents, skipped, text, commits, maps) | contract | - | done |
| A11 | Bug: Now Playing "Above" overlapped a progress bar also "Above" | 3/1/0.75/1 | 2.3 | done |
| A12 | `g_meterMutex` locked only for the Goniometer | 1/5/0.75/1 | 3.8 | done |
| - | Skip GPU-workload compute passes while idle | - | - | skipped: can't show caps/falling bars won't freeze |
| - | Duplicate `ComputeVizLayout` per tick | 1/5/0.75/1 | 3.8 | skipped: second call is in base code, ~1 us |
| - | Dots shape instance count = bars x slots | 2/2/0.75/2 | 1.5 | skipped |

## Measured (CPU, clang++ -O2, 2.1 GHz Xeon, median of 51 runs)

Map is simulated as a memory write, so driver cost is not included; "before" figures are lower bounds.

| Work per tick | Before | After |
|:--|--:|--:|
| Terminal, 65,536 cells, Columns (46% glyphs): hash + upload | 109-113 us | 0.002 us unchanged; 33.6 us changed |
| Terminal 30/60/100% glyphs, changed | 110-115 us | 34-36 us |
| Terminal upload size / VS instances (46% glyphs) | 256 KB / 65,536 | 118 KB / 30,212 |
| 256 bars + caps, tick with nothing new | 1.46 us | 0.90 us |
| Hash of 256 bars (A3) | 1.43-1.46 us | 0.89-0.92 us |
| Text frame + readout (LoudnessFull + Hz, 44-char Now Playing) | 1.26-1.46 us | 0.29-0.34 us |

At max Terminal size the hash + upload alone was 15.9 ms/s at 144 Hz (1.6% of a core). After: 4.8 ms/s if the grid changes every tick, nothing when it doesn't. `bench/test_readout.cpp`: a 2M-step random walk over 20 Hz-20 kHz; the shown frequency always stays within one display step of the live value.

## Modelled (calls per second; Windows calls can't run here)

Assumptions: 144 ticks/s playing; Trickle ~4/s; Deep idle one final tick. Defaults: Hybrid, CPU bars, panel on, Beat Flash off.

| Scenario | Commits | Maps | Presents | Text presents |
|:--|:--|:--|:--|:--|
| S1 playing | 144 -> ~0 | 432 -> 144 | 144 -> 144 | ~0.6 -> ~0.6 |
| S1 + Frequency readout | 144 -> ~0 | 432 -> 144 | 144 | up to 144 -> <=10 |
| S2 quiet, bars settled | 144 -> 0 | 288 -> 0 | ~0 -> 0 | 0 |
| S3 30 s silence (avg) | 48.7 -> 0 | 102 -> 4.8 | 4.8 -> 4.8 | 0 |
| S4 hidden | 0 | 0 | 0 | 0 |

Arithmetic: commits before = one per tick. Maps before = bars + globals every tick + frame CB per presented frame (2x144 + 144 = 432); after, only bars change while playing (144). S3 before: 1,440 playing ticks + 20 trickle + 1 deep = 1,461 ticks / 30 s = 48.7 commits/s; maps 2x1,461 + 144 = 3,066 / 30 = 102/s; after, maps only for the ~144 decay frames = 4.8/s.

GPU/DWM (modelled): A4 removes a render-target read per presented frame (~300 MB/s for a 1600x330 panel at 144 Hz). A7: with Now Playing / readout / progress off (the defaults) the text surface is always empty, yet DWM blended a full-box layer wherever the panel changed (~2.9 MB per composed frame for 1700x420, ~410 MB/s at 144 Hz).

## Files changed

- `part_gfx.cpp`: State caches (DComp state, upload keys, no-blend state, `textDetached`); `EnsureSurface` sets/commits only on change; `Render` hashes first and skips early, uploads per buffer only on key change, compacts terminal cells branch-free, rebuilds gonio points only on serial change, memcmp-gates the frame CB, draws the plate without blend or clear, perf counters; `ShowTextSurface()`; `MixF` plain-conversion rounding; caches invalidated in EnsureDevice / ReleaseDevice / ReleaseSurface / EnsureTermResources.
- `part_text.cpp`: `VizReadoutCache` / `VizFormatReadout` (10 Hz, frequency hold); `VizBuildTextFrame` clears instead of rebuilding; `RenderVisualizerD3D` perf scope + counters, text frame kept between ticks, correlation read only for the Goniometer, text surface detached when empty and restored on every fallback return; Now Playing / progress stacking fix.
- `tt_body.hlsl`: `TermPrim` reads packed cells (char | colour << 8 | grid index << 16); instance count = number of glyphs.
- `test_gfx.py`: terminal test uses the packed list; new sparse-vs-full grid check.

## Risks for the merger

1. Terminal serial contract: `g_termGridSerial` (owned by the features reviewer, in `p2_term.cpp`) must change on every cell change, Waterfall scrolls included, or frames are skipped and the grid freezes. `bench/inject_serial.py` adds a temporary definition to the generated v2b.cpp only so the syntax check runs. Don't merge it; implement the real serial in p2_term.cpp.
2. Text surface removal: any new code drawing on `g_swapChain` while D3D11 is active and the text is empty must call `ttgfx::ShowTextSurface(true)` first. All current fallback paths do.
3. The readout lags up to 100 ms and holds within one display step (+-1 count below 1 kHz, +-0.1 kHz above).
4. `MixF` rounding changed; it only feeds change-detection keys.
5. GPU buffers keep their last upload when the quantised key is unchanged; values may differ from live ones by under 1/4 px, the same tolerance the skip already used.
