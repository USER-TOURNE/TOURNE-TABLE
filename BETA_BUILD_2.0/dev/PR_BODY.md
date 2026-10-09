A major update. Every setting from 1.3.0 keeps its key, type and meaning, so existing installs upgrade in place. The one changed default is FFT Size (1024 to 2048), which only applies to fresh installs.

From the 1.3.0 review's optional items: `EndDrag()` now defers its storage write like the other low-level hook paths, and the album-art thread pointer is guarded by a mutex, including at uninit. The Power-curve idle-threshold note is left as is: it only applies to the Classic engine, and the new default Precision engine doesn't use that test.

<!-- ⚠️ Please keep the template below intact and fill in the relevant sections. Any additional content can be placed above the template. -->

## Changelog

If this pull request updates an existing mod, describe the changes inside the changelog markers:

<!-- changelog:start -->

* **Precision analysis engine**: every bar is its own frequency band measured from a real-input FFT, with three resolution tiers so bass bars are genuinely resolved. Band layouts on Log / Linear / Mel / Bark / ERB scales, IEC 61260 fractional-octave bands, or one bar per musical note. A / C / Z weighting, tilt, RMS or peak detection, a choice of FFT window, and ballistics that run on real time (same speed at any frame rate) with Analyzer, VU and PPM presets. The 1.x analysis remains as Analysis Engine = Classic.
* **Loudness readout**: momentary, short-term and integrated LUFS (ITU-R BS.1770-5 / EBU R128), 4x-oversampled true peak, PLR and stereo correlation, plus a new Goniometer shape.
* **Workload choice**: Hybrid (CPU analysis, GPU drawing, the default), GPU, or CPU, plus an experimental NPU option through Intel OpenVINO when it is installed.
* **Direct3D 11 renderer**: every shape drawn by one small shader from a buffer of bar heights; frames where nothing moved are skipped entirely, text lives on its own surface, and after a few seconds of silence the audio stream itself stops (Deep Idle). The Direct2D path remains as an option and as an automatic fallback.
* **Right-click menu** for live quick settings, applied instantly, including the **audio source**: the default output, the default input, or any active output or input by name, virtual devices included (VB-Audio, Voicemeeter, an Ableton return).
* **Terminal shape**: the visualizer drawn as text characters, in Columns, Waterfall or Meters styles.
* **Media widget options**: Now Playing on two lines with a separate artist colour, placed inside the panel with alignment, always-on mode, pixel-sharp text rendering, a track progress bar, and media controls anchored to a corner of the panel.
* **Pixel Snap** (on by default) keeps every edge on whole pixels at any size and display scaling; turn it off for subpixel placement, with fractional-pixel keyboard nudges.
* Target FPS 0 matches the display's refresh rate.

<!-- changelog:end -->

## Mod authorship

If this pull request introduces a new mod, please complete the section below.

This mod was created by:

- - [ ] The submitter, without AI assistance
- - [x] The submitter, with AI assistance
- - [x] Claude
- - [ ] ChatGPT
- - [ ] Gemini
- - [ ] Another AI (please specify): 
- - [ ] Other (please specify): 

Please select the options that best apply. Your selection does not affect the acceptance criteria, but it helps reviewers understand the context of the code and provide relevant feedback.
