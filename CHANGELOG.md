# ❀ CHANGELOG

*A lite version history for `Tourne'Table [Audio Visualizer]`.*

> Everything below `v1.0.0` is pre-release development history, numbered `0.1.0` upward toward the first release. The internal build numbers used while this was being written have been folded into that sequence, so a version referenced inside one of these entries always matches a heading in this file.

---

## ❀ v2.0.0 (beta): Every Bar Its Own Band, Your Choice of Silicon, and a Renderer That Idles to Nothing

Beta build, in `BETA_BUILD_2.0/`. Same `@id` as the 1.4 beta, so it upgrades that install in place and keeps its settings. The release build is untouched at 1.1.0.

> **Beta fix:** the Direct3D 11 shaders failed to compile on Windows ("unexpected token 'pass'"), so every PC fell back to Direct2D. Windows' own shader compiler (`fxc`) reserves `pass` and doesn't accept a macro with an empty parameter list, and the DXC compiler used for testing accepts both. Both are fixed, and `tests/fxc_lint.py` now checks for anything else in that class before a build goes out.

### ✦ New: eight styles, Reflection, a Media Card, FX and My Styles

- **Styles in the Shape list:** LED Meter (segmented, green / amber / red, peak segment held), Line Spectrum (a smooth filled curve with a glowing edge), Polar Bloom (Radial as one filled shape), Spectrogram (a scrolling colour history with a legend), VU Needles (two analog L / R meters with IEC VU ballistics: 99 % in 300 ms, 1.5 % overshoot, plus peak LEDs), Stereo Field (left channel above the centre line, right below) and Particles (sparks thrown off the bar tops on each beat). Each rides on an existing shape, so layout, panel, colours and settings work as before. They're also in the right-click menu.
- **Reflection** (Appearance, 0-100): mirrors the bars onto a floor beneath them, fading out. Horizontal, bottom-anchored bar styles, on both renderers.
- **Cost:** each style is one draw call on the same instanced quad. Nothing new is uploaded per frame unless the style has its own data: a Spectrogram row (a few hundred bytes, 60 a second), the Stereo Field levels, or live sparks. A resting VU needle leaves the frame unchanged, so it still skips presents. Spectrogram, Stereo Field and Particles keep the analysis on the CPU (Hybrid), like Terminal.
- **Media Card** (Media Controls > Layout = Card): album art with previous / play / next on hover, a seek bar (click or drag), a speaker button that switches the Windows default output in one click, and a volume slider (drag it, or scroll anywhere on the card). It repaints only when something on it changes.
- **Card theming** (Media Controls): Card Background, Card Border Color and Size, Card Corner Radius (the art follows it), Card Art Size, and Card Accent taken from the icon colour, a custom colour, the album art, or the Windows accent.
- **Glow and Bloom** (Appearance, 0-100 each, with a radius): Glow is a soft halo worked out in the same shader pass that draws each bar, so it costs next to nothing. Bloom adds a blurred copy at a quarter of the size back on top, only on frames that change. Direct3D 11 renderer.
- **My Styles and the Style Editor** (right-click > My Styles): save a look (base style, colours, peak caps, beat flash, bar sizes, corner radius, reflection, glow and bloom) under a name and pick it from the menu later. The editor is a small window where the visualizer itself is the live preview; slider changes apply about 12 times a second while dragging.

### ✦ New: finer control, bar modifiers, placement

- **Outline and Shadow** (Appearance): a stroke around each bar and a soft drop shadow, each with its own colour. Album colours now ease in over 0.6 s on a track change. Direct3D 11 renderer.
- **Reflection on Direct2D** too (drawn on the CPU there, so it costs a little more).
- **Decimal sizes** for bar width, gap, height, padding, borders, radii, font sizes, offsets and the card: turn Pixel Snap off to keep the fractions.
- **Scale numbers** on Spectrogram (dB or %) and VU Needles.
- **Click to Seek** on the progress bar (off by default), with an app list that turns it off, or only on, while those apps run.
- **Terminal: any character**, including any Unicode glyph, for the bars and the ramp.
- **Bar modifiers** (Appearance): Hollow, Dashed, Tilt, Afterimage (a trail that falls slowly) and Mirror Gap. Each is one shader branch, free when off.
- **Snap while dragging**: edges and centre catch on the screen, the taskbar edge, the tray and the Start button. Hold Shift to place freely.
- **Dock To App** (Position): sits beside a chosen app's window and follows it as it moves; back to its usual place while the app is closed or minimized.
- **Split Into Two Pieces** (Position): cut at a chosen point, the second piece moved by a gap and shift. Windows composes the second piece from the same frame, so it costs next to nothing.
- **Fix:** the drag and menu hooks no longer take clicks while another app (e.g. a full-screen game) covers the visualizer.

### ✦ Efficiency and fixes pass (review merge)

