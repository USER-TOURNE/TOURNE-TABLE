## Hardware

There are two jobs in this mod, and they can run on different silicon: **analysing** the audio (the FFTs that turn sound into frequency bands) and **drawing** the result.

**Workload**: Where the work runs.
- **Hybrid** *(default)*: analysis on the CPU, drawing on the GPU. Analysis is a fraction of a percent of one core, and the GPU draws for almost nothing, so this is the cheapest split on nearly every PC.
- **GPU**: the analysis runs on the graphics chip as well, in four small compute passes that feed the drawing directly, so the CPU only copies audio and nothing comes back each frame but 8 bytes of statistics. Needs the Precision engine and the Direct3D 11 renderer, and uses at most FFT Size 4096. Worth trying on a strong iGPU; measure it against Hybrid rather than assume.
- **CPU**: analysis and drawing both on the processor (drawing in software, through WARP). The GPU is left with nothing but Windows' own compositing. Fine for a small widget; a full-width strip at a high frame rate is real CPU work.
- **NPU** *(experimental)*: the FFTs run on an Intel AI Boost NPU, drawing on the GPU. See the NPU section below for why this rarely saves anything.

**Drawing Device**: Which GPU draws.
- **Auto** *(default)*: the GPU your chosen monitor is plugged into. On a laptop with two GPUs, or a desktop with the iGPU left on, drawing on the *other* one means every frame gets copied across to the GPU that scans it out, which is exactly the kind of uneven frame time this mod is trying to avoid. Auto asks Windows which GPU owns that screen and uses it, and follows if the answer changes.
- **Integrated GPU**: the power-saving GPU, the same one Windows picks for "Power saving" in Graphics settings. Falls back to Auto, with a warning, if there isn't one (F and KF processors have no iGPU, and desktops often switch it off in the BIOS).
- **Discrete GPU**: the high-performance GPU. Same fallback.
- **CPU**: software rendering, the same as Workload = CPU.

If the GPU is ever lost mid-session (a driver update, a driver reset, a laptop switching GPUs), the mod rebuilds itself on whatever is available instead of freezing until restart.

**Renderer**:
- **Direct3D 11** *(default)*: every bar, dot, cap, spoke and scope segment is one instanced quad from a tiny vertex shader that reads the bar heights straight from a buffer. The CPU writes two numbers per bar and issues a handful of draw calls per frame, whatever the bar count, and the pixel shader computes exact antialiased edges, so shapes look like Direct2D's. On top of that:
  - **Frames where nothing moved are skipped.** The picture is summarised in a hash (bar heights to a quarter pixel, colours, the trace) and compared with the last frame shown. If it matches, nothing is drawn and nothing is presented, so Windows has nothing to recompose.
  - **The text lives on its own surface**, redrawn only when the text or its fade changes. The animated surface covers just the panel, not the room reserved around it for text.
  - **The panel can be opaque** (see Opaque Panel).
- **Direct2D**: the 1.5 path, unchanged, including Smooth Mode's batching. Kept for comparison, and used automatically, with a warning, if Direct3D 11 can't start (a GPU below feature level 11.0, or `d3dcompiler_47.dll` missing).

**Opaque Panel**: When the background panel can't show anything behind it (**Blur** on, or a panel colour with full alpha) and **Auto-Hide** is off, the Direct3D 11 renderer creates the panel's surface with alpha ignored and lets the compositor cut the rounded corners. Windows then copies those pixels instead of blending them every frame; Chromium measured the same switch at 3.12 W against 2.28 W. **Off** always blends, for comparison.

**NPU Runtime Folder**: Where Intel's OpenVINO runtime lives. Blank searches the usual places. Only read when Workload is NPU.

**Smooth Mode**: Frame timing locked to the display, and motion on elapsed time. **Auto** *(default)* turns it on when drawing runs on an integrated GPU or the CPU; **On** works on any GPU and is worth trying on a discrete one too; **Off** is the 1.4 timing. What it changes:

- **Frame timing locked to the display.** The 1.4 loop asks for a frame every `1000 / Target FPS` milliseconds and lets the timer's wake-up latency pile up, which is why 144 requested measured 141.7 delivered. On a 144 Hz panel that is a couple of refreshes every second with no new frame: a repeat, which you see as a hitch. Smooth Mode schedules from Windows' own composition clock instead, places every frame half a refresh away from the deadline on either side, and never accumulates drift.
- **Target FPS rounded to an even fraction of the refresh rate.** 60 FPS on a 144 Hz screen means frames held for 2, 3, 2, 3 refreshes, which reads as judder however precise the timer is. Smooth Mode runs the nearest whole fraction instead: 60 becomes 72 on 144 Hz, 48 if you set it to 50, 60 on a 60 Hz screen. Asking for more than the refresh rate gets the refresh rate rather than frames nobody can see.
- **Motion that doesn't depend on frame timing.** With the Classic engine, bar rise and fall, peak caps, beat flash and scope damping were all "move this fraction per frame", so a late frame visibly slowed everything down for a moment. They now scale by how much time actually passed. (The Precision engine runs on elapsed time whatever this is set to.)
- **A proper animation clock.** Wave, Breathe and Rainbow ran off a millisecond counter that only ticks every 15.6 ms, stored in a float that loses precision the longer the PC has been on. They now run off the high-resolution clock. (Always, with the Direct3D 11 renderer.)
- **With the Direct2D renderer:** the background baked once into one image, all bars in one sprite batch, and cached text layouts.

