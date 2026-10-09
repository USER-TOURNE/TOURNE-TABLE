# ◇ RUNNING THE ANALYSIS ON AN NPU (EXPERIMENTAL)

Written against an **Alienware Aurora ACT1250** (the 2025 Aurora: Core Ultra 7 265K / 265KF or Ultra 9 285K, RTX 50-series), but it applies to any Intel Core Ultra machine.

**Read this first: it almost certainly won't save power.** An NPU is a matrix-multiply engine. It wins when the work is a large, dense block of multiply-accumulates that runs continuously, like a neural network. An FFT is neither: to run on the NPU it has to be rewritten as matrix multiplies, which is roughly 25 times more arithmetic than an FFT, and every call has a fixed start-up cost the FFT is far too small to pay back. Calling it once per frame also keeps the NPU from power-gating between calls. Meanwhile the CPU does the whole analysis in well under 1% of one core. More bars don't change that: with the Precision engine every bar adds up its own bins once per frame, so 240 bars cost the same as 7.

It stays as an option because the plumbing is the hard, reusable part (finding the runtime, compiling off the audio path, proving the result against a known answer, falling back to the CPU with a reason), and because the NPU's real use in a visualizer is still to come: neural models such as note transcription (a piano-roll mode, note names on musical bars) or stem separation (colour each bar by whether drums, bass or vocals drive it). Those are dense, continuous matrix work, which is what an NPU is for.

**What's in there.** Arrow Lake desktop chips carry Intel's NPU 3, sold as *Intel AI Boost*: about 13 TOPS, the same generation as Meteor Lake, and present on K, KF and F models alike (the F only drops the iGPU). It is a separate block from both the CPU cores and the GPU, built for large batches of multiply-accumulates in low precision (FP16 / INT8).

**Making sure it's on.**
1. Device Manager should have a **Neural processors** category with **Intel(R) AI Boost** under it. Task Manager on Windows 11 24H2 and later also shows an **NPU** graph on the Performance tab.
2. If it's missing, install the **Intel NPU driver** (from Dell's support page for the Aurora, or Intel's own "Intel NPU Driver - Windows" download). It needs Windows 11.
3. Still missing after that: check the BIOS for an NPU / AI Boost switch. Some Arrow Lake firmware ships one, and Intel's own guidance for Windows 10 machines is to turn the NPU off there.

**Giving this mod a way to talk to it.** Intel exposes the NPU through its **OpenVINO** runtime, which this mod loads at runtime if it's present. Nothing is bundled and nothing is installed for you. Any of these works:
- Download the OpenVINO archive for Windows from Intel and extract it (to `C:\Program Files (x86)\Intel\openvino_<version>` is the conventional spot, which the mod searches on its own), or
- `pip install openvino` into a Python install (the mod searches the usual Python locations too), or
- put the folder holding `openvino_c.dll` into **NPU Runtime Folder**.

Use an OpenVINO version at least as new as the one your NPU driver's release notes name. Then set **Workload** to **NPU**. The Windhawk log says what happened: which runtime and device it found, how long the model took to compile, and every 30 seconds the mean and worst NPU time per FFT. If anything is missing, the settings-problems window says which piece and what to do, and the analysis stays on the CPU in the meantime.

> **64-bit only.** OpenVINO ships 64-bit builds only. Windhawk 1.x runs tool mods in a 32-bit process, so there the NPU option reports that and stays on the CPU.

**How it works.** The FFT is rewritten as matrix multiplies (a "four-step" DFT: a DFT down the columns, a twiddle multiply, a DFT along the rows). The model is generated in memory for your FFT Size, compiled for the NPU once, and checked against a known answer before it's trusted. Its output feeds exactly the same band maths as the CPU path, for either engine, so the bars look identical. Measure it with HWiNFO against Hybrid, the same way the CPU figures above were measured, and keep whichever costs less.

