# ❀ CHANGELOG

*A lite version history for `Tourne'Table [Audio Visualizer]`.*

---

## ❀ v1.4.4 — The Buttons Were Padded, the Clicks Were Not

### ✦ Fixed: media control clicks landed left of the buttons

Introduced by the padding setting in v1.4.2 and caught on a read-through, not in use.

`PaintMediaControls` draws the first icon inset by the plate padding. The click handler in `MediaWndProc` was never updated to match, and kept testing from the left edge of the window — so every button's clickable region sat `padding` pixels to the left of the button actually drawn there.

At a small padding it read as buttons that were slightly awkward to hit near their right edge. Once padding exceeded the icon spacing, the regions shifted far enough that clicking **Play** triggered **Previous**.

Only reachable with **Backing Plate Padding** above `0`. At the default of `0` the two agreed exactly, which is why v1.4.2 looked correct.

### ✦ Note

Vertically the whole strip stays clickable, padding included, rather than only the icon row — the buttons are more forgiving to hit, and the strip already swallows the click either way. The horizontal gaps *between* icons remain dead, as before.

---

## ❀ v1.4.3 — Every Color Setting's Help Text Was Cut in Half

### ✦ Fixed: settings descriptions truncated at "Format is"

All fifteen colour settings in the mod explained their accepted formats with a line ending `Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)`. None of them ever displayed it.

In YAML, a `#` preceded by whitespace starts a comment. These descriptions were unquoted, so the parser threw away everything from the first ` #` onward — meaning the ⓘ tooltip on every colour field stopped dead at the words "Format is", precisely where the useful part began. The one piece of text whose entire job was telling you how to write a colour was the one piece that never arrived.

Every affected value is quoted now. This has been wrong since colour settings were first added; it survived this long because a truncated description still looks like a finished sentence if you don't know what was supposed to follow.

Restored in full, beyond just the format hint:

- **Peak Hold Color** — that it's independent of Color Mode, and the caps are always drawn in it even under Rainbow / Album Art / Accent / Acrylic.
- **Beat Flash Color** — that the alpha governs flash strength even at full intensity, and lowering it gives a tint rather than a full colour swap.
- **Now Playing / Peak Readout Background** — what the panel is for and that it's transparent by default.
- **Backing Plate Color** — the `#8C141414` suggestion for pale icons on a pale wallpaper.

### ✦ Note

The mod's own settings validator can't catch this class of bug: it checks what users type into fields, whereas this was wrong in the schema that *describes* the fields. Neither can the compiler, since the whole settings block is a comment as far as it's concerned. A scan for the pattern now runs against the settings block before release.

---

## ❀ v1.4.2 — Padding for the Media Controls Plate

### ✦ New: `Media Controls ▸ Backing Plate Padding`

The Now Playing and frequency readout panels each got a padding setting in v1.4.0 and the media plate did not, which left its border and fill sitting flush against the icons with no way to open them up.

It **grows the strip** rather than shrinking the icons, so turning it up never makes the buttons smaller or harder to click.

Two consequences worth knowing:

- Padding applies **whether or not the plate is visible**, so that padding, fill and border stay independent of one another. The strip's box gets bigger either way, which very slightly shifts where the Horizontal / Vertical Position percentages land — position is a percentage of the *remaining* screen space, and a wider strip leaves less of it.
- **Hide When Covered** measures the strip's rectangle, so a padded strip counts as covered marginally sooner.

Both default to no change at all, since the padding defaults to `0`.

### ✦ Under the hood

The icon blitters previously drew at a hardcoded `y = 0`, which was only safe while the window height *was* the icon height. They take a Y origin now and the icons are centred in the padded content box.

The travel-range helper used by keyboard move duplicates the strip's sizing math on purpose — so a pixel nudge converts to the same percentage the repositioning turns back into pixels — and was updated alongside it. Had it been missed, moving the strip with the keyboard would have drifted against the padding.

---

## ❀ v1.4.1 — The Oscilloscope Respects Orientation

### ✦ Fixed: the Oscilloscope drew sideways when Orientation was Vertical

