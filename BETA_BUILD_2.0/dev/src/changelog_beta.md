## ❀ v2.0.0 (beta): Every Bar Its Own Band, Your Choice of Silicon, and a Renderer That Idles to Nothing

Beta build, in `BETA_BUILD_2.0/`. Same `@id` as the 1.4 beta, so it upgrades that install in place and keeps its settings. The release build is untouched at 1.1.0.

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

