# Tourne'Table 2.0.0 beta - design notes (working file)

Base: 1.5.0 (uploaded). @id back to tourne-table-desktop-audio-visualizer-scope. Folder BETA_BUILD_2.0.

## Threads
- UI thread (own msg loop): all D3D11 / D2D / DComp work, RenderVisualizer.
- Engine thread (was RenderThreadProc pacer): now ALSO owns WASAPI loopback (capture thread merged).
  Per frame: wait until due -> drain loopback (poll, no event) -> analysis (precision CPU) or publish raw
  windows (GPU workload) or classic block loop -> post WM_APP_RENDER_TICK.
  Idle: silence > pauseWhenSilent -> wait on WASAPI event (1 s timeout). Deep idle (+5 s): Stop() client,
  poll IAudioMeterInformation at 4 Hz, Start() on peak.
- Start/StopVizCaptureThread keep their names: they now flip g_captureWanted and wait for engine ack.

## Analysis engine (analysis.engine)
- precision (default): ttdsp. Tiers: fs, fs/4, fs/16 (half-band decimator cascade), N = FFT Size per tier.
  Band layouts: scale (log/linear/mel/bark/erb across [fmin,fmax], bars = Bar Count), iec (IEC 61260-1
  base-10 fractional octave 1,3,6,12,24), musical (12-TET 1/12 or 1/24, A4 tuning). Band -> tier = least
  decimated tier whose bin width <= band width and band top inside passband (0.8*nyq).
  Level: band power (rms) or max bin (peak), dBFS sine-referenced, normalized to 1/3-oct bandwidth.
  + weighting (A/C/Z) + tilt dB/oct @1 kHz + EQ preset (zone dB) + sensitivity (0.2 dB/step from 150)
  + auto gain (boost only). Display = (L - floor)/(ceil - floor), sensitivity curve on top end.
  Ballistics in display domain: attack tau, release tau or dB/s. Presets snappy/smooth/analyzer/vu/ppm_ebu/ppm_din/custom.
  Peak hold per bar: hold ms then gravity or linear fall.
- classic: 1.4/1.5 7-band block loop, unchanged numbers.

## Workload (hardware.workload)
- hybrid (default): CPU analysis, GPU draw (Drawing Device picks GPU).
- gpu: CS_FFT/CS_Bands/CS_Reduce/CS_Shape write BarState on GPU; VS reads it. Precision only, FFT <= 4096.
- cpu: CPU analysis, WARP draw.
- npu: precision engine FFTs via ttnpu (experimental), GPU draw.

## Renderer (hardware.renderer)
- d3d11 (default): one VS builds quads from BarState + cbuffer; PS = SDF coverage. Passes: plate, bars, caps,
  dots, radial capsules, scope capsules, goniometer points, correlation bar. Text via D2D on the old
  full-layout swap chain, presented only when text state changes.
  Bars swap chain sized to panel (union bars+margin). Opaque panel (blur>0 or bg alpha 255, no auto-hide,
  bars inside panel): DXGI_ALPHA_MODE_IGNORE + DComp rounded rectangle clip.
  Present-on-change: FNV hash of quantized geometry/color state; skip EndDraw/Present when equal.
- direct2d: 1.5 path untouched (incl. Smooth Mode sprites).
- Composition tree: root (no content) -> [bars visual, text visual(g_compositionVisual)].

## Meters
- Shapes: + goniometer (M/S point cloud with persistence + correlation bar).
- Readout overlay: loudness (M/S/I LUFS, dBTP max, PLR, correlation) - BS.1770 K-weighting, gating, 4x TP.

## Settings keys (new)
analysis.engine, analysis.bandLayout, analysis.octaveFraction, analysis.minFreq, analysis.maxFreq,
analysis.tuningA4, analysis.weighting, analysis.tilt, analysis.detector, analysis.window, analysis.bassDetail,
analysis.channel, analysis.dbFloor, analysis.dbCeiling, analysis.ballistics, analysis.attackMs,
analysis.releaseDbPerSec, analysis.peakHoldMs, analysis.peakFall, analysis.loudnessReadout ...
appearance.freqScale + bark, erb. appearance.shape + goniometer.
hardware.workload, hardware.renderer, hardware.opaquePanel, hardware.framePacing?, performance.deepIdle,
performance.targetFps 0 = display.