Every other shape reads `Orientation` and lays itself out accordingly. The Oscilloscope never did — it always swept its trace left-to-right and deflected up-and-down, no matter what.

In Horizontal that happens to be correct, which is why it went unnoticed. In Vertical the two axes swap: the group becomes a tall, narrow column where the long axis is *height* and `Bar Max Size` is the *width*. The trace kept sweeping across the width, so the entire waveform was crushed into a strip as wide as the bars were long and squashed into the middle of an otherwise empty column.

It's now the horizontal layout rotated 90° clockwise: the sweep runs **top to bottom**, and what was "up" on the trace becomes "right". Everything else about the shape — colouring, multiband tinting, beat flash, stroke width — is untouched and shared between both orientations.

---

## ❀ v1.4.0 — Panels and Borders for the Overlays

Everything that draws on top of the wallpaper can now have something behind it.

### ✦ New: a border on the Media Controls backing plate

`Media Controls ▸ Backing Plate Border Size / Color`, following the plate's own corner radius.

The border draws **whether or not the plate has any fill**, so a hairline outline with nothing behind the icons is a valid look — leave the plate colour transparent and set only a border.

Fill and border are rasterized in one pass now: coverage of the outer rounded rect minus coverage of the inner one gives the ring, supersampled the same way the built-in glyphs are so the corners don't stair-step. The fill runs under the border rather than stopping at it, so a translucent border blends over the fill instead of cutting a hole through it.

### ✦ New: backgrounds for the Now Playing text and frequency readout

Both get a panel of their own — `Background`, `Background Padding`, `Corner Radius`, `Border Size`, `Border Color` — all **fully transparent by default**.

The panel is sized to **the text**, not to the visualizer. The text is centred inside a layout box far wider than itself, so using that box would have stretched the panel across the whole reserved area. Measuring means building a text layout, which `DrawText` does internally anyway — so when a panel is on, that layout is kept and drawn from, and nothing gets measured twice. With no panel the old `DrawText` path is untouched.

Two details that matter in use:

- The Now Playing panel **fades in and out with the text** rather than popping, since it shares the same alpha.
- This is most useful on the frequency readout's *Top / Middle / Bottom* alignments, where it sits directly over the bars — a moving, recolouring background is about the worst thing to ask text to be legible against, and a panel fixes it without moving the readout outside the visualization.

Layout reserves room for the padding and border as well as the offset, so a panel can't get clipped at the edge of the render surface while the text inside it stays visible.

---

## ❀ v1.3.0 — Transparent Icons Actually Transparent, Movable Text, Partial-Coverage Pausing

All four from real-world use of v1.2.0.

### ✦ Fixed: the dark box behind the media icons

v1.1.2 added a translucent dark backing plate behind the icon strip to fix pale icons vanishing against a pale wallpaper. It was drawn unconditionally — which meant icons with their own transparency could never look transparent, because there was always a plate behind them.

It's a setting now, **fully transparent by default**: `Media Controls ▸ Backing Plate Color`, with a corner radius to match. The contrast fix is still one alpha value away (`#8C141414` restores the old look) but you're no longer opted into it.

Two related things fixed while in there:

- **Custom icons punched transparent holes through the plate.** The blit was a straight `memcpy`, so an icon's fully transparent pixels *overwrote* the plate instead of letting it show through. Both the custom-icon blit and the built-in glyph rasterizer now do a proper premultiplied source-over composite.
- The plate's corners are antialiased the same way the built-in glyphs are, so a rounded plate doesn't come out with stair-stepped corners.

### ✦ New: everything movable moves with the same keys

The text overlays only had a coarse alignment dropdown, which is why they ended up sitting on the background panel with no way to get them off it. The Media Controls strip only had percentage fields.

Both text overlays now have **Offset X / Y** settings in pixels — negative is left/up, positive is right/down — and every movable piece can be nudged live with the same keyboard move that places the visualizer:

| Combo | Steers |
|:--|:--|
| `Modifier + 1` | The visualizer *(default)* |
| `Modifier + 2` | The Now Playing text |
| `Modifier + 3` | The frequency readout |
| `Modifier + 4` | The Media Controls strip |