- **Nothing changed, nothing sent**: the compositor is only committed when the panel actually moves (was every frame, 144/s), buffers are hashed first and only uploaded when the frame will be drawn and that buffer changed (432 to 144 uploads/s while playing, 0 when still), the background plate is drawn without blending and without a redundant clear.
- **No empty layer for DWM**: with Now Playing, the readout and the progress bar off (the default) the text surface is taken out of the composition instead of being blended over the whole box every frame.
- **Readout at 10 Hz**, with the frequency held within one display step, instead of redrawing text every frame.
- **Terminal**: a changed-grid serial replaces hashing 65,536 cells per tick, and only glyph cells are uploaded and drawn.
- **Silence costs nothing**: after digital silence fills the analysis, FFTs stop until a non-zero sample arrives; trickle drains audio events without full frames; one `exp()` per frame for the ballistics and a table-free `log10` per band; true peak only when the full loudness readout shows it, and skipped for windows that can't raise it; the peak-frequency search runs on power and logs only new maxima.
- **Deep Idle** no longer wakes in a loop on a noise floor between the meter and engine thresholds, applies Input Gain, and stays off for input devices (whose meter can read 0 once the stream stops).
- **Audio devices**: a chosen device that won't open (exclusive mode) falls back to the default output with a notice and is re-probed every 30 s; retries back off from 0.5 s to 30 s.
- **Fixes**: a use-after-release crash when the media session changed while its events were running; the progress bar snapping back on pause and jumping on resume; Waterfall scrolling past the end of its history; progress / two-line Now Playing clipped inside the panel; right-click on the empty desktop while Auto-Hide had faded the scene; a second right-click while the menu was open; the media strip ignoring Ctrl + Right-Click; the readout menu had no "Peak frequency and loudness" option. New: **Copy Quick Settings** in the menu.
- **Performance Stats** (Performance section): one `[Perf]` line in the Windhawk log every 30 s with engine wakes and time, analyses and FFTs, render ticks and time, frames presented vs skipped, text redraws, compositor commits, buffer uploads and the idle split, so the numbers above can be checked on any PC.

### ✦ New: a right-click menu, any audio device, a Terminal shape, a media widget

