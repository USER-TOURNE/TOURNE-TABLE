## ✧ NEW IN 2.0 (BETA)

This is a test build. It installs alongside the release build and is not for publishing. The performance figures further down were measured on 1.x and haven't been re-measured on 2.0 yet.

**Every bar is its own measurement now.** Up to 1.5 the analysis split the sound into 7 fixed bands and every bar interpolated between those 7 numbers, so Bar Count and FFT Size changed how many bars you saw, not how much they knew. The new **Precision** engine gives each bar its own frequency band with its own edges, measured straight from the spectrum:

- bass comes from long, finely resolved windows and treble from short, fast ones (three resolution tiers), so a 1/24-octave bar at 50 Hz is a real reading rather than a smear;
- band layouts on Log, Linear, Mel, Bark or ERB scales, the standard **IEC 61260** fractional-octave bands every RTA uses, or one bar per **musical note**;
- **A / C / Z weighting** (IEC 61672), a dB-per-octave **tilt**, RMS or peak detection, and a choice of FFT window;
- **ballistics on real time**, so bars move at the same speed at 60, 144 or 240 FPS, with presets for an analyzer, a VU meter and both broadcast PPMs, and gravity peak caps;
- **loudness** (ITU-R BS.1770-5 / EBU R128: momentary, short-term, integrated), 4x-oversampled **true peak**, PLR and stereo **correlation** in the readout, and a new **Goniometer** shape.

The 1.4 analysis is still there as **Analysis Engine = Classic**.

**You choose where the work runs.** **Workload** is GPU, CPU or Hybrid: Hybrid analyses on the CPU and draws on the GPU (the cheapest split almost everywhere), GPU does both on the graphics chip, CPU does both on the processor. NPU stays as an experimental option.

**A new renderer built to cost next to nothing.** **Renderer = Direct3D 11** draws everything from one tiny shader and a buffer of bar heights, **skips any frame where nothing moved**, keeps the text on its own surface that only redraws when the text changes, and tells Windows when the panel is opaque so it copies instead of blending. When audio stops, drawing stops, and after a few more seconds of silence the audio stream itself stops (**Deep Idle**). The 1.5 Direct2D path is still there for comparison.

**Target FPS 0** matches your display's refresh rate, whatever it is.

---