Pick a target once and nudge as long as you like; `Modifier + Home` resets whichever is selected.

The two kinds of placement persist differently, on purpose. The visualizer and the strip are positioned against the **monitor**, so they store a percentage that replaces their Position settings. The text overlays are positioned against the **visualizer** — "just above the panel" is a statement about the box, not about the screen — so they store a delta *on top of* their Offset settings, which means a number typed into those is never silently ignored because a nudge happens to exist.

Under the hood: the render surface grows to make room for an offset (otherwise offset text just walks off the edge and disappears), but that room is rounded up to a 32px step — following every single pixel would mean a DXGI buffer resize dozens of times a second while you hold an arrow key down. The text's anchor point was also split from the reserved room, so nudging the title sideways no longer drags the frequency readout along with it.

### ✦ New: Media Controls can get out of the way

`Media Controls ▸ Hide When Covered`, with its own 5–100% threshold. The strip is topmost — that's deliberate, it has to receive your clicks — but that also means it sits over your maximized browser forever. With this on it hides while a real window is underneath it and comes back when that window moves or closes.

Off by default: a control strip that disappears unexpectedly is worse than one that's occasionally in the way. The topmost re-assert still runs either way, so another app stealing z-order can't strand it.

Coverage is re-checked once a second, so hiding and reappearing aren't instant — and while the strip is parked it's genuinely hidden, so a keyboard nudge still applies but you won't see it land until it comes back. Both of those are now spelled out in the setting's own hover text rather than left to be discovered.

### ✦ New: Pause When Covered has a threshold

`Performance ▸ Pause When Covered - Threshold`, 5% to 100% in steps of 5, default `100%` (the old behavior — pause only when completely hidden).

The check used to be pure full-containment: a single window had to swallow the visualizer's whole box on its own. That meant two windows covering opposite halves counted as *not covered at all*. Coverage is now accumulated into a GDI region and measured as real area, so overlapping and adjacent windows add up correctly — and the occlusion log line now fires on the transition rather than once per second forever.

### ✦ The settings validator now warns, not just rejects

It could previously only say "this value couldn't be read." It can now also flag a setting that parses perfectly and works exactly as configured, but probably isn't doing what you meant.

First case: **a one-key Keyboard Move Modifier.** The hook swallows the keypress outright, so the app underneath doesn't fall back to its own behaviour — it never hears the key at all. With `Ctrl` + WASD that means no `Ctrl+A`, and no `Ctrl+S`, so a save you think you made silently doesn't happen. The warning names the exact shortcuts you've given up for your specific combo, and it's only a heads-up — nothing is blocked, because it's a legitimate choice if you know what you're trading.

The same caveat is now in the modifier's own hover text and in the README, including the reason it bites harder than a normal shortcut clash: a swallowed key isn't a conflict the app resolves in its favour, it's a key the app never sees.

---

## ❀ v1.2.0 — Keyboard Move, Media Controls Fixed, Settings That Tell You When They're Wrong

v1.1.3 moved the drag hook off the render thread and it *still* felt like dragging through mud. So this release stops trying to make cursor-chasing fast and does the thing that doesn't need to be fast.

### ✦ New: Keyboard Move

Hold a modifier, tap a direction, the visualizer steps by an exact number of pixels — the way a window manager nudges a tiled window. **Ctrl + Alt + arrows or WASD** by default, 1px per press, hold **Shift** as well for 10px. **Ctrl + Alt + Home** throws away a saved position and snaps back to the Position percentages.

The reason this is immune to the problem drag has: one keypress is one discrete jump. There's no stream of intermediate positions for the renderer to keep up with, so there's nothing to fall behind. Every press lands exactly where it says, whether the redraw arrives in 2ms or 40ms.

Step size, direction key set (arrows / WASD / both), modifier and fast key are all configurable.