- **Right-click the visualizer** for quick settings applied live: shape, colours, engine, band layout, weighting, ballistics, readout, workload, renderer, frame rate, the overlays, Pixel Snap, and Pause. Mods can't write their own settings, so menu choices are stored by the mod and laid over the settings page; **Reset Quick Settings** hands control back. The menu only opens where the visualizer is visible on the desktop, never through a covering window (**Right-Click Menu**: Right-Click, Ctrl + Right-Click or Off). The media strip opens it too.
- **Audio Source** (issue #1): the default output as before, the default input, or **any active output or input by name**, virtual devices included (VB-Audio Cable / Matrix, Voicemeeter, an Ableton return). Outputs are captured by loopback, inputs recorded directly. A missing device falls back to the default output with one heads-up, and comes back by itself when reconnected. The right-click menu lists every device and switches live.
- **Terminal shape**: the visualizer as characters, in **Columns** (cava style, hot colour above a threshold, peak glyphs), **Waterfall** (a character spectrogram) or **Meters** (`Bass: [####   40%]`). The Direct3D 11 renderer bakes the glyphs once and draws the whole grid in one instanced call.
- **Media widget layout**: Now Playing on **two lines** (title, then artist in its own colour), placed **inside the panel** above or below the bars with Left / Center / Right alignment; **Display Seconds 0** keeps it on screen; **Text Rendering = Pixel** for pixel fonts; a **Track Progress** bar from the media session's timeline; and the **media controls anchored to a corner of the panel** so they move with it.
- **Pixel Snap** (on by default) fixes softness at some sizes: DPI-scaled sizes (a 2 px bar is 2.5 px at 125%) and percentage positions used to put edges partway through pixels. Positions and sizes now round to whole pixels; bar heights stay smooth. The UI and input threads also declare per-monitor DPI awareness themselves, so Windows never stretches the picture on a differently scaled monitor.
- **Subpixel placement**: Pixel Snap off, plus **Keyboard Move Fine Step** (1/2 to 1/16 px) with a **Fine Key** or the menu's **Subpixel Nudges**.

### ✦ New: the Precision analysis engine

Up to here the analysis summed the spectrum into 7 fixed bands, and every bar interpolated between those 7 numbers. Bar Count and FFT Size changed how many bars were drawn, not how much the bars knew, which is exactly what the Windhawk reviewer pointed out. Every bar now has its own band with its own edges, measured from the spectrum:

- **Real-input FFT.** N real samples packed as N/2 complex values, one half-size FFT and a post-twiddle: half the work of a complex FFT on real data.
- **Three resolution tiers.** The audio as captured, decimated by 4 and by 16 through 59-tap half-band filters (90 dB of stopband), each analysed at the same FFT size. Every band is measured from the fastest tier whose window can resolve it, so bass comes from a 683 ms window and treble from a 43 ms one at FFT Size 2048. The deeper tiers are only re-transformed once enough new audio has arrived.
- **Band layouts.** Log, Linear, Mel, Bark or ERB scales across a set range; IEC 61260-1 fractional-octave bands (1/1 to 1/24); or one band per note of equal temperament at a chosen A4.
- **Calibrated levels.** Both detectors read a full-scale sine as 0 dBFS. RMS sums band power by Parseval's theorem; by default bands are referenced to a 1/3-octave bandwidth, so pink noise reads flat and bar heights stop depending on Bar Count. Sensitivity no longer needs retuning per FFT Size.
- **A / C / Z weighting** (IEC 61672-1) and a dB-per-octave **tilt**, applied per band.
- **Ballistics on elapsed time.** Every coefficient is `1 - exp(-dt / tau)` or a dB-per-second rate times dt, so bars move identically at 60, 144 and 240 FPS. Presets: Snappy, Smooth, Analyzer (20 dB/s), VU, PPM (EBU Type II and DIN Type I), Custom. Peak caps hold, then fall with gravity or linearly.
- **Loudness and stereo meters** in the readout: BS.1770-5 / EBU R128 momentary, short-term and gated integrated loudness, 4x-oversampled true peak, PLR, and correlation. A new **Goniometer** shape plots the stereo field.

The 1.4 analysis is kept as **Analysis Engine = Classic**, numbers unchanged.

### ✦ New: Workload (GPU / CPU / Hybrid)

- **Hybrid** (default): analysis on the CPU, drawing on the GPU.
- **GPU**: the whole precision analysis in four compute passes (FFT, bands and ballistics, reduction, shape mapping) that write the buffer the vertex shader draws from. Nothing comes back to the CPU each frame except an 8-byte stats block, read two frames late without waiting.
- **CPU**: everything on the processor, drawing through WARP.
- **NPU**: kept from 1.5 as experimental, now feeding either engine. The readme explains why an FFT is a poor fit for an NPU at any bar count.

### ✦ New: the Direct3D 11 renderer

Every bar, dot, cap, spoke and scope segment is an instanced quad positioned by a vertex shader that reads bar heights straight from a buffer; the CPU writes two floats per bar and issues a handful of draw calls per frame. The pixel shader computes exact coverage (per-axis overlap for edges, a distance field only inside rounded corners), so shapes match Direct2D's antialiasing. On top of that:

- **Frames that didn't change are skipped**, draw and present both, so DWM has nothing to recompose.
- **The text has its own surface**, redrawn only when the text or its fade changes; the animated surface covers just the panel.
- **Opaque panel.** With Blur on or a solid panel colour (and Auto-Hide off), the panel's swap chain ignores alpha and a DirectComposition clip draws the rounded corners, so DWM copies instead of blending.

Direct2D (the 1.5 path, with Smooth Mode's batching) stays as **Renderer = Direct2D** and as the automatic fallback.

### ✦ Changed: one wake per frame, and three steps of idle

The capture thread is merged into the pacing thread: once per frame it drains the loopback buffer (now 500 ms, polled), analyses, and posts the frame. Roughly 100 fewer wake-ups a second while playing. In silence it wakes only on audio or four times a second; five seconds later **Deep Idle** stops the loopback stream and reads the endpoint peak meter instead, so the mod's audio power request goes away. The thread asks for EcoQoS while idle and opts out while playing.

**Target FPS 0** now means "match the display".

### ✦ How this was checked

On Linux, before it ever touched Windows: the FFT matches numpy to 1e-7; the decimator measures 0.0003 dB ripple and 90 dB stopband; the four EBU Tech 3341 loudness cases land within 0.01 LU; true peak reads the classic inter-sample case (samples at -3 dB, true peak 0 dBTP) within 0.05 dB; ballistics are identical at 60, 144 and 240 FPS. The HLSL was compiled with DXC and also executed on the CPU through a C++ shim (with real barriers for the compute groups): the GPU analysis reproduces the CPU engine to 0.0001 dB, and the rasterised bars, caps, dots and spokes match an independent supersampled re-implementation of the 1.5 Direct2D geometry in every orientation and anchor. The whole mod passes a clang / mingw-w64 syntax check for x64 and x86.

Not yet checked: anything on real Windows hardware. Compile in Windhawk, then compare each Workload and both Renderers with HWiNFO, as 1.1.0 was measured.

---

## ❀ v1.5.0 (beta): GPU / CPU / NPU Selection and Smooth Mode

Never pushed on its own; its changes ship inside 2.0.0.

- **Drawing Device**: Auto (the GPU driving the widget's monitor), Integrated, Discrete or CPU (WARP), with a warning and fallback when the request can't be met.
- **Device-loss recovery**: a driver update, TDR or GPU switch now rebuilds the renderer instead of freezing it until restart.
- **NPU analysis** through Intel's OpenVINO runtime, with the FFT expressed as a four-step DFT in an ONNX model built in memory, compiled off the audio path and proven against a known answer before use.
- **Smooth Mode** for integrated GPUs: frame timing locked to DWM's composition clock at a whole fraction of the refresh rate, motion scaled by elapsed time, a high-resolution animation clock (the old float millisecond clock lost precision with uptime), the background baked into one image, bars and dots in one sprite batch, and cached text layouts.

---

## ❀ v1.4.0 (beta): The Oscilloscope Gets Its Own Time Base

- **Oscilloscope Time Window** decouples the trace from FFT Size: a fixed span of time, sampled about 100 times a second, triggered on a rising zero crossing so it holds still, peak-sampled so it neither aliases nor loses amplitude as the window widens, and faded flat when audio stops.
- **Oscilloscope Damping** eases the trace's shape without slowing how often it's drawn.
- **Input Gain** trims the captured level, the only control that also scales the scope, since loopback sees each app's volume but not the master slider.
- **Auto Gain** lifts quiet sources, boost only, held through silence so idle shutdown still works.

---

## ❀ v1.1.0: Target FPS Actually Works

### ✦ Fixed: every Target FPS above 64 was silently clamped to 64

The render loop measured how long the last frame took with `GetTickCount64()`. That clock only advances on the Windows system timer tick, which is **15.625 ms** by default. At a 144 FPS target the loop wants a 6.944 ms interval, but the elapsed time it read could only ever come back as 0 or about 15 to 16. It could never read 6.9.

So the loop waited while the reading was 0 and rendered the instant the tick advanced: exactly one frame per system tick. `1 / 15.625 ms = 64.0`.

Measured before the fix: **63.97 FPS** (SD 0.156) against a 144 target. Setting Target FPS to 90, 120, 144 or 240 all produced the same 64.

Two things were wrong, and both had to change:

- Elapsed time is now measured with `QueryPerformanceCounter`, which resolves well under a microsecond, so a 6.944 ms interval is actually visible to the loop.
- The interval is computed as `1000.0 / fps` in floating point rather than `1000 / fps` in integers. The old integer division turned 6.944 into 6, which is the quantization that made a requested 144 land on a different rate even in principle.

The waitable timer was never at fault. It is created with `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` and was always accurate. Only the measurement feeding it was too coarse to use. Its own unit is 100 ns, so the wait now takes a `double`: rounding up to a whole millisecond would have handed most of the precision straight back, capping a 144 target at 142.9.

After the fix: **141.71 FPS** (SD 0.104), or 98.4% of target, and **2.22x the frames**. The remaining 1.6% is timer wake latency, roughly 0.11 ms of overshoot per frame, which is about as close as a waitable timer gets.

Deliberately **not** fixed with `timeBeginPeriod()`. Raising the global timer resolution would also have worked, and it would have degraded system-wide power behaviour for every other process on the machine. That is the opposite of the point of this mod.

### ✦ Changed: the per-frame `Present` no longer waits for a vertical blank

The three `Present` calls on the per-frame path use a sync interval of `0` instead of `1`.

On a DirectComposition swap chain, DWM owns presentation timing, and the waitable timer above is already doing the pacing. Asking `Present` to block for a vblank on top of that does not make anything smoother, it just parks the render thread.

Measured across 219 five-second windows, roughly 155,000 frames, with the two settings switched live inside one session rather than compared across separate runs:

| | sync 1 | sync 0 |
|:--|--:|--:|
| Time in `Present`, average | 0.358 ms | **0.236 ms** |
| Render thread blocked | 5.07% of wall time | **3.34%** |
| Windows blocked more than 8% | 18% | **4%** |

The tail matters more than the average here. Stalls became about 4x rarer.

The swap chain stays at **2 buffers**. A third was tested over the same windows and made no measurable difference (p was 0.85 and above), while costing a swap-chain rebuild and extra video memory.

The two one-off `Present` calls are unchanged. `CaptureWallpaperBitmap` pairs its `Present` with `DwmFlush()` before a `PrintWindow` capture, where the wait is load-bearing, and `PauseForFullscreen` presents a single blank frame.

### ✦ Note on the performance figures

The numbers previously quoted for this mod were all measured while it was running at an effective 64 FPS, because of the bug above. They were accurate for what the mod was doing, but they described a frame rate nobody asked for. Delivering 2.22x the frames costs more, so they have been re-measured rather than carried over.

The method changed as well as the numbers. Previously the page quoted absolute readings, plus a per-process figure of 2.36% of one core taken from a Windows Performance Analyzer trace. Absolute readings mostly describe whatever else the machine was doing, so the page now quotes the **difference** between two captures taken back to back in one session, with the same music playing in both and the mod the only thing that changed.

That is a larger number than the old per-process one, and not because the mod got worse. The two measure different things. A per-process trace counts time inside the mod's own threads. The whole-machine difference also counts the work the mod causes elsewhere, in the compositor and the graphics driver, which is real cost the machine pays even though no profiler attributes it here. Quoting the smaller figure would have been flattering rather than accurate.

Current cost at a 144 FPS target, as medians across 543 samples running and 384 disabled: **+1.6 percentage points of total CPU** (roughly a third of one core), **+7.2 W** package power, and **+1.0 °C** package temperature.

---

## ❀ v1.0.0: First Release

The first public release of `Tourne'Table [Audio Visualizer]`. Everything below this line is the road that got here.

**What ships in 1.0:** 8 shapes and 9 color modes, a configurable FFT with log / linear / mel frequency scaling, a peak-frequency readout, Now Playing text with album-art coloring from the native Windows media session, a clickable media control strip, keyboard and drag repositioning for every movable piece, independent panels and borders behind each text overlay, per-side padding that can shrink as well as grow, settings validation that tells you when a value didn't parse instead of silently defaulting, and idle behavior that stops rendering entirely when the audio stops.

It runs as its own process rather than inside `explorer.exe`, and draws behind the desktop icons.

### ✦ Fixed: media control clicks landed left of the buttons

Introduced by the padding setting in v0.8.2 and caught on a read-through, not in use.

`PaintMediaControls` draws the first icon inset by the plate padding. The click handler in `MediaWndProc` was never updated to match, and kept testing from the left edge of the window, so every button's clickable region sat `padding` pixels to the left of the button actually drawn there.

At a small padding it read as buttons that were slightly awkward to hit near their right edge. Once padding exceeded the icon spacing, the regions shifted far enough that clicking **Play** triggered **Previous**.

Only reachable with **Backing Plate Padding** above `0`. At the default of `0` the two agreed exactly, which is why v0.8.2 looked correct.

### ✦ Note

Vertically the whole strip stays clickable, padding included, rather than only the icon row: the buttons are more forgiving to hit, and the strip already swallows the click either way. The horizontal gaps *between* icons remain dead, as before.

---

## ❀ v0.9.0: Every Color Setting's Help Text Was Cut in Half

### ✦ Fixed: settings descriptions truncated at "Format is"

All fifteen colour settings in the mod explained their accepted formats with a line ending `Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)`. None of them ever displayed it.

In YAML, a `#` preceded by whitespace starts a comment. These descriptions were unquoted, so the parser threw away everything from the first ` #` onward, meaning the ⓘ tooltip on every colour field stopped dead at the words "Format is", precisely where the useful part began. The one piece of text whose entire job was telling you how to write a colour was the one piece that never arrived.

Every affected value is quoted now. This has been wrong since colour settings were first added; it survived this long because a truncated description still looks like a finished sentence if you don't know what was supposed to follow.

Restored in full, beyond just the format hint:

- **Peak Hold Color**: that it's independent of Color Mode, and the caps are always drawn in it even under Rainbow / Album Art / Accent / Acrylic.
- **Beat Flash Color**: that the alpha governs flash strength even at full intensity, and lowering it gives a tint rather than a full colour swap.
- **Now Playing / Peak Readout Background**: what the panel is for and that it's transparent by default.
- **Backing Plate Color**: the `#8C141414` suggestion for pale icons on a pale wallpaper.

### ✦ Note

The mod's own settings validator can't catch this class of bug: it checks what users type into fields, whereas this was wrong in the schema that *describes* the fields. Neither can the compiler, since the whole settings block is a comment as far as it's concerned. A scan for the pattern now runs against the settings block before release.

---

## ❀ v0.8.2: Padding for the Media Controls Plate

### ✦ New: `Media Controls ▸ Backing Plate Padding`

The Now Playing and frequency readout panels each got a padding setting in v0.8.0 and the media plate did not, which left its border and fill sitting flush against the icons with no way to open them up.

It **grows the strip** rather than shrinking the icons, so turning it up never makes the buttons smaller or harder to click.

Two consequences worth knowing:

- Padding applies **whether or not the plate is visible**, so that padding, fill and border stay independent of one another. The strip's box gets bigger either way, which very slightly shifts where the Horizontal / Vertical Position percentages land: position is a percentage of the *remaining* screen space, and a wider strip leaves less of it.
- **Hide When Covered** measures the strip's rectangle, so a padded strip counts as covered marginally sooner.

Both default to no change at all, since the padding defaults to `0`.

### ✦ Under the hood

The icon blitters previously drew at a hardcoded `y = 0`, which was only safe while the window height *was* the icon height. They take a Y origin now and the icons are centred in the padded content box.

The travel-range helper used by keyboard move duplicates the strip's sizing math on purpose (so a pixel nudge converts to the same percentage the repositioning turns back into pixels) and was updated alongside it. Had it been missed, moving the strip with the keyboard would have drifted against the padding.

---

## ❀ v0.8.1: The Oscilloscope Respects Orientation

### ✦ Fixed: the Oscilloscope drew sideways when Orientation was Vertical

Every other shape reads `Orientation` and lays itself out accordingly. The Oscilloscope never did. It always swept its trace left-to-right and deflected up-and-down, no matter what.

In Horizontal that happens to be correct, which is why it went unnoticed. In Vertical the two axes swap: the group becomes a tall, narrow column where the long axis is *height* and `Bar Max Size` is the *width*. The trace kept sweeping across the width, so the entire waveform was crushed into a strip as wide as the bars were long and squashed into the middle of an otherwise empty column.

It's now the horizontal layout rotated 90° clockwise: the sweep runs **top to bottom**, and what was "up" on the trace becomes "right". Everything else about the shape (colouring, multiband tinting, beat flash, stroke width) is untouched and shared between both orientations.

---

## ❀ v0.8.0: Panels and Borders for the Overlays

Everything that draws on top of the wallpaper can now have something behind it.

### ✦ New: a border on the Media Controls backing plate

`Media Controls ▸ Backing Plate Border Size / Color`, following the plate's own corner radius.

The border draws **whether or not the plate has any fill**, so a hairline outline with nothing behind the icons is a valid look: leave the plate colour transparent and set only a border.

Fill and border are rasterized in one pass now: coverage of the outer rounded rect minus coverage of the inner one gives the ring, supersampled the same way the built-in glyphs are so the corners don't stair-step. The fill runs under the border rather than stopping at it, so a translucent border blends over the fill instead of cutting a hole through it.

### ✦ New: backgrounds for the Now Playing text and frequency readout

Both get a panel of their own (`Background`, `Background Padding`, `Corner Radius`, `Border Size`, `Border Color`) all **fully transparent by default**.

The panel is sized to **the text**, not to the visualizer. The text is centred inside a layout box far wider than itself, so using that box would have stretched the panel across the whole reserved area. Measuring means building a text layout, which `DrawText` does internally anyway, so when a panel is on, that layout is kept and drawn from, and nothing gets measured twice. With no panel the old `DrawText` path is untouched.

Two details that matter in use:

- The Now Playing panel **fades in and out with the text** rather than popping, since it shares the same alpha.
- This is most useful on the frequency readout's *Top / Middle / Bottom* alignments, where it sits directly over the bars: a moving, recolouring background is about the worst thing to ask text to be legible against, and a panel fixes it without moving the readout outside the visualization.

Layout reserves room for the padding and border as well as the offset, so a panel can't get clipped at the edge of the render surface while the text inside it stays visible.

---

## ❀ v0.7.0: Transparent Icons Actually Transparent, Movable Text, Partial-Coverage Pausing

All four from real-world use of v0.6.0.

### ✦ Fixed: the dark box behind the media icons

v0.5.2 added a translucent dark backing plate behind the icon strip to fix pale icons vanishing against a pale wallpaper. It was drawn unconditionally, which meant icons with their own transparency could never look transparent, because there was always a plate behind them.

It's a setting now, **fully transparent by default**: `Media Controls ▸ Backing Plate Color`, with a corner radius to match. The contrast fix is still one alpha value away (`#8C141414` restores the old look) but you're no longer opted into it.

Two related things fixed while in there:

- **Custom icons punched transparent holes through the plate.** The blit was a straight `memcpy`, so an icon's fully transparent pixels *overwrote* the plate instead of letting it show through. Both the custom-icon blit and the built-in glyph rasterizer now do a proper premultiplied source-over composite.
- The plate's corners are antialiased the same way the built-in glyphs are, so a rounded plate doesn't come out with stair-stepped corners.

### ✦ New: everything movable moves with the same keys

The text overlays only had a coarse alignment dropdown, which is why they ended up sitting on the background panel with no way to get them off it. The Media Controls strip only had percentage fields.

Both text overlays now have **Offset X / Y** settings in pixels (negative is left/up, positive is right/down) and every movable piece can be nudged live with the same keyboard move that places the visualizer:

| Combo | Steers |
|:--|:--|
| `Modifier + 1` | The visualizer *(default)* |
| `Modifier + 2` | The Now Playing text |
| `Modifier + 3` | The frequency readout |
| `Modifier + 4` | The Media Controls strip |

Pick a target once and nudge as long as you like; `Modifier + Home` resets whichever is selected.

The two kinds of placement persist differently, on purpose. The visualizer and the strip are positioned against the **monitor**, so they store a percentage that replaces their Position settings. The text overlays are positioned against the **visualizer** ("just above the panel" is a statement about the box, not about the screen) so they store a delta *on top of* their Offset settings, which means a number typed into those is never silently ignored because a nudge happens to exist.

Under the hood: the render surface grows to make room for an offset (otherwise offset text just walks off the edge and disappears), but that room is rounded up to a 32px step: following every single pixel would mean a DXGI buffer resize dozens of times a second while you hold an arrow key down. The text's anchor point was also split from the reserved room, so nudging the title sideways no longer drags the frequency readout along with it.

### ✦ New: Media Controls can get out of the way

`Media Controls ▸ Hide When Covered`, with its own 5-100% threshold. The strip is topmost (that's deliberate, it has to receive your clicks) but that also means it sits over your maximized browser forever. With this on it hides while a real window is underneath it and comes back when that window moves or closes.

Off by default: a control strip that disappears unexpectedly is worse than one that's occasionally in the way. The topmost re-assert still runs either way, so another app stealing z-order can't strand it.

Coverage is re-checked once a second, so hiding and reappearing aren't instant, and while the strip is parked it's genuinely hidden, so a keyboard nudge still applies but you won't see it land until it comes back. Both of those are now spelled out in the setting's own hover text rather than left to be discovered.

### ✦ New: Pause When Covered has a threshold

`Performance ▸ Pause When Covered - Threshold`, 5% to 100% in steps of 5, default `100%` (the old behavior: pause only when completely hidden).

The check used to be pure full-containment: a single window had to swallow the visualizer's whole box on its own. That meant two windows covering opposite halves counted as *not covered at all*. Coverage is now accumulated into a GDI region and measured as real area, so overlapping and adjacent windows add up correctly, and the occlusion log line now fires on the transition rather than once per second forever.

### ✦ The settings validator now warns, not just rejects

It could previously only say "this value couldn't be read." It can now also flag a setting that parses perfectly and works exactly as configured, but probably isn't doing what you meant.

First case: **a one-key Keyboard Move Modifier.** The hook swallows the keypress outright, so the app underneath doesn't fall back to its own behaviour. It never hears the key at all. With `Ctrl` + WASD that means no `Ctrl+A`, and no `Ctrl+S`, so a save you think you made silently doesn't happen. The warning names the exact shortcuts you've given up for your specific combo, and it's only a heads-up: nothing is blocked, because it's a legitimate choice if you know what you're trading.

The same caveat is now in the modifier's own hover text and in the README, including the reason it bites harder than a normal shortcut clash: a swallowed key isn't a conflict the app resolves in its favour, it's a key the app never sees.

---

## ❀ v0.6.0: Keyboard Move, Media Controls Fixed, Settings That Tell You When They're Wrong

v0.5.3 moved the drag hook off the render thread and it *still* felt like dragging through mud. So this release stops trying to make cursor-chasing fast and does the thing that doesn't need to be fast.

### ✦ New: Keyboard Move

Hold a modifier, tap a direction, the visualizer steps by an exact number of pixels: the way a window manager nudges a tiled window. **Ctrl + Alt + arrows or WASD** by default, 1px per press, hold **Shift** as well for 10px. **Ctrl + Alt + Home** throws away a saved position and snaps back to the Position percentages.

The reason this is immune to the problem drag has: one keypress is one discrete jump. There's no stream of intermediate positions for the renderer to keep up with, so there's nothing to fall behind. Every press lands exactly where it says, whether the redraw arrives in 2ms or 40ms.

Step size, direction key set (arrows / WASD / both), modifier and fast key are all configurable.

- **Drag-to-Move is now off by default.** It still works and all its settings are untouched. It's just no longer the way you're pointed first. Its description now says plainly why it can feel sluggish rather than leaving you to wonder whether it's broken.
- Keyboard moves also force a redraw on the spot, so you can reposition with nothing playing instead of having to start a track to see where the box went.
- The position write is debounced: key repeat can fire dozens of nudges a second, and each one used to mean a file write.

### ✦ Fixed: Media Controls

Three separate reasons the strip could fail to appear, all of them silent:

- **The UI thread never initialized COM.** Every `CoCreateInstance` made from it failed outright with `CO_E_NOTINITIALIZED`, which means WIC's imaging factory never activated and **custom icon paths have never worked, in any version.** Set one and you got the built-in glyph, indistinguishable from having left the field blank. The UI thread now initializes COM apartment-threaded.
- **`UpdateLayeredWindow` was being told to infer the window's position and size.** Passing `NULL` for those is only documented as valid when they aren't changing, and here they always are, since the window is born 1×1 and every settings change can resize it. It's now handed both explicitly, painted before it's shown, and its return value is actually checked and logged.
- **Nothing ever retried.** If window creation failed at startup, turning Media Controls on in settings did nothing at all, forever. It now recreates the window on a settings change, and a once-a-second check re-asserts the strip if it has lost topmost or gone missing entirely.

Settings changes also now route to the UI thread through the message window when the overlay doesn't exist yet, instead of running inline on Windhawk's own settings thread and creating windows on a thread that never pumps them.

### ✦ New: Settings Validation

Windhawk saves whatever you type into a free-text box. This mod used to quietly fall back to a default on anything it couldn't parse, so a typo looked exactly like a setting that had saved fine and simply did nothing. Which is a miserable thing to debug, because there's no signal at all.

Now every free-text field is validated, and anything that fails produces a summary window naming the field, what you typed, what format was expected, and what's being used instead:

- **Colors**: all nine of them, with the accepted formats spelled out.
- **Positions**: and `50px` or `50 50` is now rejected rather than silently taking the leading number.
- **Padding / corner radii**: anything other than exactly one or four values. Two in particular *looks* like it might mean horizontal/vertical, and it doesn't.
- **Icon paths**: file doesn't exist, or points at a folder. Surrounding quotes from Explorer's "Copy as path" are stripped rather than treated as an error. A file that exists but won't decode is logged too.
- **Now Playing font**: checked against the fonts actually installed, and only when the text is switched on.
- **Keyboard Move Fast Key**: flagged when it's already part of the modifier, since it could then never be "additionally held."

Off-switch under **Settings Validation ▸ Warn About Invalid Settings**. The same list goes to the mod log either way, so turning it off makes it silent, not blind.

---

## ❀ v0.5.3: Fixed: Drag Lag (For Real This Time)

The diagnostic logging added in v0.5.2 paid off: the raw numbers pointed straight at the actual cause.

- **Fixed: dragging felt laggy/resistant the whole time, not just at release**: the drag hook and the render tick were sharing one thread. Every frame, that thread renders the visualizer via Direct2D and presents it, which can block for real time; while it's blocked, queued mouse-move messages back up behind it, so the box's position only advanced in bursts whenever a render tick let go of the thread, which reads exactly like "laggy" or "resistant." The position math itself was correct throughout (confirmed from the logs), the display of it just kept getting stalled. Moved the mouse hook to its own dedicated thread, isolated from rendering, so cursor tracking is never delayed by a Present() call.
- `g_cachedMonitor` made atomic. It's now read from a genuinely separate thread (the new drag-hook thread) instead of incidentally sharing the UI thread, so it needed real cross-thread safety instead of implicit same-thread ordering.
- The double-click-to-clear path no longer touches the swap chain directly from the hook thread (that would have been a new cross-thread hazard against the render tick); it just clears the saved override and lets the next tick's own `UpdateSwapChainForLayout()` call pick it up, same as it already does every frame.

---

## ❀ v0.5.2: Media Controls Contrast Fix + Drag Diagnostics

v0.5.1's fixes didn't resolve either report on retest -- this pass fixes one for real and adds logging for the other instead of guessing a third time.

- **Fixed: Media Controls could be genuinely invisible** -- the default icon color is pure white on a fully transparent background; against a similarly light patch of wallpaper (easy to land on, since the default position sits near the bottom-center of the screen) that's close to indistinguishable. Added a translucent dark backing plate behind the whole icon strip, so it's visible against any wallpaper regardless of icon color.
- **Drag rubberbanding not yet fixed** -- v0.5.1's desync theory didn't hold up (reducing the per-move workload made no difference on retest). Rather than guess again, added throttled diagnostic logging at drag start/move/end and at each render tick during a drag (`Wh_Log` lines prefixed `[Drag]`), so the next test run produces real evidence -- cursor position, computed travel range, resulting override percentage, and what the render tick actually saw -- instead of another speculative patch.

---

## ❀ v0.5.1: Fixed: Drag Desync + Missing DPI Awareness

Found from actually running v0.5.0 for the first time -- thank you real-world testing.

- **Fixed: dragging felt "laggy" and snapped back to the old spot on release** -- the window's on-screen position (updated on every mouse-move, from the drag hook) and the box's drawn content (only recomputed on the next render tick) were two independent calls to the same layout math, taken at different moments. During a fast continuous drag they drifted apart -- the window raced ahead, the content lagged behind -- and releasing let the content's next redraw "catch up," which read as snapping back. Fixed by having the regular render tick own repositioning every single frame, instead of the drag hook doing it separately on its own schedule.
- **Added explicit Per-Monitor-V2 DPI awareness** -- running injected into `explorer.exe`, this was inherited for free (the shell is always DPI-aware). Running as our own standalone process since v0.4.0, nothing declared it, which risked every position/size calculation (the visualizer's own box, and the Media Controls window) running against a DPI-virtualized view of the desktop instead of true physical pixels on a scaled monitor -- plausibly why the Media Controls strip was hard to locate.

---

## ❀ v0.5.0: Drag-to-Move

The visualizer stays click-through by design, but now with one deliberate exception: grab it and drag it.

- **Drag-to-move**: hold a configurable modifier + mouse button (Ctrl + Middle Click by default) anywhere over the visualizer and drag to reposition it on screen.
- **Bars pause during the drag**: only the background/border box moves while you're repositioning it; bar rendering freezes and resumes once you let go, so the box doesn't visually fight with the bars while it's in motion.
- **Configurable trigger**: new Interaction section lets you pick the modifier (None/Ctrl/Alt/Shift/Win) and mouse button (Left/Middle/Right), in case the default conflicts with something else.
- **Double-click to clear**: the same combo, clicked twice without dragging, clears a saved drag position and snaps back to the Position settings.
- **Persists across restarts, quietly**: since Windhawk mods can only read settings, not write them, a dragged position is saved to its own small file (`%LOCALAPPDATA%\TourneTable\position_override.txt`) rather than back into the Position fields you see in the settings UI.

## ❀ v0.4.1: Fixed: Windows Weren't Being Pumped

A correctness bug from the v0.4.0 standalone-process conversion, caught while building the drag feature (which needed a real message loop to work at all).

- **All windows now sit on a real message-pumping thread**: the standalone conversion created the overlay, message, and media-control windows directly on `WhTool_ModInit`'s calling thread, which the tool-mod launcher terminates shortly after. Every one of those windows was at risk of losing its message queue (timers, settings-changed handling, media button clicks) once that thread went away. Fixed by spinning up a dedicated, persistent thread with an actual `GetMessage` loop and creating every window there instead.

---

## ❀ v0.4.0: Runs as Its Own Process

The mod no longer lives inside `explorer.exe`. It now runs as its own dedicated process using [Windhawk's tool-mod pattern](https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process): its own entry in Task Manager, separate from the shell.

- **Standalone process**: `@include` moved from `explorer.exe` to `windhawk.exe`; the mod now spawns and lives in its own dedicated host process.
- **Survives shell restarts**: killing or restarting `explorer.exe` no longer takes the visualizer down with it. It reattaches behind the desktop icons the moment the shell is back.
- **Bootstrap rebuilt for cross-process life**: the old explorer-injection tricks (a `CreateWindowExW` hook to detect the shell's readiness, and a cross-process code-execution trick to marshal window creation onto explorer's thread) are gone, replaced with a direct, self-rescheduling retry loop that simply waits for `WorkerW` to exist.
- **`GetWorkerW()` un-coupled from explorer**: a leftover same-process safety check would have silently broken the entire visualizer the moment it stopped running inside `explorer.exe`; removed.

*Requires a Windhawk build with tool-mod support (1.7.3+, including the 2.0 alpha line).*

---

## ❀ v0.3.0: Media Controls

A small Previous / Play-Pause / Next button strip, wired to whatever's actually playing.

- **Media Controls section**: Enabled toggle, Icon Color, and four separate icon-path fields (Previous / Play / Pause / Next).
- **Talks to real playback**: uses the same native `GlobalSystemMediaTransportControlsSession` API the Now Playing text already reads from. No per-app setup.
- **Bring your own icons**: point any button at a local PNG/JPG/BMP/ICO file; leave it blank and a clean built-in glyph is drawn instead.
- **Independent placement**: its own decimal-precision Horizontal/Vertical Position, entirely separate from where the visualizer itself sits.
- **Actually clickable**: lives in its own small always-on-top window, deliberately *not* click-through, since the visualizer itself must stay that way.

---

## ❀ v0.2.1: Smoother, More Precise Motion

- **Motion Smoothing**: new setting to slow down bar attack/decay, making small on-desktop bars easier to read without chasing every frame-to-frame jitter.
- **Finer position control**: Horizontal/Vertical Position now accept decimals (`50.25`, not just `50`), so a single step is no longer a big jump on a large monitor.

## ❀ v0.2.0: Color Fixes & Beat Flash Rework

- **Beat Flash Color**: the flash-on-beat effect gets its own dedicated color (hex or `rgba()`/`rgb()`, with alpha), replacing an old "boost toward white" effect that washed out regardless of Color Mode.
- **`rgba()` / `rgb()` support, mod-wide**: the shared color parser now accepts CSS-style color strings everywhere a hex color was accepted before.
- **Acrylic mode fixed**: removed an alpha floor that kept it from ever truly disappearing in silence, as advertised.
- **Now Playing text locale fixed**: swapped a hardcoded `en-us` for `GetUserDefaultLocaleName()`, improving font fallback/shaping for non-Latin text.

## ❀ v0.1.2: Negative Padding

- **Padding can now shrink, not just grow**: negative values pull a side of the background panel inward, past the bars if pushed far enough, instead of only ever expanding outward.
- **Degenerate-rect safety net**: an extreme negative value holds that side open to a 1px sliver rather than inverting the panel's geometry.

## ❀ v0.1.1: Independent Panel Control

- **Peak Hold Cap Color**: no longer locked to Gradient Color 2; caps get their own independent, always-applied color regardless of Color Mode.
- **Independent per-side Padding**: grow the background panel's left, right, top, or bottom edge on its own, without moving the bars or touching any other setting.

## ❀ v0.1.0: Foundation

- **Seqlock-based band publishing**: thread-safe handoff of per-band audio data from the capture thread to the render thread, replacing a per-element atomic approach that could tear under high refresh rates.
- **Sensitivity Curve**: Exponential / Knee / Power handling for how the top of the Sensitivity range compresses once a band would otherwise overshoot max bar height.

---

### A note on "efficiency improvements"

A handful of performance-adjacent spots got a close look across this span: Target FPS's integer-division quantization, `GetWorkerW`'s permanent materialization of a `WorkerW`, and whether `FillRoundedRectPerCorner` could share the background panel's geometry cache. None of them turned into changes: the FPS quantization is harmless in practice, the `WorkerW` side effect is inherent to the behind-the-icons technique itself (and matches what every comparable mod does), and the per-bar geometry changes too often each frame for a cache to help. Rather than efficiency wins, this span was mostly correctness fixes and new customization: the bigger performance rewrite predates this changelog.
