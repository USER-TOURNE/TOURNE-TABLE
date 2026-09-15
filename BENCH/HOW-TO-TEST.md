# Present() pacing bench — method and results

`tourne-table-bench.wh.cpp` is the shipping mod with **143 lines added and 6 changed**. It exists to answer one question with microsecond measurement instead of argument, and it has now answered it.

**It is not for publication.** Different `@id` (`tourne-table-bench`) so it installs alongside the real mod; `@name` marks it. Keep it that way.

---

## The result

**Review finding #5 does not apply to this mod.**

48 windows × 5 s = **240 seconds, 15,376 frames**, four configurations switched live in one session (2026-09-14, 23:45–23:50).

| Configuration | Windows | fps | Present avg | SD | Blocked % of wall |
|:--|--:|--:|--:|--:|--:|
| **sync=1 buffers=2** *(shipping)* | 24 | 63.96 | **0.618 ms** | 0.197 | **3.95%** |
| sync=0 buffers=2 | 4 | 64.03 | 0.631 ms | 0.096 | 4.05% |
| sync=0 buffers=3 | 5 | 64.00 | 0.656 ms | 0.155 | 4.20% |
| sync=1 buffers=3 | 11 | 63.97 | 0.638 ms | 0.197 | 4.08% |

*First window after each change excluded — those capture swap-chain rebuild cost.*

Both remedies the reviewer proposed, tested directly:

| Change | Δ Present avg | Δ blocked | Δ fps | p |
|:--|--:|--:|--:|--:|
| sync 1 → 0 | +0.012 ms | +0.100 pp | +0.067 | 0.842 |
| buffers 2 → 3 | +0.020 ms | +0.132 pp | +0.014 | 0.773 |

Both move the number by hundredths of a millisecond, **in the wrong direction**, at p ≥ 0.77.

**Frame rate is 63.97 fps (SD 0.158, range 1.1 fps across 44 windows) in every configuration.** `Present` never caps it.

### Why

On a DirectComposition swap chain, DWM owns presentation timing. The waitable timer is already doing the pacing — which is why the frame rate is identical to three significant figures regardless of sync interval — so the sync interval never becomes the gate the finding assumes it is.

### Decision

The shipping default (`Present(1, 0)`, `BufferCount = 2`) is correct and stays. The tunables were **not** merged into the production mod: they provably do nothing, and each change costs a dropped frame (below).

---

## Two things the run exposed in passing

**Swap-chain rebuild costs one frame.** The first 5 s window after each live change:

| Time | Configuration | Worst Present | Steady state |
|:--|:--|--:|--:|
| 23:47:00 | sync=0 buffers=2 | 17.646 ms | ~4 ms |
| 23:47:25 | sync=0 buffers=3 | 15.593 ms | ~4 ms |
| 23:47:55 | sync=1 buffers=3 | 11.995 ms | ~4 ms |
| 23:48:55 | sync=1 buffers=2 | 16.466 ms | ~4 ms |

One dropped frame at 64 fps. Harmless, but it is an argument against exposing these as user settings.

**The frame rate lands on the system timer tick.** 63.97 fps, and `1 ÷ 15.625 ms = 64.0` exactly. 15.625 ms is the default Windows timer period. If `Target FPS` is not set to 64, the waitable timer is quantising to the system tick rather than hitting the requested rate — a variant of the integer-division issue the reviewer noted separately, and unlike finding #5 this one may be worth fixing.

---

## What was changed from the shipping file

| | |
|:--|:--|
| `@id` | `tourne-table-bench` — installs alongside the real mod |
| `@name` | marked as a bench build |
| `scd.BufferCount` | reads the setting instead of a hardcoded `2` |
| 3 × `Present(1, 0)` | `BenchPresent()` — times the call, applies the sync setting |
| new | `BenchPresent()` and its counters |
| new | a `BENCHMARK` settings group |

Only the three `Present` calls on the **per-frame path** inside `RenderVisualizer` were swapped. The two one-off calls — `CaptureWallpaperBitmap` (blur re-bake) and `PauseForFullscreen` — were deliberately left alone so they cannot pollute the per-frame average.

**Defaults match the shipping mod exactly** (`sync=1`, `buffers=2`), so a fresh install behaves identically to production and the first reading is a true baseline.

---

## Running it again