- **Drag-to-Move is now off by default.** It still works and all its settings are untouched — it's just no longer the way you're pointed first. Its description now says plainly why it can feel sluggish rather than leaving you to wonder whether it's broken.
- Keyboard moves also force a redraw on the spot, so you can reposition with nothing playing instead of having to start a track to see where the box went.
- The position write is debounced — key repeat can fire dozens of nudges a second, and each one used to mean a file write.

### ✦ Fixed: Media Controls

Three separate reasons the strip could fail to appear, all of them silent:

- **The UI thread never initialized COM.** Every `CoCreateInstance` made from it failed outright with `CO_E_NOTINITIALIZED` — which means WIC's imaging factory never activated and **custom icon paths have never worked, in any version.** Set one and you got the built-in glyph, indistinguishable from having left the field blank. The UI thread now initializes COM apartment-threaded.
- **`UpdateLayeredWindow` was being told to infer the window's position and size.** Passing `NULL` for those is only documented as valid when they aren't changing — and here they always are, since the window is born 1×1 and every settings change can resize it. It's now handed both explicitly, painted before it's shown, and its return value is actually checked and logged.
- **Nothing ever retried.** If window creation failed at startup, turning Media Controls on in settings did nothing at all, forever. It now recreates the window on a settings change, and a once-a-second check re-asserts the strip if it has lost topmost or gone missing entirely.

Settings changes also now route to the UI thread through the message window when the overlay doesn't exist yet, instead of running inline on Windhawk's own settings thread and creating windows on a thread that never pumps them.

### ✦ New: Settings Validation

Windhawk saves whatever you type into a free-text box. This mod used to quietly fall back to a default on anything it couldn't parse — so a typo looked exactly like a setting that had saved fine and simply did nothing. Which is a miserable thing to debug, because there's no signal at all.

Now every free-text field is validated, and anything that fails produces a summary window naming the field, what you typed, what format was expected, and what's being used instead:

- **Colors** — all nine of them, with the accepted formats spelled out.
- **Positions** — and `50px` or `50 50` is now rejected rather than silently taking the leading number.
- **Padding / corner radii** — anything other than exactly one or four values. Two in particular *looks* like it might mean horizontal/vertical, and it doesn't.
- **Icon paths** — file doesn't exist, or points at a folder. Surrounding quotes from Explorer's "Copy as path" are stripped rather than treated as an error. A file that exists but won't decode is logged too.
- **Now Playing font** — checked against the fonts actually installed, and only when the text is switched on.
- **Keyboard Move Fast Key** — flagged when it's already part of the modifier, since it could then never be "additionally held."

Off-switch under **Settings Validation ▸ Warn About Invalid Settings**. The same list goes to the mod log either way, so turning it off makes it silent, not blind.

---

## ❀ v1.1.3 — Fixed: Drag Lag (For Real This Time)

The diagnostic logging added in v1.1.2 paid off — the raw numbers pointed straight at the actual cause.

- **Fixed: dragging felt laggy/resistant the whole time, not just at release** — the drag hook and the render tick were sharing one thread. Every frame, that thread renders the visualizer via Direct2D and presents it, which can block for real time; while it's blocked, queued mouse-move messages back up behind it, so the box's position only advanced in bursts whenever a render tick let go of the thread — reads exactly like "laggy" or "resistant." The position math itself was correct throughout (confirmed from the logs), the display of it just kept getting stalled. Moved the mouse hook to its own dedicated thread, isolated from rendering, so cursor tracking is never delayed by a Present() call.
- `g_cachedMonitor` made atomic — it's now read from a genuinely separate thread (the new drag-hook thread) instead of incidentally sharing the UI thread, so it needed real cross-thread safety instead of implicit same-thread ordering.
- The double-click-to-clear path no longer touches the swap chain directly from the hook thread (that would have been a new cross-thread hazard against the render tick); it just clears the saved override and lets the next tick's own `UpdateSwapChainForLayout()` call pick it up, same as it already does every frame.

---

## ❀ v1.1.2 — Media Controls Contrast Fix + Drag Diagnostics

v1.1.1's fixes didn't resolve either report on retest -- this pass fixes one for real and adds logging for the other instead of guessing a third time.

