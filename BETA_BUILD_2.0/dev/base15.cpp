// ==WindhawkMod==
// @id                  tourne-table-beta-build
// @name                Tourne'Table [Audio Visualizer] (Scope Test)
// @description         TEST BUILD. Tourne'Table with the Oscilloscope time base decoupled from FFT Size, plus a Hardware section: pick the GPU, CPU or NPU, and a Smooth Mode built for integrated GPUs. Installs alongside the release build. Not for publishing.
// @description:ru-RU   Аудиовизуализатор реального времени для рабочего стола Windows. Расширенные настройки без ущерба для экономии ресурсов. Практически безинтерфейсный (near-headless) поток рендеринга с оптимизацией процессора для захвата звука.
// @version             1.5.0
// @author              USER-TOURNE
// @github              https://github.com/USER-TOURNE
// @donateUrl           https://ko-fi.com/tourne
// @license             MIT
// @include             windhawk.exe
// @compilerOptions     -ldxgi -ld2d1 -ld3d11 -ldcomp -ldwmapi -ldwrite -lgdi32 -lshcore -lshlwapi -lole32 -lshell32 -lksuser -lwindowscodecs -lruntimeobject -lwindowsapp -luuid -luser32 -ladvapi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
![Tourne'Table - a ghost in your desktop's shell](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/BANNER/tourne-header.png)

# `Tourne'Table` **[Audio Visualizer]**

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/11.gif)

*The Oscilloscope shape running live audio: bottom-left placement, blurred panel, single-pixel border.*

> **A real-time audio visualizer that lives on your Windows desktop.**
> Built on the foundation of Salyts' Desktop Audio Visualizer, rebuilt around performance.

Play music. Bars dance on your wallpaper. That's the whole idea.

It listens to **whatever your PC is already playing** (Spotify, YouTube, a game, a call) and draws it behind your desktop icons. No virtual audio cable, no drivers, nothing to configure. It just picks up your system audio.

It is built to be cheap to run. The render thread wakes only at the frame rate you ask for, the wallpaper blur is computed once instead of every frame, and the drawing surface is sized to the widget rather than the whole desktop, so at a 144 FPS target the visualizer costs about **a third of one CPU core** and **one degree** of CPU package temperature while it plays, and nothing at all while it doesn't.

## ABOUT THIS PROJECT

Built with an emphasis on lower resource consumption, more efficient rendering, better frame pacing, expanded visualization options, dynamic album-art integration, native Windows media info, deeper customization, and power-conscious idle behavior.

The goal is simple:

> **Make the desktop move with the music, without making the CPU move mountains to do it.**

## Runs as its own process

Tourne'Table doesn't live inside `explorer.exe`. It runs as its own dedicated process using Windhawk's tool-mod pattern, so you'll see it as its own entry in Task Manager, separate from the shell. Restarting `explorer.exe` doesn't kill it. It just reattaches behind the desktop icons once the shell is back. Requires a Windhawk build with tool-mod support (1.7.3+, including the 2.0 alpha line).

---

## ◈ PERFORMANCE AT A GLANCE

Measured on an **Intel Core Ultra 265KF** (8 P-cores plus 12 E-cores), running a 120-bar oscilloscope at a 144 FPS target with background blur on, and with media controls, the peak-frequency readout, peak hold and beat flash all enabled.

Every figure below is the **change against an idle baseline**, captured back to back in the same session with the same music playing in both, so what you are reading is the cost of the mod rather than whatever else the machine happened to be doing.

| Metric | Cost of running it |
|:--|--:|
| Total CPU usage | **+1.6 percentage points** *(about 0.3 of one core)* |
| Peak single-thread | **+10.1 points** |
| CPU package power | **+7.2 W** |
| CPU package temperature | **+1.0 °C** |

Medians across 543 samples with it running and 384 without, at 1.25 s intervals. Medians rather than averages because both captures contained brief unrelated background spikes, and a median is not moved by them.

When audio stops, rendering stops, not "slows down," *stops*.

Full methodology, raw traces and caveats live in [the project repo](https://github.com/USER-TOURNE/TOURNE-TABLE) rather than on this page.

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/10.gif)

*Bottom-left placement against a dark wallpaper, with the Now Playing label sitting above the panel.*

---

## ◆ VISUAL STYLES

### 8 Shapes

| Shape | What it does |
|:--|:--|
| **Stereo** | Classic equalizer bars. Low notes left, high notes right. |
| **Mountain** | Peaks in the middle, tapers toward both edges. |
| **Mirror** | The opposite, grows from the outside edges inward. |
| **Wave** | Normal bars with a slow ripple rolling through them. |
| **Breathe** | A gentle swell that rises with the music instead of jumping. |
| **Dots** | Stacked dots instead of solid bars, old LED-meter look. |
| **Radial** | Bars shoot outward from a center point, like a sunburst. |
| **Oscilloscope** | A single line tracing the actual sound wave. |

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/8.gif)

*Closeup: the waveform drawn as one continuous line, with the Now Playing label and media buttons alongside.*

### 9 Color Modes

| Mode | What it does |
|:--|:--|
| **Solid** | One flat color. |
| **Gradient** | Fades between two colors across the bars. |
| **Reactive Gradient** | Shifts toward the second color as things get *louder*. |
| **Windows Accent** | Matches your Windows accent color, updating instantly. |
| **Album Art** | Pulls the dominant color from the playing track's cover. |
| **Dynamic Album** | Gradient between the cover art's two strongest colors. |
| **Acrylic** | Grows more opaque the louder it gets, invisible in silence. |
| **Rainbow Cycle** | Continuously cycling hue, adjustable speed. |
| **Tourne** | Built-in teal → red gradient from my personal palette. |

### Plus

- **2 orientations**: bars grow vertically or horizontally
- **3 anchors**: grow from the Bottom, the Top, or **both directions from the Middle**
- **Peak Hold Caps**: thin markers hang at each bar's recent peak and slowly fall *(classic hardware EQ)*
- **Beat Flash**: bars brighten on detected bass hits, on top of any color mode
- **Multiband Oscilloscope**: the waveform tints toward whichever part of the spectrum is loudest

---

## ♪ NOW PLAYING DISPLAY

Shows the current **artist and title** above the visualizer, pulled from the same Windows media session that powers your volume popup. Works with Spotify, browsers, and most media players.

Fades in on track change, fades out after a configurable delay. Custom color, font family and size.

---

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/7.gif)

*A narrower panel. The trace scales to whatever width the bar settings give it.*

## ◐ THE PALETTE

This project follows my personal theming palette: reflected in the repo screenshots. It can be changed to any hex / RGB / RGBA value you want.

| Name | Hex | RGB |
|:--|:--|:--|
| Green | `#15ffa2` | `21, 255, 162` |
| Red | `#ff8f8f` | `255, 143, 143` |
| Pale Green | `#64aa89` | `100, 170, 137` |
| Alt Pale Green | `#6ba6a0` | `107, 166, 160` |
| Pale Red | `#a95d5d` | `169, 93, 93` |
| Dark Teal *(background)* | `#121c21` | `18, 28, 33` |

The built-in **Tourne** color mode is drawn from these.

---

# ✦ EVERY SETTING, EXPLAINED

**You don't need any of this to use the mod.** The defaults work. This is for when you want to tweak something and you're wondering what a word means.

## Appearance

**Shape**: Which of the 8 styles above to draw.

**Orientation**: *Horizontal* = a row of bars growing up and down. *Vertical* = a column growing left and right.

**Bar Count**: How many bars. More = finer detail, wider visualizer. Range **1-2048** (enough to span a 4K or ultrawide screen).

**Bar Width**: How fat each bar is, in pixels.

**Bar Gap**: Space between bars, in pixels. Set to `0` and they touch.

**Bar Max Size**: How tall a bar gets at full volume. This is the overall height of the visualizer.

**Bar Idle Size**: How tall bars sit in silence. `0` makes them vanish completely; a few pixels leaves a thin resting line.

**Bar Corner Radius**: How rounded the bar corners are. One number rounds all four equally, or give four numbers separated by spaces for individual control: `top-left top-right bottom-right bottom-left`. Example: `5 5 0 0` rounds only the top.

**Color Mode**: Which of the 9 coloring styles above to use.

**Color**: The color used in Solid mode. Format is `#AARRGGBB` or `#RRGGBB`: that's **A**lpha (transparency), then **R**ed, **G**reen, **B**lue in hex. Lower the first two digits to make it see-through.

**Gradient Color 1 / 2**: Start and end colors for the gradient modes.

**Sensitivity**: How hard the bars react. Too low and quiet music barely moves them; too high and everything slams to max. Range 0-300. Turn it down for bass-heavy tracks, up for quiet recordings.

**Input Gain (dB)**: A fixed level trim on the captured audio, applied before everything else. Range -24 to +24, default 0.

> **Why this exists.** Loopback capture sees the mix *after* each app's own volume slider but *before* the Windows master slider. So if you keep Spotify at 40% and the system at 80%, the visualizer only ever sees that 40%, and turning the system up does not help it. Input Gain is the control that does. It is also the only setting that scales the **Oscilloscope** waveform directly, which otherwise has no level control of its own. Auto Gain scales the trace too when it is on, and the trace clips flat rather than running off the panel.
>
> **It moves the idle thresholds with it.** Pause When Silent and Auto-Hide When Idle both key off how far the bars are moving, so raising Input Gain lowers the level that counts as silence, and lowering it raises that level. Sensitivity has always behaved the same way for the same reason. If you push Input Gain a long way up, expect the mod to consider quieter things "playing".

**Auto Gain**: Adapts the level continuously so quiet sources still fill the bars, instead of retuning Sensitivity per app or per track. Off by default.

It only ever boosts, never cuts, so loud material behaves exactly as it does with this off. Through silence it holds its last value rather than winding up, which means the noise floor is never lifted and **idle shutdown still works normally**. The idle test reads the un-boosted level against the same threshold it always used, so turning Auto Gain on neither extends nor shortens how long the mod stays awake.

Two things it does affect, worth knowing before you go hunting for them:

- Because it normalizes the loudest band to a fixed target, **Sensitivity stops doing much for quiet material** while it is on. That is inherent to how a levelling gain works, not a bug.
- On the **Oscilloscope**, a lower Sensitivity produces a *larger* trace, since a smaller measured peak asks for a bigger boost.

**Auto Gain Max Boost (dB)**: The ceiling on that lift. Range 0-24, default 12. Lower it if quiet passages are being flattened more than you want, raise it if a very quiet source still will not fill the bars.

**EQ Preset**: Which frequencies get emphasized *visually*. Doesn't touch your actual audio.
`Default` no adjustment · `Bass` boosts lows · `Rock` boosts mids and highs · `Pop` heavy on highs · `Jazz` warmer, gentler highs · `Electronic` boosts bass and treble, scoops the middle.

**FFT Size**: How finely sound gets analyzed. An FFT is the math that splits audio into separate frequencies: think of it as sorting sound into buckets by pitch. More buckets = finer detail, slightly more CPU.
`1024` fastest, plenty for most · `2048` / `4096` noticeably crisper · `8192` maximum detail.

> **The Oscilloscope no longer cares what this is set to.** It used to take its trace over a window of exactly one FFT block, so raising FFT Size stretched the trace to cover more time and updated it less often, which is why `8192` used to look so rough on that shape. The trace now runs on its own time base, set by **Oscilloscope Time Window** below. Pick FFT Size for the bars and leave the scope out of it.
>
> **One more note:** the higher the FFT Size, the more accurate the Sensitivity slider becomes for your specific audio setup. The correlation generally runs: **as FFT Size goes up, your Sensitivity will need to go up too.** For now that means fine-tuning Sensitivity per EQ Preset *and* per FFT Size, for your particular placement, size and personal adjustments.

**Frequency Scale**: How the frequency range spreads across the bars. This matters more than it sounds.
- **Log**: the natural-feeling default. Gives bass and treble roughly equal visual space, matching how humans hear pitch.
- **Linear**: spreads by raw Hz. Since most musical energy lives low, this crams all the action into a sliver on the left and leaves the right mostly dead. Technically accurate, visually dull.
- **Mel**: uses the *mel scale*, built from research on how people actually perceive pitch. Like Log, tuned to human hearing.

**Peak Frequency Readout**: Shows the loudest note as a live number, e.g. `1.2 kHz`.

**Peak Readout Position**: Where that number sits. Horizontal: Left / Center / Right. Vertical: Above / Top / Middle / Bottom / Below.
> **Above** and **Below** place it fully *outside* the bars so it never overlaps the visualization.

**Multiband Oscilloscope Coloring**: Only affects the Oscilloscope shape. The line tints toward whatever part of the spectrum is loudest: warm for bass, green for mids, blue for treble. Overrides Color Mode for that shape.

**Oscilloscope Time Window (ms)**: How much audio the trace shows end to end. Range 5-250, default `24`.

> This is the scope's time base, and it is the setting that decides what the trace *looks* like. Small values stretch out individual wave cycles, so a bass note reads as a few big smooth curves. Large values pack more of the signal into the same width, so it reads as a rolling envelope. Around `20-30` is the hardware-scope look; **`150-180` is close to what FFT Size `8192` used to give**.
>
> It is deliberately independent of **FFT Size**. The trace is sampled on its own clock at roughly 100 updates a second whatever the FFT is doing, peak-sampled rather than point-sampled so it neither aliases nor loses amplitude as the window widens, and aligned to a rising zero crossing so the waveform holds still instead of sliding sideways.

**Oscilloscope Damping**: Slows how fast the trace changes *shape*, without slowing how often it is *drawn*. Range 0-100, default `0`.

> These are two different things and the distinction is the whole point. The trace is still redrawn every frame at your Target FPS, so nothing gets choppier. What damping changes is how far each point is allowed to move per frame, so the waveform evolves slowly enough to follow with your eyes instead of being a new picture every frame.
>
> This is what makes a wide **Time Window** legible rather than busy. On its own a 170 ms window is a lot of information changing fast; damped, it becomes a shape you can actually track. Try `150` window with `60-80` damping.
>
> It costs nothing. The easing is a single multiply per point on the render thread, and it does not touch the capture path or the frame rate.

**Anchor**: Which edge bars grow from. `Bottom` rise upward *(classic)* · `Top` hang downward · `Middle` grow **both directions** from a center line.

**Peak Hold Caps**: Leaves a thin marker floating at each bar's recent peak, which slowly drifts down.

**Beat Flash** + **Intensity**: Flashes bars brighter on bass hits. Intensity controls how hard.

**Rainbow Cycle Speed**: How fast the rainbow rotates. Rainbow mode only.

**Now Playing Text**: Toggles the artist/title display.

**Now Playing Color / Font / Font Size**: Styling for that text. The font must be **installed on your system**: type the exact family name. Windows silently falls back to a default on a typo rather than erroring, so double-check spelling if nothing changes.
> The Peak Frequency Readout shares these same font settings.

**Now Playing Display Seconds**: How long the text stays up after a track change before fading.

## Position

**Horizontal Position**: Left-to-right placement as a percentage. `0` hard left, `50` centered, `100` hard right.

**Vertical Position**: Top-to-bottom, same idea. `0` top, `100` bottom.

**Monitor**: Which screen to draw on. `1` is your first monitor.

## Background

**Enabled**: Draws a panel behind the bars. Turn off for bars floating directly on the wallpaper.

**Color**: Panel color in `#AARRGGBB`. The alpha controls transparency.

**Padding**: Breathing room between the bars and the panel edge.

**Corner Radius**: How rounded the panel corners are. Same one-or-four-value rules as bar radius.

**Blur**: Frosted-glass blur of your wallpaper behind the panel. `0` disables.
> This used to be the single most expensive setting in the mod. It's now computed once and cached, so it's essentially free per frame.

**Border Size / Border Color**: A thin outline around the panel. `0` for none.

## Performance

**Target FPS**: How many times per second it redraws. Higher = smoother, more CPU. Little point exceeding your monitor's refresh rate.

**Pause On Fullscreen**: Stops completely when a fullscreen app runs. Detects both true fullscreen *(games)* and borderless windows. Since the visualizer lives on the desktop it's invisible anyway. This just stops it burning power. Resumes automatically.

**Pause When Silent (seconds)**: After this long without audio, drops to a trickle instead of full speed. `0` disables.

**Auto-Hide When Idle** + **Delay**: Fades out entirely after prolonged silence. Once fully faded it **stops rendering completely**, not just invisible, genuinely doing nothing until audio returns.

**Pause When Covered**: Stops rendering *and* audio capture while fully hidden behind another window.
> **Off by default.** Reliably detecting "am I covered?" on Windows 11 is genuinely tricky: the shell is full of invisible windows that report themselves as visible. The check is deliberately conservative (only a fully-covering, real application window counts), but if the visualizer ever vanishes when it shouldn't, this is the switch to flip.

## Hardware

There are two jobs in this mod, and they can run on different silicon: **drawing** the bars, and **analysing** the audio (the FFT that turns sound into frequency bands). A GPU can do both. A CPU can do both. An NPU can only do the second, because it has no graphics hardware at all, so picking the NPU moves the analysis and leaves drawing wherever Drawing Device says.

**Drawing Device**: Which processor draws.
- **Auto** *(default)*: the GPU your chosen monitor is plugged into. On a laptop with two GPUs, or a desktop with the iGPU left on, drawing on the *other* one means every frame gets copied across to the GPU that scans it out, which is exactly the kind of uneven frame time this mod is trying to avoid. Auto asks Windows which GPU owns that screen and uses it, and follows if the answer changes.
- **Integrated GPU**: the power-saving GPU, the same one Windows picks for "Power saving" in Graphics settings. Falls back to Auto, with a warning, if there isn't one (F and KF processors have no iGPU, and desktops often switch it off in the BIOS).
- **Discrete GPU**: the high-performance GPU. Same fallback.
- **CPU**: software rendering through WARP. The GPU is left with nothing but Windows' own compositing. Fine for a small widget; a full-width strip at 144 FPS is real CPU work.

If the GPU is ever lost mid-session (a driver update, a driver reset, a laptop switching GPUs), the mod now rebuilds itself on whatever is available instead of freezing until restart.

**Audio Analysis Device**: Where the FFT runs. **CPU** *(default, recommended)* or **NPU**. The NPU path is covered in its own section below, including an honest note on whether it saves anything.

**NPU Runtime Folder**: Where Intel's OpenVINO runtime lives. Blank searches the usual places. Only read when Audio Analysis Device is NPU.

**Smooth Mode**: A render path built for integrated GPUs. **Auto** *(default)* turns it on when drawing runs on an integrated GPU or the CPU; **On** works on any GPU, and is worth trying on a discrete one too; **Off** is the 1.4 render path, untouched, for comparison. What it changes:

- **Frame timing locked to the display.** The 1.4 loop asks for a frame every `1000 / Target FPS` milliseconds and lets the timer's wake-up latency pile up, which is why 144 requested measured 141.7 delivered. On a 144 Hz panel that is a couple of refreshes every second with no new frame: a repeat, which you see as a hitch. Smooth Mode schedules from Windows' own composition clock instead, places every frame half a refresh away from the deadline on either side, and never accumulates drift.
- **Target FPS rounded to an even fraction of the refresh rate.** 60 FPS on a 144 Hz screen means frames held for 2, 3, 2, 3 refreshes, which reads as judder however precise the timer is. Smooth Mode runs the nearest whole fraction instead: 60 becomes 72 on 144 Hz, 48 if you set it to 50, 60 on a 60 Hz screen. Asking for more than the refresh rate gets the refresh rate rather than frames nobody can see.
- **Motion that doesn't depend on frame timing.** Bar rise and fall, peak caps, beat flash and scope damping were all "move this fraction per frame", so a late frame visibly slowed everything down for a moment. They now scale by how much time actually passed, measured against your Target FPS, so a frame that arrives on time looks exactly as it did in 1.4.
- **A proper animation clock.** Wave, Breathe and Rainbow ran off a millisecond counter that only ticks every 15.6 ms, stored in a float that loses precision the longer the PC has been on. After a day of uptime those shapes could only move in 8 ms steps; after ten days, 64 ms. They now run off the high-resolution clock and stay smooth however long Windows has been up.
- **The background baked once.** The blur, the panel fill and the border used to be redrawn every frame, the blur through a geometric mask layer, which costs an offscreen surface and extra full-panel passes. On an integrated GPU sharing memory bandwidth with the CPU, that is a real slice of the budget for pixels that never change. Smooth Mode renders all three into one image and blits it.
- **All bars in one draw call.** Every bar used to be its own rounded-rectangle fill, and with per-corner radii a brand new path geometry, per bar, per frame. Dots were worse: one shape per dot. Smooth Mode pre-draws the bar's antialiased caps once and sends every bar (or every dot) to the GPU as a single sprite batch with per-bar color. Square corners, rounded corners and the peak caps look the same; at whole-pixel bar widths the sides are pixel-for-pixel identical.
- **Cached text.** The Now Playing and frequency labels used to be laid out from scratch every frame. They are now laid out once per change of text.

---

![Oscilloscope closeup](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/4.gif)

*The same shape in a warm colour: the Oscilloscope picks up Color Mode like every other shape.*

# ◇ RUNNING THE ANALYSIS ON AN NPU

Written against an **Alienware Aurora ACT1250** (the 2025 Aurora: Core Ultra 7 265K / 265KF or Ultra 9 285K, RTX 50-series), but it applies to any Intel Core Ultra machine.

**What's in there.** Arrow Lake desktop chips carry Intel's NPU 3, sold as *Intel AI Boost*: about 13 TOPS, the same generation as Meteor Lake, and present on K, KF and F models alike (the F only drops the iGPU). It is a separate block from both the CPU cores and the GPU, built for one thing: large batches of multiply-accumulates in low precision (FP16 / INT8).

**Making sure it's on.**
1. Device Manager should have a **Neural processors** category with **Intel(R) AI Boost** under it. Task Manager on Windows 11 24H2 and later also shows an **NPU** graph on the Performance tab.
2. If it's missing, install the **Intel NPU driver** (from Dell's support page for the Aurora, or Intel's own "Intel NPU Driver - Windows" download). It needs Windows 11.
3. Still missing after that: check the BIOS for an NPU / AI Boost switch. Some Arrow Lake firmware ships one, and Intel's own guidance for Windows 10 machines is to turn the NPU off there.

**Giving this mod a way to talk to it.** Intel exposes the NPU through its **OpenVINO** runtime, which this mod loads at runtime if it's present. Nothing is bundled and nothing is installed for you. Any of these works:
- Download the OpenVINO archive for Windows from Intel and extract it (to `C:\Program Files (x86)\Intel\openvino_<version>` is the conventional spot, which the mod searches on its own), or
- `pip install openvino` into a Python install (the mod searches the usual Python locations too), or
- put the folder holding `openvino_c.dll` into **NPU Runtime Folder**.

Use an OpenVINO version at least as new as the one your NPU driver's release notes name. Then set **Audio Analysis Device** to **NPU**. The Windhawk log says what happened: which runtime and device it found, how long the model took to compile, and every 30 seconds the mean and worst NPU time per audio block. If anything is missing, the settings-problems window says which piece and what to do, and the analysis stays on the CPU in the meantime.

> **64-bit only.** OpenVINO ships 64-bit builds only. Windhawk 1.x runs tool mods in a 32-bit process, so there the NPU option reports that and stays on the CPU.

**How it works.** The FFT is rewritten as matrix multiplies (a "four-step" DFT: a DFT down the columns, a twiddle multiply, a DFT along the rows), which is the shape of work an NPU is built for. The model is generated in memory for your FFT Size, compiled for the NPU once, and checked against a known answer before it's trusted. Its output feeds the exact same band maths the CPU path does, so the bars look identical.

**Is it worth it?** Honestly: measure it, don't assume it. The CPU FFT costs a fraction of a millisecond per block, around 90 blocks a second at FFT Size 1024 and 12 at 8192, so there isn't much CPU time to free. Handing it to the NPU wakes the NPU at the same rate, and a part that would otherwise be power-gated waking 90 times a second can easily cost more package power than the CPU work it replaces. The bigger FFT sizes are where it has the best chance, since that's the most work per wake-up. Two back-to-back HWiNFO captures of package power, one per setting, plus this mod's own log lines will settle it for your machine, the same way the CPU figures above were measured.

# ▲ WHERE THE EFFICIENCY COMES FROM

In rough order of measured impact.

### 1. Precision frame pacing: the biggest single win

The obvious way to pace a desktop widget is `DwmFlush()`, which blocks until the monitor's next refresh. That wakes the render thread **on every vertical blank, forever** (60, 144, 240+ times a second) regardless of the target FPS, whether anything needs redrawing, or whether the visualizer is even visible.

This barely registers as CPU% in Task Manager, because the thread is blocked, not spinning. But every wake-up drags a core out of deep idle. Do that continuously and the core never settles into its efficient sleep states, which reads as a small, permanent bump in package power and temperature. The classic "low usage, still runs warm" signature.

> **Note:** this becomes exponentially more noticeable on AMD architecture.
>
> **Note:** also exponentially more noticeable if you have **C-States disabled** in your BIOS or elsewhere. Shoutout to Process Lasso, Core Director, Park Control and HWiNFO64 for helping me work out why all my E-cores were sitting at 65-70 °C when they were supposed to be idle.

**Instead:** a high-resolution waitable timer firing only at the configured rate. Plain `Sleep()` isn't good enough. It's quantized to ~15.6 ms, which would turn a 60 FPS target into stuttery 30-40 FPS.

### 2. Pre-rendered background blur

A Gaussian blur is a full-image convolution: the most expensive thing Direct2D does in this scene. Recomputing it every frame is pure waste, because its input (your wallpaper) never changes.

**Instead:** it is computed exactly once into a cached bitmap, and each frame just copies that. The cache covers only the widget's bounding box, which is **tens of KB of video memory rather than several MB**. It re-bakes automatically if the widget moves or resizes.

### 3. Widget-sized render surface

The visualizer occupies a thin strip, so a desktop-spanning render surface would clear and present millions of untouched pixels every frame, and park two full-desktop buffers in VRAM.

**Instead:** the surface is sized to the widget's bounding box, and the composition layer is offset to position it. That is roughly an order of magnitude less per-frame pixel work and VRAM.

### 4. Cached geometry

Building the background panel and border means allocating a path geometry and constructing four lines and four arcs by hand. It is rebuilt only when size, padding, radii or border width actually change (in normal use, almost never) rather than every frame.

### 5. Cached monitor lookup

`EnumDisplayMonitors()` is a real round-trip through the display driver stack. The monitor is resolved once and cached, refreshed on display change, instead of being looked up per frame.

### 6. Reduced frame latency

DXGI queues up to three frames ahead by default. For a passive widget that's pure latency and power draw with no upside. Capped at **1**.

### 7. Genuine idle shutdown

**Auto-Hide** now stops rendering completely once faded: presents one blank frame, then exits the render path entirely. **Pause When Covered** stops rendering and capture while hidden, checked once per second rather than per frame.

---

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/2.gif)

*The full-width strip in a red colour mode.*

# ▦ ON THE NUMBERS

The figures above come from HWiNFO64 sensor logging: two captures taken back to back in one session, one with the mod running and one with it disabled, with the same music playing throughout both so the only thing that changed was the mod itself. 543 samples running, 384 disabled, at 1.25 s intervals.

The full write-up (raw tables, the method, run-to-run variance, and an honest account of where the measurements fall short) is in [the project repo](https://github.com/USER-TOURNE/TOURNE-TABLE). It does not belong on a catalog page, so it is not reproduced here.

The caveats worth stating up front: **HWiNFO measures the whole machine, not this process alone.** A number quoted as the mod's cost therefore includes the work the mod causes elsewhere, in the compositor and the graphics driver, not only the time spent in its own threads. That makes it larger than a per-process trace would show, and it is the honest figure to quote, because it is what the machine actually pays. Both captures also contained short unrelated background spikes, which is why the numbers above are medians rather than averages.

---

![Tourne'Table Audio Visualizer](https://raw.githubusercontent.com/USER-TOURNE/TOURNE-TABLE/main/GIF/3.gif)

*Near-silence: the trace flattens out. When audio stops entirely, the render loop stops with it.*

## ♥ CREDITS

**[USER-TOURNE](https://github.com/USER-TOURNE)**: Author and maintainer: performance work, new features, benchmarking and documentation.

**[Salyts](https://github.com/Salyts)**: Original author of Desktop Audio Visualizer. This project exists because the foundation was good enough to be worth optimizing. Author of his own mod and repo; and a base for this one.

**[GR0UD](https://github.com/GR0UD)**: Audio visualizer code the original was adapted from. Author of his own work; and the base for Salyts.

### A coincidence worth acknowledging

While I was midway through my initial testing, **NeiZ** (author/maintainer, with **SuperSmile123** contributing) released [Desktop Audio Visualizer Plus](https://github.com/ramensoftware/windhawk-mods/commit/e01d0d0dbd6204804235831fd7f68821e4614028) (`neiz-supersmile-audio-visualizer`). I had planned to publish my own commit that same night: holy coincidence.

His release spurred another round of testing on my end, and I've since gone through roughly twelve more iterations. I didn't want to ship something that essentially achieved what I was already going for, especially if they'd figuratively led me out to pasture to put a bullet in me: aha.

To be explicit about attribution: **I did not borrow from or reference NeiZ or SuperSmile123's work at any point**, other than benchmarking theirs for efficiency to decide whether continuing development was worth it. Thanks to them and their contributors regardless.

---

## ◘ LICENSE

Released under the **MIT License**.

```
MIT License

Copyright (c) 2026 USER-TOURNE

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### In the spirit of the WTFPL

> **0. You just do what the fuck you want to.**
>
> Take it. Fork it. Gut it. Rewrite the parts I got wrong. Ship it. Sell it.
> Learn something from it and never speak to me again.
>
> The only thing MIT actually asks is that the copyright line rides along: 
> keep the notice, and we're square.
>
> *(I went with MIT over the real WTFPL for one boring reason: this thing
> runs a DLL loaded by Windhawk, and MIT comes with a warranty disclaimer.
> The WTFPL does not. Same energy, fewer ways for my life to get complicated.)*

### Third-party notices

This project builds on upstream work by **Salyts** and **GR0UD**. Their original
code carries its own license terms: if you redistribute this, carry their notices
along with mine.

---

## ❖ SUPPORT

If this saved you some degrees, some watts, or just made your desktop nicer to
look at, you can throw something in the hat. Entirely optional, genuinely appreciated.

**[ko-fi.com/tourne](https://ko-fi.com/tourne)**
*/
// ==/WindhawkModReadme==
 
// ==WindhawkModSettings==
/*
- appearance:
    - shape: stereo
      $name: Shape
      $description: Visual style of the bars
      $options:
        - stereo: Stereo
        - mountain: Mountain
        - mirror: Mirror
        - wave: Wave
        - breathe: Breathe
        - dots: Dots
        - radial: Radial
        - oscilloscope: Oscilloscope
    - orientation: horizontal
      $name: Orientation
      $description: Whether bars run left-to-right or bottom-to-top
      $options:
        - horizontal: Horizontal
        - vertical: Vertical
    - barCount: 32
      $name: Bar Count
      $description: How many bars are drawn across the visualizer
    - barWidth: 6
      $name: Bar Width
      $description: Thickness of each bar, in pixels
    - barGap: 4
      $name: Bar Gap
      $description: Space between bars, in pixels
    - barMaxSize: 140
      $name: Bar Max Size
      $description: Maximum height (or length, if vertical) a bar can reach at full volume, in pixels
    - barIdleSize: 4
      $name: Bar Idle Size
      $description: Minimum height bars keep when there's no audio, so the visualizer never looks completely flat
    - barCornerRadius: '3'
      $name: Bar Corner Radius
      $description: One value for all corners, or four space-separated values for top-left, top-right, bottom-right, bottom-left
    - colorMode: solid
      $name: Color Mode
      $description: How bars are colored. See the Color, Gradient Color 1/2 and Rainbow Cycle Speed settings below for the modes that use them
      $options:
        - solid: Solid
        - gradient: Gradient
        - reactive_gradient: Reactive Gradient
        - accent: Windows Accent Color
        - album_art: Album Art Color
        - dynamic_album: Dynamic Album Gradient
        - acrylic: Acrylic
        - rainbow: Rainbow Cycle
        - tourne: Tourne (Teal/Red)
    - color: '#FFFFFFFF'
      $name: Color
      $description: 'Used when Color Mode is Solid. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - gradientColor1: '#FF1ED760'
      $name: Gradient Color 1
      $description: 'Start color for Gradient and Reactive Gradient modes. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - gradientColor2: '#FF00B4FF'
      $name: Gradient Color 2
      $description: 'End color for Gradient and Reactive Gradient modes. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - sensitivity: 150
      $name: Sensitivity
      $description: How hard the bars react to audio. Range 0-300. Past a point, raising this mostly makes quiet passages louder rather than making loud passages hit any harder, since loud signal is already at max bar height, see Sensitivity Curve
    - sensitivityCurve: knee
      $name: Sensitivity Curve
      $description: How the top of the Sensitivity range is handled once a band's signal would otherwise overshoot max bar height. Exponential is smoothest but softens loud hits earliest; Knee keeps quiet-to-moderate signal untouched and only compresses the loud end; Power is the cheapest and gives the most headroom before compressing
      $options:
        - exponential: Exponential (Soft)
        - knee: Knee (Balanced)
        - power: Power (Headroom)
    - inputGain: 0
      $name: Input Gain (dB)
      $description: A fixed level adjustment applied to the captured audio before anything else. Range -24 to +24, where 0 leaves the signal exactly as captured. Loopback capture sees each app's own volume slider but not the Windows master slider, so a music app sitting at 40% reads as quiet no matter how loud the system sounds. This is the control for that. Unlike Sensitivity it also scales the Oscilloscope waveform
    - autoGain: false
      $name: Auto Gain
      $description: Continuously adapts the level so quiet sources still fill the bars, without retuning Sensitivity per app or per track. Boost only, so loud material behaves exactly as it does with this off. Held steady through silence, which means the noise floor is never lifted and idle shutdown still works normally
    - autoGainMaxBoost: 12
      $name: Auto Gain Max Boost (dB)
      $description: The ceiling on how far Auto Gain may lift a quiet signal. Range 0 to 24. Lower it if quiet passages are getting flattened more than you want, raise it if a very quiet source still will not fill the bars. Only used when Auto Gain is on
    - smoothing: 0
      $name: Motion Smoothing
      $description: Slows down how quickly bars rise and fall toward the target level, at the cost of a bit of lag. 0 is the original snappy response; higher values trade responsiveness for a steadier, easier-to-read motion -- helpful when bars are small on the desktop and fast jitter is hard to track
    - eqPreset: default
      $name: EQ Preset
      $description: Reshapes how much each frequency range (bass/mid/treble) is boosted, tuned per genre
      $options:
        - default: Default
        - bass: Bass
        - rock: Rock
        - pop: Pop
        - jazz: Jazz
        - electronic: Electronic
    - fftSize: '1024'
      $name: FFT Size
      $description: Higher values give finer frequency detail at a small extra CPU cost
      $options:
        - '1024': 1024 (Fastest)
        - '2048': '2048'
        - '4096': '4096'
        - '8192': 8192 (Most Detailed)
    - freqScale: log
      $name: Frequency Scale
      $description: How bar position maps to frequency across the spectrum
      $options:
        - log: Log (natural, matches previous behavior)
        - linear: Linear (Hz-even spacing)
        - mel: Mel (perceptual pitch spacing)
    - peakFreqEnabled: false
      $name: Peak Frequency Readout
      $description: Shows the current dominant frequency as a small numeric overlay
    - peakFreqAlignH: right
      $name: Peak Readout - Horizontal Position
      $description: Where the frequency readout sits left-to-right
      $options:
        - left: Left
        - center: Center
        - right: Right
    - peakFreqAlignV: top
      $name: Peak Readout - Vertical Position
      $description: Above and Below place it fully clear of the bars or waveform
      $options:
        - above: Above (outside)
        - top: Top (inside)
        - middle: Middle
        - bottom: Bottom (inside)
        - below: Below (outside)
    - oscilloscopeMultibandEnabled: false
      $name: Multiband Oscilloscope Coloring
      $description: Tints the Oscilloscope trace by which part of the spectrum (low/mid/high) is currently dominant, overriding Color Mode for that shape
    - oscilloscopeWindowMs: 24
      $name: Oscilloscope Time Window (ms)
      $description: How much audio the trace shows end to end, in milliseconds. Range 5 to 250. Low values stretch out individual wave cycles and look like a hardware scope; high values compress more of the signal into the same width and read as a rolling envelope. This is a real time base, independent of FFT Size, so changing frequency resolution no longer changes what the trace looks like. Around 150 to 180 is close to what FFT Size 8192 used to give. Only affects the Oscilloscope shape
    - oscilloscopeDamping: 0
      $name: Oscilloscope Damping
      $description: Slows how fast the trace changes shape, without slowing how often it is drawn. Range 0 to 100. 0 is the raw trace, redrawn every frame. Higher values ease each point toward its new position so the waveform moves slowly enough to follow with your eyes, which is what makes a wide time window legible rather than busy. Pair it with a high Time Window. Costs nothing and does not change your frame rate. Only affects the Oscilloscope shape
    - verticalAnchor: bottom
      $name: Anchor
      $description: Where bars grow from
      $options:
        - top: Top
        - middle: Middle
        - bottom: Bottom
    - peakHoldEnabled: false
      $name: Peak Hold Caps
      $description: Draw a thin cap that hangs at each bar's recent peak
    - peakHoldColor: '#FF00B4FF'
      $name: Peak Hold Cap Color
      $description: 'Color of the peak hold caps. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b). Independent of Color Mode -- caps are always drawn in this color, even with Rainbow/Album Art/Accent/Acrylic'
    - beatFlashEnabled: false
      $name: Beat Flash
      $description: Flash bars toward Beat Flash Color on detected bass hits
    - beatFlashColor: '#FFFFFFFF'
      $name: Beat Flash Color
      $description: 'Color bars flash toward on a beat. Accepts #AARRGGBB, #RRGGBB, rgba(r, g, b, a) with alpha 0-1, or rgb(r, g, b). Alpha controls how strong the flash gets even at full intensity -- lower it for a subtler tint instead of a full color swap'
    - beatFlashIntensity: 80
      $name: Beat Flash Intensity
      $description: How quickly a beat reaches full Beat Flash Color. Only used when Beat Flash is on
    - rainbowSpeed: 40
      $name: Rainbow Cycle Speed
      $description: Only used when Color Mode is Rainbow Cycle
    - nowPlayingEnabled: false
      $name: Now Playing Text
      $description: Show artist and title above the visualizer, using Windows media session info
    - nowPlayingColor: '#FFFFFFFF'
      $name: Now Playing Text Color
      $description: 'Color of the artist/title text. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - nowPlayingFont: Segoe UI
      $name: Now Playing Font
      $description: Font family name, must be installed on your system (e.g. a Nerd Font for glyph support)
    - nowPlayingFontSize: 16
      $name: Now Playing Font Size
      $description: Text size, in points
    - nowPlayingDisplaySeconds: 6
      $name: Now Playing Display Seconds
      $description: How long the text stays visible after a track changes, before fading out
    - nowPlayingOffsetX: 0
      $name: Now Playing Offset X
      $description: Shifts the artist/title text sideways from where it normally sits, in pixels. Negative moves it left, positive right. It still travels with the visualizer -- this only changes where it sits relative to it, which is how you move it clear of the background panel. Can also be nudged live with the keyboard (Interaction, move target 2). A keyboard nudge ADDS to this number rather than replacing it, so whatever you type here always counts
    - nowPlayingOffsetY: 0
      $name: Now Playing Offset Y
      $description: Shifts the artist/title text up or down from where it normally sits, in pixels. Negative moves it up, positive down. As with Offset X, a keyboard nudge adds to this rather than replacing it
    - nowPlayingBgColor: '#00000000'
      $name: Now Playing Background
      $description: 'A panel drawn behind the artist/title text, sized to the text itself rather than to the visualizer. Fully transparent by default, meaning no panel. Give it some alpha to make the text readable over a busy wallpaper without having to enlarge the main Background panel. It fades in and out with the text. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - nowPlayingBgPadding: 6
      $name: Now Playing Background Padding
      $description: Space between the text and the edge of its panel, in pixels
    - nowPlayingBgCornerRadius: 6
      $name: Now Playing Background Corner Radius
      $description: Roundness of the text panel's corners, in pixels
    - nowPlayingBgBorderSize: 0
      $name: Now Playing Background Border Size
      $description: Outline thickness around the text panel, in pixels. 0 disables it. Draws whether or not the panel itself has any fill, so an outline on its own is possible
    - nowPlayingBgBorderColor: '#40FFFFFF'
      $name: Now Playing Background Border Color
      $description: 'Color of the text panel''s outline. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - peakFreqOffsetX: 0
      $name: Peak Readout Offset X
      $description: Shifts the frequency readout sideways from its chosen alignment, in pixels. Negative moves it left, positive right. Use this for fine placement -- the alignment options above put it in the right general area, this puts it exactly where you want it. Can also be nudged live with the keyboard (Interaction, move target 3), which adds to this number rather than replacing it
    - peakFreqOffsetY: 0
      $name: Peak Readout Offset Y
      $description: Shifts the frequency readout up or down from its chosen alignment, in pixels. Negative moves it up, positive down. As with Offset X, a keyboard nudge adds to this rather than replacing it
    - peakFreqBgColor: '#00000000'
      $name: Peak Readout Background
      $description: 'A panel drawn behind the frequency readout, sized to the text itself. Fully transparent by default, meaning no panel. Particularly useful for this one, since the readout sits over the bars on the inside alignments and can be hard to read against them. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - peakFreqBgPadding: 6
      $name: Peak Readout Background Padding
      $description: Space between the readout and the edge of its panel, in pixels
    - peakFreqBgCornerRadius: 6
      $name: Peak Readout Background Corner Radius
      $description: Roundness of the readout panel's corners, in pixels
    - peakFreqBgBorderSize: 0
      $name: Peak Readout Background Border Size
      $description: Outline thickness around the readout panel, in pixels. 0 disables it. Draws whether or not the panel itself has any fill
    - peakFreqBgBorderColor: '#40FFFFFF'
      $name: Peak Readout Background Border Color
      $description: 'Color of the readout panel''s outline. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
  $name: Appearance
- position:
    - horizontalPosition: '50'
      $name: Horizontal Position
      $description: 0-100, percentage across the monitor's work area. Decimals are allowed (e.g. 50.25) for fine-grained placement -- useful since a whole-number step can be a big jump in pixels on a large monitor. IMPORTANT -- once you have moved the visualizer by keyboard or by dragging, that saved position REPLACES this field, and editing this will appear to do nothing until you clear it with the move modifier + Home (see Interaction)
    - verticalPosition: '88'
      $name: Vertical Position
      $description: 0-100, percentage down the monitor's work area. Decimals are allowed (e.g. 88.5) for fine-grained placement. Same caveat as Horizontal Position -- a saved keyboard/drag position takes priority over this field until it is cleared with the move modifier + Home
    - monitor: 1
      $name: Monitor
      $description: 1-based monitor index
  $name: Position
- interaction:
    - keyMoveEnabled: true
      $name: Enable Keyboard Move
      $description: Hold the modifier below and tap a direction key to nudge the visualizer one pixel at a time, the way a window manager moves a tiled window. This is the precise way to place it -- it lands exactly where you put it, with no cursor to keep up with. Hold the Fast Key as well to jump by the larger step instead. The same combo with 1, 2, 3 or 4 switches what the direction keys steer -- 1 the visualizer, 2 the Now Playing text, 3 the frequency readout, 4 the Media Controls strip -- so everything movable is placed with one shortcut, and the choice sticks until you change it. Modifier + Home resets whichever is selected. Worth knowing -- the visualizer and the strip store a percentage that REPLACES their Position settings, so those fields stop having any effect until you reset; the two text overlays instead store a delta ADDED to their Offset settings, so those fields always still count. Also, if the strip is currently parked by Hide When Covered, nudging it still works but you will not see it move until it comes back
    - keyMoveModifier: ctrl_alt
      $name: Keyboard Move Modifier
      $description: Held down while tapping a direction key. Use a two-key combo. This mod swallows the keypress outright, so the app underneath does not fall back to its own behaviour -- it simply never hears about it. With a one-key modifier that means real losses -- Ctrl with WASD eats Ctrl+A and, worse, Ctrl+S, so a save you think you made silently does not happen. Ctrl with arrows eats word-by-word cursor movement. Pick a one-key modifier and the mod will warn you on load listing exactly what you gave up
      $options:
        - ctrl_alt: Ctrl + Alt
        - ctrl_shift: Ctrl + Shift
        - alt_shift: Alt + Shift
        - win_alt: Win + Alt
        - win_shift: Win + Shift
        - ctrl: Ctrl
        - alt: Alt
        - shift: Shift
        - win: Win
    - keyMoveKeys: both
      $name: Keyboard Move Direction Keys
      $options:
        - both: Arrow Keys and WASD
        - arrows: Arrow Keys only
        - wasd: WASD only
    - keyMoveStep: 1
      $name: Keyboard Move Step
      $description: How far one press moves the visualizer, in pixels. 1 gives true pixel-by-pixel placement
    - keyMoveFastStep: 10
      $name: Keyboard Move Fast Step
      $description: How far one press moves it while the Fast Key below is also held, in pixels
    - keyMoveFastKey: shift
      $name: Keyboard Move Fast Key
      $description: Held alongside the modifier to use the larger step. Must not be one of the keys already used by the modifier above
      $options:
        - shift: Shift
        - ctrl: Ctrl
        - alt: Alt
        - win: Win
        - none: None (disable fast step)
    - dragEnabled: false
      $name: Enable Drag-to-Move
      $description: Hold the modifier + mouse button below anywhere over the visualizer and drag to reposition it. Bar rendering pauses for the duration of the drag -- only the background/border box (if Background is enabled) moves. Double-click the same combo without dragging to clear a dragged position. Note this rides a global mouse hook and has to repaint the whole visualizer to keep up with the cursor, so on a heavy shape or a high bar count it can feel sluggish next to the keyboard move above -- which is why it's off by default now
    - dragModifier: ctrl
      $name: Drag Modifier Key
      $description: Held together with the mouse button below to start a drag
      $options:
        - none: None
        - ctrl: Ctrl
        - alt: Alt
        - shift: Shift
        - win: Win
    - dragButton: middle
      $name: Drag Mouse Button
      $options:
        - left: Left Click
        - middle: Middle Click
        - right: Right Click
  $name: Interaction
- media_controls:
    - enabled: false
      $name: Enabled
      $description: Adds a small Previous / Play-Pause / Next control strip, wired to whatever app is currently playing media (Spotify, a browser, etc.) via the same native Windows media session API the Now Playing text reads from. Unlike the visualizer itself, this strip sits on top of other windows rather than behind the desktop icons, since it has to actually receive your clicks
    - iconColor: '#FFFFFFFF'
      $name: Icon Color
      $description: 'Color for the built-in Previous/Play/Pause/Next glyphs, used whenever a custom icon path below is left blank. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - iconPrevPath: ''
      $name: Previous Icon Path
      $description: Full path to a local image file (PNG, JPG, BMP, or ICO -- transparency supported) to use for the Previous button. Leave blank to use the built-in icon
    - iconPlayPath: ''
      $name: Play Icon Path
      $description: Full path to a local image file for the Play button, shown while paused/stopped. Leave blank to use the built-in icon
    - iconPausePath: ''
      $name: Pause Icon Path
      $description: Full path to a local image file for the Pause button, shown while something is playing. Leave blank to use the built-in icon
    - iconNextPath: ''
      $name: Next Icon Path
      $description: Full path to a local image file for the Next button. Leave blank to use the built-in icon
    - iconSize: 32
      $name: Icon Size
      $description: Size of each button, in pixels (square)
    - iconSpacing: 14
      $name: Icon Spacing
      $description: Gap between buttons, in pixels
    - plateColor: '#00000000'
      $name: Backing Plate Color
      $description: 'A panel drawn behind the whole icon strip. Fully transparent by default, so your icons sit directly on the wallpaper with nothing behind them. Raise the alpha if pale icons are hard to pick out against a light wallpaper -- e.g. #8C141414 for a soft dark plate. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - platePadding: 0
      $name: Backing Plate Padding
      $description: Breathing room between the icons and the edge of the backing plate, in pixels. This grows the strip itself rather than shrinking the icons, so raising it never makes the buttons smaller or harder to click. Note that it grows the strip whether or not the plate is visible, which very slightly shifts where the Horizontal/Vertical Position percentages land
    - plateCornerRadius: 8
      $name: Backing Plate Corner Radius
      $description: Roundness of the backing plate corners, in pixels. Only used when the plate color above has some alpha
    - plateBorderSize: 0
      $name: Backing Plate Border Size
      $description: Outline thickness around the backing plate, in pixels. 0 disables it. The border draws whether or not the plate itself has any fill, so you can have an outline on its own with nothing behind the icons
    - plateBorderColor: '#40FFFFFF'
      $name: Backing Plate Border Color
      $description: 'Color of the backing plate outline. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - hideWhenCovered: false
      $name: Hide When Covered
      $description: The strip is topmost, so by default it stays on screen over whatever else you have open. Turn this on to have it get out of the way while a real application window sits underneath it, and come back when that window moves or closes. Note that while it is parked out of the way it is genuinely hidden, so keyboard nudges (move target 4) still apply but you will not see them land until it comes back. Coverage is re-checked once a second, not instantly
    - coveredThresholdPercent: '50'
      $name: Hide When Covered - Threshold
      $description: How much of the strip has to be underneath a window before it hides. Only used when Hide When Covered is on
      $options:
        - '5': 5% covered
        - '10': 10% covered
        - '15': 15% covered
        - '20': 20% covered
        - '25': 25% covered
        - '30': 30% covered
        - '35': 35% covered
        - '40': 40% covered
        - '45': 45% covered
        - '50': 50% covered
        - '55': 55% covered
        - '60': 60% covered
        - '65': 65% covered
        - '70': 70% covered
        - '75': 75% covered
        - '80': 80% covered
        - '85': 85% covered
        - '90': 90% covered
        - '95': 95% covered
        - '100': 100% covered (fully hidden)
    - horizontalPosition: '50'
      $name: Horizontal Position
      $description: 0-100, percentage across the monitor's work area. Decimals are allowed (e.g. 50.25) for precise placement, independent of where the visualizer itself sits. The strip can also be nudged a pixel at a time with the keyboard -- see Interaction, where it is move target 4. IMPORTANT -- once nudged, that saved position REPLACES this field, and editing this will appear to do nothing until you clear it with the move modifier + 4 then Home
    - verticalPosition: '95'
      $name: Vertical Position
      $description: 0-100, percentage down the monitor's work area. Decimals allowed. Same caveat as Horizontal Position -- a saved keyboard position takes priority over this field until it is cleared
  $name: Media Controls
- background:
    - enabled: true
      $name: Enabled
      $description: Draws a rounded panel behind the visualizer
    - color: '#60000000'
      $name: Color
      $description: 'Panel fill color. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
    - padding: '24'
      $name: Padding
      $description: Space between the bars/waveform and the panel edge, in pixels. One value for all sides, or four space-separated values for left, right, top, bottom -- so you can grow the panel more on one side without moving the bars or resizing anything else. Negative values shrink that side inward instead, past the bars if pushed far enough
    - cornerRadius: '14'
      $name: Corner Radius
      $description: Roundness of the panel corners, in pixels. One value for all corners, or four space-separated values for top-left, top-right, bottom-right, bottom-left
    - blur: 0
      $name: Blur
      $description: Gaussian blur strength behind the panel, in pixels. 0 disables it. Computed once and cached, not redrawn every frame, so raising this has minimal ongoing CPU cost
    - borderSize: 0
      $name: Border Size
      $description: Panel border thickness, in pixels. 0 disables it
    - borderColor: '#40FFFFFF'
      $name: Border Color
      $description: 'Panel border color. Format is #AARRGGBB, #RRGGBB, rgba(r, g, b, a), or rgb(r, g, b)'
  $name: Background
- performance:
    - targetFps: 60
      $name: Target FPS
      $description: Caps how often the visualizer redraws itself. Match your monitor's refresh rate for the smoothest motion at the lowest overhead
    - pauseOnFullscreen: true
      $name: Pause On Fullscreen
      $description: Stops rendering while any window is fullscreen, since the visualizer would be hidden behind it anyway
    - pauseWhenSilentSeconds: 10
      $name: Pause When Silent (seconds)
      $description: 0 disables this behavior
    - autoHideEnabled: false
      $name: Auto-Hide When Idle
      $description: Fades the whole visualizer out after prolonged silence, instead of just idling
    - autoHideDelaySeconds: 15
      $name: Auto-Hide Delay (seconds)
      $description: How long to wait after audio goes silent before fading the visualizer out. Only used when Auto-Hide When Idle is on
    - pauseWhenObscured: false
      $name: Pause When Covered
      $description: Stops rendering entirely while the visualizer is hidden behind another window, since nothing it draws would be visible anyway. How much of it has to be covered before that kicks in is set below
    - obscuredThresholdPercent: '100'
      $name: Pause When Covered - Threshold
      $description: How much of the visualizer's box (bars plus background padding) has to be underneath real application windows before rendering pauses. 100% means it only pauses when completely hidden, which is the safest setting and the old behavior. Lower values pause sooner and save more power, at the risk of stopping while a sliver of it is still peeking out. Coverage is measured as actual area across every covering window combined, not per-window, so two half-covering windows count as fully covered
      $options:
        - '5': 5% covered
        - '10': 10% covered
        - '15': 15% covered
        - '20': 20% covered
        - '25': 25% covered
        - '30': 30% covered
        - '35': 35% covered
        - '40': 40% covered
        - '45': 45% covered
        - '50': 50% covered
        - '55': 55% covered
        - '60': 60% covered
        - '65': 65% covered
        - '70': 70% covered
        - '75': 75% covered
        - '80': 80% covered
        - '85': 85% covered
        - '90': 90% covered
        - '95': 95% covered
        - '100': 100% covered (fully hidden)
  $name: Performance
- hardware:
    - renderDevice: auto
      $name: Drawing Device
      $description: Which processor draws the visualizer. Auto uses the GPU your chosen monitor is plugged into, which avoids copying every frame from one GPU to another and is the right answer almost everywhere. Integrated and Discrete force a GPU type, and fall back to Auto (with a warning) if this PC does not have one. CPU draws in software through WARP, so the GPU is left with nothing but Windows' own compositing - fine for a small widget, expensive for a wide strip at a high frame rate
      $options:
        - auto: Auto (GPU driving the monitor)
        - integrated: Integrated GPU (lowest power)
        - discrete: Discrete GPU (highest performance)
        - cpu: CPU (software rendering)
    - analysisDevice: cpu
      $name: Audio Analysis Device
      $description: Where the FFT runs - the math that splits captured audio into frequency bands. CPU is the default and the cheapest, a fraction of a millisecond per block. NPU runs it on an Intel AI Boost NPU through Intel's OpenVINO runtime, which is installed separately (see NPU Runtime Folder). An NPU has no graphics hardware, so drawing still follows Drawing Device. If the NPU or the runtime is missing it falls back to the CPU on its own and tells you why. Needs a 64-bit Windhawk tool process
      $options:
        - cpu: CPU (recommended)
        - npu: NPU (Intel AI Boost via OpenVINO)
    - npuRuntimePath: ''
      $name: NPU Runtime Folder
      $description: The folder that contains openvino_c.dll, or the root of an extracted OpenVINO archive. Leave blank to search the usual places - INTEL_OPENVINO_DIR, Program Files\Intel\openvino*, C:\Intel\openvino*, a pip-installed openvino package, and PATH. Only used when Audio Analysis Device is NPU
    - smoothMode: auto
      $name: Smooth Mode
      $description: A render path built for integrated GPUs. Frame timing is locked to the display's refresh so every frame lands on its own refresh, motion speed stays constant even when a frame runs late, the background panel is baked into one cached image instead of being masked and filled every frame, and all bars are drawn in one batched call instead of one shape each. Target FPS is rounded to an even fraction of the refresh rate (60 on a 144 Hz screen runs at 72) because that is what makes motion even. Auto turns it on when drawing runs on an integrated GPU or the CPU. On works with any GPU. Off is the 1.4 render path, unchanged
      $options:
        - auto: Auto (integrated GPU or CPU drawing)
        - 'on': 'On'
        - 'off': Off (1.4 render path)
  $name: Hardware
- validation:
    - showErrors: true
      $name: Warn About Invalid Settings
      $description: Several settings here are free-text fields -- colors, positions, padding, corner radii, icon paths. Windhawk will happily save a typo in one of them, and the mod then quietly falls back to a default, so the setting looks saved but does nothing. With this on, a summary window appears listing exactly which fields couldn't be read, what you typed, and what format was expected. Turn it off if you'd rather it stayed silent -- the same list still goes to the Windhawk mod log either way
  $name: Settings Validation
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <d2d1_1.h>
// d2d1_3 for ID2D1DeviceContext3 / ID2D1SpriteBatch (Smooth Mode's batched
// bars), dxgi1_6 for EnumAdapterByGpuPreference (Drawing Device). Both are
// only used after a runtime QueryInterface, so older Windows still loads.
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <shellscalingapi.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <dwrite.h>
#include <windowsx.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

using Microsoft::WRL::ComPtr;

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

#define TIMER_ID_MSG_DISPLAY_CHANGE 1
#define TIMER_ID_MSG_RECREATE_OVERLAY 2
#define TIMER_ID_MSG_WALLPAPER_REFRESH 3
#define TIMER_ID_MSG_FULLSCREEN_WATCH 4
#define TIMER_ID_MSG_SAVE_POSITION 5
#define TIMER_ID_MSG_FONT_RECHECK 6
#define TIMER_ID_MSG_REBUILD_DEVICE 7

#define WM_APP_CLEANUP (WM_APP + 1)
#define WM_APP_SETTINGS_CHANGED (WM_APP + 2)
#define WM_APP_RENDER_TICK (WM_APP + 3)
#define WM_APP_MEDIA_REPAINT (WM_APP + 4)
#define WM_APP_REQUEST_SAVE_POSITION (WM_APP + 5)
#define WM_APP_FORCE_REDRAW (WM_APP + 6)
// Hardware section. A worker thread (NPU build) or the render path (device
// loss) can't touch the settings-issue list or rebuild D3D objects itself, so
// both hand the job to the message window, which lives on the UI thread.
#define WM_APP_HW_NOTICE (WM_APP + 7)        // lParam: heap std::wstring*, owned by receiver
#define WM_APP_REBUILD_DEVICE (WM_APP + 8)   // wParam: 1 = device was lost
#define OVERLAY_WINDOW_CLASS (L"DesktopAudioVisOverlay_" WH_MOD_ID)
#define MESSAGE_WINDOW_CLASS (L"DesktopAudioVisMessage_" WH_MOD_ID)
#define MEDIA_WINDOW_CLASS (L"DesktopAudioVisMedia_" WH_MOD_ID)

enum class VizShape { Stereo, Mountain, Mirror, Wave, Breathe, Dots, Radial, Oscilloscope };
enum class VizColorMode { Solid, Gradient, ReactiveGradient, Accent, AlbumArt, DynamicAlbum, Acrylic, RainbowCycle, Tourne };
enum class VizEQ { Default, Bass, Rock, Pop, Jazz, Electronic };
enum class VizSensitivityCurve { Exponential, Knee, Power };
enum class VizOrientation { Horizontal, Vertical };
enum class VizAnchor { Top, Middle, Bottom };
enum class VizFreqScale { Log, Linear, Mel };
enum class VizTextAlignH { Left, Center, Right };
enum class VizTextAlignV { Above, Top, Middle, Bottom, Below };
enum class VizDragModifier { None, Ctrl, Alt, Shift, Win };
enum class VizDragButton { Left, Middle, Right };
// Bit flags rather than an enum of named combos, so "is every key in the combo
// currently down" is one mask test and "does the fast key overlap the modifier"
// is one AND -- which is what the settings validator checks for.
enum VizModKeyFlags : unsigned {
    VIZ_MOD_NONE  = 0,
    VIZ_MOD_CTRL  = 1u << 0,
    VIZ_MOD_ALT   = 1u << 1,
    VIZ_MOD_SHIFT = 1u << 2,
    VIZ_MOD_WIN   = 1u << 3,
};
enum class VizKeyMoveKeys { Both, Arrows, Wasd };
// What the keyboard-move keys are currently steering. Switched live with the
// move modifier + 1/2/3, so the same combo places all three pieces without
// needing a separate shortcut per piece.
enum class VizMoveTarget { Visualizer, NowPlaying, PeakFreq, MediaControls };
// Hardware section.
enum class VizRenderDevice { Auto, Integrated, Discrete, Cpu };
enum class VizAnalysisDevice { Cpu, Npu };
enum class VizSmoothMode { Auto, On, Off };

struct Settings {
    VizShape shape = VizShape::Stereo;
    VizOrientation orientation = VizOrientation::Horizontal;
    int barCount = 32;
    int barWidth = 6;
    int barGap = 4;
    int barMaxSize = 140;
    int barIdleSize = 4;
    float barRadiusTL = 3.0f, barRadiusTR = 3.0f, barRadiusBR = 3.0f, barRadiusBL = 3.0f;
    VizColorMode colorMode = VizColorMode::Solid;
    BYTE colorA = 255, colorR = 255, colorG = 255, colorB = 255;
    BYTE grad1A = 255, grad1R = 30, grad1G = 215, grad1B = 96;
    BYTE grad2A = 255, grad2R = 0, grad2G = 180, grad2B = 255;
    int sensitivity = 150;
    VizSensitivityCurve sensitivityCurve = VizSensitivityCurve::Knee;
    float inputGainDb = 0.0f;
    bool autoGain = false;
    float autoGainMaxDb = 12.0f;
    int smoothing = 0;
    VizEQ eq = VizEQ::Default;

    float horizontalPosition = 50.0f;
    float verticalPosition = 88.0f;
    int monitor = 1;
    VizAnchor verticalAnchor = VizAnchor::Bottom;

    bool keyMoveEnabled = true;
    unsigned keyMoveModifier = VIZ_MOD_CTRL | VIZ_MOD_ALT;
    VizKeyMoveKeys keyMoveKeys = VizKeyMoveKeys::Both;
    int keyMoveStep = 1;
    int keyMoveFastStep = 10;
    unsigned keyMoveFastKey = VIZ_MOD_SHIFT;

    bool dragEnabled = false;
    VizDragModifier dragModifier = VizDragModifier::Ctrl;
    VizDragButton dragButton = VizDragButton::Middle;

    bool showSettingsErrors = true;

    bool backgroundEnabled = true;
    BYTE bgA = 0x60, bgR = 0, bgG = 0, bgB = 0;
    int bgPaddingL = 24, bgPaddingR = 24, bgPaddingT = 24, bgPaddingB = 24;
    float bgRadiusTL = 14.0f, bgRadiusTR = 14.0f, bgRadiusBR = 14.0f, bgRadiusBL = 14.0f;
    int bgBlur = 0;
    int bgBorderSize = 0;
    BYTE borderA = 0x40, borderR = 255, borderG = 255, borderB = 255;

    int targetFps = 60;
    bool pauseOnFullscreen = true;
    int pauseWhenSilentSeconds = 10;

    bool peakHoldEnabled = false;
    BYTE peakHoldA = 255, peakHoldR = 0, peakHoldG = 180, peakHoldB = 255;
    bool beatFlashEnabled = false;
    BYTE beatFlashA = 255, beatFlashR = 255, beatFlashG = 255, beatFlashB = 255;
    int beatFlashIntensity = 80;
    int rainbowSpeed = 40;

    bool nowPlayingEnabled = false;
    BYTE nowPlayingA = 255, nowPlayingR = 255, nowPlayingG = 255, nowPlayingB = 255;
    std::wstring nowPlayingFont = L"Segoe UI";
    int nowPlayingFontSize = 16;
    int nowPlayingDisplaySeconds = 6;
    int nowPlayingOffsetX = 0, nowPlayingOffsetY = 0;
    int peakFreqOffsetX = 0, peakFreqOffsetY = 0;

    // Per-overlay text panels. Alpha 0 on the fill means "no panel"; the border
    // is independent, so an outline with nothing behind it is a valid look.
    BYTE npBgA = 0, npBgR = 0, npBgG = 0, npBgB = 0;
    int npBgPadding = 6, npBgCornerRadius = 6, npBgBorderSize = 0;
    BYTE npBgBorderA = 0x40, npBgBorderR = 255, npBgBorderG = 255, npBgBorderB = 255;

    BYTE pfBgA = 0, pfBgR = 0, pfBgG = 0, pfBgB = 0;
    int pfBgPadding = 6, pfBgCornerRadius = 6, pfBgBorderSize = 0;
    BYTE pfBgBorderA = 0x40, pfBgBorderR = 255, pfBgBorderG = 255, pfBgBorderB = 255;

    bool autoHideEnabled = false;
    int autoHideDelaySeconds = 15;
    bool pauseWhenObscured = false;
    int obscuredThresholdPercent = 100;

    int fftSize = 1024;
    VizFreqScale freqScale = VizFreqScale::Log;
    bool peakFreqEnabled = false;
    VizTextAlignH peakFreqAlignH = VizTextAlignH::Right;
    VizTextAlignV peakFreqAlignV = VizTextAlignV::Top;
    bool oscilloscopeMultibandEnabled = false;
    int oscilloscopeWindowMs = 24;
    int oscilloscopeDamping = 0;

    bool mediaControlsEnabled = false;
    BYTE mediaIconColorA = 255, mediaIconColorR = 255, mediaIconColorG = 255, mediaIconColorB = 255;
    std::wstring mediaIconPrevPath;
    std::wstring mediaIconPlayPath;
    std::wstring mediaIconPausePath;
    std::wstring mediaIconNextPath;
    int mediaIconSize = 32;
    int mediaIconSpacing = 14;
    BYTE mediaPlateA = 0, mediaPlateR = 0, mediaPlateG = 0, mediaPlateB = 0;
    int mediaPlatePadding = 0;
    int mediaPlateCornerRadius = 8;
    int mediaPlateBorderSize = 0;
    BYTE mediaPlateBorderA = 0x40, mediaPlateBorderR = 255, mediaPlateBorderG = 255,
         mediaPlateBorderB = 255;
    bool mediaHideWhenCovered = false;
    int mediaCoveredThresholdPercent = 50;
    float mediaHorizontalPosition = 50.0f;
    float mediaVerticalPosition = 95.0f;

    VizRenderDevice renderDevice = VizRenderDevice::Auto;
    VizAnalysisDevice analysisDevice = VizAnalysisDevice::Cpu;
    std::wstring npuRuntimePath;
    VizSmoothMode smoothMode = VizSmoothMode::Auto;
};

// A 4K display fits ~1280 bars at 2 px wide with 1 px gaps, and ultrawides more
// still, so the cap has room above that. Each bar costs a handful of floats.
constexpr int VIZ_BARS_MAX = 2048;
constexpr int VIZ_FFT_SIZE_MAX = 8192;
constexpr int VIZ_NUM_BANDS = 7;
constexpr float VIZ_PI = 3.14159265f;

// Frequency-band edges (Hz), shared between BuildLogBins (capture thread) and
// HzToBandPos (render thread) so the Log/Linear/Mel scale mapping always
// warps onto the exact same band boundaries the audio analysis actually uses.
constexpr float VIZ_FREQ_EDGES[VIZ_NUM_BANDS + 1] = {
    20.f, 120.f, 300.f, 800.f, 2500.f, 6000.f, 14000.f, 20000.f};

// Which EQ zone (0=low, 1=mid, 2=high) each band belongs to. Shared between
// the capture thread's EQ preset weighting and the multiband oscilloscope
// coloring so both agree on the same low/mid/high split.
constexpr int VIZ_BAND_EQ_ZONE[VIZ_NUM_BANDS] = {0, 0, 1, 1, 2, 2, 2};

Settings g_settings;

std::atomic<bool> g_lazyInitialized{false};
std::atomic<bool> g_initSucceeded{false};
std::atomic<bool> g_unloading{false};

ComPtr<ID3D11Device> g_d3dDevice;
ComPtr<IDXGIDevice> g_dxgiDevice;
ComPtr<IDXGIFactory2> g_dxgiFactory;
ComPtr<ID2D1Factory1> g_d2dFactory;
ComPtr<ID2D1Device> g_d2dDevice;

HWND g_messageWnd;
HWND g_mediaWnd;
std::atomic<bool> g_mediaIsPlaying{false};

// Runtime position override for the media strip, same shape as the one the
// visualizer uses: percentages of the work area, replacing the Media Controls
// Position settings once a keyboard nudge has moved it. Declared up here rather
// than with the rest of the move code because RepositionAndRepaintMediaControls
// -- which is defined well before that -- has to read it.
std::atomic<bool> g_mediaOverrideActive{false};
std::atomic<float> g_mediaOverrideH{50.0f};
std::atomic<float> g_mediaOverrideV{95.0f};

std::atomic<HWND> g_overlayWnd{nullptr};
ComPtr<IDXGISwapChain1> g_swapChain;
ComPtr<ID2D1DeviceContext> g_dc;
ComPtr<IDCompositionDevice> g_compositionDevice;
ComPtr<IDCompositionTarget> g_compositionTarget;
ComPtr<IDCompositionVisual> g_compositionVisual;
ComPtr<ID2D1SolidColorBrush> g_barBrush;
ComPtr<ID2D1SolidColorBrush> g_barBrush2;
ComPtr<ID2D1SolidColorBrush> g_backgroundBrush;
ComPtr<ID2D1SolidColorBrush> g_borderBrush;
ComPtr<ID2D1Bitmap> g_wallpaperBitmap;
ComPtr<ID2D1Effect> g_blurEffect;
// Pre-rendered result of the Gaussian blur. The blur is a full-image convolution
// and by far the most expensive Direct2D operation in the scene, but its input
// (the wallpaper) doesn't change frame to frame -- so it's evaluated once here
// and then simply blitted each frame instead of being recomputed.
ComPtr<ID2D1Bitmap1> g_blurredBitmap;

ComPtr<ID2D1PathGeometry> g_bgGeoCache;
ComPtr<ID2D1GeometryGroup> g_borderRingCache;
D2D1_RECT_F g_bgGeoCacheRect = {-1.f, -1.f, -1.f, -1.f};
float g_bgGeoCacheRadii[4] = {-1.f, -1.f, -1.f, -1.f};
int g_borderCacheBorderSize = -1;

ComPtr<ID2D1StrokeStyle> g_roundCapStrokeStyle;
ComPtr<IDWriteFactory> g_dwriteFactory;
ComPtr<IDWriteTextFormat> g_dwriteTextFormat;
ComPtr<ID2D1SolidColorBrush> g_nowPlayingBrush;
ComPtr<ID2D1SolidColorBrush> g_textPanelBrush;
int g_dwriteTextFormatFontSize = -1;
std::wstring g_dwriteTextFormatFontName;

static const IID kCLSID_D2D1GaussianBlur = {
    0x1feb6d69, 0x2fe6, 0x4ac9, {0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5}};

// ---- Hardware state ----------------------------------------------------------
// What the current D3D device actually landed on, which is not always what
// was asked for: Integrated on a desktop with no iGPU falls back, and a failed
// hardware device falls back to WARP. Smooth Mode's Auto keys off this rather
// than off the setting, so it follows the hardware that is really drawing.
struct VizRenderAdapterInfo {
    bool valid = false;
    bool warp = false;        // software rasterizer (CPU drawing)
    bool integrated = false;  // shares system memory with the CPU
    LUID luid = {};
    std::wstring name;
};
VizRenderAdapterInfo g_renderAdapter;

// Resolved Smooth Mode. Read by the render thread for pacing and by the UI
// thread for drawing, so it is an atomic rather than a settings field.
std::atomic<bool> g_smoothActive{false};

// Device-loss recovery is rate-limited so a driver that keeps failing can't
// put the UI thread into a rebuild loop.
std::atomic<bool> g_deviceRebuildQueued{false};
ULONGLONG g_lastDeviceRebuildTick = 0;

// ---- Smooth Mode resources ---------------------------------------------------
// All device-dependent, all released with the other visual resources.
//
// The background plate is the blurred wallpaper slice, the panel fill and the
// border ring rendered once into a bitmap. The 1.4 path re-does all three
// every frame, and the blur goes through a layer with a geometric mask, which
// on an integrated GPU means an offscreen allocation plus extra full-surface
// passes per frame for pixels that never change.
ComPtr<ID2D1Bitmap1> g_plateBitmap;
D2D1_RECT_F g_plateRect = {0.f, 0.f, 0.f, 0.f};

// Batched bars. One small atlas holds a pre-antialiased cap/body/cap strip of
// the bar shape; each frame becomes a single DrawSpriteBatch call, instead of
// one FillRoundedRectangle per bar (or, with per-corner radii, one brand new
// path geometry per bar, per frame).
ComPtr<ID2D1DeviceContext3> g_dc3;
ComPtr<ID2D1SpriteBatch> g_spriteBatch;
ComPtr<ID2D1Bitmap1> g_spriteAtlas;
struct VizAtlasKey {
    int kind = -1;  // 0 = bars, 1 = dots
    bool horizontal = true;
    float thick = 0.f, rTL = 0.f, rTR = 0.f, rBR = 0.f, rBL = 0.f, frac = 0.f;
    bool operator==(const VizAtlasKey& o) const {
        return kind == o.kind && horizontal == o.horizontal && thick == o.thick &&
               rTL == o.rTL && rTR == o.rTR && rBR == o.rBR && rBL == o.rBL && frac == o.frac;
    }
};
VizAtlasKey g_spriteAtlasKey;
// Atlas geometry, in atlas pixels. capA is the leading cap (top for vertical
// bars, left for horizontal ones), capB the trailing cap.
struct VizAtlasLayout {
    UINT pad = 1, capA = 0, mid = 3, capB = 0, across = 0;  // across = thickness incl. padding
    UINT along = 0;                                         // pad + capA + mid + capB + pad
} g_spriteAtlasLayout;
std::vector<D2D1_RECT_F> g_spriteDst;
std::vector<D2D1_RECT_U> g_spriteSrc;
std::vector<D2D1_COLOR_F> g_spriteCol;

// Text layouts, cached per overlay. DrawText builds a fresh layout (font
// fallback, shaping, line breaking) on every call; the strings here change a
// few times a minute at most, so Smooth Mode keeps the last one.
struct VizTextLayoutCache {
    std::wstring text;
    float w = -1.f, h = -1.f;
    ComPtr<IDWriteTextLayout> layout;
    void Reset() { text.clear(); w = h = -1.f; layout.Reset(); }
};
VizTextLayoutCache g_npLayoutCache, g_pfLayoutCache;

// Per-frame motion scale for Smooth Mode: 1.0 when the frame arrived exactly
// one Target-FPS interval after the last, larger when it was late. Written and
// read on the UI thread only.
float g_frameScale = 1.0f;

inline LONGLONG VizQpcFreq() {
    static LONGLONG s_freq = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f.QuadPart;
    }();
    return s_freq;
}

// Animation clock for Smooth Mode: seconds since the first call, from QPC.
//
// The 1.4 path drives Wave, Breathe and Rainbow from
// (float)GetTickCount64() * 0.001f. That has two problems, and both show up as
// stepping rather than motion. GetTickCount64 only moves every 15.6 ms, so at
// 144 FPS two frames in three repeat the previous animation phase. And a float
// holding a millisecond uptime runs out of mantissa: after a day of uptime the
// value can only change in 8 ms steps, after ten days in 64 ms steps, so the
// longer the PC has been on, the choppier those shapes get. This clock is QPC
// in a double, and callers reduce phase * time modulo one cycle while still in
// double (VizClockPhase) before narrowing, so it stays exact however long the
// mod has been running.
inline double VizClockSeconds() {
    static LONGLONG s_start = [] {
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        return q.QuadPart;
    }();
    LARGE_INTEGER q;
    QueryPerformanceCounter(&q);
    return (double)(q.QuadPart - s_start) / (double)VizQpcFreq();
}

// clock * rate, reduced modulo `cycle` in double precision.
inline float VizClockPhase(double clock, double rate, double cycle) {
    return (float)fmod(clock * rate, cycle);
}

// Converts a per-frame easing factor tuned at the Target FPS into the one
// that covers the time that actually passed. At g_frameScale == 1 this is the
// identity, so a frame that arrives on time moves exactly as in 1.4; a late
// frame catches up instead of visibly slowing the motion down.
inline float VizEaseForFrame(float perFrame) {
    if (g_frameScale == 1.0f) return perFrame;
    perFrame = std::clamp(perFrame, 0.0f, 1.0f);
    return 1.0f - powf(1.0f - perFrame, g_frameScale);
}

float g_dpiScale = 1.0f;
FILETIME g_lastWallpaperTime = {};
std::atomic<HMONITOR> g_cachedMonitor{nullptr};

std::atomic<bool> g_captureRunning{false};
std::thread* g_captureThread = nullptr;
HANDLE g_captureEvent = nullptr;
std::atomic<bool> g_deviceChanged{false};

// Per-band energy and oscilloscope waveform are each produced as a whole array
// by the capture thread and consumed as a whole array by the render thread.
// A plain std::atomic per element only guarantees each float is torn-free on
// its own; the render thread could still read some elements from one capture
// iteration and the rest from the next, visible as a tiny inconsistency in
// the drawn trace at high refresh rates. A seqlock makes the entire array
// publish/read atomic as a unit instead, with no lock contention.
std::atomic<uint32_t> g_bandsSeq{0};
float g_bandsData[VIZ_NUM_BANDS] = {};

void PublishBands(const float (&src)[VIZ_NUM_BANDS]) {
    uint32_t seq = g_bandsSeq.load(std::memory_order_relaxed);
    g_bandsSeq.store(seq + 1, std::memory_order_release);
    memcpy(g_bandsData, src, sizeof(g_bandsData));
    g_bandsSeq.store(seq + 2, std::memory_order_release);
}

void ReadBands(float (&dst)[VIZ_NUM_BANDS]) {
    for (;;) {
        uint32_t seq1 = g_bandsSeq.load(std::memory_order_acquire);
        if (seq1 & 1) continue;
        memcpy(dst, g_bandsData, sizeof(dst));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq1 == g_bandsSeq.load(std::memory_order_relaxed)) return;
    }
}

std::thread* g_renderThread = nullptr;
std::atomic<bool> g_renderThreadRunning{false};
std::atomic<bool> g_renderTickPending{false};

float g_hannWindow[VIZ_FFT_SIZE_MAX] = {};
float g_twiddleRe[VIZ_FFT_SIZE_MAX / 2] = {};
float g_twiddleIm[VIZ_FFT_SIZE_MAX / 2] = {};
int g_logBinStart[VIZ_NUM_BANDS + 1] = {};

float g_vizPeak[VIZ_BARS_MAX] = {};
float g_vizTarget[VIZ_BARS_MAX] = {};
float g_vizPeakHold[VIZ_BARS_MAX] = {};
float g_vizBreatheEnv = 0.f;

constexpr int VIZ_WAVE_SAMPLES = 256;

std::atomic<uint32_t> g_waveformSeq{0};
float g_waveformData[VIZ_WAVE_SAMPLES] = {};

void PublishWaveform(const float (&src)[VIZ_WAVE_SAMPLES]) {
    uint32_t seq = g_waveformSeq.load(std::memory_order_relaxed);
    g_waveformSeq.store(seq + 1, std::memory_order_release);
    memcpy(g_waveformData, src, sizeof(g_waveformData));
    g_waveformSeq.store(seq + 2, std::memory_order_release);
}

void ReadWaveform(float (&dst)[VIZ_WAVE_SAMPLES]) {
    for (;;) {
        uint32_t seq1 = g_waveformSeq.load(std::memory_order_acquire);
        if (seq1 & 1) continue;
        memcpy(dst, g_waveformData, sizeof(dst));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq1 == g_waveformSeq.load(std::memory_order_relaxed)) return;
    }
}

std::atomic<float> g_beatPulse{0.f};
std::atomic<float> g_dominantFreqHz{0.f};

// Screen-space bounds of the last drawn frame, used by the occlusion check so
// it can test the region we actually occupy rather than the whole monitor.
std::atomic<LONG> g_drawRectL{0}, g_drawRectT{0}, g_drawRectR{0}, g_drawRectB{0};
std::atomic<bool> g_drawRectValid{false};
// Set once the auto-hide fade has reached full transparency and a single blank
// frame has been presented. While set, the render path exits immediately.
bool g_autoHideBlanked = false;

// Current swap chain dimensions and composition-visual offset. The swap chain
// covers only the widget's bounding box rather than the whole desktop, so these
// are tracked to detect when a settings/display change requires a resize.
UINT  g_swapChainWidth = 0, g_swapChainHeight = 0;
float g_visualOffsetX = 0.f, g_visualOffsetY = 0.f;

std::mutex g_nowPlayingMutex;
std::wstring g_nowPlayingDisplay;
std::atomic<ULONGLONG> g_nowPlayingChangedTick{0};

static float VIZ_SEEDS[VIZ_BARS_MAX] = {};

void BuildVizSeeds() {
    unsigned int state = 0x12345678u;
    for (int i = 0; i < VIZ_BARS_MAX; i++) {
        state = state * 1664525u + 1013904223u;
        float t = (float)(state >> 8) / (float)(1u << 24);
        VIZ_SEEDS[i] = 0.25f + t * 1.20f;
    }
}

std::atomic<bool> g_fullscreenPaused{false};
std::atomic<ULONGLONG> g_lastAudibleTickMs{0};
bool g_slowMode = false;

static std::atomic<DWORD> g_albumArtColor{0xFFFFFFFF};
static std::atomic<DWORD> g_albumArtColorSecondary{0xFFAAAAAA};
static std::atomic<bool>  g_albumArtColorReady{false};
static std::atomic<bool>  g_albumArtFetchPending{false};
static std::atomic<DWORD> g_accentColorCache{0xFF0078D4};

static HANDLE g_gsmtcStopEvent = nullptr;
// std::optional rather than a bare std::thread: there is no assignment that
// empties a std::thread (move-assigning over a joinable one calls
// std::terminate), so the optional is what gives teardown a join() + reset()
// pair. This is the documented form for a global worker thread.
[[clang::no_destroy]] static std::optional<std::thread> g_gsmtcThread;
static std::thread* g_albumArtThread = nullptr;

HMODULE GetCurrentModuleHandle() {
    HMODULE module;
    if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           L"", &module)) {
        return nullptr;
    }
    return module;
}

float GetMonitorDpiScale(HMONITOR monitor) {
    UINT dpiX = 96, dpiY = 96;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        return dpiX / 96.0f;
    }
    return 1.0f;
}

HMONITOR GetMonitorById(int monitorId) {
    HMONITOR monitorResult = nullptr;
    int currentMonitorId = 0;

    auto monitorEnumProc = [&monitorResult, &currentMonitorId,
                            monitorId](HMONITOR hMonitor) -> BOOL {
        if (currentMonitorId == monitorId) {
            monitorResult = hMonitor;
            return FALSE;
        }
        currentMonitorId++;
        return TRUE;
    };

    EnumDisplayMonitors(
        nullptr, nullptr,
        // __stdcall is required: windhawk.exe is 32-bit on Windhawk 1.x, and
        // there a capture-less lambda otherwise converts to a __cdecl pointer
        // that will not bind to MONITORENUMPROC. On x86-64 there is only one
        // calling convention, so this is a no-op there.
        [](HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) __stdcall -> BOOL {
            auto& proc = *reinterpret_cast<decltype(monitorEnumProc)*>(dwData);
            return proc(hMonitor);
        },
        reinterpret_cast<LPARAM>(&monitorEnumProc));

    return monitorResult;
}

// ---- DXCore: adapter classification and NPU discovery -------------------------
//
// DXGI can enumerate GPUs but can't say which one is integrated, and can't see
// an NPU at all: NPUs are compute-only (MCDM) devices with no display engine,
// so they never show up as a DXGI adapter. DXCore sees both. It ships in the
// box from Windows 10 2004 on, but the toolchain's headers don't carry it, so
// it is loaded at runtime and the handful of methods used here are declared
// by hand, in vtable order, from the Windows SDK's dxcore_interface.h. Only
// the leading methods are declared, and nothing past the last one is called.
//
// None of this runs per frame: it runs when the device is created and when the
// NPU path explains why it can't start.
namespace ttdxcore {

enum : uint32_t {
    kPropInstanceLuid = 0,
    kPropDriverDescription = 2,
    kPropIsIntegrated = 12,
};

struct IAdapter : public IUnknown {
    virtual bool STDMETHODCALLTYPE IsValid() = 0;
    virtual bool STDMETHODCALLTYPE IsAttributeSupported(REFGUID attributeGUID) = 0;
    virtual bool STDMETHODCALLTYPE IsPropertySupported(uint32_t property) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProperty(uint32_t property, size_t bufferSize,
                                                  void* propertyData) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertySize(uint32_t property, size_t* bufferSize) = 0;
};

struct IAdapterList : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetAdapter(uint32_t index, REFIID riid, void** ppvAdapter) = 0;
    virtual uint32_t STDMETHODCALLTYPE GetAdapterCount() = 0;
};

struct IAdapterFactory : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateAdapterList(uint32_t numAttributes,
                                                        const GUID* filterAttributes,
                                                        REFIID riid, void** ppvAdapterList) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdapterByLuid(const LUID& adapterLUID, REFIID riid,
                                                       void** ppvAdapter) = 0;
};

static const GUID kIID_Factory = {
    0x78ee5945, 0xc36e, 0x4b13, {0xa6, 0x69, 0x00, 0x5d, 0xd1, 0x1c, 0x0f, 0x06}};
static const GUID kIID_List = {
    0x526c7776, 0x40e9, 0x459b, {0xb7, 0x11, 0xf3, 0x2a, 0xd7, 0x6d, 0xfc, 0x28}};
static const GUID kIID_Adapter = {
    0xf0db4c7f, 0xfe5a, 0x42a2, {0xbd, 0x62, 0xf2, 0xa6, 0xcf, 0x6f, 0xc8, 0x3e}};
static const GUID kAttrD3D11Graphics = {
    0x8c47866b, 0x7583, 0x450d, {0xf0, 0xf0, 0x6b, 0xad, 0xa8, 0x95, 0xaf, 0x4b}};
static const GUID kAttrD3D12Graphics = {
    0x0c9ece4d, 0x2f6e, 0x4f01, {0x8c, 0x96, 0xe8, 0x9e, 0x33, 0x1b, 0x47, 0xb1}};
static const GUID kAttrGenericML = {
    0xb71b0d41, 0x1088, 0x422f, {0xa2, 0x7c, 0x02, 0x50, 0xb7, 0xd3, 0xa9, 0x88}};
// Windows 11 24H2 and later. Older builds simply return an empty list for it.
static const GUID kAttrHardwareNpu = {
    0xd46140c4, 0xadd7, 0x451b, {0x9e, 0x56, 0x06, 0xfe, 0x8c, 0x3b, 0x58, 0xed}};

typedef HRESULT(WINAPI* CreateFactoryFn)(REFIID riid, void** ppvFactory);

inline ComPtr<IAdapterFactory> CreateFactory() {
    // Kept loaded for the life of the process: it's a system DLL, and
    // FreeLibrary on it buys nothing.
    static HMODULE s_module = LoadLibraryExW(L"dxcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    ComPtr<IAdapterFactory> factory;
    if (!s_module) return factory;
    auto create = (CreateFactoryFn)(void*)GetProcAddress(s_module, "DXCoreCreateAdapterFactory");
    if (!create) return factory;
    if (FAILED(create(kIID_Factory, (void**)factory.GetAddressOf()))) factory.Reset();
    return factory;
}

// 1 = integrated, 0 = discrete, -1 = DXCore can't say (pre-2004 Windows, or a
// LUID it doesn't know).
inline int IsIntegrated(const LUID& luid) {
    ComPtr<IAdapterFactory> factory = CreateFactory();
    if (!factory) return -1;
    ComPtr<IAdapter> adapter;
    if (FAILED(factory->GetAdapterByLuid(luid, kIID_Adapter, (void**)adapter.GetAddressOf())) ||
        !adapter) {
        return -1;
    }
    bool integrated = false;
    if (FAILED(adapter->GetProperty(kPropIsIntegrated, sizeof(integrated), &integrated))) return -1;
    return integrated ? 1 : 0;
}

inline std::wstring AdapterDescription(IAdapter* adapter) {
    size_t size = 0;
    if (FAILED(adapter->GetPropertySize(kPropDriverDescription, &size)) || size == 0 || size > 1024)
        return L"";
    std::string narrow(size, '\0');
    if (FAILED(adapter->GetProperty(kPropDriverDescription, size, narrow.data()))) return L"";
    narrow.resize(strnlen(narrow.c_str(), size));
    int wlen = MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), (int)narrow.size(), nullptr, 0);
    std::wstring wide(wlen > 0 ? wlen : 0, L'\0');
    if (wlen > 0) MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), (int)narrow.size(), wide.data(), wlen);
    return wide;
}

// Name of the first NPU Windows can see, e.g. "Intel(R) AI Boost", or empty.
// Used to tell "the NPU is there but the runtime isn't" apart from "Windows
// doesn't see an NPU at all", which need very different fixes.
inline std::wstring FindNpuName() {
    ComPtr<IAdapterFactory> factory = CreateFactory();
    if (!factory) return L"";

    auto firstMatching = [&](const GUID& attr, bool skipGraphics) -> std::wstring {
        ComPtr<IAdapterList> list;
        if (FAILED(factory->CreateAdapterList(1, &attr, kIID_List, (void**)list.GetAddressOf())) ||
            !list) {
            return L"";
        }
        uint32_t count = list->GetAdapterCount();
        for (uint32_t i = 0; i < count; i++) {
            ComPtr<IAdapter> adapter;
            if (FAILED(list->GetAdapter(i, kIID_Adapter, (void**)adapter.GetAddressOf())) || !adapter)
                continue;
            // Every GPU also advertises generic ML, so on builds that predate
            // the NPU attribute the only tell is "compute-only, no graphics".
            if (skipGraphics && (adapter->IsAttributeSupported(kAttrD3D12Graphics) ||
                                 adapter->IsAttributeSupported(kAttrD3D11Graphics))) {
                continue;
            }
            std::wstring name = AdapterDescription(adapter.Get());
            if (!name.empty()) return name;
        }
        return L"";
    };

    std::wstring name = firstMatching(kAttrHardwareNpu, false);
    if (name.empty()) name = firstMatching(kAttrGenericML, true);
    return name;
}

}  // namespace ttdxcore

bool ParseColorHex(PCWSTR colorStr, BYTE* a, BYTE* r, BYTE* g, BYTE* b) {
    if (!colorStr || !*colorStr) {
        return false;
    }
    while (*colorStr == L' ' || *colorStr == L'\t') colorStr++;

    if (_wcsnicmp(colorStr, L"rgba", 4) == 0 || _wcsnicmp(colorStr, L"rgb", 3) == 0) {
        const WCHAR* p = wcschr(colorStr, L'(');
        if (!p) return false;
        p++;
        float v[4] = {0.f, 0.f, 0.f, 1.0f};
        int n = swscanf_s(p, L"%f , %f , %f , %f", &v[0], &v[1], &v[2], &v[3]);
        if (n < 3) return false;
        *r = (BYTE)std::clamp((int)std::lround(v[0]), 0, 255);
        *g = (BYTE)std::clamp((int)std::lround(v[1]), 0, 255);
        *b = (BYTE)std::clamp((int)std::lround(v[2]), 0, 255);
        // Alpha is CSS-style 0-1, but 0-255 is tolerated too in case someone
        // carries over an integer alpha from elsewhere.
        float alpha = (n == 4) ? v[3] : 1.0f;
        if (alpha > 1.0f) alpha /= 255.0f;
        *a = (BYTE)std::clamp((int)std::lround(alpha * 255.0f), 0, 255);
        return true;
    }

    if (*colorStr == L'#') {
        colorStr++;
    }
    size_t len = wcslen(colorStr);
    unsigned int value = 0;
    for (size_t i = 0; i < len; i++) {
        WCHAR c = colorStr[i];
        int digit;
        if (c >= L'0' && c <= L'9') digit = c - L'0';
        else if (c >= L'A' && c <= L'F') digit = c - L'A' + 10;
        else if (c >= L'a' && c <= L'f') digit = c - L'a' + 10;
        else return false;
        value = (value << 4) | digit;
    }
    if (len == 6) {
        *a = 255;
        *r = (value >> 16) & 0xFF;
        *g = (value >> 8) & 0xFF;
        *b = value & 0xFF;
    } else if (len == 8) {
        *a = (value >> 24) & 0xFF;
        *r = (value >> 16) & 0xFF;
        *g = (value >> 8) & 0xFF;
        *b = value & 0xFF;
    } else {
        return false;
    }
    return true;
}

// ---- Settings validation ---------------------------------------------------
// Windhawk's settings UI has no notion of "this field is malformed" -- it takes
// whatever you type into a free-text box and saves it. Every free-text setting
// here (colors, positions, padding, corner radii, icon paths) used to fall back
// to a default on a parse failure without saying so, which means a typo looks
// exactly like a saved setting that simply doesn't do anything. These helpers
// collect every field that failed to parse during a LoadSettings() pass so the
// mod can hand the user one summary of what it couldn't read.

std::vector<std::wstring> g_settingsIssues;

void ReportSettingIssue(PCWSTR group, PCWSTR name, PCWSTR typed, PCWSTR expected,
                        PCWSTR usedInstead) {
    // Built by concatenation rather than into a fixed buffer -- an icon path can
    // run to MAX_PATH on its own and the surrounding text is longer still.
    PCWSTR shown = (typed && *typed) ? typed : L"(blank)";
    std::wstring line = std::wstring(group) + L" \x25B8 " + name +
                        L"\r\n      you typed:  " + shown +
                        L"\r\n      expected:   " + expected +
                        L"\r\n      using:      " + usedInstead;
    g_settingsIssues.push_back(std::move(line));
    Wh_Log(L"[Settings] INVALID %s/%s value=\"%s\" expected=%s using=%s",
           group, name, shown, expected, usedInstead);
}

// Same channel as ReportSettingIssue, for a setting that parsed perfectly well
// but is going to do something you probably didn't intend. There's no "wrong
// value" to quote here, so this is free-form.
void ReportSettingWarning(PCWSTR group, PCWSTR name, const std::wstring& detail) {
    std::wstring line = std::wstring(group) + L" \x25B8 " + name + L"\r\n      " + detail;
    g_settingsIssues.push_back(std::move(line));
    Wh_Log(L"[Settings] WARNING %s/%s -- %s", group, name, line.c_str());
}

// Reads a free-text setting that must be a single number in [lo, hi]. Rejects
// trailing junk ("50px", "50 50") rather than silently taking the leading
// number, since that's exactly the kind of near-miss that looks like it worked.
float ReadNumberSetting(PCWSTR key, PCWSTR group, PCWSTR name, float def, float lo, float hi) {
    PCWSTR str = Wh_GetStringSetting(key);
    float result = def;

    WCHAR* end = nullptr;
    double parsed = (str && *str) ? wcstod(str, &end) : 0.0;
    bool parsedAnything = (str && end && end != str);
    if (parsedAnything) {
        while (*end == L' ' || *end == L'\t') end++;
    }

    if (!parsedAnything || *end) {
        WCHAR expected[128], using_[64];
        swprintf_s(expected, L"a single number from %g to %g (decimals allowed)", lo, hi);
        swprintf_s(using_, L"%g", def);
        ReportSettingIssue(group, name, str, expected, using_);
    } else if (parsed < lo || parsed > hi) {
        WCHAR expected[128], using_[64];
        swprintf_s(expected, L"a number from %g to %g", lo, hi);
        result = std::clamp((float)parsed, lo, hi);
        swprintf_s(using_, L"%g (clamped into range)", result);
        ReportSettingIssue(group, name, str, expected, using_);
    } else {
        result = (float)parsed;
    }

    Wh_FreeStringSetting(str);
    return result;
}

// Reads a color setting, reporting the exact accepted formats on failure.
void ReadColorSetting(PCWSTR key, PCWSTR group, PCWSTR name, BYTE defA, BYTE defR, BYTE defG,
                      BYTE defB, BYTE* a, BYTE* r, BYTE* g, BYTE* b) {
    PCWSTR str = Wh_GetStringSetting(key);
    if (!ParseColorHex(str, a, r, g, b)) {
        *a = defA; *r = defR; *g = defG; *b = defB;
        WCHAR using_[64];
        swprintf_s(using_, L"#%02X%02X%02X%02X", defA, defR, defG, defB);
        ReportSettingIssue(group, name, str,
                           L"#AARRGGBB, #RRGGBB, rgba(r, g, b, a) or rgb(r, g, b)", using_);
    }
    Wh_FreeStringSetting(str);
}

// Reads a "one value, or four space-separated values" box (padding, corner
// radii). Anything other than exactly 1 or 4 numbers is a mistake worth saying
// out loud -- two values in particular reads as if it might mean
// horizontal/vertical, and it doesn't.
void ReadQuadSetting(PCWSTR key, PCWSTR group, PCWSTR name, PCWSTR meaning, float def,
                     bool allowNegative, float out[4]) {
    PCWSTR str = Wh_GetStringSetting(key);
    out[0] = out[1] = out[2] = out[3] = def;

    float v[4] = {def, def, def, def};
    int n = (str && *str) ? swscanf_s(str, L"%f %f %f %f", &v[0], &v[1], &v[2], &v[3]) : 0;

    if (n == 1 || n == 4) {
        if (n == 1) v[1] = v[2] = v[3] = v[0];
        for (int k = 0; k < 4; k++) out[k] = allowNegative ? v[k] : std::max(0.0f, v[k]);
    } else {
        WCHAR expected[256], using_[64];
        swprintf_s(expected, L"one number for all sides, or four space-separated numbers for %s",
                   meaning);
        swprintf_s(using_, L"%g on all sides", def);
        ReportSettingIssue(group, name, str, expected, using_);
    }

    Wh_FreeStringSetting(str);
}

// Reads one of the "how much has to be covered" dropdowns. Stored as a string
// rather than an int because Windhawk only accepts $options on string settings
// -- same as the FFT Size dropdown. Picking from a list means it can't normally
// be wrong, but a hand-edited settings file still can be.
int ReadThresholdPercentSetting(PCWSTR key, PCWSTR group, PCWSTR name, int def) {
    PCWSTR str = Wh_GetStringSetting(key);
    int v = (str && *str) ? _wtoi(str) : 0;

    if (v < 5 || v > 100) {
        WCHAR using_[32];
        swprintf_s(using_, L"%d%%", def);
        ReportSettingIssue(group, name, str, L"a percentage from 5 to 100, in steps of 5",
                           using_);
        v = def;
    }

    Wh_FreeStringSetting(str);
    return v;
}

// Reads an optional path to an image file. Blank is valid and means "use the
// built-in glyph" -- but a path that's set and wrong is silent breakage, since
// the fallback looks identical to having left it blank.
std::wstring ReadIconPathSetting(PCWSTR key, PCWSTR group, PCWSTR name) {
    PCWSTR str = Wh_GetStringSetting(key);
    std::wstring path = str ? str : L"";

    // Users paste paths out of Explorer's address bar or "Copy as path", both of
    // which can bring quotes along; strip them rather than calling it invalid.
    while (!path.empty() && (path.front() == L'"' || path.front() == L' ')) path.erase(path.begin());
    while (!path.empty() && (path.back() == L'"' || path.back() == L' ')) path.pop_back();

    if (!path.empty()) {
        DWORD attr = GetFileAttributes(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            ReportSettingIssue(group, name, path.c_str(),
                               L"a full path to an existing image file (PNG, JPG, BMP or ICO)",
                               L"the built-in icon");
            path.clear();
        } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            ReportSettingIssue(group, name, path.c_str(),
                               L"a path to an image file, not to a folder",
                               L"the built-in icon");
            path.clear();
        }
    }

    Wh_FreeStringSetting(str);
    return path;
}

// A font that really is installed can still enumerate as missing right after a
// cold boot: mods start before the font service has finished registering
// everything, so the check runs against an incomplete list and warns about a
// font that works perfectly the moment anything draws with it.
//
// So a failed check does not report straight away. It arms a one-second retry
// on the message window and only reports if the font is still missing once the
// system has had time to settle. A genuinely wrong name still gets flagged,
// just ten seconds later than it used to.
static bool g_fontCheckPending = false;
static int g_fontCheckAttempts = 0;
static constexpr int FONT_CHECK_MAX_ATTEMPTS = 10;

// Checks a font family name is actually installed. A missing font falls back to
// whatever DirectWrite substitutes, which is silently not what was asked for.
bool IsFontInstalled(const std::wstring& family) {
    if (family.empty()) return false;
    HDC hdc = GetDC(nullptr);
    if (!hdc) return true;  // can't tell -- don't cry wolf

    LOGFONT lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyn(lf.lfFaceName, family.c_str(), LF_FACESIZE);

    bool found = false;
    EnumFontFamiliesEx(
        hdc, &lf,
        [](const LOGFONT*, const TEXTMETRIC*, DWORD, LPARAM param) __stdcall -> int {
            *reinterpret_cast<bool*>(param) = true;
            return 0;
        },
        reinterpret_cast<LPARAM>(&found), 0);

    ReleaseDC(nullptr, hdc);
    return found;
}

PCWSTR ModKeyFlagsName(unsigned flags) {
    switch (flags) {
        case VIZ_MOD_CTRL:  return L"Ctrl";
        case VIZ_MOD_ALT:   return L"Alt";
        case VIZ_MOD_SHIFT: return L"Shift";
        case VIZ_MOD_WIN:   return L"Win";
        case VIZ_MOD_CTRL | VIZ_MOD_ALT:   return L"Ctrl + Alt";
        case VIZ_MOD_CTRL | VIZ_MOD_SHIFT: return L"Ctrl + Shift";
        case VIZ_MOD_ALT  | VIZ_MOD_SHIFT: return L"Alt + Shift";
        case VIZ_MOD_WIN  | VIZ_MOD_ALT:   return L"Win + Alt";
        case VIZ_MOD_WIN  | VIZ_MOD_SHIFT: return L"Win + Shift";
    }
    return L"None";
}

// Shown on its own thread: this runs from LoadSettings, which is called on the
// UI thread out of the settings-changed message, and a modal box there would
// wedge rendering until the user clicked OK.
DWORD WINAPI SettingsIssueDialogThread(LPVOID param) {
    std::wstring* text = (std::wstring*)param;
    MessageBox(nullptr, text->c_str(), L"Tourne'Table - Settings Problems",
               MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    delete text;
    return 0;
}

void FlushSettingsIssues() {
    if (g_settingsIssues.empty()) return;

    std::wstring body =
        L"Tourne'Table found something worth flagging in your settings.\r\n\r\n"
        L"Anything listed as \"you typed / expected\" couldn't be read at all, so a fallback "
        L"value is being used -- the setting is saved, it just isn't doing anything. Anything "
        L"else is a heads-up about a setting that works exactly as configured but may not do "
        L"what you expect.\r\n\r\n";
    for (const auto& issue : g_settingsIssues) {
        body += L"  \x2022  " + issue + L"\r\n\r\n";
    }
    body += L"(Turn this off under Settings Validation \x25B8 Warn About Invalid Settings.)";

    g_settingsIssues.clear();

    if (!g_settings.showSettingsErrors) return;

    HANDLE h = CreateThread(nullptr, 0, SettingsIssueDialogThread,
                            new std::wstring(std::move(body)), 0, nullptr);
    if (h) CloseHandle(h);
}

HWND GetWorkerW() {
    HWND hProgman = FindWindow(L"Progman", nullptr);
    if (!hProgman) return nullptr;

    // Progman belongs to explorer.exe, not to us -- this mod now runs as its
    // own process (see the tool-mod boilerplate at the end of this file), so
    // there's no same-process check to make here. SendMessage/FindWindowEx
    // work fine across process boundaries; this is the same technique
    // standalone desktop-overlay tools use to reach behind the icons without
    // ever injecting into explorer.
    SendMessage(hProgman, 0x052C, 0xD, 0);
    SendMessage(hProgman, 0x052C, 0xD, 1);

    HWND hWorkerW = nullptr;
    EnumWindows(
        [](HWND hWnd, LPARAM lParam) __stdcall -> BOOL {
            if (!FindWindowEx(hWnd, nullptr, L"SHELLDLL_DefView", nullptr)) {
                return TRUE;
            }
            HWND hWorker = FindWindowEx(nullptr, hWnd, L"WorkerW", nullptr);
            if (hWorker) {
                *(HWND*)lParam = hWorker;
                return FALSE;
            }
            return TRUE;
        },
        (LPARAM)&hWorkerW);

    if (!hWorkerW) {
        SendMessage(hProgman, 0x052C, 0, 0);
        EnumWindows(
            [](HWND hWnd, LPARAM lParam) __stdcall -> BOOL {
                if (!FindWindowEx(hWnd, nullptr, L"SHELLDLL_DefView", nullptr)) {
                    return TRUE;
                }
                HWND hWorker = FindWindowEx(nullptr, hWnd, L"WorkerW", nullptr);
                if (hWorker) {
                    *(HWND*)lParam = hWorker;
                    return FALSE;
                }
                return TRUE;
            },
            (LPARAM)&hWorkerW);
    }

    if (!hWorkerW) {
        hWorkerW = FindWindowEx(hProgman, nullptr, L"WorkerW", nullptr);
    }
    if (!hWorkerW) {
        hWorkerW = hProgman;
    }

    return hWorkerW;
}

bool IsWindowFullscreen(HWND hwnd, HMONITOR targetMonitor = nullptr) {
    if (!hwnd || !IsWindowVisible(hwnd)) return false;

    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) return false;

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (targetMonitor && monitor != targetMonitor) return false;

    MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
    if (!GetMonitorInfo(monitor, &monitorInfo)) return false;

    return windowRect.left <= monitorInfo.rcMonitor.left &&
           windowRect.top <= monitorInfo.rcMonitor.top &&
           windowRect.right >= monitorInfo.rcMonitor.right &&
           windowRect.bottom >= monitorInfo.rcMonitor.bottom;
}

bool IsFullscreenOrGameActive() {
    HMONITOR targetMonitor = GetMonitorById(g_settings.monitor - 1);
    if (!targetMonitor) targetMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);

    HWND hwndForeground = GetForegroundWindow();
    if (!hwndForeground) return false;

    WCHAR className[256];
    if (GetClassName(hwndForeground, className, ARRAYSIZE(className))) {
        if (_wcsicmp(className, L"Progman") == 0 ||
            _wcsicmp(className, L"WorkerW") == 0 ||
            _wcsicmp(className, L"Shell_TrayWnd") == 0) {
            return false;
        }
    }

    HMONITOR foregroundMonitor = MonitorFromWindow(hwndForeground, MONITOR_DEFAULTTONEAREST);
    if (foregroundMonitor != targetMonitor) return false;

    QUERY_USER_NOTIFICATION_STATE state;
    if (SUCCEEDED(SHQueryUserNotificationState(&state))) {
        if (state == QUNS_RUNNING_D3D_FULL_SCREEN || state == QUNS_PRESENTATION_MODE)
            return true;
    }

    if (IsWindowFullscreen(hwndForeground, targetMonitor))
        return true;

    return false;
}

// Returns true when the region the visualizer occupies is fully covered by some
// ordinary window. The widget lives on the desktop (behind icons), so any
// maximized app hides it completely -- yet without this check we'd keep running
// the full render path at the target frame rate drawing pixels nobody can see.
// Deliberately conservative: only *full* containment counts, so partial overlap
// never causes the visualizer to vanish while it's still partly visible.
// Returns how much of `target` (in virtual-screen coordinates) is underneath
// real application windows, as a percentage of its area.
//
// Coverage is accumulated into a GDI region rather than tested per window, so
// several windows that each hide part of the box add up correctly instead of
// each being judged on its own -- two windows covering opposite halves read as
// 100%, which is what you see on screen. GetRegionData hands back a set of
// non-overlapping rectangles, so summing their areas double-counts nothing.
int ComputeRectCoveragePercent(const RECT& targetIn) {
    if (targetIn.right <= targetIn.left || targetIn.bottom <= targetIn.top) return 0;

    // Judge only the part that is actually on a screen.
    //
    // Area hanging off an edge can never intersect a window rect, so leaving it
    // in the denominator permanently caps the result below 100%. At the default
    // "100% covered" threshold that makes Pause When Covered unreachable for any
    // visualizer positioned partly off-screen, which is a common placement along
    // the bottom edge, and the feature simply appears to do nothing.
    RECT screen;
    screen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    screen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    screen.right = screen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    screen.bottom = screen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    RECT target;
    if (!IntersectRect(&target, &targetIn, &screen)) {
        // Nothing of it is on screen at all. Reported as uncovered rather than
        // fully covered, staying with this function's existing bias: a wrong
        // answer that keeps drawing is a far smaller problem than one that makes
        // the visualizer vanish.
        return 0;
    }

    HRGN coveredRgn = CreateRectRgn(0, 0, 0, 0);
    if (!coveredRgn) return 0;

    struct EnumCtx {
        RECT viz;
        HRGN covered;
    } ctx{target, coveredRgn};

    EnumWindows(
        [](HWND hWnd, LPARAM lParam) __stdcall -> BOOL {
            auto* c = reinterpret_cast<EnumCtx*>(lParam);

            if (!IsWindowVisible(hWnd) || IsIconic(hWnd)) return TRUE;

            // Only genuine application windows count as covering the desktop.
            // This is the standard "would it show up in Alt-Tab" test, and it is
            // what makes this check reliable on Windows 11: the shell is full of
            // XAML islands, composition bridges and flyout hosts (Quick Settings,
            // media popups, the taskbar's own sub-windows) that report themselves
            // as visible, often span large areas, and hide nothing at all.
            if (hWnd != GetAncestor(hWnd, GA_ROOTOWNER)) return TRUE;

            LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
            if (exStyle & WS_EX_TOOLWINDOW) return TRUE;
            if (exStyle & WS_EX_TRANSPARENT) return TRUE;
            if (exStyle & WS_EX_NOACTIVATE) return TRUE;

            // A real app window has a caption or is a normal overlapped window.
            LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
            if (style & WS_CHILD) return TRUE;

            WCHAR cls[64] = {};
            if (GetClassName(hWnd, cls, ARRAYSIZE(cls))) {
                if (_wcsicmp(cls, L"Progman") == 0 || _wcsicmp(cls, L"WorkerW") == 0 ||
                    _wcsicmp(cls, L"Shell_TrayWnd") == 0 ||
                    _wcsicmp(cls, L"Shell_SecondaryTrayWnd") == 0 ||
                    _wcsicmp(cls, L"Windows.UI.Core.CoreWindow") == 0 ||
                    _wcsicmp(cls, L"Windows.UI.Composition.DesktopWindowContentBridge") == 0 ||
                    _wcsicmp(cls, L"XamlExplorerHostIslandWindow") == 0 ||
                    _wcsicmp(cls, L"TopLevelWindowForOverflowXamlIsland") == 0 ||
                    _wcsicmp(cls, L"ForegroundStaging") == 0 ||
                    _wcsicmp(cls, L"MultitaskingViewFrame") == 0) {
                    return TRUE;
                }
            }

            // An untitled window is almost always infrastructure rather than a
            // real application the user is looking at.
            if (GetWindowTextLength(hWnd) == 0) return TRUE;

            // Skip DWM-cloaked windows. This is the important one: on Windows
            // 10/11 a great many UWP/XAML windows report IsWindowVisible() ==
            // TRUE while being entirely invisible (cloaked), and several of them
            // are sized to the full screen. Without this check they register as
            // covering the visualizer and hide it while nothing is really there.
            int cloaked = 0;
            if (SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked,
                                                sizeof(cloaked))) && cloaked) {
                return TRUE;
            }

            RECT wr;
            if (!GetWindowRect(hWnd, &wr)) return TRUE;

            // Ignore degenerate/zero-area windows.
            if (wr.right <= wr.left || wr.bottom <= wr.top) return TRUE;

            RECT hit;
            if (!IntersectRect(&hit, &wr, &c->viz)) return TRUE;

            HRGN piece = CreateRectRgn(hit.left, hit.top, hit.right, hit.bottom);
            if (piece) {
                CombineRgn(c->covered, c->covered, piece, RGN_OR);
                DeleteObject(piece);
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&ctx));

    long long coveredArea = 0;
    DWORD dataSize = GetRegionData(coveredRgn, 0, nullptr);
    if (dataSize) {
        std::vector<BYTE> buf(dataSize);
        RGNDATA* rd = reinterpret_cast<RGNDATA*>(buf.data());
        if (GetRegionData(coveredRgn, dataSize, rd) == dataSize) {
            const RECT* rects = reinterpret_cast<const RECT*>(rd->Buffer);
            for (DWORD i = 0; i < rd->rdh.nCount; i++) {
                coveredArea += (long long)(rects[i].right - rects[i].left) *
                               (long long)(rects[i].bottom - rects[i].top);
            }
        }
    }
    DeleteObject(coveredRgn);

    long long totalArea = (long long)(target.right - target.left) *
                          (long long)(target.bottom - target.top);
    if (totalArea <= 0) return 0;
    return (int)std::min<long long>(100, (coveredArea * 100) / totalArea);
}

bool IsVisualizerOccluded() {
    if (!g_drawRectValid.load(std::memory_order_relaxed)) return false;

    RECT viz;
    viz.left   = g_drawRectL.load(std::memory_order_relaxed);
    viz.top    = g_drawRectT.load(std::memory_order_relaxed);
    viz.right  = g_drawRectR.load(std::memory_order_relaxed);
    viz.bottom = g_drawRectB.load(std::memory_order_relaxed);

    int threshold = std::clamp(g_settings.obscuredThresholdPercent, 5, 100);
    int covered = ComputeRectCoveragePercent(viz);
    bool occluded = covered >= threshold;

    // Logged on the transition only -- this runs once a second, and a line per
    // second for as long as a window happens to be open would bury everything
    // else in the log.
    static bool s_lastOccluded = false;
    if (occluded != s_lastOccluded) {
        Wh_Log(L"[Viz] %s: %d%% covered (threshold %d%%)",
               occluded ? L"occluded" : L"visible again", covered, threshold);
        s_lastOccluded = occluded;
    }
    return occluded;
}

// Same measurement, aimed at the media strip's own rect. The strip is topmost,
// so nothing ever covers it on screen -- "covered" here means a real window is
// sitting underneath it, which is when it's in the way rather than useful.
bool IsMediaStripCovered() {
    if (!g_mediaWnd) return false;

    RECT strip;
    if (!GetWindowRect(g_mediaWnd, &strip)) return false;

    int threshold = std::clamp(g_settings.mediaCoveredThresholdPercent, 5, 100);
    return ComputeRectCoveragePercent(strip) >= threshold;
}

void RefreshAccentColorCache() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD color = 0, size = sizeof(color), type = 0;
        if (RegQueryValueExW(hKey, L"AccentColorMenu", nullptr, &type,
                             (LPBYTE)&color, &size) == ERROR_SUCCESS && type == REG_DWORD) {
            RegCloseKey(hKey);
            BYTE r = (color >>  0) & 0xFF;
            BYTE g = (color >>  8) & 0xFF;
            BYTE b = (color >> 16) & 0xFF;
            g_accentColorCache.store(0xFF000000 | ((DWORD)r << 16) | ((DWORD)g << 8) | b,
                                     std::memory_order_relaxed);
            return;
        }
        if (RegQueryValueExW(hKey, L"AccentColor", nullptr, &type,
                             (LPBYTE)&color, &size) == ERROR_SUCCESS && type == REG_DWORD) {
            RegCloseKey(hKey);
            BYTE r = (color >>  0) & 0xFF;
            BYTE g = (color >>  8) & 0xFF;
            BYTE b = (color >> 16) & 0xFF;
            g_accentColorCache.store(0xFF000000 | ((DWORD)r << 16) | ((DWORD)g << 8) | b,
                                     std::memory_order_relaxed);
            return;
        }
        RegCloseKey(hKey);
    }

    DWORD color = 0; BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque)))
        g_accentColorCache.store(0xFF000000 | (color & 0x00FFFFFF), std::memory_order_relaxed);
}

DWORD GetWindowsAccentColor() {
    return g_accentColorCache.load(std::memory_order_relaxed);
}

void FetchAlbumArtColorAsync() {
    bool expected = false;
    if (!g_albumArtFetchPending.compare_exchange_strong(expected, true))
        return;

    if (g_albumArtThread) {
        if (g_albumArtThread->joinable())
            g_albumArtThread->join();
        delete g_albumArtThread;
        g_albumArtThread = nullptr;
    }

    g_albumArtThread = new std::thread([]() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            auto mgr = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            if (!mgr) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }
            auto session = mgr.GetCurrentSession();
            if (!session) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            auto props = session.TryGetMediaPropertiesAsync().get();
            if (!props) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            {
                std::wstring title(props.Title());
                std::wstring artist(props.Artist());
                std::wstring display = artist.empty() ? title
                                      : title.empty()  ? artist
                                                        : (artist + L" - " + title);
                if (!display.empty()) {
                    std::lock_guard<std::mutex> lock(g_nowPlayingMutex);
                    if (g_nowPlayingDisplay != display) {
                        g_nowPlayingDisplay = display;
                        g_nowPlayingChangedTick.store(GetTickCount64(), std::memory_order_relaxed);
                    }
                }
            }

            auto thumbRef = props.Thumbnail();
            if (!thumbRef) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            auto stream = thumbRef.OpenReadAsync().get();
            if (!stream) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            UINT64 sz = stream.Size();
            if (sz == 0 || sz > 4 * 1024 * 1024) { winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            DataReader reader(stream);
            reader.LoadAsync((UINT32)sz).get();
            std::vector<BYTE> thumbBytes((size_t)sz);
            reader.ReadBytes(winrt::array_view<BYTE>(thumbBytes));
            reader.DetachStream();

            IWICImagingFactory* pFactory = nullptr;
            if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&pFactory))) || !pFactory) {
                winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return;
            }
            IStream* pStream = SHCreateMemStream(thumbBytes.data(), (UINT)thumbBytes.size());
            if (!pStream) { pFactory->Release(); winrt::uninit_apartment(); g_albumArtFetchPending.store(false); return; }

            IWICBitmapDecoder* pDecoder = nullptr;
            IWICBitmapFrameDecode* pFrame = nullptr;
            IWICFormatConverter* pConv = nullptr;
            std::vector<BYTE> pixels;
            int imgW = 0, imgH = 0;

            if (SUCCEEDED(pFactory->CreateDecoderFromStream(pStream, nullptr,
                    WICDecodeMetadataCacheOnDemand, &pDecoder)) &&
                SUCCEEDED(pDecoder->GetFrame(0, &pFrame)) &&
                SUCCEEDED(pFactory->CreateFormatConverter(&pConv))) {
                if (SUCCEEDED(pConv->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA,
                        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut))) {
                    UINT w = 0, h = 0;
                    pConv->GetSize(&w, &h);
                    if (w > 0 && h > 0) {
                        pixels.resize((size_t)w * h * 4);
                        if (SUCCEEDED(pConv->CopyPixels(nullptr, w * 4,
                                (UINT)pixels.size(), pixels.data()))) {
                            imgW = (int)w; imgH = (int)h;
                        }
                    }
                }
            }
            if (pConv)    pConv->Release();
            if (pFrame)   pFrame->Release();
            if (pDecoder) pDecoder->Release();
            pStream->Release();
            pFactory->Release();

            if (!pixels.empty()) {
                struct Bucket { uint32_t r=0,g=0,b=0,n=0; };
                Bucket buckets[16][16][16]{};
                for (int y = 0; y < imgH; y += 4) {
                    for (int x = 0; x < imgW; x += 4) {
                        size_t idx = ((size_t)y * imgW + x) * 4;
                        if (idx + 4 > pixels.size()) continue;
                        BYTE pb = pixels[idx], pg = pixels[idx+1], pr = pixels[idx+2];
                        int luma = (pr * 299 + pg * 587 + pb * 114) / 1000;
                        if (luma < 24 || luma > 235) continue;
                        auto& bk = buckets[pr >> 4][pg >> 4][pb >> 4];
                        bk.r += pr; bk.g += pg; bk.b += pb; bk.n++;
                    }
                }

                struct Cand { float w; BYTE r,g,b; };
                std::vector<Cand> cands;
                cands.reserve(64);
                for (int R = 0; R < 16; R++) for (int G = 0; G < 16; G++) for (int B = 0; B < 16; B++) {
                    auto& bk = buckets[R][G][B];
                    if (bk.n < 8) continue;
                    float fr = bk.r/(float)bk.n/255.f, fg = bk.g/(float)bk.n/255.f, fb = bk.b/(float)bk.n/255.f;
                    float mx = std::max({fr,fg,fb}), mn = std::min({fr,fg,fb});
                    float sat = mx > 0 ? (mx - mn) / mx : 0;
                    cands.push_back({ bk.n * (0.3f + sat),
                                     (BYTE)(fr*255), (BYTE)(fg*255), (BYTE)(fb*255) });
                }

                if (!cands.empty()) {
                    std::sort(cands.begin(), cands.end(),
                              [](const Cand& a, const Cand& b){ return a.w > b.w; });

                    BYTE pR = cands[0].r, pG = cands[0].g, pB = cands[0].b;

                    BYTE sR = pR, sG = pG, sB = pB;
                    for (auto& c : cands) {
                        int dr = (int)c.r - (int)pR;
                        int dg = (int)c.g - (int)pG;
                        int db = (int)c.b - (int)pB;
                        if (dr*dr + dg*dg + db*db > 3264) {
                            sR = c.r; sG = c.g; sB = c.b;
                            break;
                        }
                    }

                    DWORD col  = 0xFF000000 | ((DWORD)pR << 16) | ((DWORD)pG << 8) | pB;
                    DWORD col2 = 0xFF000000 | ((DWORD)sR << 16) | ((DWORD)sG << 8) | sB;
                    g_albumArtColor.store(col,  std::memory_order_relaxed);
                    g_albumArtColorSecondary.store(col2, std::memory_order_relaxed);
                    g_albumArtColorReady.store(true, std::memory_order_relaxed);
                }
            }
        } catch (...) {}
        try { winrt::uninit_apartment(); } catch (...) {}
        g_albumArtFetchPending.store(false);
    });
}

static winrt::event_token g_gsmtcMediaPropsToken{};
static winrt::event_token g_gsmtcPlaybackToken{};
static winrt::event_token g_gsmtcSessionToken{};
[[clang::no_destroy]] static GlobalSystemMediaTransportControlsSessionManager g_gsmtcMgr{ nullptr };
[[clang::no_destroy]] static GlobalSystemMediaTransportControlsSession        g_gsmtcSession{ nullptr };

void RefreshMediaPlaybackStatus() {
    if (!g_gsmtcSession) return;
    try {
        auto info = g_gsmtcSession.GetPlaybackInfo();
        bool playing = info && info.PlaybackStatus() ==
            GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        g_mediaIsPlaying.store(playing, std::memory_order_relaxed);
    } catch (...) {}
    if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
}

void SetupGsmtcSessionListener() {
    if (!g_gsmtcMgr) return;
    try {
        if (g_gsmtcSession) {
            try { g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken); } catch (...) {}
            try { g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken); } catch (...) {}
            g_gsmtcSession = nullptr;
        }
        g_gsmtcSession = g_gsmtcMgr.GetCurrentSession();
        if (!g_gsmtcSession) return;
        g_gsmtcMediaPropsToken = g_gsmtcSession.MediaPropertiesChanged(
            [](auto const&, auto const&) {
                if (g_settings.colorMode == VizColorMode::AlbumArt ||
                    g_settings.colorMode == VizColorMode::DynamicAlbum ||
                    g_settings.nowPlayingEnabled)
                    FetchAlbumArtColorAsync();
            });
        g_gsmtcPlaybackToken = g_gsmtcSession.PlaybackInfoChanged(
            [](auto const&, auto const&) { RefreshMediaPlaybackStatus(); });
        RefreshMediaPlaybackStatus();
    } catch (...) {}
}

void InitGsmtcListener() {
    if (g_gsmtcStopEvent) return;
    g_gsmtcStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_gsmtcStopEvent) return;

    g_gsmtcThread.emplace([]() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            g_gsmtcMgr = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            if (!g_gsmtcMgr) { winrt::uninit_apartment(); return; }

            g_gsmtcSessionToken = g_gsmtcMgr.CurrentSessionChanged(
                [](auto const&, auto const&) {
                    SetupGsmtcSessionListener();
                    if (g_settings.colorMode == VizColorMode::AlbumArt ||
                        g_settings.colorMode == VizColorMode::DynamicAlbum ||
                        g_settings.nowPlayingEnabled)
                        FetchAlbumArtColorAsync();
                });

            SetupGsmtcSessionListener();
            if (g_settings.colorMode == VizColorMode::AlbumArt ||
                g_settings.colorMode == VizColorMode::DynamicAlbum ||
                g_settings.nowPlayingEnabled)
                FetchAlbumArtColorAsync();
        } catch (...) {}

        if (g_gsmtcStopEvent)
            WaitForSingleObject(g_gsmtcStopEvent, INFINITE);

        try {
            if (g_gsmtcSession) {
                g_gsmtcSession.MediaPropertiesChanged(g_gsmtcMediaPropsToken);
                g_gsmtcSession.PlaybackInfoChanged(g_gsmtcPlaybackToken);
            }
            if (g_gsmtcMgr) {
                g_gsmtcMgr.CurrentSessionChanged(g_gsmtcSessionToken);
            }
            g_gsmtcSession = nullptr;
            g_gsmtcMgr     = nullptr;
            winrt::uninit_apartment();
        } catch (...) {}
    });
}

static bool g_gsmtcStarted = false;

// ---- Media control buttons -------------------------------------------------
// A small always-on-top, clickable strip (Previous / Play-Pause / Next), wired
// to whichever app is currently playing media via the same GSMTC session used
// for Now Playing. This lives in its own layered window rather than the main
// overlay's swap chain: the overlay is WS_EX_TRANSPARENT on purpose (it sits
// behind the desktop icons and must never eat clicks), so a window that needs
// to actually receive clicks has to be a separate, non-transparent surface.

std::thread* g_mediaCmdThread = nullptr;
std::atomic<bool> g_mediaCmdPending{false};
std::atomic<int> g_mediaCmdQueued{-1};

void SendMediaCommand(int cmd) {
    g_mediaCmdQueued.store(cmd, std::memory_order_relaxed);

    bool expected = false;
    if (!g_mediaCmdPending.compare_exchange_strong(expected, true))
        return;

    if (g_mediaCmdThread) {
        if (g_mediaCmdThread->joinable())
            g_mediaCmdThread->join();
        delete g_mediaCmdThread;
        g_mediaCmdThread = nullptr;
    }

    g_mediaCmdThread = new std::thread([]() {
        int cmd = g_mediaCmdQueued.load(std::memory_order_relaxed);
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            auto mgr = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            if (mgr) {
                auto session = mgr.GetCurrentSession();
                if (session) {
                    if (cmd == 0)      session.TrySkipPreviousAsync().get();
                    else if (cmd == 1) session.TryTogglePlayPauseAsync().get();
                    else if (cmd == 2) session.TrySkipNextAsync().get();
                }
            }
        } catch (...) {}
        try { winrt::uninit_apartment(); } catch (...) {}
        g_mediaCmdPending.store(false, std::memory_order_relaxed);
    });
}

// icon slot order: 0=prev, 1=play, 2=pause, 3=next. An empty vector means "no
// custom icon loaded for this slot -- draw the built-in glyph instead."
std::vector<BYTE> g_mediaIconPixels[4];
int g_mediaIconLoadedSize = 0;

float GetMediaControlsDpiScale() {
    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    return GetMonitorDpiScale(monitor);
}

// Decodes a user-supplied image file to a square, premultiplied 32bpp BGRA
// buffer at targetSize -- the format UpdateLayeredWindow's AC_SRC_ALPHA blend
// expects. Returns false (and clears outPixels) on any failure, so callers can
// fall back to the built-in glyph rather than showing a blank button.
bool LoadMediaIconFromFile(const std::wstring& path, int targetSize, std::vector<BYTE>& outPixels) {
    outPixels.clear();
    if (path.empty() || targetSize <= 0) return false;

    IWICImagingFactory* pFactory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&pFactory))) || !pFactory) {
        return false;
    }

    bool ok = false;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConv = nullptr;
    IWICBitmapScaler* pScaler = nullptr;

    if (SUCCEEDED(pFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &pDecoder)) &&
        SUCCEEDED(pDecoder->GetFrame(0, &pFrame)) &&
        SUCCEEDED(pFactory->CreateFormatConverter(&pConv)) &&
        SUCCEEDED(pConv->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut)) &&
        SUCCEEDED(pFactory->CreateBitmapScaler(&pScaler)) &&
        SUCCEEDED(pScaler->Initialize(pConv, (UINT)targetSize, (UINT)targetSize,
            WICBitmapInterpolationModeFant))) {
        outPixels.resize((size_t)targetSize * targetSize * 4);
        if (SUCCEEDED(pScaler->CopyPixels(nullptr, targetSize * 4, (UINT)outPixels.size(),
                outPixels.data()))) {
            for (size_t i = 0; i + 3 < outPixels.size(); i += 4) {
                BYTE a = outPixels[i + 3];
                outPixels[i + 0] = (BYTE)(outPixels[i + 0] * a / 255);
                outPixels[i + 1] = (BYTE)(outPixels[i + 1] * a / 255);
                outPixels[i + 2] = (BYTE)(outPixels[i + 2] * a / 255);
            }
            ok = true;
        }
    }

    if (pScaler)  pScaler->Release();
    if (pConv)    pConv->Release();
    if (pFrame)   pFrame->Release();
    if (pDecoder) pDecoder->Release();
    pFactory->Release();

    if (!ok) outPixels.clear();
    return ok;
}

// LoadSettings already rejected paths that don't exist. A path that exists but
// still won't decode means the file isn't an image format WIC can read, and
// that's worth saying out loud rather than silently drawing the built-in glyph.
void LoadMediaIconSlot(const std::wstring& path, int sizePx, std::vector<BYTE>& out, PCWSTR label) {
    if (!LoadMediaIconFromFile(path, sizePx, out) && !path.empty()) {
        Wh_Log(L"[Media] %s icon could not be decoded: %s", label, path.c_str());
    }
}

void RecreateMediaControlResources() {
    float dpiScale = GetMediaControlsDpiScale();
    int sizePx = std::max(1, (int)std::lround(g_settings.mediaIconSize * dpiScale));
    g_mediaIconLoadedSize = sizePx;
    LoadMediaIconSlot(g_settings.mediaIconPrevPath, sizePx, g_mediaIconPixels[0], L"Previous");
    LoadMediaIconSlot(g_settings.mediaIconPlayPath, sizePx, g_mediaIconPixels[1], L"Play");
    LoadMediaIconSlot(g_settings.mediaIconPausePath, sizePx, g_mediaIconPixels[2], L"Pause");
    LoadMediaIconSlot(g_settings.mediaIconNextPath, sizePx, g_mediaIconPixels[3], L"Next");
}

// Source-over blend of one premultiplied BGRA pixel onto another. Everything in
// this buffer has to stay premultiplied because it goes straight to
// UpdateLayeredWindow's AC_SRC_ALPHA blend, which assumes exactly that.
inline void BlendPremultipliedOver(BYTE* dst, BYTE sb, BYTE sg, BYTE sr, BYTE sa) {
    if (sa == 255) {
        dst[0] = sb; dst[1] = sg; dst[2] = sr; dst[3] = sa;
        return;
    }
    if (sa == 0) return;
    unsigned inv = 255u - sa;
    dst[0] = (BYTE)(sb + (dst[0] * inv + 127) / 255);
    dst[1] = (BYTE)(sg + (dst[1] * inv + 127) / 255);
    dst[2] = (BYTE)(sr + (dst[2] * inv + 127) / 255);
    dst[3] = (BYTE)(sa + (dst[3] * inv + 127) / 255);
}

bool PointInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) {
    float d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
    float d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
    float d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
}

// kind: 0=prev ("<<"), 1=play, 2=pause ("||"), 3=next (">>"). Rasterized
// directly (2x2 supersampled for a soft edge) rather than pulled in through a
// vector library, since it's just four simple shapes and this keeps the mod
// dependency-free for the case where the user hasn't picked their own icons.
void DrawBuiltinGlyph(BYTE* buf, int stride, int originX, int originY, int size, int kind) {
    float margin = size * 0.24f;
    float cy = size * 0.5f;
    float trisize = (size - 2 * margin) * 0.5f;
    BYTE ir = g_settings.mediaIconColorR, ig = g_settings.mediaIconColorG,
         ib = g_settings.mediaIconColorB, ia = g_settings.mediaIconColorA;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float cov = 0.f;
            const int SS = 2;
            for (int sy = 0; sy < SS; sy++) {
                for (int sx = 0; sx < SS; sx++) {
                    float px = x + (sx + 0.5f) / SS;
                    float py = y + (sy + 0.5f) / SS;
                    bool inside = false;
                    switch (kind) {
                        case 1:
                            inside = PointInTriangle(px, py, margin, margin, margin, size - margin,
                                                      size - margin, cy);
                            break;
                        case 2: {
                            float barW = size * 0.16f, gap = size * 0.12f;
                            float b1x0 = size * 0.5f - gap * 0.5f - barW, b1x1 = size * 0.5f - gap * 0.5f;
                            float b2x0 = size * 0.5f + gap * 0.5f, b2x1 = size * 0.5f + gap * 0.5f + barW;
                            bool inBand = (py >= margin && py <= size - margin);
                            inside = inBand && ((px >= b1x0 && px <= b1x1) || (px >= b2x0 && px <= b2x1));
                            break;
                        }
                        case 0:
                            inside = PointInTriangle(px, py, size - margin, margin, size - margin,
                                                      size - margin, size - margin - trisize, cy) ||
                                     PointInTriangle(px, py, size - margin - trisize, margin,
                                                      size - margin - trisize, size - margin,
                                                      size - margin - 2 * trisize, cy);
                            break;
                        case 3:
                        default:
                            inside = PointInTriangle(px, py, margin, margin, margin, size - margin,
                                                      margin + trisize, cy) ||
                                     PointInTriangle(px, py, margin + trisize, margin,
                                                      margin + trisize, size - margin,
                                                      margin + 2 * trisize, cy);
                            break;
                    }
                    if (inside) cov += 1.0f / (SS * SS);
                }
            }
            if (cov <= 0.f) continue;
            float a = (ia / 255.0f) * cov;
            BYTE* p = buf + (size_t)(originY + y) * stride + (size_t)(originX + x) * 4;
            BlendPremultipliedOver(p, (BYTE)std::lround(ib * a), (BYTE)std::lround(ig * a),
                                   (BYTE)std::lround(ir * a), (BYTE)std::lround(a * 255.0f));
        }
    }
}

void BlitIcon(BYTE* buf, int stride, int originX, int originY, int size,
              const std::vector<BYTE>& src) {
    if ((int)src.size() < size * size * 4) return;
    for (int y = 0; y < size; y++) {
        const BYTE* s = src.data() + (size_t)y * size * 4;
        BYTE* d = buf + (size_t)(originY + y) * stride + (size_t)originX * 4;
        for (int x = 0; x < size; x++, s += 4, d += 4) {
            // Composited rather than copied: a straight memcpy would stamp the
            // icon's fully transparent pixels over the backing plate, punching
            // an icon-shaped hole through it instead of letting the plate show
            // through where the icon has nothing.
            BlendPremultipliedOver(d, s[0], s[1], s[2], s[3]);
        }
    }
}

// x/y are the strip's screen position. They're passed in rather than left for
// UpdateLayeredWindow to infer, because its "pass NULL and I'll use the current
// values" shortcuts are only documented as valid when the position and size
// aren't changing -- and here they usually are, since the window is born 1x1
// and every settings change can resize it. Handing it both explicitly is what
// makes the strip reliably appear instead of staying an invisible stub.
// Breathing room between the icon row and the edge of the backing plate. This
// grows the strip window itself rather than shrinking the icons, so raising it
// never makes the controls smaller or harder to hit. Applied whether or not the
// plate is actually visible, so that padding, fill and border stay independent
// of one another -- with the default of 0 that costs existing setups nothing.
int GetMediaPlatePaddingPx() {
    return std::max(0, (int)std::lround(g_settings.mediaPlatePadding * GetMediaControlsDpiScale()));
}

void PaintMediaControls(int x, int y, int width, int height) {
    if (!g_mediaWnd || width <= 0 || height <= 0) return;

    HDC screenDC = GetDC(nullptr);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, screenDC);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(memDC);
        return;
    }

    memset(bits, 0, (size_t)width * height * 4);
    BYTE* buf = (BYTE*)bits;
    int stride = width * 4;

    // Optional backing plate behind the whole strip, with an optional outline.
    // v0.5.2 drew a plate unconditionally as a fix for pale icons vanishing
    // against a pale wallpaper, which meant icons that were meant to sit
    // transparently on the desktop always had a dark box behind them. Both
    // pieces are settings now and both default to nothing.
    if (g_settings.mediaPlateA > 0 ||
        (g_settings.mediaPlateBorderSize > 0 && g_settings.mediaPlateBorderA > 0)) {
        float dpiScale = GetMediaControlsDpiScale();
        float radius = std::min({(float)std::max(0, g_settings.mediaPlateCornerRadius) * dpiScale,
                                 width * 0.5f, height * 0.5f});
        // Border draws inward from the edge, so it can never be thicker than
        // half the strip without the two sides meeting in the middle.
        float bw = std::min({(float)std::max(0, g_settings.mediaPlateBorderSize) * dpiScale,
                             width * 0.5f, height * 0.5f});
        bool wantBorder = bw > 0.f && g_settings.mediaPlateBorderA > 0;

        float innerRadius = std::max(0.0f, radius - bw);
        float fillAlpha = g_settings.mediaPlateA / 255.0f;
        float borderAlpha = g_settings.mediaPlateBorderA / 255.0f;

        // Coverage of a rounded rect inset by `inset` on every side: clamp the
        // sample into the rect shrunk by its corner radius, and whatever
        // distance is left over is the distance to the nearest corner circle's
        // centre (zero anywhere along the straight-edged middle).
        auto insideRounded = [&](float px, float py, float inset, float r) {
            float l = inset, t = inset, rt = width - inset, b = height - inset;
            if (px < l || py < t || px > rt || py > b) return false;
            float nx = std::clamp(px, l + r, rt - r);
            float ny = std::clamp(py, t + r, b - r);
            float dx = px - nx, dy = py - ny;
            return dx * dx + dy * dy <= r * r;
        };

        for (int y = 0; y < height; y++) {
            BYTE* row = buf + (size_t)y * stride;
            for (int x = 0; x < width; x++) {
                // 2x2 supersampled, matching how the built-in glyphs are
                // rasterized, so rounded corners don't come out jagged.
                float outerCov = 0.f, innerCov = 0.f;
                const int SS = 2;
                for (int sy = 0; sy < SS; sy++) {
                    for (int sx = 0; sx < SS; sx++) {
                        float px = x + (sx + 0.5f) / SS;
                        float py = y + (sy + 0.5f) / SS;
                        if (insideRounded(px, py, 0.f, radius)) outerCov += 1.0f / (SS * SS);
                        if (wantBorder && insideRounded(px, py, bw, innerRadius))
                            innerCov += 1.0f / (SS * SS);
                    }
                }
                if (outerCov <= 0.f) continue;

                BYTE* p = row + (size_t)x * 4;

                // Fill covers the whole plate including under the border, so a
                // translucent border blends over the fill rather than cutting a
                // hole in it.
                if (g_settings.mediaPlateA > 0) {
                    float a = fillAlpha * outerCov;
                    p[0] = (BYTE)std::lround(g_settings.mediaPlateB * a);
                    p[1] = (BYTE)std::lround(g_settings.mediaPlateG * a);
                    p[2] = (BYTE)std::lround(g_settings.mediaPlateR * a);
                    p[3] = (BYTE)std::lround(a * 255.0f);
                }

                if (wantBorder) {
                    float ringCov = std::max(0.0f, outerCov - innerCov);
                    if (ringCov > 0.f) {
                        float a = borderAlpha * ringCov;
                        BlendPremultipliedOver(
                            p, (BYTE)std::lround(g_settings.mediaPlateBorderB * a),
                            (BYTE)std::lround(g_settings.mediaPlateBorderG * a),
                            (BYTE)std::lround(g_settings.mediaPlateBorderR * a),
                            (BYTE)std::lround(a * 255.0f));
                    }
                }
            }
        }
    }

    // The icons occupy the content box -- the window inset by the plate padding
    // -- not the whole window.
    int pad = GetMediaPlatePaddingPx();
    int contentW = std::max(1, width  - pad * 2);
    int contentH = std::max(1, height - pad * 2);

    // Custom icons are rasterized once at a fixed size, so they can only be
    // blitted at exactly that size; anything else would read the source with the
    // wrong stride and come out scrambled. If a settings change has left them
    // out of step with the box they now have to fit in, fall back to the
    // built-in glyphs for this frame -- the reload that follows will restore
    // them at the right size.
    bool iconsFit = g_mediaIconLoadedSize > 0 && g_mediaIconLoadedSize <= contentH &&
                    g_mediaIconLoadedSize * 3 <= contentW;
    int size = iconsFit ? g_mediaIconLoadedSize : contentH;
    int spacing = std::max(0, (contentW - size * 3) / 2);
    int xPrev = pad;
    int xPlay = pad + size + spacing;
    int xNext = pad + 2 * size + 2 * spacing;
    int iconY = pad + (contentH - size) / 2;

    bool playing = g_mediaIsPlaying.load(std::memory_order_relaxed);

    if (iconsFit && !g_mediaIconPixels[0].empty())
        BlitIcon(buf, stride, xPrev, iconY, size, g_mediaIconPixels[0]);
    else DrawBuiltinGlyph(buf, stride, xPrev, iconY, size, 0);

    const auto& playPauseSrc = playing ? g_mediaIconPixels[2] : g_mediaIconPixels[1];
    if (iconsFit && !playPauseSrc.empty())
        BlitIcon(buf, stride, xPlay, iconY, size, playPauseSrc);
    else DrawBuiltinGlyph(buf, stride, xPlay, iconY, size, playing ? 2 : 1);

    if (iconsFit && !g_mediaIconPixels[3].empty())
        BlitIcon(buf, stride, xNext, iconY, size, g_mediaIconPixels[3]);
    else DrawBuiltinGlyph(buf, stride, xNext, iconY, size, 3);

    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, dib);

    POINT ptSrc = {0, 0};
    POINT ptDst = {x, y};
    SIZE  szWnd = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    BOOL ulwOk = UpdateLayeredWindow(g_mediaWnd, nullptr, &ptDst, &szWnd, memDC, &ptSrc, 0,
                                     &blend, ULW_ALPHA);
    if (!ulwOk) {
        Wh_Log(L"[Media] UpdateLayeredWindow failed, error=%lu xywh=(%d,%d,%d,%d)",
               GetLastError(), x, y, width, height);
    }

    SelectObject(memDC, oldBmp);
    DeleteObject(dib);
    DeleteDC(memDC);
}

// Set while the strip is being kept out of the way by Hide When Covered, so a
// repaint triggered by something else (a track change, a playback state flip)
// doesn't pop it back on screen over the window it just got out from under.
bool g_mediaHiddenByCover = false;

void RepositionAndRepaintMediaControls() {
    if (!g_mediaWnd) return;
    if (!g_settings.mediaControlsEnabled) {
        ShowWindow(g_mediaWnd, SW_HIDE);
        return;
    }
    if (g_mediaHiddenByCover && g_settings.mediaHideWhenCovered) return;

    float dpiScale = GetMediaControlsDpiScale();
    int size = std::max(1, (int)std::lround(g_settings.mediaIconSize * dpiScale));
    int spacing = std::max(0, (int)std::lround(g_settings.mediaIconSpacing * dpiScale));
    int pad = GetMediaPlatePaddingPx();
    int width = size * 3 + spacing * 2 + pad * 2;
    int height = size + pad * 2;

    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {sizeof(mi)};
    if (!GetMonitorInfo(monitor, &mi)) return;

    int workWidth = mi.rcWork.right - mi.rcWork.left;
    int workHeight = mi.rcWork.bottom - mi.rcWork.top;

    bool override_ = g_mediaOverrideActive.load(std::memory_order_relaxed);
    float hPercent = override_ ? g_mediaOverrideH.load(std::memory_order_relaxed)
                               : g_settings.mediaHorizontalPosition;
    float vPercent = override_ ? g_mediaOverrideV.load(std::memory_order_relaxed)
                               : g_settings.mediaVerticalPosition;

    int x = mi.rcWork.left + (int)std::lround((workWidth - width) * (hPercent / 100.0f));
    int y = mi.rcWork.top + (int)std::lround((workHeight - height) * (vPercent / 100.0f));

    Wh_Log(L"[Media] Reposition xywh=(%d,%d,%d,%d) work=(%ld,%ld,%ld,%ld) monitor=%d dpiScale=%.2f",
           x, y, width, height, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom,
           g_settings.monitor, dpiScale);

    // Paint before showing: UpdateLayeredWindow sets the position, size and
    // content in one shot, so by the time the window is shown there's already
    // something in it. A layered window that has never been given content is
    // invisible, which is indistinguishable from "the mod isn't working."
    PaintMediaControls(x, y, width, height);

    BOOL posOk = SetWindowPos(g_mediaWnd, HWND_TOPMOST, x, y, width, height,
                              SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);

    RECT actualRect{};
    GetWindowRect(g_mediaWnd, &actualRect);
    Wh_Log(L"[Media] SetWindowPos ok=%d actualRect=(%ld,%ld,%ld,%ld) visible=%d topmost=%d",
           (int)posOk, actualRect.left, actualRect.top, actualRect.right, actualRect.bottom,
           (int)IsWindowVisible(g_mediaWnd),
           (int)((GetWindowLongPtr(g_mediaWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0));
}

LRESULT CALLBACK MediaWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONUP: {
            int x = GET_X_LPARAM(lParam);
            float dpiScale = GetMediaControlsDpiScale();
            int size = std::max(1, (int)std::lround(g_settings.mediaIconSize * dpiScale));
            int spacing = std::max(0, (int)std::lround(g_settings.mediaIconSpacing * dpiScale));
            // The icons are inset by the plate padding (see PaintMediaControls),
            // so the hit test has to start from there as well. Testing from 0
            // instead puts every button's clickable area `pad` pixels left of
            // where the button is actually drawn -- and once padding exceeds the
            // gap between icons, that lands on the wrong button entirely.
            int pad = GetMediaPlatePaddingPx();

            int cmd = -1;
            if (x >= pad && x < pad + size) cmd = 0;
            else if (x >= pad + size + spacing && x < pad + 2 * size + spacing) cmd = 1;
            else if (x >= pad + 2 * size + 2 * spacing &&
                     x < pad + 3 * size + 2 * spacing) cmd = 2;

            if (cmd >= 0) SendMediaCommand(cmd);
            return 0;
        }

        case WM_APP_MEDIA_REPAINT:
            RepositionAndRepaintMediaControls();
            return 0;

        case WM_DESTROY:
            g_mediaWnd = nullptr;
            return 0;

        case WM_APP_CLEANUP:
            DestroyWindow(hWnd);
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool g_mediaClassRegistered = false;

bool RegisterMediaWindowClass() {
    if (g_mediaClassRegistered) return true;
    WNDCLASS wc = {};
    wc.lpfnWndProc = MediaWndProc;
    wc.hInstance = GetCurrentModuleHandle();
    wc.lpszClassName = MEDIA_WINDOW_CLASS;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    if (!RegisterClass(&wc)) return false;
    g_mediaClassRegistered = true;
    return true;
}

void UnregisterMediaWindowClass() {
    if (g_mediaClassRegistered) {
        UnregisterClass(MEDIA_WINDOW_CLASS, GetCurrentModuleHandle());
        g_mediaClassRegistered = false;
    }
}

void CreateMediaControlWindow() {
    if (g_mediaWnd) return;
    if (!RegisterMediaWindowClass()) {
        Wh_Log(L"[Media] RegisterMediaWindowClass failed, error %u", GetLastError());
        return;
    }

    HINSTANCE hInstance = GetCurrentModuleHandle();
    g_mediaWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        MEDIA_WINDOW_CLASS, nullptr, WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, hInstance, nullptr);
    if (!g_mediaWnd) {
        Wh_Log(L"[Media] CreateWindowEx failed, error %u", GetLastError());
        return;
    }
    Wh_Log(L"[Media] Window created hwnd=%p enabled=%d iconSize=%d spacing=%d pos=(%.2f,%.2f) monitor=%d",
           (void*)g_mediaWnd, (int)g_settings.mediaControlsEnabled, g_settings.mediaIconSize,
           g_settings.mediaIconSpacing, g_settings.mediaHorizontalPosition,
           g_settings.mediaVerticalPosition, g_settings.monitor);

    RecreateMediaControlResources();
    RepositionAndRepaintMediaControls();
}

void BuildHannWindow(int n) {
    for (int i = 0; i < n; i++)
        g_hannWindow[i] = 0.5f * (1.f - cosf(2.f * VIZ_PI * i / (n - 1)));
}

void BuildTwiddleFactors(int n) {
    for (int i = 0; i < n / 2; i++) {
        float ang = -2.0f * VIZ_PI * i / n;
        g_twiddleRe[i] = cosf(ang);
        g_twiddleIm[i] = sinf(ang);
    }
}

void BuildLogBins(UINT32 sampleRate, int fftSize) {
    for (int b = 0; b <= VIZ_NUM_BANDS; b++) {
        int bin = (int)(VIZ_FREQ_EDGES[b] * fftSize / (float)sampleRate);
        g_logBinStart[b] = std::max(1, std::min(fftSize / 2 - 1, bin));
    }
}

// Converts a target frequency (Hz) into an equivalent fractional band index
// (0..VIZ_NUM_BANDS-1) by inverse-interpolating against the shared band-edge
// table. This lets a warped frequency curve (log/linear/mel) still be sampled
// through the existing 7-band energy data without touching the analysis side.
float HzToBandPos(float hz) {
    for (int b = 0; b < VIZ_NUM_BANDS; b++) {
        if (hz <= VIZ_FREQ_EDGES[b + 1]) {
            float lo = VIZ_FREQ_EDGES[b], hi = VIZ_FREQ_EDGES[b + 1];
            float frac = (hi > lo) ? (hz - lo) / (hi - lo) : 0.f;
            return (float)b + std::max(0.f, std::min(1.f, frac));
        }
    }
    return (float)(VIZ_NUM_BANDS - 1);
}

void VizFFT(std::vector<float>& re, std::vector<float>& im) {
    int n = (int)re.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        int halfLen = len / 2;
        int stride = n / len;
        for (int i = 0; i < n; i += len) {
            for (int j = 0; j < halfLen; j++) {
                float wRe = g_twiddleRe[j * stride];
                float wIm = g_twiddleIm[j * stride];
                float uRe = re[i + j], uIm = im[i + j];
                float vRe = re[i + j + halfLen] * wRe - im[i + j + halfLen] * wIm;
                float vIm = re[i + j + halfLen] * wIm + im[i + j + halfLen] * wRe;
                re[i + j] = uRe + vRe;
                im[i + j] = uIm + vIm;
                re[i + j + halfLen] = uRe - vRe;
                im[i + j + halfLen] = uIm - vIm;
            }
        }
    }
}

struct VizEQMul { float low, mid, high; };
VizEQMul GetVizEQMultipliers(VizEQ eq) {
    switch (eq) {
    case VizEQ::Bass: return {2.0f, 0.6f, 0.4f};
    case VizEQ::Rock: return {1.3f, 1.5f, 1.2f};
    case VizEQ::Pop: return {0.8f, 1.2f, 1.8f};
    case VizEQ::Jazz: return {1.1f, 0.8f, 0.6f};
    case VizEQ::Electronic: return {1.7f, 0.6f, 1.7f};
    default: return {1.0f, 1.0f, 1.0f};
    }
}

// ---- NPU audio analysis (Audio Analysis Device = NPU) -------------------------
//
// What the NPU can and can't do here. An NPU is a matrix-multiply engine with
// no graphics pipeline: it can't rasterise a bar or present a frame, so
// drawing always stays on Drawing Device. What it can take is the FFT, as long
// as the FFT is written as matrix multiplies, which is the shape of work an
// NPU is built for.
//
// The FFT is expressed as a "four-step" DFT. With N = N1 * N2 and the input
// laid out as an N1 x N2 matrix (row-major, so no reshuffling of the samples):
//
//   Y = F1 . x          N1-point DFT down each column   (two real matmuls)
//   Z = Y (*) T         twiddle factors, elementwise    (complex multiply)
//   X = [Zre|Zim] . G   N2-point DFT along each row     (one real matmul)
//
// G packs the complex N2-point DFT as a real 2*N2 x 2*N2 block matrix, so the
// last step is a single matmul that emits [Xre|Xim]. Bin k = k1 + N1*k2 lands
// at row k1, column k2 (real) and N2 + k2 (imaginary). For N = 8192 that is
// 64 x 128, and the constant matrices total about 350 KB.
//
// F1 and G carry 1/N1 and 1/N2, so the NPU computes X/N. Intel's NPU runs in
// FP16, whose ceiling is 65504; unnormalised, an 8192-point bin can reach
// 8192 times the input level and would overflow. The CPU multiplies N back
// in, so everything downstream of VizFFT sees exactly the numbers it always
// has.
//
// The model is built in memory as ONNX and handed to Intel's OpenVINO runtime,
// which is how Intel exposes its NPU. OpenVINO is loaded at runtime through its
// flat C API, so nothing here links against it and the mod still builds and
// runs on machines without it. ONNX rather than OpenVINO's own IR because ONNX
// carries its weights inline: IR would need a weights tensor created through
// an element-type enum whose numbering has changed between OpenVINO releases.
// Input and output tensors are the request's own, for the same reason.
//
// An honest note on cost, because this mod is built around measuring it: the
// CPU FFT is already a fraction of a millisecond per block, roughly 90 blocks a
// second at FFT Size 1024. Moving it to the NPU frees that CPU time but wakes
// the NPU at the same rate, so whether package power goes down is something to
// measure on your own machine, not something to assume. The log reports mean
// and worst-case NPU time per block every 30 s for exactly that reason.
namespace ttnpu {

// ---- ONNX ModelProto writer --------------------------------------------------
// Just enough protobuf to emit one ModelProto. Field numbers are from
// onnx/onnx.proto: ModelProto{ir_version=1, producer_name=2, graph=7,
// opset_import=8}, GraphProto{node=1, name=2, initializer=5, input=11,
// output=12}, NodeProto{input=1, output=2, name=3, op_type=4, attribute=5},
// AttributeProto{name=1, i=3, type=20}, TensorProto{dims=1, data_type=2,
// name=8, raw_data=9}, ValueInfoProto{name=1, type=2}, TypeProto{tensor_type=1},
// TypeProto.Tensor{elem_type=1, shape=2}, TensorShapeProto{dim=1},
// Dimension{dim_value=1}, OperatorSetIdProto{domain=1, version=2}.
struct Pb {
    std::string b;
    void Varint(uint64_t v) {
        while (v >= 0x80) {
            b.push_back((char)((v & 0x7F) | 0x80));
            v >>= 7;
        }
        b.push_back((char)v);
    }
    void Key(uint32_t field, uint32_t wire) { Varint(((uint64_t)field << 3) | wire); }
    void Int(uint32_t field, int64_t v) {
        Key(field, 0);
        Varint((uint64_t)v);
    }
    void Bytes(uint32_t field, const void* p, size_t n) {
        Key(field, 2);
        Varint(n);
        b.append((const char*)p, n);
    }
    void Str(uint32_t field, const char* s) { Bytes(field, s, strlen(s)); }
    void Msg(uint32_t field, const Pb& m) { Bytes(field, m.b.data(), m.b.size()); }
};

constexpr int64_t kOnnxFloat = 1;      // TensorProto.DataType.FLOAT
constexpr int64_t kOnnxAttrInt = 2;    // AttributeProto.AttributeType.INT
constexpr int64_t kOnnxIrVersion = 7;  // IR_VERSION_2020_5_8, the first to allow opset 13
constexpr int64_t kOnnxOpset = 13;

inline Pb OnnxInitializer(const char* name, int rows, int cols, const std::vector<float>& data) {
    Pb t;
    t.Int(1, rows);
    t.Int(1, cols);
    t.Int(2, kOnnxFloat);
    t.Str(8, name);
    // raw_data is little-endian by spec, which is the native order on every
    // Windows target.
    t.Bytes(9, data.data(), data.size() * sizeof(float));
    return t;
}

inline Pb OnnxValueInfo(const char* name, int rows, int cols) {
    Pb d0, d1, shape, tensorType, type, vi;
    d0.Int(1, rows);
    d1.Int(1, cols);
    shape.Msg(1, d0);
    shape.Msg(1, d1);
    tensorType.Int(1, kOnnxFloat);
    tensorType.Msg(2, shape);
    type.Msg(1, tensorType);
    vi.Str(1, name);
    vi.Msg(2, type);
    return vi;
}

inline Pb OnnxNode(const char* op, const char* in0, const char* in1, const char* out,
                   bool hasAxis = false, int64_t axis = 0) {
    Pb n;
    n.Str(1, in0);
    n.Str(1, in1);
    n.Str(2, out);
    n.Str(3, out);
    n.Str(4, op);
    if (hasAxis) {
        Pb a;
        a.Str(1, "axis");
        a.Int(3, axis);
        a.Int(20, kOnnxAttrInt);
        n.Msg(5, a);
    }
    return n;
}

// Splits an FFT size into the N1 x N2 factors used by the four-step layout.
// Near-square keeps every constant matrix small.
inline bool SplitSize(int n, int* n1, int* n2) {
    switch (n) {
        case 1024: *n1 = 32; *n2 = 32; return true;
        case 2048: *n1 = 32; *n2 = 64; return true;
        case 4096: *n1 = 64; *n2 = 64; return true;
        case 8192: *n1 = 64; *n2 = 128; return true;
    }
    return false;
}

// The whole model: input "x" [N1, N2] float, output "y" [N1, 2*N2] float.
inline std::string BuildDftModel(int n1, int n2) {
    const long long n = (long long)n1 * n2;
    const double tau = 6.283185307179586476925;

    // Angles are reduced with integer arithmetic before the trig call, so
    // large index products don't lose precision in the argument.
    std::vector<float> f1re((size_t)n1 * n1), f1im((size_t)n1 * n1);
    for (int k1 = 0; k1 < n1; k1++) {
        for (int j = 0; j < n1; j++) {
            double ang = tau * (double)(((long long)k1 * j) % n1) / (double)n1;
            f1re[(size_t)k1 * n1 + j] = (float)(cos(ang) / n1);
            f1im[(size_t)k1 * n1 + j] = (float)(-sin(ang) / n1);
        }
    }

    std::vector<float> tre((size_t)n1 * n2), tim((size_t)n1 * n2);
    for (int k1 = 0; k1 < n1; k1++) {
        for (int j = 0; j < n2; j++) {
            double ang = tau * (double)(((long long)k1 * j) % n) / (double)n;
            tre[(size_t)k1 * n2 + j] = (float)cos(ang);
            tim[(size_t)k1 * n2 + j] = (float)(-sin(ang));
        }
    }

    // G = [[F2re, F2im], [-F2im, F2re]], so [Zre | Zim] . G = [Xre | Xim].
    const int g = 2 * n2;
    std::vector<float> gm((size_t)g * g);
    for (int j = 0; j < n2; j++) {
        for (int k = 0; k < n2; k++) {
            double ang = tau * (double)(((long long)j * k) % n2) / (double)n2;
            float fr = (float)(cos(ang) / n2);
            float fi = (float)(-sin(ang) / n2);
            gm[(size_t)j * g + k] = fr;
            gm[(size_t)j * g + n2 + k] = fi;
            gm[(size_t)(n2 + j) * g + k] = -fi;
            gm[(size_t)(n2 + j) * g + n2 + k] = fr;
        }
    }

    Pb graph;
    graph.Msg(1, OnnxNode("MatMul", "f1re", "x", "yre"));
    graph.Msg(1, OnnxNode("MatMul", "f1im", "x", "yim"));
    graph.Msg(1, OnnxNode("Mul", "yre", "tre", "a"));
    graph.Msg(1, OnnxNode("Mul", "yim", "tim", "b"));
    graph.Msg(1, OnnxNode("Sub", "a", "b", "zre"));
    graph.Msg(1, OnnxNode("Mul", "yre", "tim", "c"));
    graph.Msg(1, OnnxNode("Mul", "yim", "tre", "d"));
    graph.Msg(1, OnnxNode("Add", "c", "d", "zim"));
    graph.Msg(1, OnnxNode("Concat", "zre", "zim", "z", true, 1));
    graph.Msg(1, OnnxNode("MatMul", "z", "g", "y"));
    graph.Str(2, "tourne_table_dft");
    graph.Msg(5, OnnxInitializer("f1re", n1, n1, f1re));
    graph.Msg(5, OnnxInitializer("f1im", n1, n1, f1im));
    graph.Msg(5, OnnxInitializer("tre", n1, n2, tre));
    graph.Msg(5, OnnxInitializer("tim", n1, n2, tim));
    graph.Msg(5, OnnxInitializer("g", g, g, gm));
    graph.Msg(11, OnnxValueInfo("x", n1, n2));
    graph.Msg(12, OnnxValueInfo("y", n1, g));

    Pb opset;
    opset.Str(1, "");
    opset.Int(2, kOnnxOpset);

    // Order matters for one reason: OpenVINO sniffs the format by reading the
    // first three top-level fields and wants three different ones.
    Pb model;
    model.Int(1, kOnnxIrVersion);
    model.Str(2, "TourneTable");
    model.Msg(7, graph);
    model.Msg(8, opset);
    return model.b;
}

// Bin k of the four-step output, scaled back up to VizFFT's (unnormalised)
// units. y is [N1, 2*N2] row-major.
inline void UnpackBins(const float* y, int n1, int n2, std::vector<float>& re,
                       std::vector<float>& im) {
    const int n = n1 * n2;
    const int stride = 2 * n2;
    const float scale = (float)n;
    for (int k = 0; k < n / 2; k++) {
        int k1 = k % n1, k2 = k / n1;
        re[k] = y[k1 * stride + k2] * scale;
        im[k] = y[k1 * stride + n2 + k2] * scale;
    }
}

// ---- OpenVINO C API, resolved at runtime ----------------------------------------
// Signatures are copied from openvino/c/*.h. ov_status_e is a plain enum (0 =
// OK). Every handle is opaque, so they are all void* here.
struct OvDevices {
    char** devices;
    size_t size;
};
struct OvVersion {
    const char* buildNumber;
    const char* description;
};
struct OvApi {
    HMODULE lib = nullptr;
    int (*core_create)(void** core) = nullptr;
    void (*core_free)(void* core) = nullptr;
    int (*core_get_available_devices)(const void* core, OvDevices* devices) = nullptr;
    void (*available_devices_free)(OvDevices* devices) = nullptr;
    int (*core_read_model_from_memory_buffer)(const void* core, const char* model, size_t len,
                                              const void* weights, void** outModel) = nullptr;
    int (*core_compile_model)(const void* core, const void* model, const char* device,
                              size_t propertyArgsSize, void** compiled, ...) = nullptr;
    void (*model_free)(void* model) = nullptr;
    int (*compiled_model_create_infer_request)(const void* compiled, void** request) = nullptr;
    void (*compiled_model_free)(void* compiled) = nullptr;
    int (*infer_request_get_input_tensor_by_index)(const void* request, size_t idx,
                                                   void** tensor) = nullptr;
    int (*infer_request_get_output_tensor_by_index)(const void* request, size_t idx,
                                                    void** tensor) = nullptr;
    int (*infer_request_infer)(void* request) = nullptr;
    void (*infer_request_free)(void* request) = nullptr;
    int (*tensor_data)(const void* tensor, void** data) = nullptr;
    int (*tensor_get_byte_size)(const void* tensor, size_t* size) = nullptr;
    void (*tensor_free)(void* tensor) = nullptr;
    // Optional: absent from older runtimes, only used for nicer log lines.
    int (*core_get_property)(const void* core, const char* device, const char* key,
                             char** value) = nullptr;
    void (*free_string)(const char* s) = nullptr;
    const char* (*get_last_err_msg)() = nullptr;
    int (*get_openvino_version)(OvVersion* v) = nullptr;
    void (*version_free)(OvVersion* v) = nullptr;
};

struct Engine {
    void* compiled = nullptr;
    void* request = nullptr;
    void* in = nullptr;
    void* out = nullptr;
    int n = 0, n1 = 0, n2 = 0;
};

// Shared state. The capture thread never takes this lock on its hot path: it
// owns one Engine outright and only reaches for the lock when it needs a new
// one, which is at start-up and on an FFT Size change.
std::mutex g_mutex;
std::condition_variable g_cv;
std::thread* g_worker = nullptr;
bool g_shutdown = false;
bool g_enabled = false;
bool g_runtimeRetry = false;  // set by Configure, consumed by the worker
std::wstring g_runtimePath;  // copy of the setting, taken on the UI thread
int g_wantSize = 0;          // FFT size the capture thread is asking for
int g_builtSize = 0;         // size of the engine that currently exists, 0 if none
int g_failedSize = 0;        // size whose build failed; not retried until settings change
Engine* g_pending = nullptr; // built (or handed back after a pause), not in use
std::wstring g_lastNotice;

// Runtime and core: created by the worker, live for the process, and touched
// by the worker thread only (Shutdown frees the core after joining it). The
// DLLs are never unloaded -- OpenVINO's thread pool doesn't survive being
// pulled out from under it, and the process exits right after the mod unloads
// anyway.
OvApi g_api;
void* g_core = nullptr;
bool g_runtimeAttempted = false;
bool g_runtimeReady = false;
std::wstring g_runtimeDir;
std::wstring g_runtimeError;
std::string g_deviceName;  // FULL_DEVICE_NAME, e.g. "Intel(R) AI Boost"

// Hands a message to the UI thread for the settings-problems summary, once per
// distinct message per settings change.
inline void Notice(const std::wstring& msg) {
    Wh_Log(L"[NPU] %s", msg.c_str());
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (msg == g_lastNotice) return;
        g_lastNotice = msg;
    }
    HWND wnd = g_messageWnd;
    if (!wnd) return;
    auto* heap = new std::wstring(msg);
    if (!PostMessage(wnd, WM_APP_HW_NOTICE, 0, (LPARAM)heap)) delete heap;
}

inline std::wstring Widen(const char* s) {
    if (!s) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 1) return L"";
    std::wstring w(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), len);
    return w;
}

inline std::wstring LastOvError(int status) {
    WCHAR buf[64];
    swprintf_s(buf, L"status %d", status);
    std::wstring out = buf;
    if (g_api.get_last_err_msg) {
        const char* m = g_api.get_last_err_msg();
        if (m && *m) {
            std::wstring w = Widen(m);
            // OpenVINO's messages carry a full C++ backtrace-ish preamble;
            // the first line is the useful part.
            size_t nl = w.find_first_of(L"\r\n");
            if (nl != std::wstring::npos) w.resize(nl);
            if (w.size() > 300) w.resize(300);
            out += L": " + w;
        }
    }
    return out;
}

inline bool DirHasRuntime(const std::wstring& dir) {
    if (dir.empty()) return false;
    DWORD attr = GetFileAttributesW((dir + L"\\openvino_c.dll").c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

inline std::wstring EnvVar(PCWSTR name) {
    WCHAR buf[MAX_PATH * 2];
    DWORD n = GetEnvironmentVariableW(name, buf, ARRAYSIZE(buf));
    return (n > 0 && n < ARRAYSIZE(buf)) ? std::wstring(buf, n) : std::wstring();
}

// Directories matching parent\pattern, newest-looking (highest name) first.
inline std::vector<std::wstring> GlobDirs(const std::wstring& parent, PCWSTR pattern) {
    std::vector<std::wstring> out;
    if (parent.empty()) return out;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((parent + L"\\" + pattern).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != L'.')
            out.push_back(parent + L"\\" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end(), [](const std::wstring& a, const std::wstring& b) {
        return _wcsicmp(a.c_str(), b.c_str()) > 0;
    });
    return out;
}

// Every place OpenVINO's C runtime is commonly found, most specific first.
inline std::vector<std::wstring> CandidateDirs(const std::wstring& userPath) {
    std::vector<std::wstring> dirs;
    auto addRoot = [&](const std::wstring& root) {
        if (root.empty()) return;
        dirs.push_back(root);                                   // the folder itself
        dirs.push_back(root + L"\\runtime\\bin\\intel64\\Release");  // extracted archive
        dirs.push_back(root + L"\\openvino\\libs");             // a site-packages folder
        dirs.push_back(root + L"\\libs");                       // the openvino package folder
    };

    addRoot(userPath);
    addRoot(EnvVar(L"INTEL_OPENVINO_DIR"));

    for (PCWSTR base : {L"ProgramFiles(x86)", L"ProgramFiles", L"ProgramW6432"}) {
        std::wstring pf = EnvVar(base);
        for (const auto& d : GlobDirs(pf + L"\\Intel", L"openvino*")) addRoot(d);
    }
    for (const auto& d : GlobDirs(L"C:\\Intel", L"openvino*")) addRoot(d);
    for (const auto& d : GlobDirs(L"C:", L"openvino*")) addRoot(d);

    // pip: per-user and all-users CPython installs, plus `pip install --user`.
    std::wstring localApp = EnvVar(L"LOCALAPPDATA");
    for (const auto& py : GlobDirs(localApp + L"\\Programs\\Python", L"Python3*"))
        dirs.push_back(py + L"\\Lib\\site-packages\\openvino\\libs");
    for (PCWSTR base : {L"ProgramFiles", L"ProgramW6432"}) {
        for (const auto& py : GlobDirs(EnvVar(base), L"Python3*"))
            dirs.push_back(py + L"\\Lib\\site-packages\\openvino\\libs");
    }
    for (const auto& py : GlobDirs(EnvVar(L"APPDATA") + L"\\Python", L"Python3*"))
        dirs.push_back(py + L"\\site-packages\\openvino\\libs");

    // Finally PATH, the way setupvars.bat leaves things.
    WCHAR found[MAX_PATH];
    if (SearchPathW(nullptr, L"openvino_c.dll", nullptr, ARRAYSIZE(found), found, nullptr)) {
        std::wstring p = found;
        size_t slash = p.find_last_of(L"\\/");
        if (slash != std::wstring::npos) dirs.push_back(p.substr(0, slash));
    }
    return dirs;
}

template <class T>
inline bool Resolve(HMODULE lib, const char* name, T& fn) {
    fn = (T)(void*)GetProcAddress(lib, name);
    return fn != nullptr;
}

inline bool LoadRuntimeFrom(const std::wstring& dir, std::wstring* err) {
    // The extracted archive keeps TBB (which openvino.dll imports) in a sibling
    // tree rather than next to it; pip puts everything in one folder. Both
    // directories are added to this process's DLL search for the loads below
    // and for the plugins OpenVINO loads later. The cookies are kept for the
    // life of the process, like the DLLs themselves.
    AddDllDirectory(dir.c_str());
    WCHAR tbb[MAX_PATH * 2];
    if (GetFullPathNameW((dir + L"\\..\\..\\..\\3rdparty\\tbb\\bin").c_str(), ARRAYSIZE(tbb), tbb,
                         nullptr)) {
        DWORD attr = GetFileAttributesW(tbb);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) AddDllDirectory(tbb);
    }

    std::wstring dll = dir + L"\\openvino_c.dll";
    HMODULE lib = LoadLibraryExW(dll.c_str(), nullptr,
                                 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                                     LOAD_LIBRARY_SEARCH_USER_DIRS);
    if (!lib) {
        WCHAR buf[96];
        swprintf_s(buf, L"Windows error %lu", GetLastError());
        *err = L"Found " + dll + L" but it would not load (" + buf +
               L"). Usually a missing dependency: an incomplete copy, or a 32-bit/ARM build.";
        return false;
    }

    OvApi api;
    api.lib = lib;
    bool ok = Resolve(lib, "ov_core_create", api.core_create) &&
              Resolve(lib, "ov_core_free", api.core_free) &&
              Resolve(lib, "ov_core_get_available_devices", api.core_get_available_devices) &&
              Resolve(lib, "ov_available_devices_free", api.available_devices_free) &&
              Resolve(lib, "ov_core_read_model_from_memory_buffer",
                      api.core_read_model_from_memory_buffer) &&
              Resolve(lib, "ov_core_compile_model", api.core_compile_model) &&
              Resolve(lib, "ov_model_free", api.model_free) &&
              Resolve(lib, "ov_compiled_model_create_infer_request",
                      api.compiled_model_create_infer_request) &&
              Resolve(lib, "ov_compiled_model_free", api.compiled_model_free) &&
              Resolve(lib, "ov_infer_request_get_input_tensor_by_index",
                      api.infer_request_get_input_tensor_by_index) &&
              Resolve(lib, "ov_infer_request_get_output_tensor_by_index",
                      api.infer_request_get_output_tensor_by_index) &&
              Resolve(lib, "ov_infer_request_infer", api.infer_request_infer) &&
              Resolve(lib, "ov_infer_request_free", api.infer_request_free) &&
              Resolve(lib, "ov_tensor_data", api.tensor_data) &&
              Resolve(lib, "ov_tensor_get_byte_size", api.tensor_get_byte_size) &&
              Resolve(lib, "ov_tensor_free", api.tensor_free);
    if (!ok) {
        *err = L"The OpenVINO runtime in " + dir +
               L" is too old: reading a model from memory needs OpenVINO 2023.1 or newer.";
        return false;
    }
    Resolve(lib, "ov_core_get_property", api.core_get_property);
    Resolve(lib, "ov_free", api.free_string);
    Resolve(lib, "ov_get_last_err_msg", api.get_last_err_msg);
    Resolve(lib, "ov_get_openvino_version", api.get_openvino_version);
    Resolve(lib, "ov_version_free", api.version_free);

    g_api = api;
    g_runtimeDir = dir;
    return true;
}

// Loads the runtime and creates the core, once per process. Runs on the
// worker thread only. On failure the reason is kept, and a later settings
// change (a corrected folder, say) gets one fresh attempt.
inline bool EnsureRuntime(const std::wstring& userPath, std::wstring* err) {
    if (g_runtimeReady) return true;
    if (g_runtimeAttempted) {
        *err = g_runtimeError;
        return false;
    }
    g_runtimeAttempted = true;

#if !defined(_WIN64)
    std::wstring npu = ttdxcore::FindNpuName();
    g_runtimeError =
        L"NPU analysis needs a 64-bit Windhawk tool process, because OpenVINO only ships "
        L"64-bit. This one is 32-bit (Windhawk 1.x runs tool mods that way). " +
        (npu.empty() ? std::wstring(L"")
                     : L"Windows does see your NPU (" + npu + L"), so it will work on a 64-bit host. ") +
        L"Staying on the CPU.";
    *err = g_runtimeError;
    return false;
#else
    std::wstring loadErr;
    bool loaded = false;
    for (const auto& dir : CandidateDirs(userPath)) {
        if (!DirHasRuntime(dir)) continue;
        if (LoadRuntimeFrom(dir, &loadErr)) {
            loaded = true;
            break;
        }
    }
    if (!loaded) {
        std::wstring npu = ttdxcore::FindNpuName();
        if (!loadErr.empty()) {
            g_runtimeError = loadErr;
        } else if (!userPath.empty()) {
            g_runtimeError = L"No openvino_c.dll in \"" + userPath +
                             L"\" (or its runtime\\bin\\intel64\\Release folder).";
        } else {
            g_runtimeError = L"The OpenVINO runtime isn't installed, or isn't anywhere this mod looks.";
        }
        g_runtimeError +=
            npu.empty()
                ? L" Windows doesn't report an NPU either - check Device Manager for a \"Neural "
                  L"processors\" entry; if it's missing, install the Intel NPU driver and check the "
                  L"BIOS for an NPU / AI Boost switch."
                : L" Windows does see your NPU (" + npu +
                      L"), so only the runtime is missing: extract the OpenVINO archive from "
                      L"Intel (or pip install openvino) and point NPU Runtime Folder at it.";
        g_runtimeError += L" Staying on the CPU.";
        *err = g_runtimeError;
        return false;
    }

    int st = g_api.core_create(&g_core);
    if (st != 0 || !g_core) {
        g_runtimeError = L"OpenVINO loaded from " + g_runtimeDir + L" but could not start (" +
                         LastOvError(st) + L"). Staying on the CPU.";
        *err = g_runtimeError;
        g_core = nullptr;
        return false;
    }

    OvDevices devices{};
    bool hasNpu = false;
    std::wstring deviceList;
    if (g_api.core_get_available_devices(g_core, &devices) == 0) {
        for (size_t i = 0; i < devices.size; i++) {
            const char* d = devices.devices[i];
            if (!d) continue;
            if (!deviceList.empty()) deviceList += L", ";
            deviceList += Widen(d);
            if (strncmp(d, "NPU", 3) == 0) hasNpu = true;
        }
        g_api.available_devices_free(&devices);
    }
    if (!hasNpu) {
        std::wstring npu = ttdxcore::FindNpuName();
        g_runtimeError = L"OpenVINO (" + g_runtimeDir + L") is working but lists no NPU (it sees: " +
                         (deviceList.empty() ? std::wstring(L"nothing") : deviceList) + L"). " +
                         (npu.empty() ? std::wstring(L"Windows doesn't report one either - install the "
                                                     L"Intel NPU driver and check the BIOS.")
                                      : L"Windows does see " + npu +
                                            L", so the NPU driver and this OpenVINO are likely "
                                            L"mismatched: update the Intel NPU driver, or use the "
                                            L"OpenVINO version its release notes name.") +
                         L" Staying on the CPU.";
        *err = g_runtimeError;
        return false;
    }

    if (g_api.core_get_property && g_api.free_string) {
        char* name = nullptr;
        if (g_api.core_get_property(g_core, "NPU", "FULL_DEVICE_NAME", &name) == 0 && name) {
            g_deviceName = name;
            g_api.free_string(name);
        }
    }
    std::wstring version;
    if (g_api.get_openvino_version && g_api.version_free) {
        OvVersion v{};
        if (g_api.get_openvino_version(&v) == 0) {
            version = Widen(v.buildNumber);
            g_api.version_free(&v);
        }
    }
    Wh_Log(L"[NPU] OpenVINO %s from %s, NPU: %s", version.c_str(), g_runtimeDir.c_str(),
           g_deviceName.empty() ? L"(unnamed)" : Widen(g_deviceName.c_str()).c_str());
    g_runtimeReady = true;
    return true;
#endif
}

inline void FreeEngine(Engine* e) {
    if (!e) return;
    if (e->in) g_api.tensor_free(e->in);
    if (e->out) g_api.tensor_free(e->out);
    if (e->request) g_api.infer_request_free(e->request);
    if (e->compiled) g_api.compiled_model_free(e->compiled);
    delete e;
}

// One inference. `windowed` may alias `re`: it is copied in before anything is
// written back.
inline bool Run(Engine* e, const float* windowed, std::vector<float>& re, std::vector<float>& im) {
    void* inData = nullptr;
    void* outData = nullptr;
    if (g_api.tensor_data(e->in, &inData) != 0 || !inData) return false;
    memcpy(inData, windowed, (size_t)e->n * sizeof(float));
    if (g_api.infer_request_infer(e->request) != 0) return false;
    if (g_api.tensor_data(e->out, &outData) != 0 || !outData) return false;
    UnpackBins((const float*)outData, e->n1, e->n2, re, im);
    return true;
}

// Compiles the model for one FFT size on the NPU and proves it against a known
// answer before anything relies on it. Worker thread only.
inline bool BuildEngine(int n, Engine** out, std::wstring* err) {
    int n1 = 0, n2 = 0;
    if (!SplitSize(n, &n1, &n2)) {
        *err = L"Unsupported FFT size for the NPU path.";
        return false;
    }

    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);

    std::string onnx = BuildDftModel(n1, n2);
    void* model = nullptr;
    int st = g_api.core_read_model_from_memory_buffer(g_core, onnx.data(), onnx.size(), nullptr, &model);
    if (st != 0 || !model) {
        *err = L"OpenVINO rejected the FFT model (" + LastOvError(st) + L"). Staying on the CPU.";
        return false;
    }

    auto* e = new Engine();
    e->n = n;
    e->n1 = n1;
    e->n2 = n2;
    // LATENCY: one request in flight at a time, which is exactly this workload.
    // If a runtime refuses the hint, compile again with its defaults rather
    // than give up over a tuning knob.
    st = g_api.core_compile_model(g_core, model, "NPU", 2, &e->compiled, "PERFORMANCE_HINT",
                                  "LATENCY");
    if (st != 0 || !e->compiled) {
        Wh_Log(L"[NPU] compile with LATENCY hint failed (%s), retrying with defaults",
               LastOvError(st).c_str());
        e->compiled = nullptr;
        st = g_api.core_compile_model(g_core, model, "NPU", 0, &e->compiled);
    }
    g_api.model_free(model);
    if (st != 0 || !e->compiled) {
        e->compiled = nullptr;
        FreeEngine(e);
        *err = L"The NPU couldn't compile the FFT model (" + LastOvError(st) + L"). Staying on the CPU.";
        return false;
    }
    if (g_api.compiled_model_create_infer_request(e->compiled, &e->request) != 0 || !e->request ||
        g_api.infer_request_get_input_tensor_by_index(e->request, 0, &e->in) != 0 || !e->in ||
        g_api.infer_request_get_output_tensor_by_index(e->request, 0, &e->out) != 0 || !e->out) {
        FreeEngine(e);
        *err = L"The NPU compiled the FFT model but wouldn't create a request for it. Staying on the CPU.";
        return false;
    }

    // Shapes are fixed by the model, so a size mismatch means the runtime did
    // something unexpected with the I/O precision; refuse rather than guess.
    size_t inBytes = 0, outBytes = 0;
    g_api.tensor_get_byte_size(e->in, &inBytes);
    g_api.tensor_get_byte_size(e->out, &outBytes);
    if (inBytes != (size_t)n * sizeof(float) || outBytes != (size_t)n * 2 * sizeof(float)) {
        FreeEngine(e);
        WCHAR buf[160];
        swprintf_s(buf, L"Unexpected NPU tensor sizes (%zu in, %zu out). Staying on the CPU.", inBytes,
                   outBytes);
        *err = buf;
        return false;
    }

    // Known-answer test: a cosine at bin 5 must come back as a single real
    // line of height N/2 and nothing much anywhere else. This catches a wrong
    // layout, a precision problem, or a driver that quietly returns zeros.
    std::vector<float> probe(n), re(n, 0.f), im(n, 0.f);
    for (int i = 0; i < n; i++) probe[i] = (float)cos(6.283185307179586 * 5.0 * i / n);
    if (!Run(e, probe.data(), re, im)) {
        FreeEngine(e);
        *err = L"The NPU failed its first test run. Staying on the CPU.";
        return false;
    }
    float half = n * 0.5f, worst = 0.f;
    for (int k = 1; k < n / 2; k++) {
        if (k == 5) continue;
        worst = std::max(worst, sqrtf(re[k] * re[k] + im[k] * im[k]));
    }
    float peakErr = fabsf(re[5] - half) / half;
    if (!(peakErr < 0.03f) || !(worst < 0.02f * half) || !(fabsf(im[5]) < 0.02f * half)) {
        FreeEngine(e);
        WCHAR buf[200];
        swprintf_s(buf,
                   L"The NPU's FFT didn't match the reference (peak off by %.1f%%, leakage %.1f%%). "
                   L"Staying on the CPU.",
                   peakErr * 100.f, worst / half * 100.f);
        *err = buf;
        return false;
    }

    QueryPerformanceCounter(&t1);
    Wh_Log(L"[NPU] FFT %d ready on %s: compiled and verified in %.0f ms (peak error %.2f%%, "
           L"leakage %.2f%%)",
           n, g_deviceName.empty() ? L"NPU" : Widen(g_deviceName.c_str()).c_str(),
           (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)f.QuadPart, peakErr * 100.f,
           worst / half * 100.f);
    *out = e;
    return true;
}

inline void WorkerProc() {
    SetThreadDescription(GetCurrentThread(), L"TourneTable-NPU");
    std::unique_lock<std::mutex> lock(g_mutex);
    for (;;) {
        g_cv.wait(lock, [] {
            return g_shutdown || (g_enabled && g_wantSize > 0 && g_wantSize != g_builtSize &&
                                  g_wantSize != g_failedSize);
        });
        if (g_shutdown) break;

        int n = g_wantSize;
        std::wstring path = g_runtimePath;
        if (g_runtimeRetry) {
            // A settings change since the last failure: one fresh attempt at
            // finding and loading the runtime.
            g_runtimeRetry = false;
            if (!g_runtimeReady) g_runtimeAttempted = false;
        }
        lock.unlock();

        std::wstring err;
        Engine* e = nullptr;
        bool ok = EnsureRuntime(path, &err) && BuildEngine(n, &e, &err);

        lock.lock();
        if (g_shutdown) {
            if (e) FreeEngine(e);
            break;
        }
        if (ok) {
            if (g_pending) FreeEngine(g_pending);
            g_pending = e;
            g_builtSize = n;
            g_failedSize = 0;
        } else {
            g_failedSize = n;
            lock.unlock();
            Notice(err);
            lock.lock();
        }
    }
}

// UI thread, from LoadSettings. Cheap; safe to call on every settings change.
inline void Configure(bool enabled, const std::wstring& runtimePath) {
    std::lock_guard<std::mutex> lock(g_mutex);
    bool changed = enabled != g_enabled || runtimePath != g_runtimePath;
    g_enabled = enabled;
    g_runtimePath = runtimePath;
    if (changed) {
        // A settings change is the user's way of saying "try again": clear the
        // failure so the next block retries, and let the same notice show again.
        g_failedSize = 0;
        g_lastNotice.clear();
        g_runtimeRetry = true;
    }
    if (enabled && !g_worker && !g_shutdown) g_worker = new std::thread(WorkerProc);
    g_cv.notify_all();
}

// Capture-thread side. One per capture thread; owns its engine outright.
struct Session {
    Engine* engine = nullptr;
    LONGLONG qpcFreq = 0;
    LONGLONG statSince = 0;
    LONGLONG statTotal = 0, statWorst = 0;
    unsigned statCount = 0;
};

// Runs one FFT block on the NPU. Returns false whenever the CPU should do it
// instead (not ready yet, not enabled, failed); never blocks on a compile.
inline bool Analyze(Session& s, int n, const float* windowed, std::vector<float>& re,
                    std::vector<float>& im) {
    if (s.engine && s.engine->n != n) {
        FreeEngine(s.engine);
        s.engine = nullptr;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_builtSize = g_pending ? g_pending->n : 0;
    }
    if (!s.engine) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_pending && g_pending->n == n) {
            s.engine = g_pending;
            g_pending = nullptr;
        } else if (g_wantSize != n) {
            g_wantSize = n;
            g_cv.notify_all();
        }
        if (!s.engine) return false;
    }

    if (!s.qpcFreq) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        s.qpcFreq = f.QuadPart;
    }
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    if (!Run(s.engine, windowed, re, im)) {
        FreeEngine(s.engine);
        s.engine = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_builtSize = g_pending ? g_pending->n : 0;
            g_failedSize = n;
        }
        Notice(L"An NPU inference call failed mid-stream (driver reset or device removed?). Back on "
               L"the CPU; change any setting to retry.");
        return false;
    }
    QueryPerformanceCounter(&t1);

    LONGLONG dt = t1.QuadPart - t0.QuadPart;
    if (!s.statSince) s.statSince = t1.QuadPart;
    s.statTotal += dt;
    s.statWorst = std::max(s.statWorst, dt);
    s.statCount++;
    if (t1.QuadPart - s.statSince >= 30 * s.qpcFreq) {
        Wh_Log(L"[NPU] %u blocks in the last 30 s, mean %.3f ms, worst %.3f ms per block", s.statCount,
               (double)s.statTotal * 1000.0 / (double)s.qpcFreq / (double)s.statCount,
               (double)s.statWorst * 1000.0 / (double)s.qpcFreq);
        s.statSince = t1.QuadPart;
        s.statTotal = s.statWorst = 0;
        s.statCount = 0;
    }
    return true;
}

// Capture thread exit (pause, device change, unload): hand the engine back so
// resuming doesn't pay for another compile.
inline void EndSession(Session& s) {
    if (!s.engine) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_pending && !g_shutdown) {
        g_pending = s.engine;
    } else {
        FreeEngine(s.engine);
        g_builtSize = g_pending ? g_pending->n : 0;
    }
    s.engine = nullptr;
}

// Unload. The capture thread must already have stopped.
inline void Shutdown() {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_shutdown = true;
    }
    g_cv.notify_all();
    bool joined = true;
    if (g_worker) {
        // A compile in flight can't be cancelled; don't hold unload hostage to
        // it. The process exits right after this, which ends the thread.
        if (WaitForSingleObject(g_worker->native_handle(), 3000) == WAIT_OBJECT_0) {
            g_worker->join();
        } else {
            g_worker->detach();
            joined = false;
        }
        delete g_worker;
        g_worker = nullptr;
    }
    if (!joined) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_pending) {
        FreeEngine(g_pending);
        g_pending = nullptr;
    }
    if (g_core && g_api.core_free) {
        g_api.core_free(g_core);
        g_core = nullptr;
    }
}

}  // namespace ttnpu

class VizEndpointNotificationClient : public IMMNotificationClient {
   public:
    virtual ~VizEndpointNotificationClient() = default;

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = InterlockedDecrement(&m_ref);
        if (ref == 0) delete this;
        return ref;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
            *ppv = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole, LPCWSTR) override {
        if (flow == eRender) g_deviceChanged.store(true, std::memory_order_relaxed);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override {
        g_deviceChanged.store(true, std::memory_order_relaxed);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override {
        g_deviceChanged.store(true, std::memory_order_relaxed);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override {
        g_deviceChanged.store(true, std::memory_order_relaxed);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override {
        return S_OK;
    }

   private:
    LONG m_ref = 1;
};

bool VizInitAudioClient(IMMDeviceEnumerator* pEnum, ComPtr<IAudioClient>& pClient,
                        ComPtr<IAudioCaptureClient>& pCapture, UINT32& sampleRate,
                        UINT32& channels, bool& isFloat, HANDLE hEvent) {
    pClient.Reset();
    pCapture.Reset();

    ComPtr<IMMDevice> pDev;
    if (FAILED(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev))) return false;

    ComPtr<IAudioClient> pC;
    if (FAILED(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              (void**)pC.GetAddressOf())))
        return false;

    WAVEFORMATEX* pwfx = nullptr;
    pC->GetMixFormat(&pwfx);
    if (!pwfx) return false;

    sampleRate = pwfx->nSamplesPerSec;
    channels = pwfx->nChannels;
    isFloat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
              (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
               reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx)->SubFormat ==
                   KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    HRESULT hr = pC->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        200000, 0, pwfx, nullptr);
    CoTaskMemFree(pwfx);
    if (FAILED(hr)) return false;

    if (hEvent) pC->SetEventHandle(hEvent);

    ComPtr<IAudioCaptureClient> pCap;
    if (FAILED(pC->GetService(__uuidof(IAudioCaptureClient), (void**)pCap.GetAddressOf())))
        return false;

    if (FAILED(pC->Start())) return false;

    pClient = pC;
    pCapture = pCap;
    return true;
}

void VizCaptureThreadProc() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    BuildVizSeeds();

    ComPtr<IMMDeviceEnumerator> pEnum;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)pEnum.GetAddressOf()))) {
        g_captureRunning.store(false);
        CoUninitialize();
        return;
    }

    auto* notifyClient = new VizEndpointNotificationClient();
    bool notifyRegistered = SUCCEEDED(pEnum->RegisterEndpointNotificationCallback(notifyClient));

    ComPtr<IAudioClient> pClient;
    ComPtr<IAudioCaptureClient> pCapture;
    UINT32 sampleRate = 48000, channels = 2;
    bool isFloat = true;

    static constexpr int RING_CAP = VIZ_FFT_SIZE_MAX * 4;
    std::vector<float> ringBuf(RING_CAP, 0.f);
    int ringHead = 0, ringCount = 0;
    std::vector<float> re, im;
    ttnpu::Session npuSession;

    float bandEnv[VIZ_NUM_BANDS] = {};
    static constexpr float GRAVITY[VIZ_NUM_BANDS] = {0.018f, 0.020f, 0.022f, 0.025f,
                                                     0.030f, 0.036f, 0.042f};

    // Auto Gain state, owned entirely by this thread.
    //
    // Feed-forward, not feedback: the loudest band of each analysed block is
    // measured BEFORE any boost is applied, so the gain is derived from the
    // block's own level rather than from its own output, and cannot chase
    // itself.
    //
    // What that peak is NOT is the signal as captured. It is read from
    // rawBand[], which is already through sliderGain and the EQ multipliers, so
    // both thresholds below are levels on the drawn bars, not on the input.
    // They therefore move with Sensitivity and the EQ Preset, the same way the
    // idle test they mirror always has.
    //
    // Boost only. The sensitivity curves already handle hot signal, so this is
    // never allowed below 1.0 and cannot make an existing tuning worse at the
    // loud end.
    float agcPeakEnv = 0.f;
    float agcGain = 1.f;
    static constexpr float AGC_TARGET = 0.85f;    // where the loudest band should land
    static constexpr float AGC_ATTACK = 0.35f;    // envelope rise: fast, so peaks aren't missed
    static constexpr float AGC_RELEASE = 0.012f;  // envelope fall: slow, so quiet bars don't pump
    static constexpr float AGC_GLIDE = 0.04f;     // how fast the applied gain chases the target

    // Two floors, two jobs, deliberately not the same number.
    //
    // AGC_FLOOR is anti-windup for the tracker alone: under it the envelope
    // stops moving, so a noise floor can never wind the gain up to its ceiling
    // and there is no overshoot when audio returns.
    //
    // AGC_IDLE_AUDIBLE gates the idle-shutdown timer, and has to be the same
    // number that test compares against. Below the knee mag == rawGained, so
    // "un-boosted peak over 0.03" is what re-armed the timer before Auto Gain
    // existed. Gating on the lower floor instead would let anything in
    // [AGC_FLOOR, 0.03] hold the render loop awake once the boost carried it
    // over the line, which is quiet-but-not-silent audio keeping the mod out
    // of its idle path for as long as it plays.
    static constexpr float AGC_FLOOR = 0.010f;
    static constexpr float AGC_IDLE_AUDIBLE = 0.030f;

    // ---- Oscilloscope sampling -------------------------------------------
    //
    // The trace used to be taken once per FFT hop, over a window of exactly
    // fftSize samples. That tied two unrelated things together: the FFT size
    // sets frequency resolution for the bars, while the scope needs a time
    // base. At 8192 the consequences were that the trace updated about 12
    // times a second while the renderer drew at 144, and that it tried to show
    // 171 ms of audio in 256 points, which no waveform survives.
    //
    // It is now sampled from the capture loop, which wakes on the WASAPI event
    // roughly every 10 ms, so the update rate is constant on every FFT size.
    // The window is a fixed span of TIME taken from the newest samples, so it
    // looks the same at 1024 and 8192, and on a 44.1 kHz device as on 48 kHz.
    ULONGLONG lastAudioTick = GetTickCount64();
    bool scopeFlatPublished = false;
    static constexpr double SCOPE_HOLD_MS = 60.0;   // hold the last trace this long
    static constexpr double SCOPE_FADE_MS = 140.0;  // then fade it flat over this long

    auto PublishScope = [&]() {
        if (g_settings.shape != VizShape::Oscilloscope) return;

        // WASAPI reports true silence with a flag and no samples, so on silence
        // nothing is written to the ring at all and its contents simply stop
        // changing. Left alone, the trace freezes mid-waveform for as long as
        // the music is stopped. Fading it to flat is both what a scope with no
        // input looks like and what the rest of the mod does when audio ends.
        double sinceAudioMs = (double)(GetTickCount64() - lastAudioTick);
        float staleFade = 1.0f;
        if (sinceAudioMs > SCOPE_HOLD_MS) {
            staleFade =
                std::max(0.0f, 1.0f - (float)((sinceAudioMs - SCOPE_HOLD_MS) / SCOPE_FADE_MS));
        }
        // Once it has reached flat there is nothing left to say until audio
        // returns, so stop republishing the same empty trace.
        if (staleFade <= 0.0f && scopeFlatPublished) return;

        auto ringAt = [&](int i) -> float {
            int m = i % RING_CAP;
            if (m < 0) m += RING_CAP;
            return ringBuf[m];
        };

        int windowMs = std::clamp(g_settings.oscilloscopeWindowMs, 5, 250);
        int span = (int)((double)sampleRate * (double)windowMs / 1000.0);
        span = std::clamp(span, VIZ_WAVE_SAMPLES, RING_CAP / 2);

        // Trigger. Without one, each snapshot starts wherever the buffer
        // happens to be and the waveform slides sideways between frames. A
        // hardware scope locks to a rising zero crossing; this looks backward
        // from the window's natural start for the nearest one, which keeps the
        // window inside data we already have. If nothing crosses, which is the
        // case for silence or a DC offset, it falls back to the natural start
        // rather than hunting.
        int start = ringHead - span;
        int searchSpan = std::min(span / 2, RING_CAP - span - 1);
        for (int k = 1; k <= searchSpan; k++) {
            if (ringAt(start - k - 1) <= 0.f && ringAt(start - k) > 0.f) {
                start -= k;
                break;
            }
        }

        float waveSnap[VIZ_WAVE_SAMPLES];
        for (int w = 0; w < VIZ_WAVE_SAMPLES; w++) {
            // Peak sampling: the largest-magnitude sample this point covers,
            // sign kept. This is what an audio editor draws, and it is the only
            // choice that survives a wide window.
            //
            // Point sampling, which is what this used to do, took one sample and
            // discarded the rest, so everything above (sampleRate / step / 2)
            // folded back as alias noise that changed randomly between
            // snapshots. Averaging fixes the noise but cancels the signal: a
            // mean over nineteen samples of an oscillating waveform, which is
            // what a 100 ms window works out to, is close to zero, so the trace
            // flattens as the window widens. A peak is stable like the mean and
            // keeps amplitude like the sample.
            int from = start + (int)((long long)w * span / VIZ_WAVE_SAMPLES);
            int to = start + (int)((long long)(w + 1) * span / VIZ_WAVE_SAMPLES);
            if (to <= from) to = from + 1;

            float peak = 0.f;
            for (int s = from; s < to; s++) {
                float v = ringAt(s);
                if (fabsf(v) > fabsf(peak)) peak = v;
            }

            // agcGain is exactly 1.0 unless Auto Gain is on. Clamped because
            // nothing downstream does it: the bars are held at or under 1.0 by
            // the sensitivity curve, but the trace is drawn straight as
            // center + sample * ampScale, so an over-driven sample would be
            // plotted off the panel with no upper bound.
            waveSnap[w] = std::clamp(peak * agcGain * staleFade, -1.0f, 1.0f);
        }

        PublishWaveform(waveSnap);
        scopeFlatPublished = (staleFade <= 0.0f);
    };

    int currentFftSize = 0;
    auto ValidFftSize = [](int n) -> int {
        return (n == 1024 || n == 2048 || n == 4096 || n == 8192) ? n : 1024;
    };
    auto ApplyFftSize = [&](int newSize) {
        currentFftSize = newSize;
        BuildHannWindow(newSize);
        BuildTwiddleFactors(newSize);
        re.assign(newSize, 0.f);
        im.assign(newSize, 0.f);
        ringHead = 0;
        ringCount = 0;
        for (int b = 0; b < VIZ_NUM_BANDS; b++) {
            bandEnv[b] = 0.f;
        }
        PublishBands(bandEnv);
        BuildLogBins(sampleRate, newSize);
    };

    g_deviceChanged.store(false, std::memory_order_relaxed);
    VizInitAudioClient(pEnum.Get(), pClient, pCapture, sampleRate, channels, isFloat,
                       g_captureEvent);
    ApplyFftSize(ValidFftSize(g_settings.fftSize));

    ULONGLONG lastReinitAttempt = GetTickCount64() - 1000;

    while (g_captureRunning.load(std::memory_order_relaxed)) {
        if (g_captureEvent)
            WaitForSingleObject(g_captureEvent, 20);
        else
            Sleep(8);

        bool needsReinit = g_deviceChanged.exchange(false, std::memory_order_relaxed) || !pClient;
        if (needsReinit) {
            ULONGLONG now = GetTickCount64();
            if (now - lastReinitAttempt >= 500) {
                lastReinitAttempt = now;
                if (pClient) pClient->Stop();
                ringHead = 0;
                ringCount = 0;
                for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                    bandEnv[b] = 0.f;
                }
                PublishBands(bandEnv);
                if (VizInitAudioClient(pEnum.Get(), pClient, pCapture, sampleRate, channels,
                                       isFloat, g_captureEvent))
                    BuildLogBins(sampleRate, currentFftSize);
            }
        }

        int wantedFftSize = ValidFftSize(g_settings.fftSize);
        if (wantedFftSize != currentFftSize) {
            ApplyFftSize(wantedFftSize);
        }

        if (!pCapture) continue;

        UINT32 packetSize = 0;
        HRESULT hr = pCapture->GetNextPacketSize(&packetSize);
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
            g_deviceChanged.store(true, std::memory_order_relaxed);
            continue;
        }
        if (FAILED(hr) || packetSize == 0) {
            for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                bandEnv[b] = std::max(0.f, bandEnv[b] - GRAVITY[b]);
            }
            PublishBands(bandEnv);
            // Also on this path, otherwise the trace would never fade out: no
            // packets at all is the most common shape of silence.
            PublishScope();
            continue;
        }

        // Input Gain folded into the mono mixdown. Applying it here rather than
        // further down means it reaches the Oscilloscope waveform too, which is
        // read straight out of this ring buffer and so has never had a level
        // control of its own. It also replaces the per-frame divide by channel
        // count with a single multiply, so the feature costs slightly less than
        // what the mixdown was already doing.
        float inputGainLin = (g_settings.inputGainDb == 0.0f)
                                 ? 1.0f
                                 : powf(10.f, g_settings.inputGainDb / 20.f);
        float monoScale = inputGainLin / (float)((channels > 0) ? channels : 1);

        while (packetSize > 0) {
            BYTE* pData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;
            HRESULT hrBuf = pCapture->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
            if (hrBuf == AUDCLNT_E_DEVICE_INVALIDATED) {
                g_deviceChanged.store(true, std::memory_order_relaxed);
                break;
            }
            if (FAILED(hrBuf)) break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFrames > 0) {
                lastAudioTick = GetTickCount64();
                scopeFlatPublished = false;
                if (isFloat) {
                    float* src = reinterpret_cast<float*>(pData);
                    for (UINT32 f = 0; f < numFrames; f++) {
                        float mono = 0.f;
                        for (UINT32 c = 0; c < channels; c++) mono += src[f * channels + c];
                        mono *= monoScale;
                        ringBuf[ringHead] = mono;
                        ringHead = (ringHead + 1) % RING_CAP;
                        if (ringCount < RING_CAP) ringCount++;
                    }
                } else {
                    INT16* src = reinterpret_cast<INT16*>(pData);
                    for (UINT32 f = 0; f < numFrames; f++) {
                        float mono = 0.f;
                        for (UINT32 c = 0; c < channels; c++)
                            mono += src[f * channels + c] / 32768.f;
                        mono *= monoScale;
                        ringBuf[ringHead] = mono;
                        ringHead = (ringHead + 1) % RING_CAP;
                        if (ringCount < RING_CAP) ringCount++;
                    }
                }
            }
            pCapture->ReleaseBuffer(numFrames);
            hr = pCapture->GetNextPacketSize(&packetSize);
            if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
                g_deviceChanged.store(true, std::memory_order_relaxed);
                break;
            }
            if (FAILED(hr)) break;
        }

        // Taken once per wake of this loop, which the WASAPI event drives at
        // roughly 100 Hz regardless of FFT size, rather than once per FFT hop.
        PublishScope();

        while (ringCount >= currentFftSize) {
            int readStart = (ringHead - ringCount + RING_CAP) % RING_CAP;

            // The oscilloscope snapshot used to be taken here, once per FFT hop.
            // It is now driven from the capture loop instead: see PublishScope.

            for (int i = 0; i < currentFftSize; i++) {
                re[i] = ringBuf[(readStart + i) % RING_CAP] * g_hannWindow[i];
                im[i] = 0.f;
            }
            ringCount -= currentFftSize / 2;
            // Audio Analysis Device. The NPU path writes the same re/im bins
            // VizFFT would (it reads the windowed block out of re first), and
            // declines -- falling through to the CPU -- until its model is
            // compiled and verified, or if anything about it fails.
            bool analysedOnNpu =
                g_settings.analysisDevice == VizAnalysisDevice::Npu &&
                ttnpu::Analyze(npuSession, currentFftSize, re.data(), re, im);
            if (!analysedOnNpu) VizFFT(re, im);

            float t_sens = g_settings.sensitivity / 100.0f;
            float sliderGain = (t_sens <= 1.0f) ? 0.25f + t_sens * t_sens * 2.75f
                                                : 3.0f + (t_sens - 1.0f) * 4.0f;
            auto eq = GetVizEQMultipliers(g_settings.eq);

            static constexpr float BAND_SENSITIVITY[VIZ_NUM_BANDS] = {
                0.30f, 0.22f, 0.12f, 0.06f, 0.030f, 0.018f, 0.010f};

            // Pass one: the band levels as captured, with no auto boost applied.
            // VIZ_NUM_BANDS is 7, so holding them costs a stack array and lets the
            // gain be chosen from the whole block rather than band by band.
            float rawBand[VIZ_NUM_BANDS];
            float blockPeak = 0.f;
            for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                int bStart = g_logBinStart[b];
                int bEnd = g_logBinStart[b + 1];
                if (bEnd <= bStart) bEnd = bStart + 1;

                float sumSq = 0.f;
                int count = 0;
                for (int k = bStart; k < bEnd; k++) {
                    sumSq += re[k] * re[k] + im[k] * im[k];
                    count++;
                }
                float rms = (count > 0) ? sqrtf(sumSq / (float)count) : 0.f;
                float eqM = (VIZ_BAND_EQ_ZONE[b] == 0)   ? eq.low
                            : (VIZ_BAND_EQ_ZONE[b] == 1) ? eq.mid
                                                         : eq.high;
                float rawGained = (rms / (currentFftSize * 0.5f)) / BAND_SENSITIVITY[b] *
                                   sliderGain * eqM;
                rawBand[b] = std::max(0.f, rawGained);
                blockPeak = std::max(blockPeak, rawBand[b]);
            }

            // Auto Gain. blockPeak is the un-boosted level, so the silence test is
            // honest regardless of where the gain currently sits. Through silence
            // the envelope and the gain are both frozen rather than decayed: that
            // stops the noise floor being lifted while nothing is playing, and it
            // stops the loud overshoot that a decayed gain would produce the
            // instant music comes back.
            bool blockAudible = (blockPeak >= AGC_FLOOR);
            bool blockIdleAudible = (blockPeak >= AGC_IDLE_AUDIBLE);
            if (g_settings.autoGain) {
                if (blockAudible) {
                    float k = (blockPeak > agcPeakEnv) ? AGC_ATTACK : AGC_RELEASE;
                    agcPeakEnv += (blockPeak - agcPeakEnv) * k;

                    float maxBoost = powf(10.f, g_settings.autoGainMaxDb / 20.f);
                    float want = (agcPeakEnv > 1e-6f) ? (AGC_TARGET / agcPeakEnv) : maxBoost;
                    want = std::clamp(want, 1.f, maxBoost);
                    agcGain += (want - agcGain) * AGC_GLIDE;
                }
            } else {
                agcPeakEnv = 0.f;
                agcGain = 1.f;
            }

            // Pass two: shaping and envelope, unchanged apart from the gain factor,
            // which is exactly 1.0 whenever Auto Gain is off.
            float maxMag = 0.f;
            for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                float rawGained = rawBand[b] * agcGain;

                float mag;
                switch (g_settings.sensitivityCurve) {
                    case VizSensitivityCurve::Exponential:
                        mag = 1.f - expf(-rawGained);
                        break;
                    case VizSensitivityCurve::Power:
                        mag = std::min(1.f, powf(rawGained, 0.6f));
                        break;
                    case VizSensitivityCurve::Knee:
                    default: {
                        constexpr float knee = 0.7f;
                        mag = (rawGained <= knee)
                                  ? rawGained
                                  : knee + (1.f - knee) * tanhf((rawGained - knee) / (1.f - knee));
                        break;
                    }
                }
                mag = std::max(0.f, std::min(1.f, mag));

                bandEnv[b] = (mag >= bandEnv[b]) ? mag : std::max(0.f, bandEnv[b] - GRAVITY[b]);
                maxMag = std::max(maxMag, bandEnv[b]);
            }
            PublishBands(bandEnv);

            // Idle shutdown is judged on the un-boosted level as well as the drawn
            // one, against the same 0.03 the drawn test uses. Without the second
            // clause, Auto Gain would carry anything it could lift over 0.03 into
            // "audible" and hold the render loop awake, which is the whole power
            // story gone. Because both clauses compare against the same number,
            // this is numerically the original test: Auto Gain neither extends
            // nor shortens how long the mod stays awake. With Auto Gain off it
            // short-circuits to the original test outright.
            if (maxMag > 0.03f && (!g_settings.autoGain || blockIdleAudible)) {
                g_lastAudibleTickMs.store(GetTickCount64(), std::memory_order_relaxed);
            }

            if (g_settings.beatFlashEnabled) {
                static float prevBassEnv = 0.f;
                if (bandEnv[0] - prevBassEnv > 0.12f) {
                    g_beatPulse.store(1.0f, std::memory_order_relaxed);
                }
                prevBassEnv = bandEnv[0];
            }

            // Dominant-frequency readout: only searched when the setting is on,
            // and only over the audible range actually representable at this
            // sample rate/FFT size, so this never runs by default.
            if (g_settings.peakFreqEnabled) {
                int nyquistBin = currentFftSize / 2;
                int loBin = std::max(1, (int)(20.f * currentFftSize / (float)sampleRate));
                int hiBin = std::min(nyquistBin - 1,
                                     (int)(20000.f * currentFftSize / (float)sampleRate));
                int bestBin = -1;
                float bestMag = 0.f;
                for (int k = loBin; k <= hiBin; k++) {
                    float mag = re[k] * re[k] + im[k] * im[k];
                    if (mag > bestMag) { bestMag = mag; bestBin = k; }
                }
                if (bestBin > 0 && sqrtf(bestMag) / (currentFftSize * 0.5f) > 0.02f) {
                    float hz = (float)bestBin * (float)sampleRate / (float)currentFftSize;
                    g_dominantFreqHz.store(hz, std::memory_order_relaxed);
                }
            }
        }
    }

    if (pClient) pClient->Stop();
    if (notifyRegistered) pEnum->UnregisterEndpointNotificationCallback(notifyClient);
    notifyClient->Release();
    ttnpu::EndSession(npuSession);
    CoUninitialize();
}

void StartVizCaptureThread() {
    if (g_captureRunning.load()) return;
    if (g_captureThread) {
        if (g_captureThread->joinable()) g_captureThread->join();
        delete g_captureThread;
        g_captureThread = nullptr;
    }
    if (!g_captureEvent) g_captureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    g_captureRunning.store(true);
    g_captureThread = new std::thread(VizCaptureThreadProc);
    HANDLE hCaptureThread = g_captureThread->native_handle();
    SetThreadDescription(hCaptureThread, L"TourneTable-Capture");
}

void StopVizCaptureThread() {
    g_captureRunning.store(false);
    if (g_captureEvent) SetEvent(g_captureEvent);
    if (g_captureThread) {
        if (g_captureThread->joinable()) g_captureThread->join();
        delete g_captureThread;
        g_captureThread = nullptr;
    }
    if (g_captureEvent) {
        CloseHandle(g_captureEvent);
        g_captureEvent = nullptr;
    }
    float zeroBands[VIZ_NUM_BANDS] = {};
    PublishBands(zeroBands);
}

void UpdateVisualizerTargets() {
    const int vizBars = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));

    float bands[VIZ_NUM_BANDS];
    ReadBands(bands);
    float masterPeak = 0.f;
    for (int i = 0; i < VIZ_NUM_BANDS; i++) {
        masterPeak = std::max(masterPeak, bands[i]);
    }

    auto eq = GetVizEQMultipliers(g_settings.eq);

    auto sampleBands = [&](float t) -> float {
        float pos = t * (VIZ_NUM_BANDS - 1);
        int lo = (int)pos;
        int hi = std::min(lo + 1, VIZ_NUM_BANDS - 1);
        return bands[lo] * (1.f - (pos - (float)lo)) + bands[hi] * (pos - (float)lo);
    };
    auto eqForT = [&](float t) -> float {
        return (t < 0.33f) ? eq.low : (t < 0.66f) ? eq.mid : eq.high;
    };

    auto warpT = [&](float pos) -> float {
        pos = std::max(0.f, std::min(1.f, pos));
        switch (g_settings.freqScale) {
            case VizFreqScale::Linear: {
                float hz = 20.f + pos * (20000.f - 20.f);
                return HzToBandPos(hz) / (float)(VIZ_NUM_BANDS - 1);
            }
            case VizFreqScale::Mel: {
                auto melOf = [](float f) { return 2595.f * log10f(1.f + f / 700.f); };
                float melMin = melOf(20.f), melMax = melOf(20000.f);
                float mel = melMin + pos * (melMax - melMin);
                float hz = 700.f * (powf(10.f, mel / 2595.f) - 1.f);
                return HzToBandPos(hz) / (float)(VIZ_NUM_BANDS - 1);
            }
            default:  // Log -- matches the original band-index-linear mapping
                return pos;
        }
    };

    // Smooth Mode swaps the animation clock (see VizClockSeconds). With it
    // off, phaseOf(rate) is exactly the 1.4 expression t * rate.
    const bool smoothClock = g_smoothActive.load(std::memory_order_relaxed);
    const double clock = smoothClock ? VizClockSeconds() : 0.0;
    float t = smoothClock ? 0.f : (float)GetTickCount64() * 0.001f;
    auto phaseOf = [&](float rate) -> float {
        return smoothClock ? VizClockPhase(clock, (double)rate, 2.0 * 3.14159265358979323846)
                           : t * rate;
    };
    float center = (vizBars - 1) * 0.5f;

    for (int i = 0; i < vizBars; i++) {
        float freqT = (vizBars > 1) ? (float)i / (float)(vizBars - 1) : 0.5f;
        float target = 0.f;

        switch (g_settings.shape) {
            case VizShape::Stereo:
                target = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));
                break;
            case VizShape::Mountain: {
                float dist = fabsf((float)i - center) / std::max(1.f, center);
                float energy = sampleBands(warpT(dist)) * eqForT(warpT(dist));
                float taper = 1.6f - dist * 0.9f;
                target = std::max(0.f, std::min(1.f, (energy + masterPeak * (0.2f - dist * 0.12f)) * taper));
                break;
            }
            case VizShape::Mirror: {
                float mirT = 1.f - fabsf((float)i - center) / std::max(1.f, center);
                float energy = sampleBands(warpT(mirT)) * eqForT(warpT(mirT));
                target = std::max(0.f, std::min(1.f, (energy + masterPeak * (0.1f + mirT * 0.12f)) * 1.3f));
                break;
            }
            case VizShape::Wave: {
                float phase = (float)i * (2.f * VIZ_PI / (float)vizBars);
                float wave = 0.55f + 0.45f * sinf(phaseOf(3.5f) - phase);
                float energy = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));
                target = std::max(0.f, std::min(1.f, energy * wave + masterPeak * 0.15f));
                break;
            }
            case VizShape::Breathe: {
                if (i == 0) {
                    float k = (masterPeak > g_vizBreatheEnv) ? 0.04f : 0.015f;
                    if (smoothClock) k = VizEaseForFrame(k);
                    g_vizBreatheEnv += (masterPeak - g_vizBreatheEnv) * k;
                }
                float rate = 0.55f + VIZ_SEEDS[i % VIZ_BARS_MAX] * 0.18f;
                float inhale = 0.5f + 0.5f * sinf(phaseOf(rate) + VIZ_SEEDS[i % VIZ_BARS_MAX] * 1.2f);
                target = std::max(0.f, std::min(1.f, inhale * (0.12f + g_vizBreatheEnv * 0.88f)));
                break;
            }
            case VizShape::Dots:
                target = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));
                break;
            case VizShape::Radial:
                target = sampleBands(warpT(freqT)) * eqForT(warpT(freqT));
                break;
            case VizShape::Oscilloscope:
                target = 0.f;  // drawn directly from the raw waveform buffer, not per-bar targets
                break;
        }

        g_vizTarget[i] = std::max(0.f, std::min(1.f, target));
    }
}

struct RGBA { BYTE a, r, g, b; };

RGBA LerpColor(RGBA a, RGBA b, float t) {
    auto L = [](BYTE x, BYTE y, float tt) -> BYTE {
        return (BYTE)((int)x + (int)((float)((int)y - (int)x) * tt));
    };
    return {L(a.a, b.a, t), L(a.r, b.r, t), L(a.g, b.g, t), L(a.b, b.b, t)};
}

RGBA HSVtoRGB(float h, float s, float v, BYTE alpha) {
    h = fmodf(h, 360.f);
    if (h < 0) h += 360.f;
    float c = v * s;
    float x = c * (1.f - fabsf(fmodf(h / 60.0f, 2.f) - 1.f));
    float m = v - c;
    float r, g, b;
    if (h < 60)       { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return {alpha, (BYTE)((r + m) * 255.f), (BYTE)((g + m) * 255.f), (BYTE)((b + m) * 255.f)};
}

// ---- Drawing Device -------------------------------------------------------------
//
// The 1.4 build created its device on "whatever D3D picks", which is adapter 0.
// On a desktop that is normally the GPU the monitor hangs off, but on a laptop
// with a hybrid GPU, or a desktop with the iGPU left enabled, it can be the
// other one -- and then every frame is rendered on one GPU and copied across
// to the one that scans it out. That copy is the classic source of uneven
// frame times on integrated graphics, so Auto now asks the question directly:
// which adapter owns the output this widget is on?

bool VizAdapterIsSoftware(const DXGI_ADAPTER_DESC1& d) {
    // The flag covers WARP; the IDs cover the Basic Render Driver on systems
    // where it doesn't set the flag.
    return (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 ||
           (d.VendorId == 0x1414 && d.DeviceId == 0x8c);
}

bool VizAdapterIsIntegrated(const DXGI_ADAPTER_DESC1& d) {
    int fromDxcore = ttdxcore::IsIntegrated(d.AdapterLuid);
    if (fromDxcore >= 0) return fromDxcore == 1;
    // Pre-2004 Windows: integrated GPUs report little or no dedicated memory
    // (Intel 128 MB; AMD APUs a BIOS carve-out, usually 512 MB). An APU with a
    // bigger carve-out reads as discrete here, which only changes what Smooth
    // Mode's Auto does -- On still forces it.
    return d.DedicatedVideoMemory <= (SIZE_T)512 * 1024 * 1024;
}

bool VizLuidEqual(const LUID& a, const LUID& b) {
    return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
}

// The hardware adapter with an output on `monitor`, or null.
ComPtr<IDXGIAdapter1> VizAdapterForMonitor(IDXGIFactory1* factory, HMONITOR monitor) {
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;
        if (!adapter) continue;
        DXGI_ADAPTER_DESC1 d;
        if (FAILED(adapter->GetDesc1(&d)) || VizAdapterIsSoftware(d)) continue;
        for (UINT j = 0;; j++) {
            ComPtr<IDXGIOutput> output;
            if (adapter->EnumOutputs(j, &output) == DXGI_ERROR_NOT_FOUND) break;
            if (!output) continue;
            DXGI_OUTPUT_DESC od;
            if (SUCCEEDED(output->GetDesc(&od)) && od.Monitor == monitor) return adapter;
        }
    }
    return nullptr;
}

// The first integrated (or discrete) hardware adapter, in the order Windows
// itself uses for its power-saving / high-performance GPU preference.
ComPtr<IDXGIAdapter1> VizAdapterOfKind(IDXGIFactory1* factory, bool wantIntegrated) {
    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))) && factory6) {
        DXGI_GPU_PREFERENCE pref = wantIntegrated ? DXGI_GPU_PREFERENCE_MINIMUM_POWER
                                                  : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        for (UINT i = 0;; i++) {
            ComPtr<IDXGIAdapter1> adapter;
            if (FAILED(factory6->EnumAdapterByGpuPreference(i, pref, IID_PPV_ARGS(&adapter))) ||
                !adapter)
                break;
            DXGI_ADAPTER_DESC1 d;
            if (FAILED(adapter->GetDesc1(&d)) || VizAdapterIsSoftware(d)) continue;
            if (VizAdapterIsIntegrated(d) == wantIntegrated) return adapter;
        }
        return nullptr;
    }
    // Windows 10 before 1803: no preference ordering, so classify each one.
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;
        if (!adapter) continue;
        DXGI_ADAPTER_DESC1 d;
        if (FAILED(adapter->GetDesc1(&d)) || VizAdapterIsSoftware(d)) continue;
        if (VizAdapterIsIntegrated(d) == wantIntegrated) return adapter;
    }
    return nullptr;
}

struct VizAdapterChoice {
    ComPtr<IDXGIAdapter1> adapter;  // null with warp=false means "let D3D pick"
    bool warp = false;
    std::wstring warning;           // set when the request couldn't be honoured
};

HMONITOR VizTargetMonitor() {
    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    return monitor;
}

VizAdapterChoice VizPickRenderAdapter(IDXGIFactory1* factory, VizRenderDevice want,
                                      HMONITOR monitor) {
    VizAdapterChoice choice;
    if (want == VizRenderDevice::Cpu) {
        choice.warp = true;
        return choice;
    }
    if (want == VizRenderDevice::Integrated || want == VizRenderDevice::Discrete) {
        bool integrated = (want == VizRenderDevice::Integrated);
        choice.adapter = VizAdapterOfKind(factory, integrated);
        if (!choice.adapter) {
            choice.warning =
                integrated
                    ? L"No integrated GPU was found, so drawing is on Auto instead. On a desktop "
                      L"the iGPU is often switched off in the BIOS, and F / KF processors don't "
                      L"have one at all."
                    : L"No discrete GPU was found, so drawing is on Auto instead.";
        }
    }
    if (!choice.adapter) choice.adapter = VizAdapterForMonitor(factory, monitor);
    return choice;
}

void VizUpdateSmoothActive() {
    bool on = g_settings.smoothMode == VizSmoothMode::On ||
              (g_settings.smoothMode == VizSmoothMode::Auto && g_renderAdapter.valid &&
               (g_renderAdapter.integrated || g_renderAdapter.warp));
    bool was = g_smoothActive.exchange(on);
    if (was != on) Wh_Log(L"[Hardware] Smooth Mode %s", on ? L"on" : L"off");
}

bool InitDirectX() {
    HRESULT hr;

    // The factory comes first now: picking an adapter needs it.
    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&g_dxgiFactory));
    if (FAILED(hr)) return false;

    VizAdapterChoice choice =
        VizPickRenderAdapter(g_dxgiFactory.Get(), g_settings.renderDevice, VizTargetMonitor());

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (choice.warp) {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0,
                               D3D11_SDK_VERSION, &g_d3dDevice, nullptr, nullptr);
    } else if (choice.adapter) {
        // An explicit adapter requires DRIVER_TYPE_UNKNOWN.
        hr = D3D11CreateDevice(choice.adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
                               nullptr, 0, D3D11_SDK_VERSION, &g_d3dDevice, nullptr, nullptr);
    } else {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0,
                               D3D11_SDK_VERSION, &g_d3dDevice, nullptr, nullptr);
    }
    if (FAILED(hr) && !choice.warp) {
        // Last resort rather than no visualizer at all: a GPU in the middle of
        // a driver update, or a remote session without one.
        Wh_Log(L"D3D11CreateDevice on hardware failed: 0x%08X, falling back to WARP", hr);
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0,
                               D3D11_SDK_VERSION, &g_d3dDevice, nullptr, nullptr);
        if (SUCCEEDED(hr)) {
            if (!choice.warning.empty()) choice.warning += L"\r\n      ";
            choice.warning += L"The GPU wouldn't create a device, so drawing fell back to the CPU.";
        }
    }
    if (FAILED(hr)) {
        Wh_Log(L"D3D11CreateDevice failed: 0x%08X", hr);
        return false;
    }

    hr = g_d3dDevice.As(&g_dxgiDevice);
    if (FAILED(hr)) return false;

    {
        ComPtr<IDXGIDevice1> dxgiDevice1;
        if (SUCCEEDED(g_dxgiDevice.As(&dxgiDevice1)) && dxgiDevice1) {
            dxgiDevice1->SetMaximumFrameLatency(1);
        }
    }

    // Record what the device actually landed on.
    g_renderAdapter = VizRenderAdapterInfo();
    {
        ComPtr<IDXGIAdapter> used;
        ComPtr<IDXGIAdapter1> used1;
        DXGI_ADAPTER_DESC1 d;
        if (SUCCEEDED(g_dxgiDevice->GetAdapter(&used)) && used && SUCCEEDED(used.As(&used1)) &&
            SUCCEEDED(used1->GetDesc1(&d))) {
            g_renderAdapter.valid = true;
            g_renderAdapter.luid = d.AdapterLuid;
            g_renderAdapter.name = d.Description;
            g_renderAdapter.warp = VizAdapterIsSoftware(d);
            g_renderAdapter.integrated = !g_renderAdapter.warp && VizAdapterIsIntegrated(d);
        }
    }
    Wh_Log(L"[Hardware] drawing on %s (%s)",
           g_renderAdapter.name.empty() ? L"unknown adapter" : g_renderAdapter.name.c_str(),
           g_renderAdapter.warp         ? L"CPU / WARP"
           : g_renderAdapter.integrated ? L"integrated GPU"
                                        : L"discrete GPU");
    if (g_renderAdapter.valid && !g_renderAdapter.warp) {
        ComPtr<IDXGIAdapter1> owner = VizAdapterForMonitor(g_dxgiFactory.Get(), VizTargetMonitor());
        DXGI_ADAPTER_DESC1 od;
        if (owner && SUCCEEDED(owner->GetDesc1(&od)) &&
            !VizLuidEqual(od.AdapterLuid, g_renderAdapter.luid)) {
            Wh_Log(L"[Hardware] note: the monitor is driven by %s, so each frame is copied across "
                   L"GPUs. Drawing Device = Auto avoids that.",
                   od.Description);
        }
    }
    if (!choice.warning.empty()) {
        ReportSettingWarning(L"Hardware", L"Drawing Device", choice.warning);
        FlushSettingsIssues();
    }
    VizUpdateSmoothActive();

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&g_d2dFactory));
    if (FAILED(hr)) return false;

    hr = g_d2dFactory->CreateDevice(g_dxgiDevice.Get(), &g_d2dDevice);
    if (FAILED(hr)) return false;

    D2D1_STROKE_STYLE_PROPERTIES capProps = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND);
    g_d2dFactory->CreateStrokeStyle(&capProps, nullptr, 0, &g_roundCapStrokeStyle);

    // Device-independent, so a device rebuild keeps it.
    if (!g_dwriteFactory) {
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                            (IUnknown**)g_dwriteFactory.GetAddressOf());
    }

    return true;
}

// Would the current settings put the device on a different adapter than the
// one it is on? Asked on settings and display changes; a fresh factory, so a
// GPU that was just plugged in, enabled or removed is seen.
bool VizRenderDeviceNeedsRebuild() {
    if (!g_d3dDevice) return false;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) return false;
    VizAdapterChoice choice =
        VizPickRenderAdapter(factory.Get(), g_settings.renderDevice, VizTargetMonitor());
    if (choice.warp != g_renderAdapter.warp) return true;
    if (choice.warp || !choice.adapter) return false;
    DXGI_ADAPTER_DESC1 d;
    if (FAILED(choice.adapter->GetDesc1(&d))) return false;
    return !g_renderAdapter.valid || !VizLuidEqual(d.AdapterLuid, g_renderAdapter.luid);
}

void ReleaseSwapChainResources();
bool CreateSwapChainResources();
void RenderVisualizer();

// Tears the whole D3D/D2D stack down and builds it again on whatever adapter
// the settings now resolve to. UI thread only. Used for a Drawing Device
// change and for recovering from a lost device (driver update, TDR, a hybrid
// laptop switching GPUs), which the 1.4 build had no answer to -- the
// visualizer simply froze until the mod was restarted.
void VizRebuildRenderDevice(PCWSTR reason) {
    Wh_Log(L"[Hardware] rebuilding the render device: %s", reason);
    g_lastDeviceRebuildTick = GetTickCount64();

    ReleaseSwapChainResources();
    g_roundCapStrokeStyle.Reset();
    g_d2dDevice.Reset();
    g_d2dFactory.Reset();
    g_dxgiDevice.Reset();
    g_d3dDevice.Reset();
    g_dxgiFactory.Reset();

    if (!InitDirectX()) {
        Wh_Log(L"[Hardware] InitDirectX failed during rebuild");
        return;
    }
    if (g_overlayWnd && CreateSwapChainResources() && !g_fullscreenPaused.load()) {
        RenderVisualizer();
    }
}

// Called with the results of a frame's EndDraw and Present. Device loss shows
// up there first; the rebuild itself is posted rather than run inline, since
// this is called from inside the render path.
void VizCheckDeviceLost(HRESULT hrEndDraw, HRESULT hrPresent) {
    bool lost = hrEndDraw == D2DERR_RECREATE_TARGET || hrPresent == DXGI_ERROR_DEVICE_REMOVED ||
                hrPresent == DXGI_ERROR_DEVICE_RESET;
    if (!lost || !g_messageWnd) return;
    if (!g_deviceRebuildQueued.exchange(true)) {
        HRESULT reason = g_d3dDevice ? g_d3dDevice->GetDeviceRemovedReason() : S_OK;
        Wh_Log(L"[Hardware] device lost (EndDraw 0x%08X, Present 0x%08X, reason 0x%08X)", hrEndDraw,
               hrPresent, reason);
        PostMessage(g_messageWnd, WM_APP_REBUILD_DEVICE, 1, 0);
    }
}

void UninitDirectX() {
    g_dwriteTextFormat.Reset();
    g_dwriteFactory.Reset();
    g_roundCapStrokeStyle.Reset();
    g_d2dDevice.Reset();
    g_d2dFactory.Reset();
    g_dxgiFactory.Reset();
    g_dxgiDevice.Reset();
    g_d3dDevice.Reset();
    g_dwriteTextFormatFontSize = -1;
    g_dwriteTextFormatFontName.clear();
}

bool RecreateVisualResources();

// Geometry of the visualizer, resolved once and used both to size the swap
// chain and to draw into it.
//
// The swap chain used to span the entire desktop even though the widget
// occupies a thin strip of it, which meant every frame cleared and presented
// millions of untouched pixels and held two full-screen buffers in VRAM. It is
// now sized to the widget's bounding box, with the composition visual offset to
// position it -- so drawing happens in local coordinates starting at the box.
struct VizLayout {
    float originX, originY;         // virtual-screen coords of the box's top-left
    UINT  width, height;            // swap chain size, in pixels
    float blockX, blockY;           // bar-group origin, in LOCAL coords
    float totalWidth, totalHeight;  // bar-group extent
    float textSide;                 // horizontal room reserved for overlay text
    float textAnchorSide;           // where text hangs off the bars; offset-independent
    float textTop;                  // vertical room reserved above the bars
    float textBottom;               // vertical room reserved below the bars
};

// ---- Drag-to-move ----------------------------------------------------------
// Ctrl (or whichever modifier/button is configured) + a click-drag over the
// visualizer repositions it. Windhawk mods can only READ settings, not write
// them back, so a dragged position can't land in the Position % fields the
// settings UI shows -- instead it's kept as a runtime override here (which
// ComputeVizLayout consults ahead of the Position settings) and persisted via
// Wh_SetStringValue so it survives restarts silently. Settings are read-only
// to a mod, but mod-owned values are not, and Windhawk removes them with the
// mod rather than leaving anything behind.
std::atomic<bool> g_dragOverrideActive{false};
std::atomic<float> g_dragOverrideH{50.0f};
std::atomic<float> g_dragOverrideV{50.0f};
// True only while a drag is actively in progress -- tells the render path to
// freeze the bars and show just the background/border box moving.
std::atomic<bool> g_dragRenderPauseActive{false};

// Live keyboard nudges for the two text overlays, in physical pixels. Unlike
// the visualizer's position -- where the override replaces the Position
// percentages outright -- these are a delta *on top of* the Offset settings, so
// a value typed into settings is never silently ignored because a nudge exists.
// A delta of zero means "no nudge", which needs no separate active flag.
std::atomic<int> g_npNudgeX{0}, g_npNudgeY{0};
std::atomic<int> g_pfNudgeX{0}, g_pfNudgeY{0};

float EffectiveNowPlayingOffsetX() {
    return (float)g_settings.nowPlayingOffsetX * g_dpiScale +
           (float)g_npNudgeX.load(std::memory_order_relaxed);
}
float EffectiveNowPlayingOffsetY() {
    return (float)g_settings.nowPlayingOffsetY * g_dpiScale +
           (float)g_npNudgeY.load(std::memory_order_relaxed);
}
float EffectivePeakFreqOffsetX() {
    return (float)g_settings.peakFreqOffsetX * g_dpiScale +
           (float)g_pfNudgeX.load(std::memory_order_relaxed);
}
float EffectivePeakFreqOffsetY() {
    return (float)g_settings.peakFreqOffsetY * g_dpiScale +
           (float)g_pfNudgeY.load(std::memory_order_relaxed);
}

// Runtime placements go through Wh_SetStringValue / Wh_GetStringValue, which
// are per-mod, work on portable Windhawk, and are removed along with the mod.
//
// v1.2.0 also carried a one-shot import of a file the pre-catalog builds kept
// under %LOCALAPPDATA%. Nothing in the catalog ever wrote that file, so it has
// been removed here as agreed during the 1.2.0 review.
void LoadPositionOverride() {
    WCHAR buf[192] = {};
    Wh_GetStringValue(L"positionOverride", buf, ARRAYSIZE(buf));
    if (!buf[0]) return;

    // Two floats is the original format; the four trailing ints carrying the
    // text nudges came later, so a value written by an older build still parses
    // and simply leaves the nudges at zero. A negative percentage is the
    // sentinel for "no position override" -- needed now that the record can
    // exist purely to hold text nudges, and an old one never contains one.
    float hPct = -1.0f, vPct = -1.0f;
    int npX = 0, npY = 0, pfX = 0, pfY = 0;
    float mediaH = -1.0f, mediaV = -1.0f;
    int n = swscanf_s(buf, L"%f %f %d %d %d %d %f %f", &hPct, &vPct, &npX, &npY, &pfX, &pfY,
                      &mediaH, &mediaV);
    if (n >= 2 && hPct >= 0.0f && vPct >= 0.0f) {
        g_dragOverrideH.store(std::clamp(hPct, 0.0f, 100.0f), std::memory_order_relaxed);
        g_dragOverrideV.store(std::clamp(vPct, 0.0f, 100.0f), std::memory_order_relaxed);
        g_dragOverrideActive.store(true, std::memory_order_relaxed);
    }
    if (n >= 6) {
        g_npNudgeX.store(npX, std::memory_order_relaxed);
        g_npNudgeY.store(npY, std::memory_order_relaxed);
        g_pfNudgeX.store(pfX, std::memory_order_relaxed);
        g_pfNudgeY.store(pfY, std::memory_order_relaxed);
    }
    if (n >= 8 && mediaH >= 0.0f && mediaV >= 0.0f) {
        g_mediaOverrideH.store(std::clamp(mediaH, 0.0f, 100.0f), std::memory_order_relaxed);
        g_mediaOverrideV.store(std::clamp(mediaV, 0.0f, 100.0f), std::memory_order_relaxed);
        g_mediaOverrideActive.store(true, std::memory_order_relaxed);
    }
}

// One value holds all three runtime placements, so any change rewrites the
// whole record rather than touching a field -- and when everything is back to
// its default the value is cleared outright, so nothing lingers to be reloaded.
void PersistOverrideState() {
    bool posActive = g_dragOverrideActive.load(std::memory_order_relaxed);
    bool mediaActive = g_mediaOverrideActive.load(std::memory_order_relaxed);
    int npX = g_npNudgeX.load(std::memory_order_relaxed);
    int npY = g_npNudgeY.load(std::memory_order_relaxed);
    int pfX = g_pfNudgeX.load(std::memory_order_relaxed);
    int pfY = g_pfNudgeY.load(std::memory_order_relaxed);

    if (!posActive && !mediaActive && !npX && !npY && !pfX && !pfY) {
        Wh_SetStringValue(L"positionOverride", L"");
        return;
    }

    WCHAR buf[192];
    int len = swprintf_s(buf, L"%.4f %.4f %d %d %d %d %.4f %.4f",
                         posActive ? g_dragOverrideH.load(std::memory_order_relaxed) : -1.0f,
                         posActive ? g_dragOverrideV.load(std::memory_order_relaxed) : -1.0f,
                         npX, npY, pfX, pfY,
                         mediaActive ? g_mediaOverrideH.load(std::memory_order_relaxed) : -1.0f,
                         mediaActive ? g_mediaOverrideV.load(std::memory_order_relaxed) : -1.0f);
    if (len > 0) Wh_SetStringValue(L"positionOverride", buf);
}

void RequestPositionOverrideSave();  // defined with the keyboard-nudge helpers

// Both callers reach this from a low-level input hook: Ctrl+Alt+Home through
// ResetMoveTarget, and the double-click-in-place path in DragMouseHookProc.
// Everything in a WH_KEYBOARD_LL / WH_MOUSE_LL callback blocks all system input
// until it returns, so the write goes through the same deferred save the nudge
// paths already use rather than calling Wh_SetStringValue inline.
void ClearPositionOverride() {
    g_dragOverrideActive.store(false, std::memory_order_relaxed);
    RequestPositionOverrideSave();
}

UINT DragButtonDownMsg() {
    switch (g_settings.dragButton) {
        case VizDragButton::Left:  return WM_LBUTTONDOWN;
        case VizDragButton::Right: return WM_RBUTTONDOWN;
        default:                   return WM_MBUTTONDOWN;
    }
}

UINT DragButtonUpMsg() {
    switch (g_settings.dragButton) {
        case VizDragButton::Left:  return WM_LBUTTONUP;
        case VizDragButton::Right: return WM_RBUTTONUP;
        default:                   return WM_MBUTTONUP;
    }
}

bool DragModifierHeld() {
    switch (g_settings.dragModifier) {
        case VizDragModifier::None:  return true;
        case VizDragModifier::Ctrl:  return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        case VizDragModifier::Alt:   return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        case VizDragModifier::Shift: return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        case VizDragModifier::Win:   return (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
                                            (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
    }
    return false;
}

// Hit-tests against the bounds of the last drawn frame (already tracked for
// the occlusion check), so "over the visualizer" means the box as it's
// actually being shown right now, including mid-drag.
bool PointInVisualizerBounds(POINT pt) {
    if (!g_drawRectValid.load(std::memory_order_relaxed)) return false;
    LONG l = g_drawRectL.load(std::memory_order_relaxed);
    LONG t = g_drawRectT.load(std::memory_order_relaxed);
    LONG r = g_drawRectR.load(std::memory_order_relaxed);
    LONG b = g_drawRectB.load(std::memory_order_relaxed);
    return pt.x >= l && pt.x < r && pt.y >= t && pt.y < b;
}

bool ComputeVizLayout(VizLayout* out) {
    if (!out) return false;

    int barCount  = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
    float barW    = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    float barGap  = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;

    bool horizontal = (g_settings.orientation == VizOrientation::Horizontal);
    float barsThickness = barCount * barW + (barCount - 1) * barGap;

    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }

    HMONITOR monitor = g_cachedMonitor;
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{.cbSize = sizeof(mi)};
    if (!GetMonitorInfo(monitor, &mi)) return false;

    int vsx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vsy = GetSystemMetrics(SM_YVIRTUALSCREEN);

    float waLeft   = (float)(mi.rcWork.left   - vsx);
    float waTop    = (float)(mi.rcWork.top    - vsy);
    float workWidth  = (float)(mi.rcWork.right  - mi.rcWork.left);
    float workHeight = (float)(mi.rcWork.bottom - mi.rcWork.top);

    bool dragOverride = g_dragOverrideActive.load(std::memory_order_relaxed);
    float hPercent = dragOverride ? g_dragOverrideH.load(std::memory_order_relaxed)
                                   : g_settings.horizontalPosition;
    float vPercent = dragOverride ? g_dragOverrideV.load(std::memory_order_relaxed)
                                   : g_settings.verticalPosition;

    float blockX = waLeft + (workWidth  - totalWidth)  * (hPercent / 100.0f);
    float blockY = waTop  + (workHeight - totalHeight) * (vPercent / 100.0f);

    float padL = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingL * g_dpiScale : 0.f;
    float padR = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingR * g_dpiScale : 0.f;
    float padT = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingT * g_dpiScale : 0.f;
    float padB = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingB * g_dpiScale : 0.f;

    // Safety margin so nothing gets clipped at the edges of the smaller target:
    // stroked shapes (oscilloscope, radial) centre their line on the path and so
    // bleed half a stroke outward, and the radial shape's spokes reach further
    // than its nominal box (inner radius plus full bar length).
    float margin = std::max(4.0f * g_dpiScale, barW);
    if (g_settings.shape == VizShape::Radial) margin += maxSize * 0.15f;

    // Text overlays can sit outside the bars, so the box reserves room for them.
    // Without this they fall outside the render surface and get clipped away.
    float fontPx = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale;
    float textTop = 0.f, textBottom = 0.f, textSide = 0.f;

    // textAnchorSide is where the draw code hangs the text off the bar group and
    // must not move when an offset changes -- otherwise nudging the Now Playing
    // text sideways would drag the frequency readout along with it, since both
    // anchor off the same number. textSide is the room actually reserved on the
    // render surface, which does have to grow by the offsets or offset text
    // simply walks off the surface and disappears.
    float textAnchorSide = 0.f;
    float extraSide = 0.f;

    // Reserved room is rounded up to a coarse step rather than tracking the
    // offset exactly. The swap chain is sized from this, and a keyboard nudge
    // is one pixel per press with key repeat behind it -- following every pixel
    // would mean a DXGI buffer resize dozens of times a second while someone is
    // simply holding an arrow key down.
    auto reserveFor = [](float offset) { return std::ceil(offset / 32.0f) * 32.0f; };

    // A text panel grows past the text by its padding and border, so that has to
    // be reserved too or the panel gets clipped at the edge of the surface while
    // the text inside it stays visible.
    float npPanel = (float)(g_settings.npBgPadding + g_settings.npBgBorderSize) * g_dpiScale;
    float pfPanel = (float)(g_settings.pfBgPadding + g_settings.pfBgBorderSize) * g_dpiScale;

    if (g_settings.nowPlayingEnabled) {
        float npOffX = reserveFor(std::abs(EffectiveNowPlayingOffsetX()) + npPanel);
        float npOffY = reserveFor(std::abs(EffectiveNowPlayingOffsetY()) + npPanel);
        textTop    = std::max(textTop, fontPx * 1.6f + 8.0f * g_dpiScale + npOffY);
        textBottom = std::max(textBottom, npOffY);
        textAnchorSide = std::max(textAnchorSide, 100.0f * g_dpiScale);
        extraSide  = std::max(extraSide, npOffX);
    }
    if (g_settings.peakFreqEnabled) {
        float pfOffX = reserveFor(std::abs(EffectivePeakFreqOffsetX()) + pfPanel);
        float pfOffY = reserveFor(std::abs(EffectivePeakFreqOffsetY()) + pfPanel);
        float pfH = fontPx * 1.4f + 4.0f * g_dpiScale;
        if (g_settings.peakFreqAlignV == VizTextAlignV::Above)
            textTop = std::max(textTop, pfH + pfOffY);
        else if (g_settings.peakFreqAlignV == VizTextAlignV::Below)
            textBottom = std::max(textBottom, pfH + pfOffY);
        else {
            // Inside placements still need room once an offset can push them
            // past the bars in either direction.
            textTop    = std::max(textTop, pfOffY);
            textBottom = std::max(textBottom, pfOffY);
        }
        textAnchorSide = std::max(textAnchorSide, 60.0f * g_dpiScale);
        extraSide = std::max(extraSide, pfOffX);
    }

    textSide = textAnchorSide + extraSide;

    float insetL = padL + margin + textSide;
    float insetT = padT + margin + textTop;
    float insetR = padR + margin + textSide;
    float insetB = padB + margin + textBottom;

    float rawOriginX = blockX - insetL;
    float rawOriginY = blockY - insetT;

    // The composition visual's offset MUST land on whole pixels. A fractional
    // offset makes DirectComposition resample the entire surface bilinearly,
    // which softens every edge and reads as an unwanted blur -- even with the
    // blur effect switched off entirely.
    //
    // The offset is floored to an integer and the leftover fraction is folded
    // back into the local drawing origin, so the widget still lands exactly
    // where the position settings ask for, without resampling the surface.
    out->originX = floorf(rawOriginX);
    out->originY = floorf(rawOriginY);

    float fracX = rawOriginX - out->originX;
    float fracY = rawOriginY - out->originY;

    out->blockX      = insetL + fracX;   // local coords
    out->blockY      = insetT + fracY;
    out->totalWidth  = totalWidth;
    out->totalHeight = totalHeight;
    out->textSide       = textSide;
    out->textAnchorSide = textAnchorSide;
    out->textTop        = textTop;
    out->textBottom     = textBottom;

    // +1 px of slack absorbs the sub-pixel offset folded in above.
    out->width  = (UINT)std::max(1.0f, ceilf(totalWidth  + insetL + insetR) + 1.0f);
    out->height = (UINT)std::max(1.0f, ceilf(totalHeight + insetT + insetB) + 1.0f);
    return true;
}

bool CreateSwapChainResources() {
    HRESULT hr;

    // Monitor and DPI must be resolved before the layout, since bar sizes are
    // DPI-scaled and the box is positioned against the monitor's work area.
    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    g_cachedMonitor = monitor;
    g_dpiScale = GetMonitorDpiScale(monitor);

    VizLayout layout;
    if (!ComputeVizLayout(&layout)) return false;

    DXGI_SWAP_CHAIN_DESC1 scd = {};
    scd.Width = layout.width;
    scd.Height = layout.height;
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.Scaling = DXGI_SCALING_STRETCH;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    hr = g_dxgiFactory->CreateSwapChainForComposition(g_dxgiDevice.Get(), &scd, nullptr,
                                                      &g_swapChain);
    if (FAILED(hr)) return false;

    hr = g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_dc);
    if (FAILED(hr)) return false;

    // Sprite batches arrived with Windows 10 1607. Without them Smooth Mode
    // still paces and bakes the background; bars just draw the 1.4 way.
    g_dc3.Reset();
    g_spriteBatch.Reset();
    if (SUCCEEDED(g_dc.As(&g_dc3)) && g_dc3) {
        if (FAILED(g_dc3->CreateSpriteBatch(&g_spriteBatch))) {
            g_spriteBatch.Reset();
            g_dc3.Reset();
        }
    }

    ComPtr<IDXGISurface2> surface;
    hr = g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = {};
    bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmapProperties.bitmapOptions =
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ComPtr<ID2D1Bitmap1> targetBitmap;
    hr = g_dc->CreateBitmapFromDxgiSurface(surface.Get(), bitmapProperties, &targetBitmap);
    if (FAILED(hr)) return false;

    g_dc->SetTarget(targetBitmap.Get());

    hr = DCompositionCreateDevice(g_dxgiDevice.Get(), IID_PPV_ARGS(&g_compositionDevice));
    if (FAILED(hr)) return false;

    hr = g_compositionDevice->CreateTargetForHwnd(g_overlayWnd, TRUE, &g_compositionTarget);
    if (FAILED(hr)) return false;

    hr = g_compositionDevice->CreateVisual(&g_compositionVisual);
    if (FAILED(hr)) return false;

    hr = g_compositionVisual->SetContent(g_swapChain.Get());
    if (FAILED(hr)) return false;

    // Position the (now much smaller) swap chain where the widget belongs.
    // The overlay window spans the whole virtual desktop starting at its origin,
    // so window-client coordinates match the virtual-screen-relative coordinates
    // the layout produces.
    g_compositionVisual->SetOffsetX(layout.originX);
    g_compositionVisual->SetOffsetY(layout.originY);

    hr = g_compositionTarget->SetRoot(g_compositionVisual.Get());
    if (FAILED(hr)) return false;

    hr = g_compositionDevice->Commit();
    if (FAILED(hr)) return false;

    g_swapChainWidth  = layout.width;
    g_swapChainHeight = layout.height;
    g_visualOffsetX   = layout.originX;
    g_visualOffsetY   = layout.originY;

    return RecreateVisualResources();
}

// Re-sizes and re-positions the swap chain when the widget's bounding box
// changes (settings edits, display changes). Does nothing when the box is
// unchanged, so it is cheap to call defensively.
void UpdateSwapChainForLayout() {
    if (!g_swapChain || !g_dc || !g_compositionVisual) return;

    VizLayout layout;
    if (!ComputeVizLayout(&layout)) return;

    bool sizeChanged = (layout.width != g_swapChainWidth || layout.height != g_swapChainHeight);
    bool moved = (fabsf(layout.originX - g_visualOffsetX) > 0.5f ||
                  fabsf(layout.originY - g_visualOffsetY) > 0.5f);
    if (!sizeChanged && !moved) return;

    if (sizeChanged) {
        g_dc->SetTarget(nullptr);
        if (FAILED(g_swapChain->ResizeBuffers(0, layout.width, layout.height,
                                              DXGI_FORMAT_UNKNOWN, 0)))
            return;

        ComPtr<IDXGISurface2> surface;
        if (FAILED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface)))) return;

        D2D1_BITMAP_PROPERTIES1 bp = {};
        bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        ComPtr<ID2D1Bitmap1> targetBitmap;
        if (FAILED(g_dc->CreateBitmapFromDxgiSurface(surface.Get(), bp, &targetBitmap))) return;

        g_dc->SetTarget(targetBitmap.Get());
        g_swapChainWidth  = layout.width;
        g_swapChainHeight = layout.height;
    }

    if (moved) {
        g_compositionVisual->SetOffsetX(layout.originX);
        g_compositionVisual->SetOffsetY(layout.originY);
        g_visualOffsetX = layout.originX;
        g_visualOffsetY = layout.originY;
    }

    if (g_compositionDevice) g_compositionDevice->Commit();

    // The cached blur is baked for one specific box, so a box that has moved or
    // resized leaves it showing the wrong slice of wallpaper. Re-bake via the
    // existing wallpaper-refresh timer rather than inline, since re-capturing
    // involves a present and a DwmFlush.
    if (g_messageWnd && g_settings.backgroundEnabled && g_settings.bgBlur > 0) {
        SetTimer(g_messageWnd, TIMER_ID_MSG_WALLPAPER_REFRESH, 200, nullptr);
    }
}

// Session state for an in-progress drag. Touched only from the low-level mouse
// hook, which only ever runs on the UI thread that installed it -- no locking
// needed here, unlike the atomics above that the render thread also reads.
bool g_dragInProgress = false;
bool g_dragMoved = false;
POINT g_dragStartCursor{};
float g_dragStartH = 50.0f, g_dragStartV = 50.0f;
ULONGLONG g_dragLastClickTick = 0;
POINT g_dragLastClickPos{};

void BeginDrag(POINT pt) {
    g_dragInProgress = true;
    g_dragMoved = false;
    g_dragStartCursor = pt;
    g_dragStartH = g_dragOverrideActive.load(std::memory_order_relaxed)
                       ? g_dragOverrideH.load(std::memory_order_relaxed)
                       : g_settings.horizontalPosition;
    g_dragStartV = g_dragOverrideActive.load(std::memory_order_relaxed)
                       ? g_dragOverrideV.load(std::memory_order_relaxed)
                       : g_settings.verticalPosition;
    g_dragRenderPauseActive.store(true, std::memory_order_relaxed);

    Wh_Log(L"[Drag] BEGIN cursor=(%d,%d) startH=%.2f startV=%.2f",
           pt.x, pt.y, g_dragStartH, g_dragStartV);
}

// The distance, in physical pixels, that the visualizer's box can travel across
// the work area on each axis -- i.e. what 0%..100% of a Position setting spans.
// Mirrors the sizing math in ComputeVizLayout so a pixel delta converts to the
// same percent delta that layout will turn back into pixels.
bool GetVizTravelRange(float* travelX, float* travelY) {
    HMONITOR monitor = g_cachedMonitor;
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{.cbSize = sizeof(mi)};
    if (!GetMonitorInfo(monitor, &mi)) return false;

    int barCount = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
    float barW = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    float barGap = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;
    bool horizontal = (g_settings.orientation == VizOrientation::Horizontal);
    float barsThickness = barCount * barW + (barCount - 1) * barGap;
    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? barsThickness : maxSize;
        totalHeight = horizontal ? maxSize       : barsThickness;
    }

    *travelX = (float)(mi.rcWork.right  - mi.rcWork.left) - totalWidth;
    *travelY = (float)(mi.rcWork.bottom - mi.rcWork.top)  - totalHeight;
    return true;
}

void UpdateDrag(POINT pt) {
    if (!g_dragInProgress) return;

    if (std::abs(pt.x - g_dragStartCursor.x) > 3 || std::abs(pt.y - g_dragStartCursor.y) > 3)
        g_dragMoved = true;

    float travelX = 0.f, travelY = 0.f;
    if (!GetVizTravelRange(&travelX, &travelY)) return;

    float dxPercent = (travelX > 1.0f) ? ((float)(pt.x - g_dragStartCursor.x) / travelX) * 100.0f : 0.f;
    float dyPercent = (travelY > 1.0f) ? ((float)(pt.y - g_dragStartCursor.y) / travelY) * 100.0f : 0.f;

    g_dragOverrideH.store(std::clamp(g_dragStartH + dxPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    g_dragOverrideV.store(std::clamp(g_dragStartV + dyPercent, 0.0f, 100.0f), std::memory_order_relaxed);
    g_dragOverrideActive.store(true, std::memory_order_relaxed);

    static int s_dragLogCounter = 0;
    if ((++s_dragLogCounter % 15) == 0) {
        Wh_Log(L"[Drag] MOVE cursor=(%d,%d) travel=(%.1f,%.1f) delta%%=(%.2f,%.2f) newHV=(%.2f,%.2f) monitorCached=%d",
               pt.x, pt.y, travelX, travelY, dxPercent, dyPercent,
               g_dragOverrideH.load(std::memory_order_relaxed),
               g_dragOverrideV.load(std::memory_order_relaxed),
               g_cachedMonitor != nullptr);
    }

    // Deliberately NOT calling UpdateSwapChainForLayout() here. RenderVisualizer
    // already calls it once per tick, right before drawing -- doing it again
    // here, from the hook thread, on every single mouse-move (which can fire
    // far more often than the render tick during a fast drag) is exactly what
    // let the window's on-screen position race ahead of what actually got
    // drawn inside it.
}

void EndDrag() {
    if (!g_dragInProgress) return;
    g_dragInProgress = false;
    g_dragRenderPauseActive.store(false, std::memory_order_relaxed);

    float finalH = g_dragOverrideH.load(std::memory_order_relaxed);
    float finalV = g_dragOverrideV.load(std::memory_order_relaxed);
    Wh_Log(L"[Drag] END moved=%d finalHV=(%.2f,%.2f) overrideActive=%d",
           (int)g_dragMoved, finalH, finalV, (int)g_dragOverrideActive.load(std::memory_order_relaxed));

    if (g_dragMoved) {
        PersistOverrideState();
    }
}

// ---- Keyboard move ---------------------------------------------------------
// The precise way to place the visualizer: hold a modifier, tap a direction,
// and the box steps by an exact number of pixels. Nothing here has to track a
// moving cursor or keep up with the render loop -- each keypress is one
// discrete jump, applied to the same position override the drag path writes,
// and picked up by whatever the next frame happens to be.

// Asks the message window to (re)start a short timer that saves the override.
// Key repeat can fire dozens of nudges a second, so the write is deferred until
// the user stops moving rather than run on every keystroke.
void RequestPositionOverrideSave() {
    if (g_messageWnd) PostMessage(g_messageWnd, WM_APP_REQUEST_SAVE_POSITION, 0, 0);
}

bool ModKeysHeld(unsigned flags) {
    if (flags == VIZ_MOD_NONE) return false;
    if ((flags & VIZ_MOD_CTRL)  && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
    if ((flags & VIZ_MOD_ALT)   && !(GetAsyncKeyState(VK_MENU)    & 0x8000)) return false;
    if ((flags & VIZ_MOD_SHIFT) && !(GetAsyncKeyState(VK_SHIFT)   & 0x8000)) return false;
    if ((flags & VIZ_MOD_WIN)   && !((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
                                     (GetAsyncKeyState(VK_RWIN) & 0x8000))) return false;
    return true;
}

// Maps a virtual key to a direction, honouring which key set the user enabled.
// Returns false for anything that isn't a direction key we handle.
bool KeyMoveDirection(DWORD vk, int* dx, int* dy) {
    bool arrows = g_settings.keyMoveKeys != VizKeyMoveKeys::Wasd;
    bool wasd   = g_settings.keyMoveKeys != VizKeyMoveKeys::Arrows;

    *dx = *dy = 0;
    if (arrows) {
        switch (vk) {
            case VK_LEFT:  *dx = -1; return true;
            case VK_RIGHT: *dx =  1; return true;
            case VK_UP:    *dy = -1; return true;
            case VK_DOWN:  *dy =  1; return true;
        }
    }
    if (wasd) {
        switch (vk) {
            case 'A': *dx = -1; return true;
            case 'D': *dx =  1; return true;
            case 'W': *dy = -1; return true;
            case 'S': *dy =  1; return true;
        }
    }
    return false;
}

// Moves the visualizer by an exact pixel delta. The override is stored as a
// percentage (that's what ComputeVizLayout consumes), so the delta is converted
// through the same travel range layout uses -- which makes the round trip land
// on the pixel asked for rather than near it.
// Which piece the direction keys currently steer. Touched only from the input
// hook thread, and read by nothing else.
VizMoveTarget g_keyMoveTarget = VizMoveTarget::Visualizer;

void NudgeVisualizerPx(int dxPx, int dyPx) {
    float travelX = 0.f, travelY = 0.f;
    if (!GetVizTravelRange(&travelX, &travelY)) return;

    bool active = g_dragOverrideActive.load(std::memory_order_relaxed);
    float h = active ? g_dragOverrideH.load(std::memory_order_relaxed)
                     : g_settings.horizontalPosition;
    float v = active ? g_dragOverrideV.load(std::memory_order_relaxed)
                     : g_settings.verticalPosition;

    if (dxPx && travelX > 1.0f) h = std::clamp(h + (dxPx / travelX) * 100.0f, 0.0f, 100.0f);
    if (dyPx && travelY > 1.0f) v = std::clamp(v + (dyPx / travelY) * 100.0f, 0.0f, 100.0f);

    g_dragOverrideH.store(h, std::memory_order_relaxed);
    g_dragOverrideV.store(v, std::memory_order_relaxed);
    g_dragOverrideActive.store(true, std::memory_order_relaxed);

    // Rendering may well be idle right now -- repositioning is something people
    // do with nothing playing -- so ask for one frame explicitly instead of
    // waiting for a tick that isn't coming.
    if (g_overlayWnd) PostMessage(g_overlayWnd, WM_APP_FORCE_REDRAW, 0, 0);
    RequestPositionOverrideSave();
}

// The media strip's own travel range, mirroring the sizing math in
// RepositionAndRepaintMediaControls so a pixel delta converts to the same
// percent delta that repositioning will turn back into pixels.
bool GetMediaTravelRange(float* travelX, float* travelY) {
    float dpiScale = GetMediaControlsDpiScale();
    int size = std::max(1, (int)std::lround(g_settings.mediaIconSize * dpiScale));
    int spacing = std::max(0, (int)std::lround(g_settings.mediaIconSpacing * dpiScale));
    int pad = GetMediaPlatePaddingPx();
    int width = size * 3 + spacing * 2 + pad * 2;
    int height = size + pad * 2;

    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {sizeof(mi)};
    if (!GetMonitorInfo(monitor, &mi)) return false;

    *travelX = (float)(mi.rcWork.right - mi.rcWork.left - width);
    *travelY = (float)(mi.rcWork.bottom - mi.rcWork.top - height);
    return true;
}

// Moves the media strip by an exact pixel delta. Percentage-based like the
// visualizer rather than a pixel offset like the text overlays, because the
// strip is placed against the monitor rather than against anything else.
void NudgeMediaControlsPx(int dxPx, int dyPx) {
    float travelX = 0.f, travelY = 0.f;
    if (!GetMediaTravelRange(&travelX, &travelY)) return;

    bool active = g_mediaOverrideActive.load(std::memory_order_relaxed);
    float h = active ? g_mediaOverrideH.load(std::memory_order_relaxed)
                     : g_settings.mediaHorizontalPosition;
    float v = active ? g_mediaOverrideV.load(std::memory_order_relaxed)
                     : g_settings.mediaVerticalPosition;

    if (dxPx && travelX > 1.0f) h = std::clamp(h + (dxPx / travelX) * 100.0f, 0.0f, 100.0f);
    if (dyPx && travelY > 1.0f) v = std::clamp(v + (dyPx / travelY) * 100.0f, 0.0f, 100.0f);

    g_mediaOverrideH.store(h, std::memory_order_relaxed);
    g_mediaOverrideV.store(v, std::memory_order_relaxed);
    g_mediaOverrideActive.store(true, std::memory_order_relaxed);

    // Posted rather than called: the strip's window belongs to the UI thread
    // and repositioning it means touching UpdateLayeredWindow, which has no
    // business happening on this hook thread.
    if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
    RequestPositionOverrideSave();
}

// Nudges one of the text overlays. These move in raw pixels relative to the
// visualizer rather than as a percentage of the screen, because that's what
// "put the title just above the panel" actually means -- they're positioned
// against the box, not against the monitor.
void NudgeTextOverlayPx(VizMoveTarget target, int dxPx, int dyPx) {
    std::atomic<int>* ax = nullptr;
    std::atomic<int>* ay = nullptr;
    if (target == VizMoveTarget::NowPlaying) {
        ax = &g_npNudgeX; ay = &g_npNudgeY;
    } else if (target == VizMoveTarget::PeakFreq) {
        ax = &g_pfNudgeX; ay = &g_pfNudgeY;
    } else {
        return;
    }

    // Capped rather than unbounded: every pixel of offset widens the render
    // surface by the same amount (see ComputeVizLayout), so an accidental
    // key-repeat run shouldn't be able to inflate it without limit.
    constexpr int kMaxNudge = 4000;
    ax->store(std::clamp(ax->load(std::memory_order_relaxed) + dxPx, -kMaxNudge, kMaxNudge),
              std::memory_order_relaxed);
    ay->store(std::clamp(ay->load(std::memory_order_relaxed) + dyPx, -kMaxNudge, kMaxNudge),
              std::memory_order_relaxed);

    if (g_overlayWnd) PostMessage(g_overlayWnd, WM_APP_FORCE_REDRAW, 0, 0);
    RequestPositionOverrideSave();
}

// Zeroes the live nudge for whatever is selected. For the visualizer that means
// throwing away the whole saved position; for the text overlays it means
// dropping back to whatever their Offset settings say.
void ResetMoveTarget(VizMoveTarget target) {
    switch (target) {
        case VizMoveTarget::NowPlaying:
            g_npNudgeX.store(0, std::memory_order_relaxed);
            g_npNudgeY.store(0, std::memory_order_relaxed);
            RequestPositionOverrideSave();
            break;
        case VizMoveTarget::PeakFreq:
            g_pfNudgeX.store(0, std::memory_order_relaxed);
            g_pfNudgeY.store(0, std::memory_order_relaxed);
            RequestPositionOverrideSave();
            break;
        case VizMoveTarget::MediaControls:
            g_mediaOverrideActive.store(false, std::memory_order_relaxed);
            RequestPositionOverrideSave();
            if (g_mediaWnd) PostMessage(g_mediaWnd, WM_APP_MEDIA_REPAINT, 0, 0);
            return;  // nothing about the visualizer's own surface changed
        default:
            ClearPositionOverride();
            break;
    }
    if (g_overlayWnd) PostMessage(g_overlayWnd, WM_APP_FORCE_REDRAW, 0, 0);
}

LRESULT CALLBACK MoveKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && !g_unloading.load(std::memory_order_relaxed) &&
        g_settings.keyMoveEnabled) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isUp   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);

        if ((isDown || isUp) && ModKeysHeld(g_settings.keyMoveModifier)) {
            int dx = 0, dy = 0;
            if (KeyMoveDirection(kb->vkCode, &dx, &dy)) {
                if (isDown) {
                    bool fast = g_settings.keyMoveFastKey != VIZ_MOD_NONE &&
                                ModKeysHeld(g_settings.keyMoveFastKey);
                    int step = fast ? g_settings.keyMoveFastStep : g_settings.keyMoveStep;
                    if (g_keyMoveTarget == VizMoveTarget::Visualizer) {
                        NudgeVisualizerPx(dx * step, dy * step);
                    } else if (g_keyMoveTarget == VizMoveTarget::MediaControls) {
                        NudgeMediaControlsPx(dx * step, dy * step);
                    } else {
                        NudgeTextOverlayPx(g_keyMoveTarget, dx * step, dy * step);
                    }
                }
                // Swallow the key-up too, so an app underneath never sees a
                // release for a press it was never told about.
                return 1;
            }

            // Same combo + 1/2/3/4 picks what the direction keys steer. One
            // combo places every movable piece rather than needing a shortcut
            // each, and the choice sticks until it's changed again.
            if (kb->vkCode >= '1' && kb->vkCode <= '4') {
                if (isDown) {
                    g_keyMoveTarget = (kb->vkCode == '2') ? VizMoveTarget::NowPlaying
                                    : (kb->vkCode == '3') ? VizMoveTarget::PeakFreq
                                    : (kb->vkCode == '4') ? VizMoveTarget::MediaControls
                                                          : VizMoveTarget::Visualizer;
                    Wh_Log(L"[KeyMove] target = %s",
                           g_keyMoveTarget == VizMoveTarget::NowPlaying    ? L"Now Playing text"
                           : g_keyMoveTarget == VizMoveTarget::PeakFreq    ? L"frequency readout"
                           : g_keyMoveTarget == VizMoveTarget::MediaControls ? L"media controls"
                                                                            : L"visualizer");
                }
                return 1;
            }

            // Same combo + Home resets whatever is selected: the visualizer
            // hands placement back to the Position percentages, a text overlay
            // drops back to whatever its Offset settings say. There's no
            // settings-UI way to do this, since mods can't write settings back.
            if (kb->vkCode == VK_HOME) {
                if (isDown) {
                    ResetMoveTarget(g_keyMoveTarget);
                    Wh_Log(L"[KeyMove] reset target %d", (int)g_keyMoveTarget);
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK DragMouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && !g_unloading.load(std::memory_order_relaxed) &&
        g_settings.dragEnabled) {
        MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
        UINT downMsg = DragButtonDownMsg();
        UINT upMsg = DragButtonUpMsg();

        if (!g_dragInProgress) {
            if (wParam == downMsg && DragModifierHeld() && PointInVisualizerBounds(info->pt)) {
                BeginDrag(info->pt);
                return 1;
            }
        } else {
            if (wParam == WM_MOUSEMOVE) {
                UpdateDrag(info->pt);
                return 1;
            }
            if (wParam == upMsg) {
                POINT pt = info->pt;
                bool wasDrag = g_dragMoved;
                EndDrag();

                if (!wasDrag) {
                    // Not an actual drag -- a plain click-release of the combo.
                    // Two of those close together in place clear a saved
                    // override, since there's no settings-UI way to do it.
                    ULONGLONG now = GetTickCount64();
                    int dx = std::abs(pt.x - g_dragLastClickPos.x);
                    int dy = std::abs(pt.y - g_dragLastClickPos.y);
                    if (g_dragLastClickTick != 0 &&
                        now - g_dragLastClickTick < GetDoubleClickTime() && dx < 6 && dy < 6) {
                        // Not calling UpdateSwapChainForLayout() directly here:
                        // this hook now runs on its own dedicated thread (see
                        // InitInputHooks), so touching the swap chain from here
                        // would race with the render tick that owns it. The
                        // very next tick already re-derives layout from
                        // g_dragOverrideActive and will pick this up on its own.
                        ClearPositionOverride();
                        g_dragLastClickTick = 0;
                    } else {
                        g_dragLastClickTick = now;
                        g_dragLastClickPos = pt;
                    }
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

HHOOK g_dragMouseHook = nullptr;
HHOOK g_moveKeyboardHook = nullptr;
HANDLE g_inputHookThread = nullptr;
DWORD g_inputHookThreadId = 0;

// Both low-level input hooks run on this thread, deliberately kept away from
// the UI thread that also handles WM_APP_RENDER_TICK. Direct2D's Present() can
// block that thread for real time each frame; a hook living there has its
// queued input backed up behind every present, which is what made drag feel
// like pulling the cursor through mud. Windows also silently drops a
// low-level hook whose thread doesn't service messages promptly, so this
// thread does nothing but pump.
DWORD WINAPI InputHookThreadProc(LPVOID) {
    HINSTANCE hInst = (HINSTANCE)GetCurrentModuleHandle();

    g_dragMouseHook = SetWindowsHookEx(WH_MOUSE_LL, DragMouseHookProc, hInst, 0);
    if (!g_dragMouseHook) {
        Wh_Log(L"[Drag] SetWindowsHookEx(WH_MOUSE_LL) failed, error=%lu", GetLastError());
    }

    g_moveKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, MoveKeyboardHookProc, hInst, 0);
    if (!g_moveKeyboardHook) {
        Wh_Log(L"[KeyMove] SetWindowsHookEx(WH_KEYBOARD_LL) failed, error=%lu", GetLastError());
    }

    if (!g_dragMouseHook && !g_moveKeyboardHook) return 1;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Low-level hooks have to be removed by the thread that installed them,
    // which is why this happens here rather than in UninitInputHooks.
    if (g_dragMouseHook) {
        UnhookWindowsHookEx(g_dragMouseHook);
        g_dragMouseHook = nullptr;
    }
    if (g_moveKeyboardHook) {
        UnhookWindowsHookEx(g_moveKeyboardHook);
        g_moveKeyboardHook = nullptr;
    }
    return 0;
}

void InitInputHooks() {
    if (g_inputHookThread) return;
    g_inputHookThread = CreateThread(nullptr, 0, InputHookThreadProc, nullptr, 0,
                                     &g_inputHookThreadId);
    if (g_inputHookThread) {
        SetThreadDescription(g_inputHookThread, L"TourneTable-InputHooks");
    }
}

void UninitInputHooks() {
    if (g_inputHookThread) {
        PostThreadMessage(g_inputHookThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(g_inputHookThread, 3000);
        CloseHandle(g_inputHookThread);
        g_inputHookThread = nullptr;
        g_inputHookThreadId = 0;
    }
}

FILETIME GetWallpaperFileTime() {
    WCHAR path[MAX_PATH] = {};
    SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0);
    FILETIME ft = {};
    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        GetFileTime(hFile, nullptr, nullptr, &ft);
        CloseHandle(hFile);
    }
    return ft;
}

void CaptureWallpaperBitmap() {
    g_wallpaperBitmap.Reset();
    if (!g_overlayWnd || !g_dc || !g_swapChain) return;

    g_lastWallpaperTime = GetWallpaperFileTime();

    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
    g_dc->EndDraw();
    g_swapChain->Present(1, 0);
    DwmFlush();

    HWND hParent = GetParent(g_overlayWnd);
    if (!hParent) return;

    RECT rc;
    GetClientRect(hParent, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    HWND hSource = FindWindow(L"Progman", nullptr);
    if (!hSource) hSource = hParent;

    HDC hdcScreen = GetDC(nullptr);
    if (!hdcScreen) return;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) { ReleaseDC(nullptr, hdcScreen); return; }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
    if (!hBmp) { DeleteDC(hdcMem); ReleaseDC(nullptr, hdcScreen); return; }

    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBmp);

    if (!PrintWindow(hSource, hdcMem, 0x02 /*PW_RENDERFULLCONTENT*/)) {
        SelectObject(hdcMem, hOldBmp);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        return;
    }
    GdiFlush();

    BYTE* pixels = static_cast<BYTE*>(pvBits);
    for (int i = 0; i < w * h; i++) pixels[i * 4 + 3] = 255;

    D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    g_dc->CreateBitmap(D2D1::SizeU(w, h), pvBits, w * 4, bitmapProps, &g_wallpaperBitmap);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

void ReleaseVisualResources() {
    g_blurEffect.Reset();
    g_blurredBitmap.Reset();
    g_wallpaperBitmap.Reset();
    g_borderBrush.Reset();
    g_backgroundBrush.Reset();
    g_barBrush.Reset();
    g_barBrush2.Reset();
    g_bgGeoCache.Reset();
    g_borderRingCache.Reset();
    g_bgGeoCacheRect = D2D1::RectF(-1, -1, -1, -1);
    g_bgGeoCacheRadii[0] = g_bgGeoCacheRadii[1] =
        g_bgGeoCacheRadii[2] = g_bgGeoCacheRadii[3] = -1.f;
    g_borderCacheBorderSize = -1;
    g_nowPlayingBrush.Reset();
    g_textPanelBrush.Reset();
    // Smooth Mode caches. Every settings change and wallpaper re-bake comes
    // through here, so none of them needs its own invalidation for colors,
    // fonts or blur -- only for geometry, which is checked per frame.
    g_plateBitmap.Reset();
    g_plateRect = D2D1::RectF(0, 0, 0, 0);
    g_spriteAtlas.Reset();
    g_spriteAtlasKey = VizAtlasKey();
    g_npLayoutCache.Reset();
    g_pfLayoutCache.Reset();
}

void ReleaseSwapChainResources() {
    ReleaseVisualResources();
    g_spriteBatch.Reset();
    g_dc3.Reset();
    g_compositionVisual.Reset();
    g_compositionTarget.Reset();
    g_compositionDevice.Reset();
    g_dc.Reset();
    g_swapChain.Reset();
}

bool RecreateVisualResources() {
    HRESULT hr;

    D2D1_COLOR_F barColor = D2D1::ColorF(g_settings.colorR / 255.0f, g_settings.colorG / 255.0f,
                                         g_settings.colorB / 255.0f, g_settings.colorA / 255.0f);
    hr = g_dc->CreateSolidColorBrush(barColor, &g_barBrush);
    if (FAILED(hr)) return false;

    D2D1_COLOR_F peakHoldColor = D2D1::ColorF(g_settings.peakHoldR / 255.0f, g_settings.peakHoldG / 255.0f,
                                               g_settings.peakHoldB / 255.0f, g_settings.peakHoldA / 255.0f);
    g_dc->CreateSolidColorBrush(peakHoldColor, &g_barBrush2);

    if (g_settings.backgroundEnabled) {
        D2D1_COLOR_F bgColor = D2D1::ColorF(g_settings.bgR / 255.0f, g_settings.bgG / 255.0f,
                                            g_settings.bgB / 255.0f, g_settings.bgA / 255.0f);
        g_dc->CreateSolidColorBrush(bgColor, &g_backgroundBrush);

        if (g_settings.bgBlur > 0) {
            CaptureWallpaperBitmap();
            if (g_wallpaperBitmap) {
                hr = g_dc->CreateEffect(kCLSID_D2D1GaussianBlur, &g_blurEffect);
                if (SUCCEEDED(hr)) {
                    g_blurEffect->SetInput(0, g_wallpaperBitmap.Get());
                    g_blurEffect->SetValue(0, (FLOAT)g_settings.bgBlur);
                    g_blurEffect->SetValue(2, (UINT32)1);

                    // Evaluate the blur exactly once, into a bitmap covering only
                    // the widget's bounding box rather than the whole desktop.
                    //
                    // The blur still SAMPLES the full-size wallpaper (it has to --
                    // a Gaussian reads neighbouring pixels from beyond the box's
                    // edges), but only the box-sized result is kept. At 1080p that
                    // is roughly 8 MB of video memory replaced by tens of KB.
                    VizLayout blurLayout;
                    if (ComputeVizLayout(&blurLayout)) {
                        D2D1_SIZE_U blurSize = D2D1::SizeU(blurLayout.width, blurLayout.height);

                        D2D1_BITMAP_PROPERTIES1 blurProps = {};
                        blurProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
                        blurProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
                        blurProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

                        if (SUCCEEDED(g_dc->CreateBitmap(blurSize, nullptr, 0, blurProps,
                                                          &g_blurredBitmap))) {
                            ComPtr<ID2D1Image> savedTarget;
                            g_dc->GetTarget(&savedTarget);
                            g_dc->SetTarget(g_blurredBitmap.Get());
                            g_dc->BeginDraw();
                            g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
                            // Shift the wallpaper so the slice under the widget
                            // lands at the origin of this small target.
                            g_dc->SetTransform(D2D1::Matrix3x2F::Translation(
                                -blurLayout.originX, -blurLayout.originY));
                            g_dc->DrawImage(g_blurEffect.Get());
                            g_dc->SetTransform(D2D1::Matrix3x2F::Identity());
                            HRESULT hrBlur = g_dc->EndDraw();
                            g_dc->SetTarget(savedTarget.Get());
                            if (FAILED(hrBlur)) g_blurredBitmap.Reset();
                        }
                    }
                }
            }
            // Once the blur is baked, neither the effect graph nor the full-size
            // source wallpaper bitmap is needed again -- releasing both frees the
            // memory they were holding for the life of the overlay.
            if (g_blurredBitmap) {
                g_blurEffect.Reset();
                g_wallpaperBitmap.Reset();
            }
        }

        if (g_settings.bgBorderSize > 0) {
            D2D1_COLOR_F borderColor =
                D2D1::ColorF(g_settings.borderR / 255.0f, g_settings.borderG / 255.0f,
                            g_settings.borderB / 255.0f, g_settings.borderA / 255.0f);
            g_dc->CreateSolidColorBrush(borderColor, &g_borderBrush);
        }
    }

    if (g_settings.nowPlayingEnabled || g_settings.peakFreqEnabled) {
        // One scratch brush shared by both text panels and both their borders.
        // Each use sets its colour first -- four separate brushes would be four
        // objects to keep in step with four settings for no benefit, since the
        // draws are strictly sequential.
        g_dc->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &g_textPanelBrush);

        D2D1_COLOR_F npColor =
            D2D1::ColorF(g_settings.nowPlayingR / 255.0f, g_settings.nowPlayingG / 255.0f,
                        g_settings.nowPlayingB / 255.0f, g_settings.nowPlayingA / 255.0f);
        g_dc->CreateSolidColorBrush(npColor, &g_nowPlayingBrush);

        int fontSize = std::max(6, g_settings.nowPlayingFontSize);
        if (g_dwriteFactory && (!g_dwriteTextFormat || g_dwriteTextFormatFontSize != fontSize ||
                                 g_dwriteTextFormatFontName != g_settings.nowPlayingFont)) {
            g_dwriteTextFormat.Reset();
            WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
            if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) == 0) {
                wcscpy_s(localeName, L"en-us");
            }
            HRESULT hrText = g_dwriteFactory->CreateTextFormat(
                g_settings.nowPlayingFont.c_str(), nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                (FLOAT)fontSize * g_dpiScale, localeName, &g_dwriteTextFormat);
            if (SUCCEEDED(hrText)) {
                g_dwriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                g_dwriteTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                g_dwriteTextFormatFontSize = fontSize;
                g_dwriteTextFormatFontName = g_settings.nowPlayingFont;
            }
        }
    }

    return true;
}


void FillRoundedRectPerCorner(ID2D1DeviceContext* dc, ID2D1Factory1* factory,
                               const D2D1_RECT_F& r, ID2D1Brush* brush,
                               float rTL, float rTR, float rBR, float rBL) {
    if (rTL == rTR && rTR == rBR && rBR == rBL) {
        float clampedR = std::min(rTL, std::min(r.right - r.left, r.bottom - r.top) / 2.0f);
        dc->FillRoundedRectangle(
            D2D1::RoundedRect(r, clampedR, clampedR), brush);
        return;
    }

    float w = r.right  - r.left;
    float h = r.bottom - r.top;

    rTL = std::min(rTL, std::min(w, h) / 2.0f);
    rTR = std::min(rTR, std::min(w, h) / 2.0f);
    rBR = std::min(rBR, std::min(w, h) / 2.0f);
    rBL = std::min(rBL, std::min(w, h) / 2.0f);

    ComPtr<ID2D1PathGeometry> geo;
    if (FAILED(factory->CreatePathGeometry(&geo))) return;

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geo->Open(&sink))) return;

    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    sink->BeginFigure(D2D1::Point2F(r.left + rTL, r.top), D2D1_FIGURE_BEGIN_FILLED);

    sink->AddLine(D2D1::Point2F(r.right - rTR, r.top));
    if (rTR > 0)
        sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.right, r.top + rTR),
            D2D1::SizeF(rTR, rTR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.right, r.bottom - rBR));
    if (rBR > 0)
        sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.right - rBR, r.bottom),
            D2D1::SizeF(rBR, rBR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.left + rBL, r.bottom));
    if (rBL > 0)
        sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.left, r.bottom - rBL),
            D2D1::SizeF(rBL, rBL), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.left, r.top + rTL));
    if (rTL > 0)
        sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.left + rTL, r.top),
            D2D1::SizeF(rTL, rTL), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();

    dc->FillGeometry(geo.Get(), brush);
}

HRESULT CreateRoundedRectPath(ID2D1Factory1* factory, const D2D1_RECT_F& r,
                               float rTL, float rTR, float rBR, float rBL,
                               ID2D1PathGeometry** outGeo) {
    float w = r.right - r.left, h = r.bottom - r.top;
    float maxR = std::min(w, h) / 2.0f;
    rTL = std::min(rTL, maxR); rTR = std::min(rTR, maxR);
    rBR = std::min(rBR, maxR); rBL = std::min(rBL, maxR);

    ComPtr<ID2D1PathGeometry> geo;
    HRESULT hr = factory->CreatePathGeometry(&geo);
    if (FAILED(hr)) return hr;

    ComPtr<ID2D1GeometrySink> sink;
    hr = geo->Open(&sink);
    if (FAILED(hr)) return hr;

    sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    sink->BeginFigure(D2D1::Point2F(r.left + rTL, r.top), D2D1_FIGURE_BEGIN_FILLED);

    sink->AddLine(D2D1::Point2F(r.right - rTR, r.top));
    if (rTR > 0) sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.right, r.top + rTR),
        D2D1::SizeF(rTR, rTR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.right, r.bottom - rBR));
    if (rBR > 0) sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.right - rBR, r.bottom),
        D2D1::SizeF(rBR, rBR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.left + rBL, r.bottom));
    if (rBL > 0) sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.left, r.bottom - rBL),
        D2D1::SizeF(rBL, rBL), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->AddLine(D2D1::Point2F(r.left, r.top + rTL));
    if (rTL > 0) sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r.left + rTL, r.top),
        D2D1::SizeF(rTL, rTL), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    hr = sink->Close();
    if (FAILED(hr)) return hr;

    *outGeo = geo.Detach();
    return S_OK;
}

// The colours and geometry of one text overlay's panel, bundled so the two
// callers don't each pass a dozen loose arguments.
struct TextPanelStyle {
    BYTE fillA, fillR, fillG, fillB;
    BYTE borderA, borderR, borderG, borderB;
    int padding, cornerRadius, borderSize;
};

// Draws a text overlay, optionally on a panel fitted to the text.
//
// `box` is the layout rectangle the text is centred within, which is much wider
// than the text itself -- so the panel can't just use it, or it would stretch
// the full width of the reserved area. Measuring the text means building a text
// layout, which is exactly what DrawText does internally anyway; when a panel is
// wanted we keep that layout and draw from it, so nothing is measured twice.
//
// fadeAlpha scales the whole thing, so a panel fades in and out with the Now
// Playing text instead of popping.
void DrawOverlayText(PCWSTR text, UINT32 len, const D2D1_RECT_F& box, ID2D1Brush* textBrush,
                     const TextPanelStyle& style, float fadeAlpha,
                     VizTextLayoutCache* cache = nullptr) {
    bool wantFill = style.fillA > 0;
    bool wantBorder = style.borderSize > 0 && style.borderA > 0;

    ComPtr<IDWriteTextLayout> layout;

    // Smooth Mode passes a cache: the layout is rebuilt only when the string or
    // the box changes, and then serves both the panel measurement and the
    // draw. Drawing a layout at the box origin is what DrawText does
    // internally, so the result is identical.
    if (cache && g_dwriteFactory && g_dwriteTextFormat) {
        float boxW = box.right - box.left;
        float boxH = box.bottom - box.top;
        bool same = cache->layout && cache->w == boxW && cache->h == boxH &&
                    cache->text.size() == len && wmemcmp(cache->text.data(), text, len) == 0;
        if (!same) {
            cache->Reset();
            if (SUCCEEDED(g_dwriteFactory->CreateTextLayout(text, len, g_dwriteTextFormat.Get(), boxW,
                                                            boxH, &cache->layout)) &&
                cache->layout) {
                cache->text.assign(text, len);
                cache->w = boxW;
                cache->h = boxH;
            } else {
                cache->Reset();
            }
        }
        layout = cache->layout;
    }

    if ((wantFill || wantBorder) && g_dwriteFactory && g_textPanelBrush) {
        float boxW = box.right - box.left;
        float boxH = box.bottom - box.top;
        DWRITE_TEXT_METRICS m{};
        if ((layout || SUCCEEDED(g_dwriteFactory->CreateTextLayout(
                           text, len, g_dwriteTextFormat.Get(), boxW, boxH, &layout))) &&
            layout && SUCCEEDED(layout->GetMetrics(&m)) && m.width > 0.f && m.height > 0.f) {
            float pad = (float)style.padding * g_dpiScale;
            D2D1_RECT_F r = D2D1::RectF(box.left + m.left - pad, box.top + m.top - pad,
                                        box.left + m.left + m.width + pad,
                                        box.top + m.top + m.height + pad);

            float maxRadius = std::min((r.right - r.left) * 0.5f, (r.bottom - r.top) * 0.5f);
            float rad = std::min((float)style.cornerRadius * g_dpiScale, maxRadius);

            if (wantFill) {
                g_textPanelBrush->SetColor(D2D1::ColorF(
                    style.fillR / 255.0f, style.fillG / 255.0f, style.fillB / 255.0f,
                    (style.fillA / 255.0f) * fadeAlpha));
                g_dc->FillRoundedRectangle(D2D1::RoundedRect(r, rad, rad),
                                           g_textPanelBrush.Get());
            }

            if (wantBorder) {
                // DrawRoundedRectangle strokes centred on the path, so the rect
                // is inset by half the stroke to keep the whole border inside
                // the panel rather than straddling its edge.
                float bw = std::min((float)style.borderSize * g_dpiScale, maxRadius);
                D2D1_RECT_F sr = D2D1::RectF(r.left + bw * 0.5f, r.top + bw * 0.5f,
                                             r.right - bw * 0.5f, r.bottom - bw * 0.5f);
                float srad = std::max(0.0f, rad - bw * 0.5f);
                g_textPanelBrush->SetColor(D2D1::ColorF(
                    style.borderR / 255.0f, style.borderG / 255.0f, style.borderB / 255.0f,
                    (style.borderA / 255.0f) * fadeAlpha));
                g_dc->DrawRoundedRectangle(D2D1::RoundedRect(sr, srad, srad),
                                           g_textPanelBrush.Get(), bw);
            }
        }
    }

    if (layout) {
        g_dc->DrawTextLayout(D2D1::Point2F(box.left, box.top), layout.Get(), textBrush);
    } else {
        g_dc->DrawText(text, len, g_dwriteTextFormat.Get(), box, textBrush);
    }
}

bool RectsApproxEqual(const D2D1_RECT_F& a, const D2D1_RECT_F& b) {
    auto eq = [](float x, float y) { return fabsf(x - y) < 0.01f; };
    return eq(a.left, b.left) && eq(a.top, b.top) &&
           eq(a.right, b.right) && eq(a.bottom, b.bottom);
}

// ---- Smooth Mode: background plate --------------------------------------------

// The three background layers, drawn into any context. The 1.4 path calls this
// on the frame's own context every frame; Smooth Mode calls it once into the
// cached plate.
void VizDrawBackgroundLayers(ID2D1DeviceContext* dc, ID2D1Geometry* bgGeo, ID2D1Geometry* ring,
                             ID2D1Brush* fill, ID2D1Brush* border) {
    if (g_blurredBitmap && bgGeo) {
        dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), bgGeo), nullptr);
        // The cached blur is already box-sized and box-aligned, so this
        // is a straight 1:1 blit with no scaling or resampling.
        dc->DrawBitmap(g_blurredBitmap.Get());
        dc->PopLayer();
    }
    if (bgGeo && fill) dc->FillGeometry(bgGeo, fill);
    if (ring && border) dc->FillGeometry(ring, border);
}

// Renders the background into g_plateBitmap. The plate covers the panel's
// pixel-aligned bounds plus a 2 px margin for the antialiased edge, and is
// drawn back at an integer position, so the per-frame blit never resamples.
//
// It renders on a context of its own rather than by retargeting the frame's
// context, because it can be called mid-frame, possibly inside the auto-hide
// fade layer, where switching targets isn't allowed.
bool VizBakeBackgroundPlate(const D2D1_RECT_F& bgRect, ID2D1Geometry* bgGeo, ID2D1Geometry* ring) {
    if (!g_d2dDevice || !g_backgroundBrush) return false;

    D2D1_RECT_F pr = D2D1::RectF(floorf(bgRect.left) - 2.f, floorf(bgRect.top) - 2.f,
                                 ceilf(bgRect.right) + 2.f, ceilf(bgRect.bottom) + 2.f);
    UINT w = (UINT)std::max(1.f, pr.right - pr.left);
    UINT h = (UINT)std::max(1.f, pr.bottom - pr.top);

    ComPtr<ID2D1DeviceContext> bake;
    if (FAILED(g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &bake))) return false;

    // Same size as last time (a drag, say) reuses the bitmap.
    ComPtr<ID2D1Bitmap1> bmp = g_plateBitmap;
    if (!bmp || bmp->GetPixelSize().width != w || bmp->GetPixelSize().height != h) {
        D2D1_BITMAP_PROPERTIES1 props = {};
        props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
        bmp.Reset();
        if (FAILED(bake->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &bmp))) return false;
    }

    // Brushes are made on the bake context with the frame brushes' colors,
    // which RecreateVisualResources set from the settings.
    ComPtr<ID2D1SolidColorBrush> fill, border;
    bake->CreateSolidColorBrush(g_backgroundBrush->GetColor(), &fill);
    if (g_borderBrush) bake->CreateSolidColorBrush(g_borderBrush->GetColor(), &border);

    bake->SetTarget(bmp.Get());
    bake->BeginDraw();
    bake->Clear(D2D1::ColorF(0, 0, 0, 0));
    bake->SetTransform(D2D1::Matrix3x2F::Translation(-pr.left, -pr.top));
    VizDrawBackgroundLayers(bake.Get(), bgGeo, ring, fill.Get(), border.Get());
    HRESULT hr = bake->EndDraw();
    bake->SetTarget(nullptr);
    if (FAILED(hr)) {
        g_plateBitmap.Reset();
        return false;
    }
    g_plateBitmap = bmp;
    g_plateRect = pr;
    return true;
}

// ---- Smooth Mode: batched bars --------------------------------------------------
//
// The atlas holds one antialiased bar shape, white, with a transparent pixel of
// padding all round. For bars it is stored as three strips along the bar's
// length: the leading cap (padding, the rounded corners, one solid row), a
// three-row solid middle, and the trailing cap. Each bar is then three sprites:
// the caps drawn 1:1 at the bar's ends and one middle row stretched to fill the
// rest, all in one draw call with per-bar color.
//
// Two details keep it looking like the 1.4 path rather than a sprite approxi-
// mation. Each cap strip carries one solid row past the corner, so even a
// square-cornered bar has its end edge inside the texture, where bilinear
// filtering gives it a subpixel-accurate antialiased edge as it moves. And the
// shape is baked at the bars' common subpixel offset across the bar, so when
// bar positions share it (whole-pixel bar width and gap, which is every
// 100% / 200% scale setup) the sprites land on whole pixels and the sides come
// out exactly as D2D's own antialiasing draws them.

bool VizEnsureSpriteAtlas(const VizAtlasKey& key) {
    if (!g_dc3 || !g_spriteBatch || !g_d2dDevice || !g_d2dFactory) return false;
    if (g_spriteAtlas && key == g_spriteAtlasKey) return true;
    g_spriteAtlas.Reset();

    const float t = std::max(1.0f, key.thick);
    const float maxR = t * 0.5f;
    const float rTL = std::min(key.rTL, maxR), rTR = std::min(key.rTR, maxR);
    const float rBR = std::min(key.rBR, maxR), rBL = std::min(key.rBL, maxR);

    VizAtlasLayout L;
    L.pad = 1;
    L.mid = 3;
    D2D1_RECT_F shape;
    UINT bmpW, bmpH;
    if (key.kind == 1) {
        // A dot is drawn whole: one sprite each, no stretching.
        L.capA = L.capB = 0;
        L.across = L.along = (UINT)ceilf(t) + 2 * L.pad;
        shape = D2D1::RectF((float)L.pad, (float)L.pad, L.pad + t, L.pad + t);
        bmpW = bmpH = L.across;
    } else if (key.horizontal) {
        // Upright bars: caps are top (TL/TR) and bottom (BL/BR).
        L.capA = (UINT)ceilf(std::max(rTL, rTR));
        L.capB = (UINT)ceilf(std::max(rBL, rBR));
        L.across = (UINT)ceilf(t + key.frac) + 2 * L.pad;
        L.along = L.pad + L.capA + L.mid + L.capB + L.pad;
        shape = D2D1::RectF(L.pad + key.frac, (float)L.pad, L.pad + key.frac + t,
                            (float)(L.along - L.pad));
        bmpW = L.across;
        bmpH = L.along;
    } else {
        // Sideways bars: caps are left (TL/BL) and right (TR/BR).
        L.capA = (UINT)ceilf(std::max(rTL, rBL));
        L.capB = (UINT)ceilf(std::max(rTR, rBR));
        L.across = (UINT)ceilf(t + key.frac) + 2 * L.pad;
        L.along = L.pad + L.capA + L.mid + L.capB + L.pad;
        shape = D2D1::RectF((float)L.pad, L.pad + key.frac, (float)(L.along - L.pad),
                            L.pad + key.frac + t);
        bmpW = L.along;
        bmpH = L.across;
    }

    ComPtr<ID2D1DeviceContext> bake;
    if (FAILED(g_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &bake))) return false;
    D2D1_BITMAP_PROPERTIES1 props = {};
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    ComPtr<ID2D1Bitmap1> atlas;
    if (FAILED(bake->CreateBitmap(D2D1::SizeU(bmpW, bmpH), nullptr, 0, props, &atlas))) return false;
    ComPtr<ID2D1SolidColorBrush> white;
    if (FAILED(bake->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &white))) return false;

    bake->SetTarget(atlas.Get());
    bake->BeginDraw();
    bake->Clear(D2D1::ColorF(0, 0, 0, 0));
    // The same routine the 1.4 path draws each bar with, so corner shapes and
    // radius clamping match exactly.
    FillRoundedRectPerCorner(bake.Get(), g_d2dFactory.Get(), shape, white.Get(), rTL, rTR, rBR, rBL);
    HRESULT hr = bake->EndDraw();
    bake->SetTarget(nullptr);
    if (FAILED(hr)) return false;

    g_spriteAtlas = atlas;
    g_spriteAtlasKey = key;
    g_spriteAtlasLayout = L;
    return true;
}

inline D2D1_COLOR_F VizPremultiplied(BYTE a, BYTE r, BYTE g, BYTE b) {
    // Sprite colors multiply the (premultiplied) atlas texel component by
    // component, so they have to be premultiplied too for a translucent bar
    // color to blend correctly.
    float fa = a / 255.0f;
    return D2D1::ColorF(r / 255.0f * fa, g / 255.0f * fa, b / 255.0f * fa, fa);
}

inline void VizClearSprites() {
    g_spriteDst.clear();
    g_spriteSrc.clear();
    g_spriteCol.clear();
}

inline void VizPushSprite(const D2D1_RECT_F& dst, const D2D1_RECT_U& src, const D2D1_COLOR_F& col) {
    g_spriteDst.push_back(dst);
    g_spriteSrc.push_back(src);
    g_spriteCol.push_back(col);
}

// Queues the three sprites for one bar whose rectangle is r -- the same
// rectangle the 1.4 path would fill. `horizontal` is the Orientation setting
// (upright bars in a horizontal row).
void VizQueueBarSprites(const D2D1_RECT_F& r, bool horizontal, const D2D1_COLOR_F& col) {
    const VizAtlasLayout& L = g_spriteAtlasLayout;
    const float frac = g_spriteAtlasKey.frac;
    const float pad = (float)L.pad;
    // Effective cap lengths include the one solid row each strip carries.
    const float ca = (float)L.capA + 1.f;
    const float cb = (float)L.capB + 1.f;

    float a0 = horizontal ? r.top : r.left;     // start along the bar's length
    float a1 = horizontal ? r.bottom : r.right; // end along the bar's length
    float len = a1 - a0;
    if (len <= 0.01f) return;
    float s = (ca + cb > len) ? len / (ca + cb) : 1.f;

    float c0 = (horizontal ? r.left : r.top) - frac - pad;  // atlas column/row 0
    float c1 = c0 + (float)L.across;

    UINT leadEnd = L.pad + L.capA + 1;  // exclusive
    UINT bodyRow = leadEnd;             // the middle of the three solid rows
    UINT trailStart = bodyRow + 1;

    auto dstRect = [&](float along0, float along1) {
        return horizontal ? D2D1::RectF(c0, along0, c1, along1) : D2D1::RectF(along0, c0, along1, c1);
    };
    auto srcRect = [&](UINT along0, UINT along1) {
        D2D1_RECT_U u;
        if (horizontal) {
            u.left = 0; u.right = L.across; u.top = along0; u.bottom = along1;
        } else {
            u.left = along0; u.right = along1; u.top = 0; u.bottom = L.across;
        }
        return u;
    };

    VizPushSprite(dstRect(a0 - pad * s, a0 + ca * s), srcRect(0, leadEnd), col);
    float bodyFrom = a0 + ca * s, bodyTo = a1 - cb * s;
    if (bodyTo - bodyFrom > 0.001f) VizPushSprite(dstRect(bodyFrom, bodyTo), srcRect(bodyRow, bodyRow + 1), col);
    VizPushSprite(dstRect(a1 - cb * s, a1 + pad * s), srcRect(trailStart, L.along), col);
}

// One dot centred on (cx, cy).
void VizQueueDotSprite(float cx, float cy, float dotR, const D2D1_COLOR_F& col) {
    const VizAtlasLayout& L = g_spriteAtlasLayout;
    float x0 = cx - dotR - (float)L.pad, y0 = cy - dotR - (float)L.pad;
    D2D1_RECT_U src = {0, 0, L.across, L.along};
    VizPushSprite(D2D1::RectF(x0, y0, x0 + (float)L.across, y0 + (float)L.along), src, col);
}

// Draws everything queued, in one call.
void VizFlushSprites() {
    UINT32 n = (UINT32)g_spriteDst.size();
    if (!n || !g_dc3 || !g_spriteBatch || !g_spriteAtlas) return;
    g_spriteBatch->Clear();
    if (FAILED(g_spriteBatch->AddSprites(n, g_spriteDst.data(), g_spriteSrc.data(), g_spriteCol.data(),
                                         nullptr, sizeof(D2D1_RECT_F), sizeof(D2D1_RECT_U),
                                         sizeof(D2D1_COLOR_F), 0))) {
        return;
    }
    // Sprite batches only draw in aliased mode. The atlas already carries the
    // antialiasing, and bilinear sampling carries it across subpixel positions.
    D2D1_ANTIALIAS_MODE prev = g_dc->GetAntialiasMode();
    g_dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    g_dc3->DrawSpriteBatch(g_spriteBatch.Get(), 0, n, g_spriteAtlas.Get(),
                           D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1_SPRITE_OPTIONS_NONE);
    g_dc->SetAntialiasMode(prev);
}

void RenderVisualizer() {
    if (g_unloading || !g_dc || !g_swapChain) return;

    // Keeps the swap chain's on-screen position (the composition visual's
    // offset) and the content about to be drawn inside it derived from the
    // very same layout snapshot. Repositioning separately -- e.g. from the
    // drag hook on every mouse-move, independently of this tick -- let the
    // window race ahead of what got drawn inside it during a fast drag, so
    // the box and its background visibly fell out of sync until the next
    // tick caught up. Cheap to call defensively when nothing has moved.
    UpdateSwapChainForLayout();

    // Smooth Mode: how late is this frame, relative to the Target FPS interval
    // the motion constants were tuned against? Measured against the user's
    // Target FPS rather than the display-locked interval actually being run,
    // so rounding 60 up to 72 on a 144 Hz panel doesn't make bars faster.
    const bool smooth = g_smoothActive.load(std::memory_order_relaxed);
    {
        static LONGLONG s_prevFrameQpc = 0;
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        g_frameScale = 1.0f;
        if (smooth && s_prevFrameQpc) {
            double dtMs = (double)(q.QuadPart - s_prevFrameQpc) * 1000.0 / (double)VizQpcFreq();
            double refMs = 1000.0 / (double)std::max(1, g_settings.targetFps);
            g_frameScale = (float)std::clamp(dtMs / refMs, 0.25, 4.0);
        }
        s_prevFrameQpc = q.QuadPart;
    }

    float sceneAlpha = 1.0f;
    if (g_settings.autoHideEnabled) {
        ULONGLONG idleMs = GetTickCount64() - g_lastAudibleTickMs.load(std::memory_order_relaxed);
        ULONGLONG delayMs = (ULONGLONG)std::max(0, g_settings.autoHideDelaySeconds) * 1000ULL;
        constexpr ULONGLONG kFadeMs = 1500;
        if (idleMs > delayMs) {
            ULONGLONG fadeElapsed = idleMs - delayMs;
            sceneAlpha = (fadeElapsed >= kFadeMs) ? 0.f : 1.0f - (float)fadeElapsed / (float)kFadeMs;
        }
    }

    if (sceneAlpha <= 0.001f) {
        // Fully faded out. Present one blank frame to clear whatever was last
        // shown, then skip the render path entirely until audio returns --
        // there is no point drawing a scene at full detail and then making it
        // invisible.
        if (g_autoHideBlanked) return;
        g_dc->BeginDraw();
        g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
        HRESULT hrEnd = g_dc->EndDraw();
        HRESULT hrPresent = g_swapChain->Present(0, 0);
        VizCheckDeviceLost(hrEnd, hrPresent);
        g_autoHideBlanked = true;
        return;
    }
    g_autoHideBlanked = false;

    g_dc->BeginDraw();
    g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));

    UpdateVisualizerTargets();

    int barCount = std::max(1, std::min(g_settings.barCount, VIZ_BARS_MAX));
    float barW = (float)std::max(1, g_settings.barWidth) * g_dpiScale;
    float barGap = (float)std::max(0, g_settings.barGap) * g_dpiScale;
    float maxSize = (float)std::max(2, g_settings.barMaxSize) * g_dpiScale;
    float idleSize = (float)std::max(0, g_settings.barIdleSize) * g_dpiScale;
    float rTL = g_settings.barRadiusTL * g_dpiScale;
    float rTR = g_settings.barRadiusTR * g_dpiScale;
    float rBR = g_settings.barRadiusBR * g_dpiScale;
    float rBL = g_settings.barRadiusBL * g_dpiScale;

    float attack = 0.55f, decay = 0.18f;
    switch (g_settings.shape) {
        case VizShape::Stereo:  attack = 0.72f; decay = 0.22f; break;
        case VizShape::Mirror:  attack = 0.52f; decay = 0.20f; break;
        case VizShape::Wave:    attack = 0.34f; decay = 0.17f; break;
        case VizShape::Breathe: attack = 0.20f; decay = 0.11f; break;
        default: break;
    }
    if (g_settings.smoothing > 0) {
        // Scales both toward 0 (slower catch-up to the target level) without
        // ever fully freezing the bar, even at max smoothing.
        float smoothFactor = 1.0f - (g_settings.smoothing / 100.0f) * 0.9f;
        attack *= smoothFactor;
        decay *= smoothFactor;
    }
    // Identity unless Smooth Mode is on and this frame was early or late.
    attack = VizEaseForFrame(attack);
    decay = VizEaseForFrame(decay);

    bool horizontal = (g_settings.orientation == VizOrientation::Horizontal);
    
    float barsThickness = barCount * barW + (barCount - 1) * barGap;
    
    float groupThickness = barsThickness;
    float groupExtent    = maxSize;

    float totalWidth, totalHeight;
    if (g_settings.shape == VizShape::Radial) {
        totalWidth = totalHeight = maxSize * 2.0f;
    } else {
        totalWidth  = horizontal ? groupThickness : groupExtent;
        totalHeight = horizontal ? groupExtent    : groupThickness;
    }

    float animTime = (float)GetTickCount64() * 0.001f;
    // Rainbow hue offset, degrees. With Smooth Mode off this is the 1.4
    // expression exactly; with it on, it comes off the QPC clock and is reduced
    // in double before narrowing (see VizClockSeconds).
    const float rainbowBase =
        smooth ? VizClockPhase(VizClockSeconds(), (double)g_settings.rainbowSpeed, 360.0)
               : animTime * g_settings.rainbowSpeed;
    {
        float pulse = g_beatPulse.load(std::memory_order_relaxed);
        if (pulse > 0.f)
            g_beatPulse.store(std::max(0.f, pulse - 0.08f * g_frameScale), std::memory_order_relaxed);
    }

    bool useFadeLayer = sceneAlpha < 0.999f;

    VizLayout layout;
    if (ComputeVizLayout(&layout)) {
        if (useFadeLayer) {
            g_dc->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr,
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), sceneAlpha), nullptr);
        }

        // Drawing happens in swap-chain-local coordinates. The swap chain covers
        // only the widget's bounding box, and the composition visual carries the
        // offset that places it on screen.
        float blockX = layout.blockX;
        float blockY = layout.blockY;
        totalWidth   = layout.totalWidth;
        totalHeight  = layout.totalHeight;

        // Publish the bounds of what is actually VISIBLE, for the occlusion
        // check -- not the full render surface. The surface reserves invisible
        // insets (a safety margin, and room above/beside the bars for the Now
        // Playing label), and near a screen edge those insets push the surface
        // off-screen entirely. Testing that larger box would mean no window
        // could ever fully contain it, and the check would never fire.
        {
            int virtualScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
            int virtualScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
            float padL = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingL * g_dpiScale : 0.f;
            float padR = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingR * g_dpiScale : 0.f;
            float padT = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingT * g_dpiScale : 0.f;
            float padB = g_settings.backgroundEnabled ? (float)g_settings.bgPaddingB * g_dpiScale : 0.f;
            float visL = layout.originX + blockX - padL;
            float visT = layout.originY + blockY - padT;
            float visR = layout.originX + blockX + totalWidth + padR;
            float visB = layout.originY + blockY + totalHeight + padB;
            g_drawRectL.store((LONG)visL + virtualScreenX, std::memory_order_relaxed);
            g_drawRectT.store((LONG)visT + virtualScreenY, std::memory_order_relaxed);
            g_drawRectR.store((LONG)visR + virtualScreenX, std::memory_order_relaxed);
            g_drawRectB.store((LONG)visB + virtualScreenY, std::memory_order_relaxed);
            g_drawRectValid.store(true, std::memory_order_relaxed);
        }

        if (g_backgroundBrush) {
            float padL = (float)g_settings.bgPaddingL * g_dpiScale;
            float padR = (float)g_settings.bgPaddingR * g_dpiScale;
            float padT = (float)g_settings.bgPaddingT * g_dpiScale;
            float padB = (float)g_settings.bgPaddingB * g_dpiScale;
            float bgWidth  = totalWidth  + padL + padR;
            float bgHeight = totalHeight + padT + padB;

            float bgTL = g_settings.bgRadiusTL * g_dpiScale;
            float bgTR = g_settings.bgRadiusTR * g_dpiScale;
            float bgBR = g_settings.bgRadiusBR * g_dpiScale;
            float bgBL = g_settings.bgRadiusBL * g_dpiScale;

            D2D1_RECT_F bgRect = D2D1::RectF(blockX - padL, blockY - padT,
                                              blockX + totalWidth + padR,
                                              blockY + totalHeight + padB);

            // Negative padding can shrink a side past the bars entirely. Once
            // opposite sides would cross over and invert the rect, D2D just
            // draws nothing -- so instead of letting that happen, hold the box
            // open to a sliver on whichever side is collapsing.
            if (bgRect.right - bgRect.left < 1.0f) {
                float mid = (bgRect.left + bgRect.right) * 0.5f;
                bgRect.left = mid - 0.5f;
                bgRect.right = mid + 0.5f;
            }
            if (bgRect.bottom - bgRect.top < 1.0f) {
                float mid = (bgRect.top + bgRect.bottom) * 0.5f;
                bgRect.top = mid - 0.5f;
                bgRect.bottom = mid + 0.5f;
            }
            bgWidth  = bgRect.right  - bgRect.left;
            bgHeight = bgRect.bottom - bgRect.top;

            bool bgDirty = !g_bgGeoCache ||
                           !RectsApproxEqual(bgRect, g_bgGeoCacheRect) ||
                           g_bgGeoCacheRadii[0] != bgTL || g_bgGeoCacheRadii[1] != bgTR ||
                           g_bgGeoCacheRadii[2] != bgBR || g_bgGeoCacheRadii[3] != bgBL;

            if (bgDirty) {
                g_bgGeoCache.Reset();
                g_borderRingCache.Reset();
                CreateRoundedRectPath(g_d2dFactory.Get(), bgRect, bgTL, bgTR, bgBR, bgBL,
                                      &g_bgGeoCache);
                g_bgGeoCacheRect = bgRect;
                g_bgGeoCacheRadii[0] = bgTL; g_bgGeoCacheRadii[1] = bgTR;
                g_bgGeoCacheRadii[2] = bgBR; g_bgGeoCacheRadii[3] = bgBL;
            }

            ID2D1PathGeometry* bgGeo = g_bgGeoCache.Get();

            // The border ring's geometry is now built before anything is drawn
            // rather than between the fill and the border draw. Building a
            // geometry isn't drawing, so the 1.4 output is unchanged -- but the
            // plate below needs every piece in hand before it bakes.
            bool ringRebuilt = false;
            if (g_borderBrush) {
                float bw = std::min((float)g_settings.bgBorderSize * g_dpiScale,
                                    std::min(bgWidth, bgHeight) / 2.0f);

                if (bgDirty || g_borderCacheBorderSize != g_settings.bgBorderSize) {
                    ringRebuilt = true;
                    g_borderRingCache.Reset();

                    D2D1_RECT_F innerRect = D2D1::RectF(bgRect.left + bw, bgRect.top + bw,
                                                         bgRect.right - bw, bgRect.bottom - bw);
                    float iTL = std::max(0.0f, bgTL - bw);
                    float iTR = std::max(0.0f, bgTR - bw);
                    float iBR = std::max(0.0f, bgBR - bw);
                    float iBL = std::max(0.0f, bgBL - bw);

                    ComPtr<ID2D1PathGeometry> innerGeo;
                    CreateRoundedRectPath(g_d2dFactory.Get(), innerRect, iTL, iTR, iBR, iBL,
                                          &innerGeo);

                    if (bgGeo && innerGeo) {
                        ID2D1Geometry* geos[] = {bgGeo, innerGeo.Get()};
                        g_d2dFactory->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, geos, 2,
                                                          &g_borderRingCache);
                    }
                    g_borderCacheBorderSize = g_settings.bgBorderSize;
                }
            }
            ID2D1Geometry* ring = g_borderBrush ? g_borderRingCache.Get() : nullptr;

            // Smooth Mode: a single blit of the cached plate. It is re-baked
            // only when the panel's geometry changes; color, blur
            // and border-size changes all reset it through
            // ReleaseVisualResources. If it can't be baked for any reason,
            // the 1.4 path below draws instead.
            bool drawnFromPlate = false;
            if (smooth) {
                D2D1_RECT_F want = D2D1::RectF(floorf(bgRect.left) - 2.f, floorf(bgRect.top) - 2.f,
                                               ceilf(bgRect.right) + 2.f, ceilf(bgRect.bottom) + 2.f);
                bool fresh = g_plateBitmap && !bgDirty && !ringRebuilt &&
                             RectsApproxEqual(want, g_plateRect);
                if (fresh || VizBakeBackgroundPlate(bgRect, bgGeo, ring)) {
                    g_dc->DrawBitmap(g_plateBitmap.Get(), g_plateRect, 1.0f,
                                     D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                    drawnFromPlate = true;
                }
            }
            if (!drawnFromPlate) {
                VizDrawBackgroundLayers(g_dc.Get(), bgGeo, ring, g_backgroundBrush.Get(),
                                        g_borderBrush.Get());
            }
        }

        if (g_dragRenderPauseActive.load(std::memory_order_relaxed)) {
            // Mid-drag: only the background/border box (just drawn above,
            // already at its new position) should be visible while it's being
            // moved -- freeze the bars in place rather than animate them
            // under a box that's being repositioned.
            static int s_dragTickLogCounter = 0;
            if ((++s_dragTickLogCounter % 10) == 0) {
                Wh_Log(L"[Drag] TICK originXY=(%.1f,%.1f) blockXY=(%.1f,%.1f) HV=(%.2f,%.2f)",
                       layout.originX, layout.originY, layout.blockX, layout.blockY,
                       g_dragOverrideH.load(std::memory_order_relaxed),
                       g_dragOverrideV.load(std::memory_order_relaxed));
            }
            if (useFadeLayer) g_dc->PopLayer();
            HRESULT hrEnd = g_dc->EndDraw();
            HRESULT hrPresent = g_swapChain->Present(0, 0);
            VizCheckDeviceLost(hrEnd, hrPresent);
            return;
        }

        RGBA c1{g_settings.colorA, g_settings.colorR, g_settings.colorG, g_settings.colorB};
        {
            if (g_settings.colorMode == VizColorMode::Accent) {
                DWORD dw = GetWindowsAccentColor();
                c1 = {0xFF, (BYTE)((dw>>16)&0xFF), (BYTE)((dw>>8)&0xFF), (BYTE)(dw&0xFF)};
            } else if (g_settings.colorMode == VizColorMode::AlbumArt) {
                DWORD dw = g_albumArtColor.load(std::memory_order_relaxed);
                c1 = {0xFF, (BYTE)((dw>>16)&0xFF), (BYTE)((dw>>8)&0xFF), (BYTE)(dw&0xFF)};
            } else if (g_settings.colorMode == VizColorMode::DynamicAlbum) {
                DWORD dw = g_albumArtColor.load(std::memory_order_relaxed);
                c1 = {0xFF, (BYTE)((dw>>16)&0xFF), (BYTE)((dw>>8)&0xFF), (BYTE)(dw&0xFF)};
            }
        }
        RGBA c2{g_settings.grad2A, g_settings.grad2R, g_settings.grad2G, g_settings.grad2B};
        RGBA cGrad1{g_settings.grad1A, g_settings.grad1R, g_settings.grad1G, g_settings.grad1B};
        if (g_settings.colorMode == VizColorMode::Tourne) {
            cGrad1 = {255, 20, 184, 166};
            c2     = {255, 200, 29, 51};
        }
        if (g_settings.colorMode == VizColorMode::DynamicAlbum) {
            DWORD dw = g_albumArtColorSecondary.load(std::memory_order_relaxed);
            c2    = {0xFF, (BYTE)((dw>>16)&0xFF), (BYTE)((dw>>8)&0xFF), (BYTE)(dw&0xFF)};
            cGrad1 = c1;
        }

        if (g_settings.shape == VizShape::Dots) {
            float dotR  = barW * 0.5f;
            float step  = barW + barGap;
            float dTL = g_settings.barRadiusTL * g_dpiScale;
            float dTR = g_settings.barRadiusTR * g_dpiScale;
            float dBR = g_settings.barRadiusBR * g_dpiScale;
            float dBL = g_settings.barRadiusBL * g_dpiScale;

            // Smooth Mode: every dot becomes one sprite of a pre-drawn dot, and
            // the whole field goes out in one call. This is the shape that
            // gains the most -- a modest setup is well over a thousand dots,
            // each a separate rounded-rect fill (or, with per-corner radii, a
            // separate path geometry) on the 1.4 path.
            VizAtlasKey dotKey;
            dotKey.kind = 1;
            dotKey.thick = barW;
            dotKey.rTL = dTL; dotKey.rTR = dTR; dotKey.rBR = dBR; dotKey.rBL = dBL;
            const bool dotSprites = smooth && VizEnsureSpriteAtlas(dotKey);
            if (dotSprites) VizClearSprites();
            D2D1_COLOR_F dotCol = D2D1::ColorF(0, 0, 0, 0);

            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float nxt = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(nxt - cur) > 0.0005f) ? nxt : tgt;
                float fac = std::max(0.f, g_vizPeak[i]);
                float colSize = idleSize + fac * std::max(0.f, maxSize - idleSize);

                if (colSize < 0.5f) continue;

                RGBA col = c1;
                if (g_settings.colorMode == VizColorMode::Gradient ||
                    g_settings.colorMode == VizColorMode::Tourne)
                    col = LerpColor(cGrad1, c2, (barCount > 1) ? (float)i/(barCount-1) : 0.f);
                else if (g_settings.colorMode == VizColorMode::ReactiveGradient)
                    col = LerpColor(cGrad1, c2, fac);
                else if (g_settings.colorMode == VizColorMode::DynamicAlbum) {
                    float t = (barCount > 1) ? (float)i / (barCount - 1) : 0.f;
                    float freqT = std::min(1.f, t * 0.6f + fac * 0.4f);
                    col = LerpColor(cGrad1, c2, freqT);
                } else if (g_settings.colorMode == VizColorMode::RainbowCycle) {
                    float hue = fmodf(rainbowBase +
                                       (barCount > 1 ? (float)i/(barCount-1) : 0.f) * 360.f, 360.f);
                    col = HSVtoRGB(hue, 0.85f, 1.0f, c1.a);
                }
                if (g_settings.colorMode == VizColorMode::Acrylic) {
                    BYTE aa = (BYTE)std::max(0, std::min(180, (int)(180.f * fac)));
                    col = {aa, c1.r, c1.g, c1.b};
                }
                if (g_settings.beatFlashEnabled) {
                    float pulse = g_beatPulse.load(std::memory_order_relaxed);
                    if (pulse > 0.001f) {
                        float blend = std::min(1.0f, pulse * (g_settings.beatFlashIntensity / 100.0f) *
                                                      (g_settings.beatFlashA / 255.0f));
                        col.r = (BYTE)(col.r + (g_settings.beatFlashR - (int)col.r) * blend);
                        col.g = (BYTE)(col.g + (g_settings.beatFlashG - (int)col.g) * blend);
                        col.b = (BYTE)(col.b + (g_settings.beatFlashB - (int)col.b) * blend);
                    }
                }
                if (dotSprites)
                    dotCol = VizPremultiplied(col.a, col.r, col.g, col.b);
                else
                    g_barBrush->SetColor(D2D1::ColorF(col.r/255.f, col.g/255.f, col.b/255.f, col.a/255.f));

                int numDots = (step > 0.5f) ? (int)(colSize / step) : 1;
                numDots = std::max(1, numDots);

                auto drawDot = [&](float cx2, float cy2) {
                    if (dotSprites) {
                        VizQueueDotSprite(cx2, cy2, dotR, dotCol);
                        return;
                    }
                    D2D1_RECT_F r = D2D1::RectF(cx2 - dotR, cy2 - dotR, cx2 + dotR, cy2 + dotR);
                    FillRoundedRectPerCorner(g_dc.Get(), g_d2dFactory.Get(), r,
                                             g_barBrush.Get(), dTL, dTR, dBR, dBL);
                };

                for (int d = 0; d < numDots; d++) {
                    if (horizontal) {
                        float cx2 = blockX + i * step + dotR;
                        switch (g_settings.verticalAnchor) {
                            case VizAnchor::Top: {
                                float cy2 = blockY + d * step + dotR;
                                if (cy2 - dotR > blockY + maxSize) break;
                                drawDot(cx2, cy2);
                                break;
                            }
                            case VizAnchor::Middle: {
                                float centerY = blockY + maxSize * 0.5f;
                                if (d == 0) {
                                    drawDot(cx2, centerY);
                                } else {
                                    float cy2up   = centerY - d * step;
                                    float cy2down = centerY + d * step;
                                    bool upOk   = cy2up   - dotR >= blockY;
                                    bool downOk = cy2down + dotR <= blockY + maxSize;
                                    if (upOk)   drawDot(cx2, cy2up);
                                    if (downOk) drawDot(cx2, cy2down);
                                    if (!upOk && !downOk) goto next_dot_h;
                                }
                                break;
                            }
                            default: {
                                float cy2 = blockY + maxSize - d * step - dotR;
                                if (cy2 + dotR < blockY) break;
                                drawDot(cx2, cy2);
                                break;
                            }
                        }
                        continue;
                        next_dot_h: break;
                    } else {
                        float cy2 = blockY + i * step + dotR;
                        switch (g_settings.verticalAnchor) {
                            case VizAnchor::Top: {
                                float cx2 = blockX + groupExtent - d * step - dotR;
                                if (cx2 + dotR < blockX) break;
                                drawDot(cx2, cy2);
                                break;
                            }
                            case VizAnchor::Middle: {
                                float centerX = blockX + groupExtent * 0.5f;
                                if (d == 0) {
                                    drawDot(centerX, cy2);
                                } else {
                                    float cx2r = centerX + d * step;
                                    float cx2l = centerX - d * step;
                                    bool rOk = cx2r + dotR <= blockX + groupExtent;
                                    bool lOk = cx2l - dotR >= blockX;
                                    if (rOk) drawDot(cx2r, cy2);
                                    if (lOk) drawDot(cx2l, cy2);
                                    if (!rOk && !lOk) goto next_dot_v;
                                }
                                break;
                            }
                            default: {
                                float cx2 = blockX + d * step + dotR;
                                if (cx2 - dotR > blockX + groupExtent) break;
                                drawDot(cx2, cy2);
                                break;
                            }
                        }
                        continue;
                        next_dot_v: break;
                    }
                }
            }
            if (dotSprites) VizFlushSprites();
        } else if (g_settings.shape == VizShape::Radial) {
            float centerX = blockX + totalWidth * 0.5f;
            float centerY = blockY + totalHeight * 0.5f;
            float innerR = maxSize * 0.15f;
            float strokeW = std::max(1.0f, barW);

            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float next = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(next - cur) > 0.0005f) ? next : tgt;
                float fac = std::max(0.f, g_vizPeak[i]);
                float len = idleSize + fac * std::max(0.f, maxSize - idleSize);
                if (len < 0.5f) continue;

                RGBA col = c1;
                if (g_settings.colorMode == VizColorMode::Gradient ||
                    g_settings.colorMode == VizColorMode::Tourne)
                    col = LerpColor(cGrad1, c2, (barCount > 1) ? (float)i/(barCount-1) : 0.f);
                else if (g_settings.colorMode == VizColorMode::ReactiveGradient)
                    col = LerpColor(cGrad1, c2, fac);
                else if (g_settings.colorMode == VizColorMode::RainbowCycle) {
                    float hue = fmodf(rainbowBase +
                                       (float)i / std::max(1, barCount) * 360.f, 360.f);
                    col = HSVtoRGB(hue, 0.85f, 1.0f, c1.a);
                }
                if (g_settings.colorMode == VizColorMode::Acrylic) {
                    BYTE aa = (BYTE)std::max(0, std::min(180, (int)(180.f * fac)));
                    col = {aa, c1.r, c1.g, c1.b};
                }
                if (g_settings.beatFlashEnabled) {
                    float pulse = g_beatPulse.load(std::memory_order_relaxed);
                    if (pulse > 0.001f) {
                        float blend = std::min(1.0f, pulse * (g_settings.beatFlashIntensity / 100.0f) *
                                                      (g_settings.beatFlashA / 255.0f));
                        col.r = (BYTE)(col.r + (g_settings.beatFlashR - (int)col.r) * blend);
                        col.g = (BYTE)(col.g + (g_settings.beatFlashG - (int)col.g) * blend);
                        col.b = (BYTE)(col.b + (g_settings.beatFlashB - (int)col.b) * blend);
                    }
                }
                g_barBrush->SetColor(D2D1::ColorF(col.r/255.f, col.g/255.f, col.b/255.f, col.a/255.f));

                float angle = (float)i / (float)barCount * 2.0f * VIZ_PI - (VIZ_PI * 0.5f);
                float dx = cosf(angle), dy = sinf(angle);
                D2D1_POINT_2F p0 = {centerX + dx * innerR, centerY + dy * innerR};
                D2D1_POINT_2F p1 = {centerX + dx * (innerR + len), centerY + dy * (innerR + len)};
                g_dc->DrawLine(p0, p1, g_barBrush.Get(), strokeW, g_roundCapStrokeStyle.Get());
            }
        } else if (g_settings.shape == VizShape::Oscilloscope) {
            // The trace sweeps along the group's LONG axis (time) and deflects
            // across its short one (amplitude), so it has to follow the
            // orientation. Vertical is the horizontal layout rotated 90 degrees
            // clockwise: the sweep runs top-to-bottom and "up" becomes "right".
            float ampScale = maxSize * 0.5f;
            float sweepLen = horizontal ? totalWidth : totalHeight;
            float center   = horizontal ? blockY + totalHeight * 0.5f
                                        : blockX + totalWidth  * 0.5f;
            float sweepOrigin = horizontal ? blockX : blockY;
            float wstep = (VIZ_WAVE_SAMPLES > 1) ? sweepLen / (float)(VIZ_WAVE_SAMPLES - 1)
                                                 : sweepLen;

            RGBA col = c1;
            if (g_settings.colorMode == VizColorMode::Gradient ||
                g_settings.colorMode == VizColorMode::Tourne)
                col = LerpColor(cGrad1, c2, 0.5f);
            else if (g_settings.colorMode == VizColorMode::RainbowCycle) {
                float hue = fmodf(rainbowBase, 360.f);
                col = HSVtoRGB(hue, 0.85f, 1.0f, c1.a);
            }
            if (g_settings.oscilloscopeMultibandEnabled) {
                // Blends 3 reference colors (low/mid/high) by how much energy is
                // currently in each EQ zone, so the trace tints toward whichever
                // part of the spectrum is dominant right now. This overrides
                // whatever the color mode above picked, matching how RainbowCycle
                // and Tourne already take priority for this shape.
                float bandsSnap[VIZ_NUM_BANDS];
                ReadBands(bandsSnap);
                float lowE = 0.f, midE = 0.f, highE = 0.f;
                for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                    float e = bandsSnap[b];
                    if (VIZ_BAND_EQ_ZONE[b] == 0) lowE += e;
                    else if (VIZ_BAND_EQ_ZONE[b] == 1) midE += e;
                    else highE += e;
                }
                float total = lowE + midE + highE;
                const RGBA lowCol{255, 255, 90, 60}, midCol{255, 120, 220, 90}, highCol{255, 90, 180, 255};
                if (total > 0.001f) {
                    col.r = (BYTE)((lowCol.r * lowE + midCol.r * midE + highCol.r * highE) / total);
                    col.g = (BYTE)((lowCol.g * lowE + midCol.g * midE + highCol.g * highE) / total);
                    col.b = (BYTE)((lowCol.b * lowE + midCol.b * midE + highCol.b * highE) / total);
                } else {
                    col = midCol;
                }
            }
            if (g_settings.beatFlashEnabled) {
                float pulse = g_beatPulse.load(std::memory_order_relaxed);
                if (pulse > 0.001f) {
                    float blend = std::min(1.0f, pulse * (g_settings.beatFlashIntensity / 100.0f) *
                                                  (g_settings.beatFlashA / 255.0f));
                    col.r = (BYTE)(col.r + (g_settings.beatFlashR - (int)col.r) * blend);
                    col.g = (BYTE)(col.g + (g_settings.beatFlashG - (int)col.g) * blend);
                    col.b = (BYTE)(col.b + (g_settings.beatFlashB - (int)col.b) * blend);
                }
            }
            g_barBrush->SetColor(D2D1::ColorF(col.r/255.f, col.g/255.f, col.b/255.f, col.a/255.f));

            float strokeW = std::max(1.0f, barW * 0.5f);
            float waveSnap[VIZ_WAVE_SAMPLES];
            ReadWaveform(waveSnap);

            // Trace damping, eased on the RENDER thread rather than the capture
            // thread, and deliberately so. The capture side publishes a new
            // trace about 100 times a second; easing toward it here means every
            // frame draws a different trace, so the shape evolves smoothly at
            // whatever the Target FPS is instead of stepping between published
            // snapshots.
            //
            // This is what makes a slow trace legible. Damping it lets the eye
            // lock onto a shape and follow it, where an undamped trace at a
            // wide window is a new picture every frame. It is only valid
            // because the capture side triggers on a zero crossing: without
            // that, point w would mean a different phase each time and easing
            // per index would smear the waveform into mush rather than slow it
            // down.
            //
            // Same shape as the bars' Motion Smoothing above, scaled so even
            // full damping keeps moving rather than freezing.
            static float s_scopeDisp[VIZ_WAVE_SAMPLES] = {};
            float scopeEase = (g_settings.oscilloscopeDamping > 0)
                                  ? 1.0f - (g_settings.oscilloscopeDamping / 100.0f) * 0.95f
                                  : 1.0f;
            // Undamped (1.0) stays 1.0; damping becomes frame-time aware in
            // Smooth Mode like the bars' attack and decay.
            if (scopeEase < 1.0f) scopeEase = VizEaseForFrame(scopeEase);
            for (int w = 0; w < VIZ_WAVE_SAMPLES; w++) {
                s_scopeDisp[w] += (waveSnap[w] - s_scopeDisp[w]) * scopeEase;
            }

            auto wavePoint = [&](int w) -> D2D1_POINT_2F {
                float along  = sweepOrigin + w * wstep;
                float across = center + s_scopeDisp[w] * ampScale * (horizontal ? -1.0f : 1.0f);
                return horizontal ? D2D1::Point2F(along, across)
                                  : D2D1::Point2F(across, along);
            };
            D2D1_POINT_2F prev = wavePoint(0);
            for (int w = 1; w < VIZ_WAVE_SAMPLES; w++) {
                D2D1_POINT_2F pt = wavePoint(w);
                g_dc->DrawLine(prev, pt, g_barBrush.Get(), strokeW);
                prev = pt;
            }
        }
        else {
            // Smooth Mode: bars go out as one sprite batch (see
            // VizEnsureSpriteAtlas). The atlas is baked at the bar row's
            // subpixel offset across the bars, quantized so float noise in the
            // layout can't force a re-bake every frame.
            VizAtlasKey barKey;
            barKey.kind = 0;
            barKey.horizontal = horizontal;
            barKey.thick = barW;
            barKey.rTL = rTL; barKey.rTR = rTR; barKey.rBR = rBR; barKey.rBL = rBL;
            {
                float lead = horizontal ? blockX : blockY;
                float frac = roundf((lead - floorf(lead)) * 256.f) / 256.f;
                barKey.frac = (frac >= 1.f) ? 0.f : frac;
            }
            const bool barSprites = smooth && VizEnsureSpriteAtlas(barKey);
            // Peak caps stay ordinary antialiased rectangles, drawn after the
            // batch so they still sit on top of their bars.
            static std::vector<D2D1_RECT_F> s_capRects;
            if (barSprites) {
                VizClearSprites();
                s_capRects.clear();
            }

            for (int i = 0; i < barCount; i++) {
                float tgt = g_vizTarget[i], cur = g_vizPeak[i];
                float next = cur + (tgt - cur) * ((tgt > cur) ? attack : decay);
                g_vizPeak[i] = (fabsf(next - cur) > 0.0005f) ? next : tgt;

                float fac = std::max(0.f, g_vizPeak[i]);
                float size = idleSize + fac * std::max(0.f, maxSize - idleSize);

                RGBA col = c1;
                if (g_settings.colorMode == VizColorMode::Gradient ||
                    g_settings.colorMode == VizColorMode::Tourne) {
                    float t = (barCount > 1) ? (float)i / (barCount - 1) : 0.f;
                    col = LerpColor(cGrad1, c2, t);
                } else if (g_settings.colorMode == VizColorMode::ReactiveGradient) {
                    col = LerpColor(cGrad1, c2, fac);
                } else if (g_settings.colorMode == VizColorMode::DynamicAlbum) {
                    float t = (barCount > 1) ? (float)i / (barCount - 1) : 0.f;
                    float freqT = std::min(1.f, t * 0.6f + fac * 0.4f);
                    col = LerpColor(cGrad1, c2, freqT);
                } else if (g_settings.colorMode == VizColorMode::RainbowCycle) {
                    float t = (barCount > 1) ? (float)i / (barCount - 1) : 0.f;
                    float hue = fmodf(rainbowBase + t * 360.f, 360.f);
                    col = HSVtoRGB(hue, 0.85f, 1.0f, c1.a);
                }
                if (g_settings.colorMode == VizColorMode::Acrylic) {
                    BYTE aa = (BYTE)std::max(0, std::min(180, (int)(180.f * fac)));
                    col = {aa, c1.r, c1.g, c1.b};
                }
                if (g_settings.beatFlashEnabled) {
                    float pulse = g_beatPulse.load(std::memory_order_relaxed);
                    if (pulse > 0.001f) {
                        float blend = std::min(1.0f, pulse * (g_settings.beatFlashIntensity / 100.0f) *
                                                      (g_settings.beatFlashA / 255.0f));
                        col.r = (BYTE)(col.r + (g_settings.beatFlashR - (int)col.r) * blend);
                        col.g = (BYTE)(col.g + (g_settings.beatFlashG - (int)col.g) * blend);
                        col.b = (BYTE)(col.b + (g_settings.beatFlashB - (int)col.b) * blend);
                    }
                }

                D2D1_COLOR_F d2dCol = D2D1::ColorF(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f,
                                                   col.a / 255.0f);
                D2D1_COLOR_F spriteCol = d2dCol;
                if (barSprites)
                    spriteCol = VizPremultiplied(col.a, col.r, col.g, col.b);
                else
                    g_barBrush->SetColor(d2dCol);

                // Draws one bar body: queued as sprites in Smooth Mode, filled
                // directly otherwise. Peak caps likewise queue or draw.
                auto fillBar = [&](const D2D1_RECT_F& r) {
                    if (barSprites)
                        VizQueueBarSprites(r, horizontal, spriteCol);
                    else
                        FillRoundedRectPerCorner(g_dc.Get(), g_d2dFactory.Get(), r,
                                                 g_barBrush.Get(), rTL, rTR, rBR, rBL);
                };
                auto fillCap = [&](const D2D1_RECT_F& r) {
                    if (barSprites)
                        s_capRects.push_back(r);
                    else
                        g_dc->FillRectangle(r, g_barBrush2.Get());
                };

                float holdSize = idleSize;
                if (g_settings.peakHoldEnabled) {
                    g_vizPeakHold[i] = (fac >= g_vizPeakHold[i])
                        ? fac : std::max(0.f, g_vizPeakHold[i] - 0.012f * g_frameScale);
                    holdSize = idleSize + g_vizPeakHold[i] * std::max(0.f, maxSize - idleSize);
                } else {
                    g_vizPeakHold[i] = 0.f;
                }
                float capThickness = std::max(1.5f, 2.0f * g_dpiScale);

                D2D1_RECT_F barRect;
                if (horizontal) {
                    float x = blockX + i * (barW + barGap);
                    float y, yBottom, capY;
                    switch (g_settings.verticalAnchor) {
                        case VizAnchor::Top:
                            y = blockY;
                            yBottom = blockY + size;
                            capY = blockY + holdSize;
                            break;
                        case VizAnchor::Middle:
                            y = blockY + (maxSize - size) / 2.0f;
                            yBottom = y + size;
                            capY = blockY + (maxSize - holdSize) / 2.0f;
                            break;
                        default:
                            y = blockY + (maxSize - size);
                            yBottom = blockY + maxSize;
                            capY = blockY + (maxSize - holdSize);
                            break;
                    }
                    barRect = D2D1::RectF(x, y, x + barW, yBottom);
                    fillBar(barRect);
                    if (g_settings.peakHoldEnabled && holdSize > 0.5f) {
                        fillCap(D2D1::RectF(x, capY - capThickness * 0.5f,
                                            x + barW, capY + capThickness * 0.5f));
                    }
                } else {
                    float y = blockY + i * (barW + barGap);
                    float x, xRight, capX;
                    switch (g_settings.verticalAnchor) {
                        case VizAnchor::Top:
                            xRight = blockX + groupExtent;
                            x = xRight - size;
                            capX = blockX + groupExtent - holdSize;
                            break;
                        case VizAnchor::Middle: {
                            float cx = blockX + groupExtent / 2.0f;
                            x = cx - size / 2.0f;
                            xRight = cx + size / 2.0f;
                            capX = cx - holdSize / 2.0f;
                            break;
                        }
                        default:
                            x = blockX;
                            xRight = blockX + size;
                            capX = blockX + holdSize;
                            break;
                    }
                    barRect = D2D1::RectF(x, y, xRight, y + barW);
                    fillBar(barRect);
                    if (g_settings.peakHoldEnabled && holdSize > 0.5f) {
                        fillCap(D2D1::RectF(capX - capThickness * 0.5f, y,
                                            capX + capThickness * 0.5f, y + barW));
                    }
                }
            }

            if (barSprites) {
                VizFlushSprites();
                for (const auto& cap : s_capRects) g_dc->FillRectangle(cap, g_barBrush2.Get());
            }
        }

        if (g_settings.nowPlayingEnabled && g_dwriteTextFormat && g_nowPlayingBrush) {
            ULONGLONG changedAt = g_nowPlayingChangedTick.load(std::memory_order_relaxed);
            ULONGLONG npElapsed = GetTickCount64() - changedAt;
            ULONGLONG showMs = (ULONGLONG)std::max(0, g_settings.nowPlayingDisplaySeconds) * 1000ULL;
            constexpr ULONGLONG kNpFadeMs = 800;
            float npAlpha = 0.f;
            if (changedAt != 0 && npElapsed < showMs + kNpFadeMs) {
                npAlpha = (npElapsed < showMs) ? 1.0f
                                                : 1.0f - (float)(npElapsed - showMs) / (float)kNpFadeMs;
            }
            if (npAlpha > 0.01f) {
                std::wstring displayText;
                {
                    std::lock_guard<std::mutex> lock(g_nowPlayingMutex);
                    displayText = g_nowPlayingDisplay;
                }
                if (!displayText.empty()) {
                    g_nowPlayingBrush->SetColor(D2D1::ColorF(
                        g_settings.nowPlayingR / 255.0f, g_settings.nowPlayingG / 255.0f,
                        g_settings.nowPlayingB / 255.0f, (g_settings.nowPlayingA / 255.0f) * npAlpha));
                    float npMargin = 8.0f * g_dpiScale;
                    float npHeight = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale * 1.6f;
                    float npOffX = EffectiveNowPlayingOffsetX();
                    float npOffY = EffectiveNowPlayingOffsetY();
                    D2D1_RECT_F npRect = D2D1::RectF(
                        blockX - layout.textAnchorSide + npOffX,
                        blockY - npHeight - npMargin + npOffY,
                        blockX + totalWidth + layout.textAnchorSide + npOffX,
                        blockY - npMargin + npOffY);
                    TextPanelStyle npPanel{
                        g_settings.npBgA, g_settings.npBgR, g_settings.npBgG, g_settings.npBgB,
                        g_settings.npBgBorderA, g_settings.npBgBorderR,
                        g_settings.npBgBorderG, g_settings.npBgBorderB,
                        g_settings.npBgPadding, g_settings.npBgCornerRadius,
                        g_settings.npBgBorderSize};
                    DrawOverlayText(displayText.c_str(), (UINT32)displayText.length(), npRect,
                                    g_nowPlayingBrush.Get(), npPanel, npAlpha,
                                    smooth ? &g_npLayoutCache : nullptr);
                }
            }
        }

        if (g_settings.peakFreqEnabled && g_dwriteTextFormat && g_nowPlayingBrush) {
            float hz = g_dominantFreqHz.load(std::memory_order_relaxed);
            if (hz > 0.f) {
                wchar_t freqText[32];
                if (hz >= 1000.f)
                    swprintf_s(freqText, L"%.1f kHz", hz / 1000.f);
                else
                    swprintf_s(freqText, L"%.0f Hz", hz);

                g_nowPlayingBrush->SetColor(D2D1::ColorF(
                    g_settings.nowPlayingR / 255.0f, g_settings.nowPlayingG / 255.0f,
                    g_settings.nowPlayingB / 255.0f, g_settings.nowPlayingA / 255.0f));
                float pfMargin = 4.0f * g_dpiScale;
                float pfHeight = (float)std::max(6, g_settings.nowPlayingFontSize) * g_dpiScale * 1.4f;
                float pfWidth  = std::min(120.0f * g_dpiScale,
                                          totalWidth + 2.0f * layout.textAnchorSide);
                float pfOffX = EffectivePeakFreqOffsetX();
                float pfOffY = EffectivePeakFreqOffsetY();

                // Horizontal placement. Left/Right hug the bar group's edges and
                // may sit slightly outside it, which the reserved side inset covers.
                float pfX;
                switch (g_settings.peakFreqAlignH) {
                    case VizTextAlignH::Left:
                        pfX = blockX - layout.textAnchorSide;
                        break;
                    case VizTextAlignH::Center:
                        pfX = blockX + (totalWidth - pfWidth) * 0.5f;
                        break;
                    default:  // Right
                        pfX = blockX + totalWidth + layout.textAnchorSide - pfWidth;
                        break;
                }
                pfX += pfOffX;

                // Vertical placement. Above/Below sit fully clear of the bars,
                // using the room reserved for them in the layout.
                float pfY;
                switch (g_settings.peakFreqAlignV) {
                    case VizTextAlignV::Above:
                        pfY = blockY - pfHeight - pfMargin;
                        break;
                    case VizTextAlignV::Middle:
                        pfY = blockY + (totalHeight - pfHeight) * 0.5f;
                        break;
                    case VizTextAlignV::Bottom:
                        pfY = blockY + totalHeight - pfHeight - pfMargin;
                        break;
                    case VizTextAlignV::Below:
                        pfY = blockY + totalHeight + pfMargin;
                        break;
                    default:  // Top
                        pfY = blockY + pfMargin;
                        break;
                }
                pfY += pfOffY;

                D2D1_RECT_F pfRect = D2D1::RectF(pfX, pfY, pfX + pfWidth, pfY + pfHeight);
                size_t freqLen = wcslen(freqText);
                TextPanelStyle pfPanel{
                    g_settings.pfBgA, g_settings.pfBgR, g_settings.pfBgG, g_settings.pfBgB,
                    g_settings.pfBgBorderA, g_settings.pfBgBorderR,
                    g_settings.pfBgBorderG, g_settings.pfBgBorderB,
                    g_settings.pfBgPadding, g_settings.pfBgCornerRadius,
                    g_settings.pfBgBorderSize};
                DrawOverlayText(freqText, (UINT32)freqLen, pfRect, g_nowPlayingBrush.Get(),
                                pfPanel, 1.0f, smooth ? &g_pfLayoutCache : nullptr);
            }
        }

        if (useFadeLayer) {
            g_dc->PopLayer();
        }
    }

    HRESULT hrEnd = g_dc->EndDraw();
    // Sync interval 0. On a DirectComposition swap chain DWM owns presentation
    // timing, and the waitable timer below already paces the loop -- asking
    // Present to block for a vblank on top of that only parks the render thread.
    // Measured across 219 five-second windows: time in Present drops by a third
    // and windows where the thread parks for >8% of wall time fall from 18% to 4%.
    //
    // Smooth Mode keeps sync interval 0 for the same reason. What it changes is
    // WHEN this runs: the render thread schedules each frame half a refresh
    // after a vblank (see RenderThreadProc), so each present lands in the
    // middle of a refresh interval, far from the edge where some frames would
    // make DWM's next composition and some would miss it.
    HRESULT hrPresent = g_swapChain->Present(0, 0);
    VizCheckDeviceLost(hrEnd, hrPresent);
}

// ---- Smooth Mode: display-locked frame pacing -----------------------------------
//
// Why the 1.4 timer isn't enough on its own, using the 1.1.0 changelog's own
// measurement: 144 FPS requested, 141.7 delivered, the gap being timer wake
// latency added to every interval. On a 144 Hz panel that is two or three
// refreshes a second that get no new frame, so the previous one is shown
// twice -- a stutter you see several times a second. A 60 FPS target on a
// 144 Hz panel is worse in a different way: 144 / 60 = 2.4, so frames are held
// for 2, then 3, then 2 refreshes, which reads as judder however accurate the
// timer is.
//
// So in Smooth Mode the schedule comes from DWM's composition clock instead of
// from "interval since the last frame":
//   * the frame rate is the refresh rate divided by a whole number, the one
//     nearest the Target FPS, so every frame is shown for the same number of
//     refreshes;
//   * every wake is placed half a refresh after a vblank, which is as far as a
//     present can get from DWM's composition deadline on either side, so
//     ordinary scheduling jitter can't push some frames to the next refresh;
//   * deadlines advance on an absolute timeline, so wake latency doesn't
//     accumulate into drift.
// It still wakes once per frame, never once per vblank, so the idle-power
// behaviour the README measures is kept. DWM is asked for its clock twice a
// second, not per frame.
struct VizVsyncSchedule {
    // Smallest time >= t of the form vblank + (k + phase) * period.
    static LONGLONG AlignUp(LONGLONG t, LONGLONG vblank, LONGLONG period, double phase) {
        LONGLONG base = vblank + (LONGLONG)(phase * (double)period);
        LONGLONG d = t - base;
        LONGLONG k = (d >= 0) ? (d + period - 1) / period : -((-d) / period);
        return base + k * period;
    }
    // Refreshes per frame: the whole number nearest refreshHz / targetFps, >= 1.
    static int Divisor(double targetFps, LONGLONG period, LONGLONG qpcFreq) {
        double refreshHz = (double)qpcFreq / (double)period;
        long long n = llround(refreshHz / std::max(1.0, targetFps));
        return (int)std::clamp(n, 1LL, 1000LL);
    }
    // The deadline after one that was due at `due`, re-snapped to mid-refresh
    // so a drifting or re-read vblank reference is absorbed gradually. If the
    // thread has fallen more than a frame behind, it rejoins at the next slot
    // instead of rendering a burst to catch up.
    static LONGLONG Next(LONGLONG due, LONGLONG now, LONGLONG vblank, LONGLONG period, int divisor) {
        LONGLONG next = AlignUp(due + (LONGLONG)divisor * period - period / 2, vblank, period, 0.5);
        if (next <= now) next = AlignUp(now + 1, vblank, period, 0.5);
        return next;
    }
};

void RenderThreadProc() {
    ULONGLONG lastSuccessfulPostTick = 0;

    // Smooth Mode pacing state (see VizVsyncSchedule).
    LONGLONG vsPeriod = 0, vsVblank = 0, vsLastSync = 0, vsDue = 0;
    // Fallback when DWM won't report a clock (remote sessions, some virtual
    // displays): Target FPS on an absolute timeline, which at least removes
    // the accumulated wake-latency drift.
    LONGLONG absDue = 0;

    // Frame pacing is measured with QueryPerformanceCounter, not GetTickCount64.
    // GetTickCount64 only advances on the system timer tick (~15.625ms), so at a
    // 144 FPS target -- a 6.944ms interval -- the elapsed time could only ever
    // read 0 or ~15-16, never 6.9. The loop rendered exactly once per tick and
    // any target above ~64 FPS was silently clamped to 1 / 15.625ms = 64.0.
    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(&qpcFreq);
    LONGLONG lastRenderQpc = 0;

    // A high-resolution waitable timer gives sub-millisecond wait precision,
    // unlike Sleep() which is quantized to the system timer tick (~15.6ms by
    // default). This keeps frame pacing smooth at the target FPS without
    // waking the thread on every vsync the way DwmFlush() did.
    HANDLE hTimer = CreateWaitableTimerExW(nullptr, nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!hTimer) {
        hTimer = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
    }

    // Takes a double: the timer's own unit is 100ns, so rounding the wait up to
    // a whole millisecond would hand back most of that precision (a 6.944ms
    // interval rounded to 7ms caps out at 142.9 FPS).
    auto preciseWait = [hTimer](double ms) {
        if (ms <= 0.0) return;
        if (hTimer) {
            LARGE_INTEGER due;
            due.QuadPart = -(LONGLONG)(ms * 10000.0);  // 100ns units, negative = relative
            if (due.QuadPart == 0) due.QuadPart = -1;
            if (SetWaitableTimer(hTimer, &due, 0, nullptr, nullptr, FALSE)) {
                WaitForSingleObject(hTimer, INFINITE);
                return;
            }
        }
        Sleep((DWORD)std::ceil(ms));
    };

    while (g_renderThreadRunning.load(std::memory_order_relaxed)) {
        HWND overlayWnd = g_overlayWnd.load(std::memory_order_relaxed);
        bool paused = g_fullscreenPaused.load(std::memory_order_relaxed);

        if (!overlayWnd || paused) {
            lastRenderQpc = 0;
            vsDue = absDue = 0;
            preciseWait(150);
            continue;
        }

        int fps = std::max(1, g_settings.targetFps);
        double intervalMs = 1000.0 / (double)fps;

        if (g_settings.pauseWhenSilentSeconds > 0) {
            ULONGLONG lastAudible = g_lastAudibleTickMs.load(std::memory_order_relaxed);
            ULONGLONG idleMs = GetTickCount64() - lastAudible;
            g_slowMode = idleMs > (ULONGLONG)g_settings.pauseWhenSilentSeconds * 1000ULL;
            if (g_slowMode) intervalMs = 200.0;
        } else {
            g_slowMode = false;
        }

        // GetTickCount64 is still fine for the stuck-tick watchdog below -- that
        // compares against 1000ms, far above the tick resolution.
        ULONGLONG now = GetTickCount64();
        LARGE_INTEGER nowQpc;
        QueryPerformanceCounter(&nowQpc);

        // Smooth Mode, outside the silent trickle: display-locked.
        bool scheduled = false;
        if (g_smoothActive.load(std::memory_order_relaxed) && !g_slowMode) {
            if (vsPeriod == 0 || nowQpc.QuadPart - vsLastSync > qpcFreq.QuadPart / 2) {
                DWM_TIMING_INFO ti = {};
                ti.cbSize = sizeof(ti);
                // Accept 20-1000 Hz; anything else is a clock DWM isn't really
                // running (it reports zeros when composition is unavailable).
                if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &ti)) &&
                    ti.qpcRefreshPeriod >= (UINT64)(qpcFreq.QuadPart / 1000) &&
                    ti.qpcRefreshPeriod <= (UINT64)(qpcFreq.QuadPart / 20) && ti.qpcVBlank > 0) {
                    vsPeriod = (LONGLONG)ti.qpcRefreshPeriod;
                    vsVblank = (LONGLONG)ti.qpcVBlank;
                } else {
                    vsPeriod = 0;
                }
                vsLastSync = nowQpc.QuadPart;
            }

            if (vsPeriod > 0) {
                if (vsDue == 0) vsDue = VizVsyncSchedule::AlignUp(nowQpc.QuadPart, vsVblank, vsPeriod, 0.5);
                if (nowQpc.QuadPart < vsDue) {
                    preciseWait((double)(vsDue - nowQpc.QuadPart) * 1000.0 / (double)qpcFreq.QuadPart);
                    continue;
                }
                int divisor = VizVsyncSchedule::Divisor((double)fps, vsPeriod, qpcFreq.QuadPart);
                vsDue = VizVsyncSchedule::Next(vsDue, nowQpc.QuadPart, vsVblank, vsPeriod, divisor);
                absDue = 0;
            } else {
                LONGLONG interval = (LONGLONG)((double)qpcFreq.QuadPart * intervalMs / 1000.0);
                if (absDue == 0) absDue = nowQpc.QuadPart;
                if (nowQpc.QuadPart < absDue) {
                    preciseWait((double)(absDue - nowQpc.QuadPart) * 1000.0 / (double)qpcFreq.QuadPart);
                    continue;
                }
                absDue += interval;
                if (absDue <= nowQpc.QuadPart) absDue = nowQpc.QuadPart + interval;
                vsDue = 0;
            }
            lastRenderQpc = nowQpc.QuadPart;
            scheduled = true;
        } else {
            vsDue = absDue = 0;
        }

        if (!scheduled) {
            // The 1.4 timer path, unchanged.
            double elapsedMs = (lastRenderQpc == 0)
                ? intervalMs
                : (double)(nowQpc.QuadPart - lastRenderQpc) * 1000.0 / (double)qpcFreq.QuadPart;
            if (elapsedMs < intervalMs) {
                preciseWait(intervalMs - elapsedMs);
                continue;
            }
            lastRenderQpc = nowQpc.QuadPart;
        }

        if (!g_renderThreadRunning.load(std::memory_order_relaxed) || g_unloading.load())
            break;

        bool expected = false;
        if (g_renderTickPending.compare_exchange_strong(expected, true)) {
            PostMessage(overlayWnd, WM_APP_RENDER_TICK, 0, 0);
            lastSuccessfulPostTick = now;
        } else if (now - lastSuccessfulPostTick > 1000) {
            Wh_Log(L"[Viz] Render tick flag was stuck, forced a reset");
            g_renderTickPending.store(false, std::memory_order_relaxed);
            lastSuccessfulPostTick = now;
        }
    }

    if (hTimer) CloseHandle(hTimer);
}

void StartRenderThread() {
    if (g_renderThread) return;
    g_renderThreadRunning.store(true, std::memory_order_relaxed);
    g_renderThread = new std::thread(RenderThreadProc);
    HANDLE hRenderThread = g_renderThread->native_handle();
    SetThreadDescription(hRenderThread, L"TourneTable-Render");
}

void StopRenderThread() {
    g_renderThreadRunning.store(false, std::memory_order_relaxed);
    if (g_renderThread) {
        if (g_renderThread->joinable()) g_renderThread->join();
        delete g_renderThread;
        g_renderThread = nullptr;
    }
    g_renderTickPending.store(false, std::memory_order_relaxed);
}

void PauseForFullscreen() {
    if (g_fullscreenPaused.exchange(true)) return;
    Wh_Log(L"Pausing visualizer: not visible");
    StopVizCaptureThread();
    if (g_dc && g_swapChain) {
        g_dc->BeginDraw();
        g_dc->Clear(D2D1::ColorF(0, 0, 0, 0));
        g_dc->EndDraw();
        g_swapChain->Present(1, 0);
    }
}

void ResumeFromFullscreen() {
    if (!g_fullscreenPaused.exchange(false)) return;
    Wh_Log(L"Resuming visualizer: visible again");
    StartVizCaptureThread();
    if (g_overlayWnd) {
        RenderVisualizer();
    }
}

void HandleDisplayChange() {
    if (g_mediaWnd) RepositionAndRepaintMediaControls();

    if (!g_overlayWnd) return;

    HWND hWorkerW = GetParent(g_overlayWnd);
    if (!hWorkerW) return;

    RECT rc;
    GetWindowRect(hWorkerW, &rc);
    SetWindowPos(g_overlayWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    HMONITOR monitor = GetMonitorById(g_settings.monitor - 1);
    if (!monitor) monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTONEAREST);
    g_cachedMonitor = monitor;
    float newDpiScale = GetMonitorDpiScale(monitor);
    if (VizRenderDeviceNeedsRebuild()) {
        // A GPU was added, removed or enabled, or the widget's monitor now
        // hangs off a different one: Auto follows it. The rebuild re-reads
        // DPI and redraws on its own.
        VizRebuildRenderDevice(L"display configuration changed");
    } else if (newDpiScale != g_dpiScale) {
        ReleaseSwapChainResources();
        CreateSwapChainResources();
        RenderVisualizer();
    } else {
        // Same DPI, but the work area may have moved or resized, which shifts
        // where the widget's box belongs.
        UpdateSwapChainForLayout();
        RenderVisualizer();
    }

    if (g_messageWnd && g_settings.backgroundEnabled && g_settings.bgBlur > 0) {
        SetTimer(g_messageWnd, TIMER_ID_MSG_WALLPAPER_REFRESH, 500, nullptr);
    }
}

void CreateOverlayWindow();
void ApplySettingsChanged();

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_APP_RENDER_TICK:
            g_renderTickPending.store(false, std::memory_order_relaxed);
            if (!g_unloading && !g_fullscreenPaused.load()) {
                RenderVisualizer();
            }
            return 0;

        case WM_WINDOWPOSCHANGED: {
            const WINDOWPOS* wp = (const WINDOWPOS*)lParam;
            if (!(wp->flags & SWP_NOSIZE) && !g_unloading) {
                // The swap chain is sized to the widget, not to this window, so
                // a window resize only matters insofar as it changes where the
                // widget's box lands.
                UpdateSwapChainForLayout();
                if (!g_fullscreenPaused.load()) RenderVisualizer();
            }
            break;
        }

        case WM_DESTROY:
            ReleaseSwapChainResources();
            g_overlayWnd = nullptr;
            if (!g_unloading && g_messageWnd) {
                SetTimer(g_messageWnd, TIMER_ID_MSG_RECREATE_OVERLAY, 200, nullptr);
            }
            return 0;

        case WM_APP_CLEANUP:
            DestroyWindow(hWnd);
            return 0;

        case WM_APP_SETTINGS_CHANGED:
            ApplySettingsChanged();
            return 0;

        // A keyboard nudge changed where the box belongs. Rendering is often
        // idle while someone is placing it (no audio playing), so the move
        // would otherwise not show up until the next time music started.
        case WM_APP_FORCE_REDRAW:
            if (!g_unloading && !g_fullscreenPaused.load()) {
                RenderVisualizer();
            }
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DISPLAYCHANGE:
            if (!g_unloading) {
                SetTimer(hWnd, TIMER_ID_MSG_DISPLAY_CHANGE, 200, nullptr);
            }
            return 0;

        case WM_SETTINGCHANGE: {
            RefreshAccentColorCache();

            if (g_overlayWnd && !g_unloading && g_settings.backgroundEnabled &&
                g_settings.bgBlur > 0) {
                FILETIME ft = GetWallpaperFileTime();
                if (CompareFileTime(&ft, &g_lastWallpaperTime) != 0) {
                    SetTimer(hWnd, TIMER_ID_MSG_WALLPAPER_REFRESH, 2000, nullptr);
                }
            }
            return 0;
        }

        case WM_DWMCOLORIZATIONCOLORCHANGED:
            RefreshAccentColorCache();
            return 0;

        // Key repeat can fire dozens of nudges a second, so restart a short
        // timer instead and write once the user stops moving. SetTimer with an
        // existing id re-arms it.
        case WM_APP_REQUEST_SAVE_POSITION:
            if (!g_unloading) {
                SetTimer(hWnd, TIMER_ID_MSG_SAVE_POSITION, 700, nullptr);
            }
            return 0;

        // A worker thread (the NPU build) has something to tell the user. It
        // arrives here so it can go through the same settings-problems summary
        // as everything else, which is only safe on this thread.
        case WM_APP_HW_NOTICE: {
            auto* msg = reinterpret_cast<std::wstring*>(lParam);
            if (msg) {
                if (!g_unloading) {
                    ReportSettingWarning(L"Hardware", L"Audio Analysis Device", *msg);
                    FlushSettingsIssues();
                }
                delete msg;
            }
            return 0;
        }

        // Device lost (wParam 1), posted from the render path. Rebuilds are
        // spaced at least two seconds apart so a GPU that keeps failing can't
        // spin this thread.
        case WM_APP_REBUILD_DEVICE:
            g_deviceRebuildQueued.store(false);
            if (g_unloading || !g_lazyInitialized || !g_initSucceeded) return 0;
            if (GetTickCount64() - g_lastDeviceRebuildTick < 2000) {
                SetTimer(hWnd, TIMER_ID_MSG_REBUILD_DEVICE, 2000, nullptr);
            } else {
                VizRebuildRenderDevice(wParam ? L"device lost" : L"requested");
            }
            return 0;

        case WM_TIMER:
            if (g_unloading) return 0;
            if (wParam == TIMER_ID_MSG_DISPLAY_CHANGE) {
                KillTimer(hWnd, TIMER_ID_MSG_DISPLAY_CHANGE);
                HandleDisplayChange();
            } else if (wParam == TIMER_ID_MSG_RECREATE_OVERLAY) {
                KillTimer(hWnd, TIMER_ID_MSG_RECREATE_OVERLAY);
                CreateOverlayWindow();
                // Running as our own process now (see the tool-mod boilerplate at
                // the end of this file), there's no explorer-side hook to tell us
                // the moment WorkerW becomes available -- keep polling for it
                // instead of giving up after a single attempt.
                if (!g_overlayWnd && !g_unloading) {
                    SetTimer(hWnd, TIMER_ID_MSG_RECREATE_OVERLAY, 1000, nullptr);
                }
            } else if (wParam == TIMER_ID_MSG_WALLPAPER_REFRESH) {
                KillTimer(hWnd, TIMER_ID_MSG_WALLPAPER_REFRESH);
                if (g_overlayWnd && g_settings.backgroundEnabled && g_settings.bgBlur > 0) {
                    ReleaseVisualResources();
                    RecreateVisualResources();
                    if (!g_fullscreenPaused.load()) RenderVisualizer();
                }
            } else if (wParam == TIMER_ID_MSG_REBUILD_DEVICE) {
                KillTimer(hWnd, TIMER_ID_MSG_REBUILD_DEVICE);
                if (g_lazyInitialized && g_initSucceeded) VizRebuildRenderDevice(L"device lost (retry)");
            } else if (wParam == TIMER_ID_MSG_SAVE_POSITION) {
                KillTimer(hWnd, TIMER_ID_MSG_SAVE_POSITION);
                PersistOverrideState();
            } else if (wParam == TIMER_ID_MSG_FONT_RECHECK) {
                // Re-run the font lookup that failed during settings load. A
                // cold boot resolves this within a few seconds; a name that is
                // actually wrong never will, and gets reported once the
                // attempts run out.
                if (!g_fontCheckPending || !g_settings.nowPlayingEnabled) {
                    g_fontCheckPending = false;
                    KillTimer(hWnd, TIMER_ID_MSG_FONT_RECHECK);
                } else if (IsFontInstalled(g_settings.nowPlayingFont)) {
                    Wh_Log(L"Now Playing Font resolved after %d s, no issue to report",
                           g_fontCheckAttempts + 1);
                    g_fontCheckPending = false;
                    KillTimer(hWnd, TIMER_ID_MSG_FONT_RECHECK);
                } else if (++g_fontCheckAttempts >= FONT_CHECK_MAX_ATTEMPTS) {
                    g_fontCheckPending = false;
                    KillTimer(hWnd, TIMER_ID_MSG_FONT_RECHECK);
                    ReportSettingIssue(
                        L"Appearance", L"Now Playing Font", g_settings.nowPlayingFont.c_str(),
                        L"the name of a font installed on this PC, exactly as Windows spells it",
                        L"whatever Windows substitutes (usually Segoe UI)");
                    // ReportSettingIssue only queues the line. LoadSettings
                    // normally flushes at its end, but this report happens long
                    // after that has returned, so without flushing here the
                    // warning would sit in the vector until the next settings
                    // change cleared it unseen. Safe from this thread: the flush
                    // honours showSettingsErrors and puts the dialog on a thread
                    // of its own.
                    FlushSettingsIssues();
                }
            } else if (wParam == TIMER_ID_MSG_FULLSCREEN_WATCH) {
                // The media strip is a plain layered window living alongside
                // whatever else is topmost, and it's the one piece of this mod
                // that has to survive in that crowd -- another app taking
                // topmost, or the window going away with the shell, would
                // otherwise leave it gone with nothing to bring it back. This
                // second-granularity check is cheap and makes it self-healing.
                if (g_settings.mediaControlsEnabled) {
                    if (!g_mediaWnd) {
                        Wh_Log(L"[Media] window missing, recreating");
                        CreateMediaControlWindow();
                    } else {
                        if (g_settings.mediaHideWhenCovered) {
                            // Measured against the strip's own rect on the same
                            // timer as the visualizer's occlusion check, for the
                            // same reason: EnumWindows is far too heavy to run
                            // any more often than this.
                            bool covered = IsMediaStripCovered();
                            if (covered && !g_mediaHiddenByCover) {
                                Wh_Log(L"[Media] covered, hiding strip");
                                g_mediaHiddenByCover = true;
                                ShowWindow(g_mediaWnd, SW_HIDE);
                            } else if (!covered && g_mediaHiddenByCover) {
                                Wh_Log(L"[Media] uncovered, showing strip");
                                g_mediaHiddenByCover = false;
                                RepositionAndRepaintMediaControls();
                            }
                        } else if (g_mediaHiddenByCover) {
                            // The setting was turned off while the strip was
                            // parked out of the way.
                            g_mediaHiddenByCover = false;
                            RepositionAndRepaintMediaControls();
                        }

                        // Still worth re-asserting even with Hide When Covered
                        // on -- that only accounts for the times we hid it
                        // ourselves, not for another app taking topmost.
                        if (!g_mediaHiddenByCover &&
                            (!IsWindowVisible(g_mediaWnd) ||
                             !(GetWindowLongPtr(g_mediaWnd, GWL_EXSTYLE) & WS_EX_TOPMOST))) {
                            Wh_Log(L"[Media] window not visible/topmost, re-asserting");
                            RepositionAndRepaintMediaControls();
                        }
                    }
                }

                bool shouldPause = false;

                if (g_settings.pauseOnFullscreen && IsFullscreenOrGameActive())
                    shouldPause = true;

                // Occlusion is evaluated on this same 1s timer rather than per
                // frame -- EnumWindows is far too heavy to run at frame rate,
                // and a second of latency on hide/show is imperceptible.
                if (!shouldPause && g_settings.pauseWhenObscured && IsVisualizerOccluded())
                    shouldPause = true;

                if (shouldPause) {
                    PauseForFullscreen();
                } else {
                    ResumeFromFullscreen();
                }
            }
            return 0;

        case WM_APP_SETTINGS_CHANGED:
            ApplySettingsChanged();
            return 0;

        case WM_DESTROY:
            g_messageWnd = nullptr;
            return 0;

        case WM_APP_CLEANUP:
            DestroyWindow(hWnd);
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool g_overlayClassRegistered = false;

bool RegisterOverlayWindowClass() {
    if (g_overlayClassRegistered) return true;
    WNDCLASS wc = {};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetCurrentModuleHandle();
    wc.lpszClassName = OVERLAY_WINDOW_CLASS;
    if (!RegisterClass(&wc)) return false;
    g_overlayClassRegistered = true;
    return true;
}

void UnregisterOverlayWindowClass() {
    if (g_overlayClassRegistered) {
        UnregisterClass(OVERLAY_WINDOW_CLASS, GetCurrentModuleHandle());
        g_overlayClassRegistered = false;
    }
}

bool EnsureLazyInitialized() {
    if (g_lazyInitialized.exchange(true)) return g_initSucceeded;

    if (!InitDirectX()) {
        Wh_Log(L"InitDirectX failed");
        return false;
    }

    if (!g_settings.pauseOnFullscreen || !IsFullscreenOrGameActive()) {
        StartVizCaptureThread();
    } else {
        Wh_Log(L"Fullscreen active at startup, starting paused");
        g_fullscreenPaused.store(true);
    }

    StartRenderThread();

    g_initSucceeded = true;
    return true;
}

void CreateOverlayWindow() {
    if (g_overlayWnd) return;
    if (!EnsureLazyInitialized()) return;

    HWND hWorkerW = GetWorkerW();
    if (!hWorkerW) {
        Wh_Log(L"Failed to find WorkerW");
        return;
    }

    if (!RegisterOverlayWindowClass()) return;

    HINSTANCE hInstance = GetCurrentModuleHandle();

    RECT rc;
    GetWindowRect(hWorkerW, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    g_overlayWnd = CreateWindowEx(
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, OVERLAY_WINDOW_CLASS,
        nullptr, WS_CHILD | WS_VISIBLE, 0, 0, width, height, hWorkerW, nullptr, hInstance,
        nullptr);
    if (!g_overlayWnd) return;

    // A pending flag left set from a previous overlay window (e.g. one that just
    // got destroyed and is being recreated) would otherwise permanently block the
    // render thread from ever posting another tick -- nothing clears a flag whose
    // matching message was queued for a now-dead HWND.
    g_renderTickPending.store(false, std::memory_order_relaxed);

    if (!g_gsmtcStarted) {
        InitGsmtcListener();
        g_gsmtcStarted = true;
    }

    if (CreateSwapChainResources()) {
        if (!g_fullscreenPaused.load()) {
            RenderVisualizer();
        }
    }
}

bool g_messageClassRegistered = false;

bool RegisterMessageWindowClass() {
    if (g_messageClassRegistered) return true;
    WNDCLASS wc = {};
    wc.lpfnWndProc = MessageWndProc;
    wc.hInstance = GetCurrentModuleHandle();
    wc.lpszClassName = MESSAGE_WINDOW_CLASS;
    if (!RegisterClass(&wc)) return false;
    g_messageClassRegistered = true;
    return true;
}

void UnregisterMessageWindowClass() {
    if (g_messageClassRegistered) {
        UnregisterClass(MESSAGE_WINDOW_CLASS, GetCurrentModuleHandle());
        g_messageClassRegistered = false;
    }
}

void CreateMessageWindow() {
    if (g_messageWnd) return;
    if (!RegisterMessageWindowClass()) return;

    HINSTANCE hInstance = GetCurrentModuleHandle();
    g_messageWnd = CreateWindowEx(0, MESSAGE_WINDOW_CLASS, nullptr, 0, 0, 0, 0, 0, nullptr,
                                  nullptr, hInstance, nullptr);
    if (g_messageWnd) {
        SetTimer(g_messageWnd, TIMER_ID_MSG_FULLSCREEN_WATCH, 1000, nullptr);
        // Settings are loaded before this window exists on the init path, so a
        // font check that already failed has nowhere to arm its retry until now.
        if (g_fontCheckPending) {
            SetTimer(g_messageWnd, TIMER_ID_MSG_FONT_RECHECK, 1000, nullptr);
        }
    }
}

void LoadSettings() {
    g_settingsIssues.clear();
    g_settings.showSettingsErrors = Wh_GetIntSetting(L"validation.showErrors") != 0;

    PCWSTR shape = Wh_GetStringSetting(L"appearance.shape");
    g_settings.shape = (wcscmp(shape, L"mountain") == 0)     ? VizShape::Mountain
                       : (wcscmp(shape, L"mirror") == 0)     ? VizShape::Mirror
                       : (wcscmp(shape, L"wave") == 0)       ? VizShape::Wave
                       : (wcscmp(shape, L"breathe") == 0)    ? VizShape::Breathe
                       : (wcscmp(shape, L"dots") == 0)       ? VizShape::Dots
                       : (wcscmp(shape, L"radial") == 0)     ? VizShape::Radial
                       : (wcscmp(shape, L"oscilloscope") == 0) ? VizShape::Oscilloscope
                                                               : VizShape::Stereo;
    Wh_FreeStringSetting(shape);

    PCWSTR orientation = Wh_GetStringSetting(L"appearance.orientation");
    g_settings.orientation =
        (wcscmp(orientation, L"vertical") == 0) ? VizOrientation::Vertical : VizOrientation::Horizontal;
    Wh_FreeStringSetting(orientation);

    g_settings.barCount = std::clamp(Wh_GetIntSetting(L"appearance.barCount"), 1, VIZ_BARS_MAX);
    g_settings.barWidth = std::max(1, Wh_GetIntSetting(L"appearance.barWidth"));
    g_settings.barGap = std::max(0, Wh_GetIntSetting(L"appearance.barGap"));
    g_settings.barMaxSize = std::max(2, Wh_GetIntSetting(L"appearance.barMaxSize"));
    g_settings.barIdleSize = std::max(0, Wh_GetIntSetting(L"appearance.barIdleSize"));

    {
        float v[4];
        ReadQuadSetting(L"appearance.barCornerRadius", L"Appearance", L"Bar Corner Radius",
                        L"top-left, top-right, bottom-right, bottom-left", 3.0f, false, v);
        g_settings.barRadiusTL = v[0];
        g_settings.barRadiusTR = v[1];
        g_settings.barRadiusBR = v[2];
        g_settings.barRadiusBL = v[3];
    }

    PCWSTR colorMode = Wh_GetStringSetting(L"appearance.colorMode");
    g_settings.colorMode = (wcscmp(colorMode, L"gradient") == 0)          ? VizColorMode::Gradient
                           : (wcscmp(colorMode, L"reactive_gradient") == 0) ? VizColorMode::ReactiveGradient
                           : (wcscmp(colorMode, L"accent") == 0)            ? VizColorMode::Accent
                           : (wcscmp(colorMode, L"album_art") == 0)         ? VizColorMode::AlbumArt
                           : (wcscmp(colorMode, L"dynamic_album") == 0)     ? VizColorMode::DynamicAlbum
                           : (wcscmp(colorMode, L"acrylic") == 0)           ? VizColorMode::Acrylic
                           : (wcscmp(colorMode, L"rainbow") == 0)           ? VizColorMode::RainbowCycle
                           : (wcscmp(colorMode, L"tourne") == 0)            ? VizColorMode::Tourne
                                                                             : VizColorMode::Solid;
    Wh_FreeStringSetting(colorMode);

    ReadColorSetting(L"appearance.color", L"Appearance", L"Color", 255, 255, 255, 255,
                     &g_settings.colorA, &g_settings.colorR, &g_settings.colorG, &g_settings.colorB);
    ReadColorSetting(L"appearance.gradientColor1", L"Appearance", L"Gradient Color 1",
                     255, 30, 215, 96,
                     &g_settings.grad1A, &g_settings.grad1R, &g_settings.grad1G, &g_settings.grad1B);
    ReadColorSetting(L"appearance.gradientColor2", L"Appearance", L"Gradient Color 2",
                     255, 0, 180, 255,
                     &g_settings.grad2A, &g_settings.grad2R, &g_settings.grad2G, &g_settings.grad2B);

    g_settings.sensitivity = std::clamp(Wh_GetIntSetting(L"appearance.sensitivity"), 0, 300);

    PCWSTR sensCurve = Wh_GetStringSetting(L"appearance.sensitivityCurve");
    g_settings.sensitivityCurve = (wcscmp(sensCurve, L"exponential") == 0) ? VizSensitivityCurve::Exponential
                                 : (wcscmp(sensCurve, L"power") == 0)      ? VizSensitivityCurve::Power
                                                                           : VizSensitivityCurve::Knee;
    Wh_FreeStringSetting(sensCurve);

    g_settings.inputGainDb =
        (float)std::clamp(Wh_GetIntSetting(L"appearance.inputGain"), -24, 24);
    g_settings.autoGain = Wh_GetIntSetting(L"appearance.autoGain") != 0;
    g_settings.autoGainMaxDb =
        (float)std::clamp(Wh_GetIntSetting(L"appearance.autoGainMaxBoost"), 0, 24);

    g_settings.smoothing = std::clamp(Wh_GetIntSetting(L"appearance.smoothing"), 0, 100);

    PCWSTR eq = Wh_GetStringSetting(L"appearance.eqPreset");
    g_settings.eq = (wcscmp(eq, L"bass") == 0)       ? VizEQ::Bass
                   : (wcscmp(eq, L"rock") == 0)       ? VizEQ::Rock
                   : (wcscmp(eq, L"pop") == 0)        ? VizEQ::Pop
                   : (wcscmp(eq, L"jazz") == 0)       ? VizEQ::Jazz
                   : (wcscmp(eq, L"electronic") == 0) ? VizEQ::Electronic
                                                       : VizEQ::Default;
    Wh_FreeStringSetting(eq);

    g_settings.horizontalPosition = ReadNumberSetting(L"position.horizontalPosition", L"Position",
                                                      L"Horizontal Position", 50.0f, 0.0f, 100.0f);
    g_settings.verticalPosition = ReadNumberSetting(L"position.verticalPosition", L"Position",
                                                    L"Vertical Position", 88.0f, 0.0f, 100.0f);
    g_settings.monitor = std::max(1, Wh_GetIntSetting(L"position.monitor"));

    g_settings.keyMoveEnabled = Wh_GetIntSetting(L"interaction.keyMoveEnabled") != 0;

    PCWSTR keyMoveModifier = Wh_GetStringSetting(L"interaction.keyMoveModifier");
    g_settings.keyMoveModifier =
        (wcscmp(keyMoveModifier, L"ctrl_shift") == 0) ? (VIZ_MOD_CTRL | VIZ_MOD_SHIFT)
        : (wcscmp(keyMoveModifier, L"alt_shift") == 0) ? (VIZ_MOD_ALT | VIZ_MOD_SHIFT)
        : (wcscmp(keyMoveModifier, L"win_alt") == 0)   ? (VIZ_MOD_WIN | VIZ_MOD_ALT)
        : (wcscmp(keyMoveModifier, L"win_shift") == 0) ? (VIZ_MOD_WIN | VIZ_MOD_SHIFT)
        : (wcscmp(keyMoveModifier, L"ctrl") == 0)      ? (unsigned)VIZ_MOD_CTRL
        : (wcscmp(keyMoveModifier, L"alt") == 0)       ? (unsigned)VIZ_MOD_ALT
        : (wcscmp(keyMoveModifier, L"shift") == 0)     ? (unsigned)VIZ_MOD_SHIFT
        : (wcscmp(keyMoveModifier, L"win") == 0)       ? (unsigned)VIZ_MOD_WIN
                                                       : (VIZ_MOD_CTRL | VIZ_MOD_ALT);
    Wh_FreeStringSetting(keyMoveModifier);

    PCWSTR keyMoveKeys = Wh_GetStringSetting(L"interaction.keyMoveKeys");
    g_settings.keyMoveKeys = (wcscmp(keyMoveKeys, L"arrows") == 0) ? VizKeyMoveKeys::Arrows
                            : (wcscmp(keyMoveKeys, L"wasd") == 0)  ? VizKeyMoveKeys::Wasd
                                                                   : VizKeyMoveKeys::Both;
    Wh_FreeStringSetting(keyMoveKeys);

    g_settings.keyMoveStep = std::clamp(Wh_GetIntSetting(L"interaction.keyMoveStep"), 1, 500);
    g_settings.keyMoveFastStep = std::clamp(Wh_GetIntSetting(L"interaction.keyMoveFastStep"), 1, 500);

    PCWSTR keyMoveFastKey = Wh_GetStringSetting(L"interaction.keyMoveFastKey");
    g_settings.keyMoveFastKey = (wcscmp(keyMoveFastKey, L"ctrl") == 0)  ? (unsigned)VIZ_MOD_CTRL
                               : (wcscmp(keyMoveFastKey, L"alt") == 0)  ? (unsigned)VIZ_MOD_ALT
                               : (wcscmp(keyMoveFastKey, L"win") == 0)  ? (unsigned)VIZ_MOD_WIN
                               : (wcscmp(keyMoveFastKey, L"none") == 0) ? (unsigned)VIZ_MOD_NONE
                                                                        : (unsigned)VIZ_MOD_SHIFT;
    Wh_FreeStringSetting(keyMoveFastKey);

    // A single-key modifier turns everyday shortcuts into visualizer moves, and
    // this hook swallows the keypress outright -- so the app underneath doesn't
    // fall back to its own behaviour, it just never hears about it. Ctrl+S
    // quietly not saving is the kind of thing you find out about much later,
    // which makes this worth saying up front rather than leaving to be
    // discovered.
    if (g_settings.keyMoveEnabled) {
        unsigned m = g_settings.keyMoveModifier;
        bool singleKey = m != 0 && (m & (m - 1)) == 0;
        if (singleKey) {
            std::wstring modName = ModKeyFlagsName(m);
            std::wstring detail =
                L"A one-key modifier means this mod swallows ordinary shortcuts. With " +
                modName + L" you will lose ";

            bool wasd = g_settings.keyMoveKeys != VizKeyMoveKeys::Arrows;
            bool arrows = g_settings.keyMoveKeys != VizKeyMoveKeys::Wasd;
            if (wasd) {
                detail += modName + L"+W, +A, +S and +D";
                if (m == VIZ_MOD_CTRL) detail += L" (so no Select All, and no Save)";
            }
            if (arrows) {
                if (wasd) detail += L", plus ";
                detail += modName + L"+arrow keys (word-by-word cursor movement)";
            }
            detail += L", and 1-4 and Home alongside it.\r\n      "
                      L"Everything still works -- this is only a heads-up. Switch to a two-key "
                      L"combo such as Ctrl + Alt, or set Direction Keys to Arrow Keys only, to "
                      L"get those shortcuts back.";

            ReportSettingWarning(L"Interaction", L"Keyboard Move Modifier", detail);
        }
    }

    // A fast key that's already part of the modifier can never be "additionally
    // held", so the fast step would silently never fire. Say so instead.
    if (g_settings.keyMoveFastKey && (g_settings.keyMoveFastKey & g_settings.keyMoveModifier)) {
        WCHAR expected[192];
        swprintf_s(expected, L"a key that isn't already part of the modifier (%s)",
                   ModKeyFlagsName(g_settings.keyMoveModifier));
        ReportSettingIssue(L"Interaction", L"Keyboard Move Fast Key",
                           ModKeyFlagsName(g_settings.keyMoveFastKey), expected,
                           L"no fast step at all");
        g_settings.keyMoveFastKey = VIZ_MOD_NONE;
    }

    g_settings.dragEnabled = Wh_GetIntSetting(L"interaction.dragEnabled") != 0;

    PCWSTR dragModifier = Wh_GetStringSetting(L"interaction.dragModifier");
    g_settings.dragModifier = (wcscmp(dragModifier, L"none") == 0)  ? VizDragModifier::None
                             : (wcscmp(dragModifier, L"alt") == 0)   ? VizDragModifier::Alt
                             : (wcscmp(dragModifier, L"shift") == 0) ? VizDragModifier::Shift
                             : (wcscmp(dragModifier, L"win") == 0)   ? VizDragModifier::Win
                                                                     : VizDragModifier::Ctrl;
    Wh_FreeStringSetting(dragModifier);

    PCWSTR dragButton = Wh_GetStringSetting(L"interaction.dragButton");
    g_settings.dragButton = (wcscmp(dragButton, L"left") == 0)  ? VizDragButton::Left
                           : (wcscmp(dragButton, L"right") == 0) ? VizDragButton::Right
                                                                  : VizDragButton::Middle;
    Wh_FreeStringSetting(dragButton);

    PCWSTR verticalAnchor = Wh_GetStringSetting(L"appearance.verticalAnchor");
    g_settings.verticalAnchor = (wcscmp(verticalAnchor, L"top")    == 0) ? VizAnchor::Top
                              : (wcscmp(verticalAnchor, L"middle") == 0) ? VizAnchor::Middle
                                                                         : VizAnchor::Bottom;
    Wh_FreeStringSetting(verticalAnchor);

    g_settings.backgroundEnabled = Wh_GetIntSetting(L"background.enabled") != 0;

    ReadColorSetting(L"background.color", L"Background", L"Color", 0x60, 0, 0, 0,
                     &g_settings.bgA, &g_settings.bgR, &g_settings.bgG, &g_settings.bgB);

    {
        // Negative values are allowed on purpose -- they shrink that side of the
        // box inward, past the bars if pushed far enough. Extreme values are
        // clamped later, at the point the box's actual rect is built, rather
        // than here where the bar/panel size isn't known yet.
        float v[4];
        ReadQuadSetting(L"background.padding", L"Background", L"Padding",
                        L"left, right, top, bottom", 24.0f, true, v);
        g_settings.bgPaddingL = (int)v[0];
        g_settings.bgPaddingR = (int)v[1];
        g_settings.bgPaddingT = (int)v[2];
        g_settings.bgPaddingB = (int)v[3];
    }

    {
        float v[4];
        ReadQuadSetting(L"background.cornerRadius", L"Background", L"Corner Radius",
                        L"top-left, top-right, bottom-right, bottom-left", 14.0f, false, v);
        g_settings.bgRadiusTL = v[0];
        g_settings.bgRadiusTR = v[1];
        g_settings.bgRadiusBR = v[2];
        g_settings.bgRadiusBL = v[3];
    }
    g_settings.bgBlur = std::max(0, Wh_GetIntSetting(L"background.blur"));
    g_settings.bgBorderSize = std::max(0, Wh_GetIntSetting(L"background.borderSize"));

    ReadColorSetting(L"background.borderColor", L"Background", L"Border Color", 0x40, 255, 255, 255,
                     &g_settings.borderA, &g_settings.borderR, &g_settings.borderG,
                     &g_settings.borderB);

    g_settings.targetFps = std::max(1, Wh_GetIntSetting(L"performance.targetFps"));
    g_settings.pauseOnFullscreen = Wh_GetIntSetting(L"performance.pauseOnFullscreen") != 0;
    g_settings.pauseWhenSilentSeconds = std::max(0, Wh_GetIntSetting(L"performance.pauseWhenSilentSeconds"));

    g_settings.peakHoldEnabled = Wh_GetIntSetting(L"appearance.peakHoldEnabled") != 0;

    ReadColorSetting(L"appearance.peakHoldColor", L"Appearance", L"Peak Hold Cap Color",
                     255, 0, 180, 255,
                     &g_settings.peakHoldA, &g_settings.peakHoldR, &g_settings.peakHoldG,
                     &g_settings.peakHoldB);
    g_settings.beatFlashEnabled = Wh_GetIntSetting(L"appearance.beatFlashEnabled") != 0;

    ReadColorSetting(L"appearance.beatFlashColor", L"Appearance", L"Beat Flash Color",
                     255, 255, 255, 255,
                     &g_settings.beatFlashA, &g_settings.beatFlashR, &g_settings.beatFlashG,
                     &g_settings.beatFlashB);

    g_settings.beatFlashIntensity = std::clamp(Wh_GetIntSetting(L"appearance.beatFlashIntensity"), 0, 300);
    g_settings.rainbowSpeed = std::clamp(Wh_GetIntSetting(L"appearance.rainbowSpeed"), 1, 300);

    g_settings.nowPlayingEnabled = Wh_GetIntSetting(L"appearance.nowPlayingEnabled") != 0;
    ReadColorSetting(L"appearance.nowPlayingColor", L"Appearance", L"Now Playing Text Color",
                     255, 255, 255, 255,
                     &g_settings.nowPlayingA, &g_settings.nowPlayingR, &g_settings.nowPlayingG,
                     &g_settings.nowPlayingB);
    PCWSTR nowPlayingFont = Wh_GetStringSetting(L"appearance.nowPlayingFont");
    g_settings.nowPlayingFont = (nowPlayingFont && *nowPlayingFont) ? nowPlayingFont : L"Segoe UI";
    // Only worth checking when the text is actually going to be drawn -- an
    // unused font name being wrong isn't something to interrupt anyone over.
    //
    // A failure here is not reported yet: on a cold boot the font service may
    // not have registered everything, so this arms the retry instead and the
    // warning only happens if it is still missing once things have settled.
    if (g_settings.nowPlayingEnabled && !IsFontInstalled(g_settings.nowPlayingFont)) {
        g_fontCheckPending = true;
        g_fontCheckAttempts = 0;
        if (g_messageWnd) SetTimer(g_messageWnd, TIMER_ID_MSG_FONT_RECHECK, 1000, nullptr);
    } else {
        g_fontCheckPending = false;
        if (g_messageWnd) KillTimer(g_messageWnd, TIMER_ID_MSG_FONT_RECHECK);
    }
    Wh_FreeStringSetting(nowPlayingFont);
    g_settings.nowPlayingFontSize = std::max(6, Wh_GetIntSetting(L"appearance.nowPlayingFontSize"));
    g_settings.nowPlayingDisplaySeconds =
        std::max(0, Wh_GetIntSetting(L"appearance.nowPlayingDisplaySeconds"));

    // Clamped rather than free: each pixel of offset widens the render surface
    // by the same amount, so a stray extra zero shouldn't silently cost memory
    // and fill rate for a surface mostly full of nothing.
    g_settings.nowPlayingOffsetX =
        std::clamp(Wh_GetIntSetting(L"appearance.nowPlayingOffsetX"), -4000, 4000);
    g_settings.nowPlayingOffsetY =
        std::clamp(Wh_GetIntSetting(L"appearance.nowPlayingOffsetY"), -4000, 4000);
    g_settings.peakFreqOffsetX =
        std::clamp(Wh_GetIntSetting(L"appearance.peakFreqOffsetX"), -4000, 4000);
    g_settings.peakFreqOffsetY =
        std::clamp(Wh_GetIntSetting(L"appearance.peakFreqOffsetY"), -4000, 4000);

    ReadColorSetting(L"appearance.nowPlayingBgColor", L"Appearance", L"Now Playing Background",
                     0, 0, 0, 0,
                     &g_settings.npBgA, &g_settings.npBgR, &g_settings.npBgG, &g_settings.npBgB);
    g_settings.npBgPadding =
        std::clamp(Wh_GetIntSetting(L"appearance.nowPlayingBgPadding"), 0, 200);
    g_settings.npBgCornerRadius =
        std::clamp(Wh_GetIntSetting(L"appearance.nowPlayingBgCornerRadius"), 0, 200);
    g_settings.npBgBorderSize =
        std::clamp(Wh_GetIntSetting(L"appearance.nowPlayingBgBorderSize"), 0, 100);
    ReadColorSetting(L"appearance.nowPlayingBgBorderColor", L"Appearance",
                     L"Now Playing Background Border Color", 0x40, 255, 255, 255,
                     &g_settings.npBgBorderA, &g_settings.npBgBorderR,
                     &g_settings.npBgBorderG, &g_settings.npBgBorderB);

    ReadColorSetting(L"appearance.peakFreqBgColor", L"Appearance", L"Peak Readout Background",
                     0, 0, 0, 0,
                     &g_settings.pfBgA, &g_settings.pfBgR, &g_settings.pfBgG, &g_settings.pfBgB);
    g_settings.pfBgPadding =
        std::clamp(Wh_GetIntSetting(L"appearance.peakFreqBgPadding"), 0, 200);
    g_settings.pfBgCornerRadius =
        std::clamp(Wh_GetIntSetting(L"appearance.peakFreqBgCornerRadius"), 0, 200);
    g_settings.pfBgBorderSize =
        std::clamp(Wh_GetIntSetting(L"appearance.peakFreqBgBorderSize"), 0, 100);
    ReadColorSetting(L"appearance.peakFreqBgBorderColor", L"Appearance",
                     L"Peak Readout Background Border Color", 0x40, 255, 255, 255,
                     &g_settings.pfBgBorderA, &g_settings.pfBgBorderR,
                     &g_settings.pfBgBorderG, &g_settings.pfBgBorderB);

    g_settings.autoHideEnabled = Wh_GetIntSetting(L"performance.autoHideEnabled") != 0;
    g_settings.autoHideDelaySeconds = std::max(0, Wh_GetIntSetting(L"performance.autoHideDelaySeconds"));
    g_settings.pauseWhenObscured = Wh_GetIntSetting(L"performance.pauseWhenObscured") != 0;
    g_settings.obscuredThresholdPercent =
        ReadThresholdPercentSetting(L"performance.obscuredThresholdPercent", L"Performance",
                                    L"Pause When Covered - Threshold", 100);

    PCWSTR fftSizeStr = Wh_GetStringSetting(L"appearance.fftSize");
    int fftSize = _wtoi(fftSizeStr);
    g_settings.fftSize = (fftSize == 1024 || fftSize == 2048 || fftSize == 4096 || fftSize == 8192)
                              ? fftSize : 1024;
    Wh_FreeStringSetting(fftSizeStr);

    PCWSTR freqScale = Wh_GetStringSetting(L"appearance.freqScale");
    g_settings.freqScale = (wcscmp(freqScale, L"linear") == 0) ? VizFreqScale::Linear
                          : (wcscmp(freqScale, L"mel") == 0)   ? VizFreqScale::Mel
                                                                : VizFreqScale::Log;
    Wh_FreeStringSetting(freqScale);

    g_settings.peakFreqEnabled = Wh_GetIntSetting(L"appearance.peakFreqEnabled") != 0;

    PCWSTR pfH = Wh_GetStringSetting(L"appearance.peakFreqAlignH");
    g_settings.peakFreqAlignH = (wcscmp(pfH, L"left") == 0)   ? VizTextAlignH::Left
                              : (wcscmp(pfH, L"center") == 0) ? VizTextAlignH::Center
                                                              : VizTextAlignH::Right;
    Wh_FreeStringSetting(pfH);

    PCWSTR pfV = Wh_GetStringSetting(L"appearance.peakFreqAlignV");
    g_settings.peakFreqAlignV = (wcscmp(pfV, L"above") == 0)  ? VizTextAlignV::Above
                              : (wcscmp(pfV, L"middle") == 0) ? VizTextAlignV::Middle
                              : (wcscmp(pfV, L"bottom") == 0) ? VizTextAlignV::Bottom
                              : (wcscmp(pfV, L"below") == 0)  ? VizTextAlignV::Below
                                                              : VizTextAlignV::Top;
    Wh_FreeStringSetting(pfV);
    g_settings.oscilloscopeMultibandEnabled =
        Wh_GetIntSetting(L"appearance.oscilloscopeMultibandEnabled") != 0;
    g_settings.oscilloscopeWindowMs =
        std::clamp(Wh_GetIntSetting(L"appearance.oscilloscopeWindowMs"), 5, 250);
    g_settings.oscilloscopeDamping =
        std::clamp(Wh_GetIntSetting(L"appearance.oscilloscopeDamping"), 0, 100);

    g_settings.mediaControlsEnabled = Wh_GetIntSetting(L"media_controls.enabled") != 0;

    ReadColorSetting(L"media_controls.iconColor", L"Media Controls", L"Icon Color",
                     255, 255, 255, 255,
                     &g_settings.mediaIconColorA, &g_settings.mediaIconColorR,
                     &g_settings.mediaIconColorG, &g_settings.mediaIconColorB);

    g_settings.mediaIconPrevPath =
        ReadIconPathSetting(L"media_controls.iconPrevPath", L"Media Controls", L"Previous Icon Path");
    g_settings.mediaIconPlayPath =
        ReadIconPathSetting(L"media_controls.iconPlayPath", L"Media Controls", L"Play Icon Path");
    g_settings.mediaIconPausePath =
        ReadIconPathSetting(L"media_controls.iconPausePath", L"Media Controls", L"Pause Icon Path");
    g_settings.mediaIconNextPath =
        ReadIconPathSetting(L"media_controls.iconNextPath", L"Media Controls", L"Next Icon Path");

    g_settings.mediaIconSize = std::clamp(Wh_GetIntSetting(L"media_controls.iconSize"), 8, 256);
    g_settings.mediaIconSpacing = std::clamp(Wh_GetIntSetting(L"media_controls.iconSpacing"), 0, 200);

    ReadColorSetting(L"media_controls.plateColor", L"Media Controls", L"Backing Plate Color",
                     0, 0, 0, 0,
                     &g_settings.mediaPlateA, &g_settings.mediaPlateR,
                     &g_settings.mediaPlateG, &g_settings.mediaPlateB);
    g_settings.mediaPlateCornerRadius =
        std::clamp(Wh_GetIntSetting(L"media_controls.plateCornerRadius"), 0, 256);
    g_settings.mediaPlatePadding =
        std::clamp(Wh_GetIntSetting(L"media_controls.platePadding"), 0, 200);
    g_settings.mediaPlateBorderSize =
        std::clamp(Wh_GetIntSetting(L"media_controls.plateBorderSize"), 0, 100);
    ReadColorSetting(L"media_controls.plateBorderColor", L"Media Controls",
                     L"Backing Plate Border Color", 0x40, 255, 255, 255,
                     &g_settings.mediaPlateBorderA, &g_settings.mediaPlateBorderR,
                     &g_settings.mediaPlateBorderG, &g_settings.mediaPlateBorderB);
    g_settings.mediaHideWhenCovered = Wh_GetIntSetting(L"media_controls.hideWhenCovered") != 0;
    g_settings.mediaCoveredThresholdPercent =
        ReadThresholdPercentSetting(L"media_controls.coveredThresholdPercent", L"Media Controls",
                                    L"Hide When Covered - Threshold", 50);

    g_settings.mediaHorizontalPosition =
        ReadNumberSetting(L"media_controls.horizontalPosition", L"Media Controls",
                          L"Horizontal Position", 50.0f, 0.0f, 100.0f);
    g_settings.mediaVerticalPosition =
        ReadNumberSetting(L"media_controls.verticalPosition", L"Media Controls",
                          L"Vertical Position", 95.0f, 0.0f, 100.0f);

    PCWSTR renderDevice = Wh_GetStringSetting(L"hardware.renderDevice");
    g_settings.renderDevice = (wcscmp(renderDevice, L"integrated") == 0) ? VizRenderDevice::Integrated
                            : (wcscmp(renderDevice, L"discrete") == 0)   ? VizRenderDevice::Discrete
                            : (wcscmp(renderDevice, L"cpu") == 0)        ? VizRenderDevice::Cpu
                                                                         : VizRenderDevice::Auto;
    Wh_FreeStringSetting(renderDevice);

    PCWSTR analysisDevice = Wh_GetStringSetting(L"hardware.analysisDevice");
    g_settings.analysisDevice = (wcscmp(analysisDevice, L"npu") == 0) ? VizAnalysisDevice::Npu
                                                                      : VizAnalysisDevice::Cpu;
    Wh_FreeStringSetting(analysisDevice);

    {
        // Same quote-stripping as the icon paths: "Copy as path" brings quotes.
        PCWSTR raw = Wh_GetStringSetting(L"hardware.npuRuntimePath");
        std::wstring path = raw ? raw : L"";
        Wh_FreeStringSetting(raw);
        while (!path.empty() && (path.front() == L'"' || path.front() == L' ')) path.erase(path.begin());
        while (!path.empty() && (path.back() == L'"' || path.back() == L' ' || path.back() == L'\\'))
            path.pop_back();
        if (!path.empty() && g_settings.analysisDevice == VizAnalysisDevice::Npu) {
            DWORD attr = GetFileAttributes(path.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                ReportSettingIssue(L"Hardware", L"NPU Runtime Folder", path.c_str(),
                                   L"a folder that exists (the one holding openvino_c.dll, or the "
                                   L"root of the extracted OpenVINO archive)",
                                   L"the automatic search");
                path.clear();
            }
        }
        g_settings.npuRuntimePath = path;
    }

    PCWSTR smoothMode = Wh_GetStringSetting(L"hardware.smoothMode");
    g_settings.smoothMode = (wcscmp(smoothMode, L"on") == 0)    ? VizSmoothMode::On
                          : (wcscmp(smoothMode, L"off") == 0)   ? VizSmoothMode::Off
                                                                : VizSmoothMode::Auto;
    Wh_FreeStringSetting(smoothMode);

    // The NPU worker reads only its own copies, taken here.
    ttnpu::Configure(g_settings.analysisDevice == VizAnalysisDevice::Npu, g_settings.npuRuntimePath);
    // Smooth Mode's Auto depends on the device, which may not exist yet on the
    // first load; InitDirectX resolves it again once it does.
    VizUpdateSmoothActive();

    FlushSettingsIssues();
}

HANDLE g_uiThread = nullptr;
DWORD g_uiThreadId = 0;

DWORD WINAPI UiThreadProc(LPVOID) {
    // This mod runs as its own dedicated process (see the tool-mod boilerplate
    // at the end of this file). WhTool_ModInit's own calling thread does not
    // survive -- the tool-mod launcher hooks the process's real entry point
    // and exits that thread once startup continues past mod init -- so every
    // window, and the message loop that keeps them alive, has to live on a
    // thread we spin up and own ourselves rather than whatever thread called
    // WhTool_ModInit.
    // Apartment-threaded, because this thread owns windows and pumps messages.
    // Without this, every CoCreateInstance made from here fails outright with
    // CO_E_NOTINITIALIZED -- which is why custom Media Controls icon paths
    // never loaded: WIC's imaging factory is created on this thread, silently
    // failed to activate, and fell back to the built-in glyph every time.
    HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comHr)) {
        Wh_Log(L"CoInitializeEx on UI thread failed, hr=0x%08X", comHr);
    }

    LoadPositionOverride();

    CreateMessageWindow();
    CreateMediaControlWindow();
    CreateOverlayWindow();
    if (!g_overlayWnd && g_messageWnd) {
        SetTimer(g_messageWnd, TIMER_ID_MSG_RECREATE_OVERLAY, 1000, nullptr);
    }

    InitInputHooks();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (SUCCEEDED(comHr)) CoUninitialize();
    return 0;
}

BOOL WhTool_ModInit() {
    Wh_Log(L">");

    // Running injected into explorer.exe, this was inherited for free -- the
    // shell process is always Per-Monitor-V2 DPI aware. Running as our own
    // standalone process now, nothing declares that for us, so without this
    // call every GetMonitorInfo/GetDpiForMonitor-based position and size
    // calculation in the mod (the visualizer's own box, and the media
    // controls window) risks running against a DPI-virtualized view of the
    // desktop instead of true physical pixels on any scaled monitor.
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        Wh_Log(L"SetProcessDpiAwarenessContext failed, error %u", GetLastError());
    }

    LoadSettings();

    RefreshAccentColorCache();

    g_uiThread = CreateThread(nullptr, 0, UiThreadProc, nullptr, 0, &g_uiThreadId);
    return g_uiThread != nullptr;
}

void WhTool_ModUninit() {
    Wh_Log(L">");

    g_unloading = true;

    StopRenderThread();

    if (g_overlayWnd) SendMessage(g_overlayWnd, WM_APP_CLEANUP, 0, 0);
    if (g_messageWnd) SendMessage(g_messageWnd, WM_APP_CLEANUP, 0, 0);
    if (g_mediaWnd) SendMessage(g_mediaWnd, WM_APP_CLEANUP, 0, 0);

    UnregisterOverlayWindowClass();
    UnregisterMessageWindowClass();
    UnregisterMediaWindowClass();

    StopVizCaptureThread();
    // After the capture thread, which hands its NPU engine back on the way out.
    ttnpu::Shutdown();
    UninitDirectX();

    if (g_gsmtcStopEvent) {
        SetEvent(g_gsmtcStopEvent);
    }
    if (g_gsmtcThread && g_gsmtcThread->joinable()) {
        g_gsmtcThread->join();
    }
    g_gsmtcThread.reset();
    if (g_gsmtcStopEvent) {
        CloseHandle(g_gsmtcStopEvent);
        g_gsmtcStopEvent = nullptr;
    }

    if (g_albumArtThread) {
        if (g_albumArtThread->joinable()) {
            HANDLE hThread = g_albumArtThread->native_handle();
            if (WaitForSingleObject(hThread, 3000) == WAIT_OBJECT_0) {
                g_albumArtThread->join();
            } else {
                g_albumArtThread->detach();
            }
        }
        delete g_albumArtThread;
        g_albumArtThread = nullptr;
    }

    if (g_mediaCmdThread) {
        if (g_mediaCmdThread->joinable()) {
            HANDLE hThread = g_mediaCmdThread->native_handle();
            if (WaitForSingleObject(hThread, 3000) == WAIT_OBJECT_0) {
                g_mediaCmdThread->join();
            } else {
                g_mediaCmdThread->detach();
            }
        }
        delete g_mediaCmdThread;
        g_mediaCmdThread = nullptr;
    }

    g_gsmtcStarted = false;

    UninitInputHooks();

    if (g_uiThreadId) {
        PostThreadMessage(g_uiThreadId, WM_QUIT, 0, 0);
    }
    if (g_uiThread) {
        WaitForSingleObject(g_uiThread, 3000);
        CloseHandle(g_uiThread);
        g_uiThread = nullptr;
    }
}

void ApplySettingsChanged() {
    Wh_Log(L">");

    int oldMonitor = g_settings.monitor;
    VizColorMode oldColorMode = g_settings.colorMode;

    LoadSettings();

    // Creation at startup can fail (or the window can have been destroyed
    // since), and until now nothing retried -- turning Media Controls on in
    // settings then looked like it did nothing at all.
    if (!g_mediaWnd) {
        CreateMediaControlWindow();
    } else {
        // Cleared unconditionally so that turning Hide When Covered off, or
        // moving the strip somewhere clear, brings it straight back instead of
        // waiting for the next watch tick to notice.
        g_mediaHiddenByCover = false;
        RecreateMediaControlResources();
        RepositionAndRepaintMediaControls();
    }

    if ((g_settings.colorMode == VizColorMode::AlbumArt ||
         g_settings.colorMode == VizColorMode::DynamicAlbum) &&
        ((oldColorMode != VizColorMode::AlbumArt &&
          oldColorMode != VizColorMode::DynamicAlbum) || !g_albumArtColorReady.load()))
        FetchAlbumArtColorAsync();
    else if (g_settings.nowPlayingEnabled)
        FetchAlbumArtColorAsync();

    if (!g_lazyInitialized || !g_initSucceeded) return;

    if (!g_fullscreenPaused.load()) {
        if (g_settings.pauseOnFullscreen && IsFullscreenOrGameActive()) {
            PauseForFullscreen();
        }
    } else if (!g_settings.pauseOnFullscreen) {
        ResumeFromFullscreen();
    }

    // Drawing Device changed, or (on Auto) the Monitor setting moved the
    // widget onto a screen driven by another GPU. The rebuild creates the
    // swap chain and visual resources from the new settings and redraws, so
    // nothing below needs to run after it.
    if (VizRenderDeviceNeedsRebuild()) {
        VizRebuildRenderDevice(L"Drawing Device or monitor changed");
        return;
    }

    if (!g_overlayWnd) return;

    UpdateSwapChainForLayout();

    ReleaseVisualResources();
    RecreateVisualResources();

    if (oldMonitor != g_settings.monitor) HandleDisplayChange();

    if (!g_fullscreenPaused.load()) {
        RenderVisualizer();
    }
}

void WhTool_ModSettingsChanged() {
    Wh_Log(L">");

    // Has to land on the UI thread: it can create or resize windows that thread
    // owns. The overlay is preferred, but it's the one window that legitimately
    // may not exist yet (it waits on WorkerW), so fall back to the message
    // window -- which is created first and always there -- rather than running
    // this inline on Windhawk's own settings thread.
    if (g_overlayWnd) {
        SendMessage(g_overlayWnd, WM_APP_SETTINGS_CHANGED, 0, 0);
    } else if (g_messageWnd) {
        SendMessage(g_messageWnd, WM_APP_SETTINGS_CHANGED, 0, 0);
    } else {
        ApplySettingsChanged();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Windhawk tool mod implementation for mods which don't need to inject to other
// processes or hook other functions. Context:
// https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process
//
// The mod will load and run in a dedicated windhawk.exe process.
//
// Paste the code below as part of the mod code, and use these callbacks:
// * WhTool_ModInit
// * WhTool_ModSettingsChanged
// * WhTool_ModUninit
//
// Currently, other callbacks are not supported.

bool g_isToolModProcessLauncher;
HANDLE g_toolModProcessMutex;

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL Wh_ModInit() {
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) &&
        sessionId == 0) {
        return FALSE;
    }

    bool isExcluded = false;
    bool isToolModProcess = false;
    bool isCurrentToolModProcess = false;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        Wh_Log(L"CommandLineToArgvW failed");
        return FALSE;
    }

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0 ||
            wcscmp(argv[i], L"-service-start") == 0 ||
            wcscmp(argv[i], L"-service-stop") == 0) {
            isExcluded = true;
            break;
        }
    }

    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolModProcess = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isCurrentToolModProcess = true;
            }
            break;
        }
    }

    LocalFree(argv);

    if (isExcluded) {
        return FALSE;
    }

    if (isCurrentToolModProcess) {
        g_toolModProcessMutex =
            CreateMutex(nullptr, TRUE, L"windhawk-tool-mod_" WH_MOD_ID);
        if (!g_toolModProcessMutex) {
            Wh_Log(L"CreateMutex failed");
            ExitProcess(1);
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
            ExitProcess(1);
        }

        if (!WhTool_ModInit()) {
            ExitProcess(1);
        }

        IMAGE_DOS_HEADER* dosHeader =
            (IMAGE_DOS_HEADER*)GetModuleHandle(nullptr);
        IMAGE_NT_HEADERS* ntHeaders =
            (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        void* entryPoint = (BYTE*)dosHeader + entryPointRVA;

        Wh_SetFunctionHook(entryPoint, (void*)EntryPoint_Hook, nullptr);
        return TRUE;
    }

    if (isToolModProcess) {
        return FALSE;
    }

    g_isToolModProcessLauncher = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isToolModProcessLauncher) {
        return;
    }

    WCHAR currentProcessPath[MAX_PATH];
    switch (GetModuleFileName(nullptr, currentProcessPath,
                              ARRAYSIZE(currentProcessPath))) {
        case 0:
        case ARRAYSIZE(currentProcessPath):
            Wh_Log(L"GetModuleFileName failed");
            return;
    }

    WCHAR
    commandLine[MAX_PATH + 2 +
                (sizeof(L" -tool-mod \"" WH_MOD_ID "\"") / sizeof(WCHAR)) - 1];
    swprintf_s(commandLine, L"\"%s\" -tool-mod \"%s\"", currentProcessPath,
               WH_MOD_ID);

    HMODULE kernelModule = GetModuleHandle(L"kernelbase.dll");
    if (!kernelModule) {
        kernelModule = GetModuleHandle(L"kernel32.dll");
        if (!kernelModule) {
            Wh_Log(L"No kernelbase.dll/kernel32.dll");
            return;
        }
    }

    using CreateProcessInternalW_t = BOOL(WINAPI*)(
        HANDLE hUserToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles,
        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hRestrictedUserToken);
    CreateProcessInternalW_t pCreateProcessInternalW =
        (CreateProcessInternalW_t)GetProcAddress(kernelModule,
                                                 "CreateProcessInternalW");
    if (!pCreateProcessInternalW) {
        Wh_Log(L"No CreateProcessInternalW");
        return;
    }

    STARTUPINFO si{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_FORCEOFFFEEDBACK,
    };
    PROCESS_INFORMATION pi;
    if (!pCreateProcessInternalW(nullptr, currentProcessPath, commandLine,
                                 nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                                 nullptr, nullptr, &si, &pi, nullptr)) {
        Wh_Log(L"CreateProcess failed");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void Wh_ModSettingsChanged() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}