1. **Disable the real Tourne'Table mod.** Two visualizers drawing at once wrecks the numbers.
2. Install `tourne-table-bench.wh.cpp` in Windhawk.
3. Open its **mod log** (Windhawk → the mod → Advanced) and leave it visible.
4. Start music and keep it playing — silence stops rendering, which stops the measurement.
5. Change `[BENCH] Present sync interval` / `Swap chain buffer count` live and watch the lines. A line appears every 5 s:

```
[BENCH] sync=1 buffers=2 | 321 frames (64.0 fps) | Present avg 0.722 ms, worst 4.507 ms | thread blocked 4.6% of wall time
```

Discard the first window after every change — it carries the rebuild hitch.

---

## What this does not settle

**The measurement above was taken at 64 fps against a 144 fps target.** See below — the frame rate was capped by a pacing bug, not by choice.

That matters, because finding #5 predicts parking *"whenever the target FPS meets or exceeds the refresh rate."* At 64 fps on a 144 Hz display that precondition was never met. The data above is real, but it does **not** refute the finding — it was collected under conditions where the finding would not bite regardless. The question is still open until the same measurement is repeated at a genuine 144 fps.

Finding #5's premise was also *"the shell's UI thread is parked inside `Present`"*, written against the old `@include explorer.exe` design. As a tool mod, `RenderVisualizer` runs on the mod's **own process**, so even a large figure would not be touching desktop responsiveness. That architectural argument stands on its own, independent of any measurement.

---

# `tourne-table-bench-qpc.wh.cpp` — the pacing fix

A second instrument, identical to the one above except for frame pacing. Third `@id` (`tourne-table-bench-qpc`) so all three builds can be installed side by side.

## The bug it fixes

The shipping build measures elapsed frame time with `GetTickCount64()`:

```cpp
UINT interval = 1000 / (UINT)fps;              // 144 -> 6 ms (and 6.944 truncated)
ULONGLONG now = GetTickCount64();              // ~15.625 ms resolution
ULONGLONG elapsed = now - lastRenderTick;
if (elapsed < interval) { preciseWait(interval - elapsed); continue; }
```

`GetTickCount64()` only advances on the system timer tick — **15.625 ms**. So `elapsed` can only ever read 0 or ~15–16. It can never read 6. The loop waits while `elapsed == 0`, then renders the instant the tick advances: **one frame per system tick = 1 ÷ 15.625 ms = 64.0 fps**.

This caps *any* Target FPS above ~64. Measured: 63.97 fps (SD 0.158) against a 144 target.

The waitable timer was never at fault — it is created with `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` and `preciseWait()` is accurate. Only the elapsed-time measurement was too coarse to use it.

## What changed

| | |
|:--|:--|
| Elapsed time | `QueryPerformanceCounter` instead of `GetTickCount64` — sub-microsecond, so a 6.944 ms interval is resolvable |
| Interval | `1000.0 / (double)fps` instead of `1000 / (UINT)fps` — 6.944 ms, not 6 |
| `preciseWait` | takes a `double`; the timer's own unit is 100 ns, so rounding to whole milliseconds would give back most of the precision (7 ms → 142.9 fps) |
| Watchdog | still uses `GetTickCount64` — it compares against 1000 ms, far above the tick resolution |
| Log line | adds `target=N` so requested vs achieved is unambiguous |

**Not** fixed with `timeBeginPeriod()`. Raising the global timer resolution would also work, but it degrades system-wide power behaviour — the opposite of this mod's purpose.

## What to measure

Install all three, enable one at a time, **Target FPS 144** throughout.

1. **`tourne-table-bench`** — expect ~64 fps. Confirms the cap.
2. **`tourne-table-bench-qpc`** — expect ~144 fps. Confirms the fix.
3. **`tourne-table-bench-qpc` at sync=1 vs sync=0** — this is the real test of finding #5, now that the target actually meets the refresh rate.

Watch for two things beyond frame rate:

- **CPU cost.** At 144 fps the render thread wakes 144×/sec instead of 64. The mod's cost should rise, plausibly close to double. That is the price of the setting working as documented, and it needs to be known before any performance claim is repeated.
- **Whether `Present` starts blocking.** If `blocked %` jumps materially at 144 fps with `sync=1`, finding #5 was right and the precondition simply was never reached before.