- **Fixed: Media Controls could be genuinely invisible** -- the default icon color is pure white on a fully transparent background; against a similarly light patch of wallpaper (easy to land on, since the default position sits near the bottom-center of the screen) that's close to indistinguishable. Added a translucent dark backing plate behind the whole icon strip, so it's visible against any wallpaper regardless of icon color.
- **Drag rubberbanding not yet fixed** -- v1.1.1's desync theory didn't hold up (reducing the per-move workload made no difference on retest). Rather than guess again, added throttled diagnostic logging at drag start/move/end and at each render tick during a drag (`Wh_Log` lines prefixed `[Drag]`), so the next test run produces real evidence -- cursor position, computed travel range, resulting override percentage, and what the render tick actually saw -- instead of another speculative patch.

---

## ❀ v1.1.1 — Fixed: Drag Desync + Missing DPI Awareness

Found from actually running v1.1.0 for the first time -- thank you real-world testing.

- **Fixed: dragging felt "laggy" and snapped back to the old spot on release** -- the window's on-screen position (updated on every mouse-move, from the drag hook) and the box's drawn content (only recomputed on the next render tick) were two independent calls to the same layout math, taken at different moments. During a fast continuous drag they drifted apart -- the window raced ahead, the content lagged behind -- and releasing let the content's next redraw "catch up," which read as snapping back. Fixed by having the regular render tick own repositioning every single frame, instead of the drag hook doing it separately on its own schedule.
- **Added explicit Per-Monitor-V2 DPI awareness** -- running injected into `explorer.exe`, this was inherited for free (the shell is always DPI-aware). Running as our own standalone process since v1.0.0, nothing declared it, which risked every position/size calculation (the visualizer's own box, and the Media Controls window) running against a DPI-virtualized view of the desktop instead of true physical pixels on a scaled monitor -- plausibly why the Media Controls strip was hard to locate.

---

## ❀ v1.1.0 — Drag-to-Move

The visualizer stays click-through by design, but now with one deliberate exception: grab it and drag it.

- **Drag-to-move** — hold a configurable modifier + mouse button (Ctrl + Middle Click by default) anywhere over the visualizer and drag to reposition it on screen.
- **Bars pause during the drag** — only the background/border box moves while you're repositioning it; bar rendering freezes and resumes once you let go, so the box doesn't visually fight with the bars while it's in motion.
- **Configurable trigger** — new Interaction section lets you pick the modifier (None/Ctrl/Alt/Shift/Win) and mouse button (Left/Middle/Right), in case the default conflicts with something else.
- **Double-click to clear** — the same combo, clicked twice without dragging, clears a saved drag position and snaps back to the Position settings.
- **Persists across restarts, quietly** — since Windhawk mods can only read settings, not write them, a dragged position is saved to its own small file (`%LOCALAPPDATA%\TourneTable\position_override.txt`) rather than back into the Position fields you see in the settings UI.

## ❀ v1.0.1 — Fixed: Windows Weren't Being Pumped

A correctness bug from the v1.0.0 standalone-process conversion, caught while building the drag feature (which needed a real message loop to work at all).

- **All windows now sit on a real message-pumping thread** — the standalone conversion created the overlay, message, and media-control windows directly on `WhTool_ModInit`'s calling thread, which the tool-mod launcher terminates shortly after. Every one of those windows was at risk of losing its message queue (timers, settings-changed handling, media button clicks) once that thread went away. Fixed by spinning up a dedicated, persistent thread with an actual `GetMessage` loop and creating every window there instead.

---

## ❀ v1.0.0 — Runs as Its Own Process

The mod no longer lives inside `explorer.exe`. It now runs as its own dedicated process using [Windhawk's tool-mod pattern](https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process) — its own entry in Task Manager, separate from the shell.

- **Standalone process** — `@include` moved from `explorer.exe` to `windhawk.exe`; the mod now spawns and lives in its own dedicated host process.
- **Survives shell restarts** — killing or restarting `explorer.exe` no longer takes the visualizer down with it. It reattaches behind the desktop icons the moment the shell is back.
- **Bootstrap rebuilt for cross-process life** — the old explorer-injection tricks (a `CreateWindowExW` hook to detect the shell's readiness, and a cross-process code-execution trick to marshal window creation onto explorer's thread) are gone, replaced with a direct, self-rescheduling retry loop that simply waits for `WorkerW` to exist.
- **`GetWorkerW()` un-coupled from explorer** — a leftover same-process safety check would have silently broken the entire visualizer the moment it stopped running inside `explorer.exe`; removed.

*Requires a Windhawk build with tool-mod support (1.7.3+, including the 2.0 alpha line).*

---

## ❀ v0.9.0 — Media Controls

A small Previous / Play-Pause / Next button strip, wired to whatever's actually playing.

- **Media Controls section** — Enabled toggle, Icon Color, and four separate icon-path fields (Previous / Play / Pause / Next).
- **Talks to real playback** — uses the same native `GlobalSystemMediaTransportControlsSession` API the Now Playing text already reads from. No per-app setup.
- **Bring your own icons** — point any button at a local PNG/JPG/BMP/ICO file; leave it blank and a clean built-in glyph is drawn instead.
- **Independent placement** — its own decimal-precision Horizontal/Vertical Position, entirely separate from where the visualizer itself sits.
- **Actually clickable** — lives in its own small always-on-top window, deliberately *not* click-through, since the visualizer itself must stay that way.

---

## ❀ v0.8.1 — Smoother, More Precise Motion

- **Motion Smoothing** — new setting to slow down bar attack/decay, making small on-desktop bars easier to read without chasing every frame-to-frame jitter.
- **Finer position control** — Horizontal/Vertical Position now accept decimals (`50.25`, not just `50`), so a single step is no longer a big jump on a large monitor.

## ❀ v0.8.0 — Color Fixes & Beat Flash Rework

- **Beat Flash Color** — the flash-on-beat effect gets its own dedicated color (hex or `rgba()`/`rgb()`, with alpha), replacing an old "boost toward white" effect that washed out regardless of Color Mode.
- **`rgba()` / `rgb()` support, mod-wide** — the shared color parser now accepts CSS-style color strings everywhere a hex color was accepted before.
- **Acrylic mode fixed** — removed an alpha floor that kept it from ever truly disappearing in silence, as advertised.
- **Now Playing text locale fixed** — swapped a hardcoded `en-us` for `GetUserDefaultLocaleName()`, improving font fallback/shaping for non-Latin text.

## ❀ v0.7.2 — Negative Padding

- **Padding can now shrink, not just grow** — negative values pull a side of the background panel inward, past the bars if pushed far enough, instead of only ever expanding outward.
- **Degenerate-rect safety net** — an extreme negative value holds that side open to a 1px sliver rather than inverting the panel's geometry.

## ❀ v0.7.1 — Independent Panel Control

- **Peak Hold Cap Color** — no longer locked to Gradient Color 2; caps get their own independent, always-applied color regardless of Color Mode.
- **Independent per-side Padding** — grow the background panel's left, right, top, or bottom edge on its own, without moving the bars or touching any other setting.

## ❀ v0.7.0 — Foundation

- **Seqlock-based band publishing** — thread-safe handoff of per-band audio data from the capture thread to the render thread, replacing a per-element atomic approach that could tear under high refresh rates.
- **Sensitivity Curve** — Exponential / Knee / Power handling for how the top of the Sensitivity range compresses once a band would otherwise overshoot max bar height.

---

### A note on "efficiency improvements"

A handful of performance-adjacent spots got a close look across this span — Target FPS's integer-division quantization, `GetWorkerW`'s permanent materialization of a `WorkerW`, and whether `FillRoundedRectPerCorner` could share the background panel's geometry cache. None of them turned into changes: the FPS quantization is harmless in practice, the `WorkerW` side effect is inherent to the behind-the-icons technique itself (and matches what every comparable mod does), and the per-bar geometry changes too often each frame for a cache to help. Rather than efficiency wins, this span was mostly correctness fixes and new customization — the bigger performance rewrite (roughly half the CPU, 18°C cooler) predates this changelog.
