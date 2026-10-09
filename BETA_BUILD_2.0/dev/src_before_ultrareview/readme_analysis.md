## Analysis (Precision Engine)

Everything here applies to **Analysis Engine = Precision**, the default. With **Classic** the bars come from the 1.4 analysis and these settings are ignored.

**Analysis Engine**: **Precision** gives every bar its own frequency band, measured from the spectrum. **Classic** is the 1.4 analysis: 7 fixed bands the bars interpolate between. Kept for comparison, and because some people will prefer its looser look.

**Band Layout**: How the spectrum is cut into bars.
- **Frequency Scale** *(default)*: **Bar Count** bars spread from Min to Max Frequency on the **Frequency Scale** set under Appearance.
- **IEC 61260**: the standard fractional-octave bands (base-10, centred on 1 kHz) that every real-time analyzer uses. **Octave Fraction** picks the width.
- **Musical**: one bar per note of equal temperament (or per quarter tone), tuned to **Tuning (A4)**.

IEC and Musical decide how many bars there are, so **Bar Count** is ignored for them: 1/3 octave from 20 Hz to 20 kHz is 31 bars, 1/6 is 60, 1/12 is 120 and 1/24 is 240.

**Octave Fraction**: 1/1, 1/3, 1/6, 1/12 or 1/24 octave. 1/3 is the classic RTA; 1/12 is a semitone. Musical only uses 1/12 and 1/24.

**Min / Max Frequency**: The analysed range. Max is capped at 21 kHz, below the Nyquist limit of every common output device.

**Tuning (A4)**: Reference pitch for Musical, 400 to 480 Hz.

**Weighting**: **Z** is flat. **A** follows the ear at quiet listening levels and pulls the bass right down; **C** is the gentler loud-level curve. Both from IEC 61672-1, added per band, free.

**Tilt (dB per octave)**: A slope around 1 kHz.

> **What 0 means here.** Bars are measured as band power and, by default, referenced to a 1/3-octave band (see Level Reference), so **pink noise already reads flat at 0**, the way it does on a hardware RTA. Most music falls a little faster than pink. **1.5** *(default)* makes a typical modern mix read level, the same thing SPAN's 4.5 dB/oct default does on a plain FFT display (an FFT display needs 3 dB/oct just to flatten pink noise; this one doesn't).

**Detector**: **RMS** sums the power of every frequency in the band, the right reading for music and noise. **Peak** shows the band's loudest single frequency, closer to what a tone reads on a meter.

**Level Reference**: **Third-octave** *(default)* scales every band to the power a 1/3-octave band at the same centre would hold. Changing Bar Count or Band Layout then changes how fine the bars are, not how tall. **Band** shows each band's true power, so wide bars read higher than narrow ones.

**FFT Window**: **Hann** is the general-purpose choice. **Blackman-Harris** has 92 dB of dynamic range, so quiet detail next to a loud tone stays visible. **Flat-top** reads a tone's exact level wherever it falls between bins, at the cost of resolution. **Hamming** has a narrower peak and more leakage.

**Bass Detail**: The resolution tiers. The audio is analysed as captured, decimated by 4, and decimated by 16, each with the same FFT size, and each bar is measured from the fastest tier that can resolve it. **High** *(default)* uses all three, which is what makes fine bass bars real; **Standard** stops at 4; **Off** is a single FFT.

> **Why tiers instead of a bigger FFT.** Bin spacing is sample rate over FFT size, 23 Hz at 48 kHz and 2048. A 1/24-octave band at 50 Hz is 1.4 Hz wide, so one FFT would need to be enormous to resolve it, and an enormous FFT reacts slowly everywhere, treble included. Decimating first gives the bass a long window and the treble a short one. At FFT Size 2048 the three windows are 43, 171 and 683 ms long, and the two deeper tiers are only re-analysed when enough new audio has arrived to make it worthwhile.

**Channel**: Which signal the spectrum comes from. **Mix** averages all channels, as before. **Mid** is what both speakers share; **Side** is their difference, which shows exactly where a mix's stereo width lives.

**Display Floor / Ceiling (dBFS)**: The levels that draw as an empty and a full bar, before Sensitivity. A full-scale sine is 0 dBFS; loud modern music sits around -20 dBFS per third-octave band. Defaults -72 and -12.

> **Sensitivity** still works with Precision, as a gain: each step from the default 150 is 0.2 dB, so the slider covers -30 to +30 dB. **Sensitivity Curve** still shapes the top of the range, **Auto Gain** lifts quiet sources toward 85% of the range (boost only, held through silence), **EQ Preset** becomes a dB gain on the low, mid and high zones, and **Motion Smoothing** slows whichever Ballistics you pick, up to 6x.

**Ballistics**: How bars rise and fall. All of them run on elapsed time, so the motion is the same at any frame rate.
- **Snappy** *(default)*: closest to the 1.4 feel.
- **Smooth**: slower, easier to follow.
- **Analyzer**: falls a steady 20 dB per second, like a hardware RTA.
- **VU**: 300 ms integration.
- **PPM, EBU (Type II)** and **PPM, DIN (Type I)**: the IEC 60268-10 return rates (24 dB in 2.8 s, 20 dB in 1.5 s).
- **Custom**: **Custom Attack (ms)** and **Custom Release (dB/s)**.

**Peak Hold Time / Peak Fall**: With **Peak Hold Caps** on, how long a cap hangs, then whether it falls with **Gravity** (accelerating, the classic analyzer look) or at a steady **Linear** rate.

**Readout Content**: What the readout set up under **Peak Frequency Readout** shows.
- **Peak frequency**: as before.
- **Loudness**: **M**omentary (400 ms), **S**hort-term (3 s) and gated **I**ntegrated loudness in LUFS, per ITU-R BS.1770-5 and EBU R128.
- **Loudness, true peak, PLR, correlation**: adds 4x-oversampled true peak (dBTP), PLR (true peak minus integrated loudness) and stereo correlation (+1 mono, 0 wide, -1 out of phase).
- **Both**: peak frequency and loudness.

The loudness meters only run while they are on screen.

**Reset Integrated Loudness On Track Change**: Starts a new integrated measurement with each track, so the figure is per track.

> **What loopback can't see.** The mod hears what Windows mixes, after each app's own volume slider and before the master slider. Players in **WASAPI exclusive** or **ASIO** mode bypass that mix entirely, so the visualizer shows nothing while they play, and loudness figures describe the mix, not the file.

