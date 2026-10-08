# 2.0 beta - verification sources

Not part of the mod. These are the standalone sources the mod's `ttdsp` namespace and its HLSL string were written from, plus the tests that checked them on Linux before the build ever touched Windows. If you change the DSP or the shaders, change them here too and re-run.

| File | What it is |
|:--|:--|
| `ttdsp.h` | The precision analysis core, exactly as pasted into the mod. |
| `tt_prelude.hlsl`, `tt_cb.hlsl`, `tt_body.hlsl` | The shader source; the mod embeds the three concatenated. |
| `hlsl_shim.h` | Enough HLSL-in-C++ to compile `tt_cb.hlsl` / `tt_body.hlsl` as C++ and run them, with real threads and a real barrier for compute groups. |
| `test_dsp.cpp` / `.py` | FFT vs numpy, windows vs scipy, the half-band decimator's response, band layouts (IEC 61260 midbands, musical, scales), sine and pink-noise calibration, EBU Tech 3341 loudness cases, true peak, frame-rate independence of the ballistics. |
| `fxc_lint.py` | Flags what Windows' shader compiler (`fxc`) rejects but DXC accepts: reserved words such as `pass` used as names, `#define F()` macros with no parameters, non-ASCII text. Run it on the mod: `python3 fxc_lint.py ../tourne-table-scope.wh.cpp`. |
| `test_gfx.cpp` / `.py` | Runs the vertex / pixel shader logic and rasterises bars, caps, dots and radial spokes, compared with an independent supersampled re-implementation of the 1.5 Direct2D geometry; checks every Colour Mode; runs the four GPU analysis passes and compares them with `ttdsp`. |

## Running

Needs clang (C++20), Python 3 with numpy and scipy.

```
clang++ -std=c++20 -O2 -o test_dsp test_dsp.cpp && python3 test_dsp.py
clang++ -std=c++20 -O2 -pthread -o test_gfx test_gfx.cpp && python3 test_gfx.py
```

To compile the shaders the way the GPU driver will see them, concatenate the three `.hlsl` files and build each entry point (`VSMain`, `PSMain`, `CsFft`, `CsBands`, `CsReduce`, `CsShape`) with DXC (`-T vs_6_0` / `ps_6_0` / `cs_6_0`) or, on Windows, `fxc` with the `_5_0` profiles the mod uses at runtime. DXC is more lenient than `fxc`, so a DXC pass alone isn't enough: run `fxc_lint.py` too.
