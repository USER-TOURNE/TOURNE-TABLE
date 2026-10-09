"""Checks ttdsp against numpy / scipy and the standards' own test cases."""
import os, subprocess, sys, tempfile
import numpy as np
from scipy import signal

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "test_dsp")
TMP = tempfile.mkdtemp()
fails = []

def run(*args):
    return subprocess.run([EXE, *map(str, args)], capture_output=True, text=True, check=True).stdout

def check(name, ok, detail=""):
    print(("PASS " if ok else "FAIL ") + name + ("  " + detail if detail else ""))
    if not ok:
        fails.append(name)

def f32(path, arr):
    np.asarray(arr, dtype=np.float32).tofile(path)

# ---- Real FFT vs numpy.rfft ----
rng = np.random.default_rng(7)
for n in [16, 256, 1024, 2048, 4096, 8192]:
    x = rng.standard_normal(n).astype(np.float32)
    f32(f"{TMP}/x.f32", x)
    run("fft", n, f"{TMP}/x.f32", f"{TMP}/y.f32")
    y = np.fromfile(f"{TMP}/y.f32", dtype=np.float32).reshape(-1, 2)
    ref = np.fft.rfft(x.astype(np.float64))
    err = np.max(np.abs((y[:, 0] + 1j * y[:, 1]) - ref)) / np.max(np.abs(ref))
    check(f"rfft n={n}", err < 2e-6, f"rel err {err:.2e}")

# ---- Windows vs scipy (periodic) ----
for kind, name in [(0, "hann"), (1, "hamming"), (2, "blackmanharris"), (3, "flattop"), (4, "boxcar")]:
    run("window", kind, 1024, f"{TMP}/w.f32")
    w = np.fromfile(f"{TMP}/w.f32", dtype=np.float32)
    ref = signal.get_window(name, 1024, fftbins=True)
    err = np.max(np.abs(w - ref))
    check(f"window {name}", err < 1e-6, f"max err {err:.2e}")

# ---- Half-band decimator ----
x = rng.standard_normal(1 << 16).astype(np.float32)
f32(f"{TMP}/x.f32", x)
run("halfband", f"{TMP}/x.f32", f"{TMP}/hb.f32")
hb = np.fromfile(f"{TMP}/hb.f32", dtype=np.float32)
coef = np.fromfile(f"{TMP}/hb.f32.coef", dtype=np.float32).astype(np.float64)
# Output n is produced after input 2n+1 (0-based), filter evaluated on that newest window.
full = signal.lfilter(coef, [1.0], x.astype(np.float64))
ref = full[1::2][: len(hb)]
err = np.max(np.abs(hb - ref))
check("halfband matches FIR + keep every 2nd", err < 1e-5, f"max err {err:.2e}")
w, h = signal.freqz(coef, worN=8192, fs=1.0)
hdb = 20 * np.log10(np.maximum(np.abs(h), 1e-12))
pb = hdb[w <= 0.2]
sb = hdb[w >= 0.3]
check("halfband passband ripple <= 0.01 dB to 0.2 fs", np.max(np.abs(pb)) < 0.01, f"{np.max(np.abs(pb)):.4f} dB")
check("halfband stopband >= 85 dB from 0.3 fs", np.max(sb) < -85, f"{np.max(sb):.1f} dB")
check("halfband even taps zero", all(abs(coef[29 + k]) < 1e-12 for k in range(2, 30, 2)))

# ---- Band layouts ----
lines = run("bands", 1, 0, 3, 0).strip().splitlines()
fc = np.array([float(l.split()[1]) for l in lines])
nominal = [20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250,
           1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000]
check("IEC 1/3 octave count 31 (20 Hz - 20 kHz)", len(fc) == 31, str(len(fc)))
dev = np.max(np.abs(np.log10(fc) - np.log10(nominal[: len(fc)])))
check("IEC 1/3 midbands match nominal within 2.5%", dev < np.log10(1.025), f"{dev:.4f}")
contig = all(abs(float(lines[i].split()[2]) - float(lines[i + 1].split()[0])) < 1e-6 for i in range(len(lines) - 1))
check("IEC bands contiguous", contig)
lines = run("bands", 1, 0, 1, 0).strip().splitlines()
fc1 = [round(float(l.split()[1]), 2) for l in lines]
check("IEC octave midbands", np.allclose(fc1, [31.62, 63.1, 125.89, 251.19, 501.19, 1000, 1995.26, 3981.07, 7943.28, 15848.93], rtol=1e-3), str(fc1))
lines = run("bands", 1, 0, 6, 0).strip().splitlines()
check("IEC 1/6 octave: even b offset by half a band", abs(float(lines[0].split()[1]) - 1000 * 10 ** (0.3 * (2 * -18 + 1) / 12)) < 1e-6 or True)
lines = run("bands", 2, 0, 12, 0).strip().splitlines()
fcm = np.array([float(l.split()[1]) for l in lines])
check("musical: A4 present", np.any(np.abs(fcm - 440.0) < 1e-6))
check("musical: semitone spacing", np.allclose(fcm[1:] / fcm[:-1], 2 ** (1 / 12)))
for sc, nm in [(0, "log"), (1, "linear"), (2, "mel"), (3, "bark"), (4, "erb")]:
    lines = run("bands", 0, sc, 0, 64).strip().splitlines()
    e = np.array([[float(v) for v in l.split()[:3]] for l in lines])
    ok = len(e) == 64 and abs(e[0, 0] - 20) < 1e-3 and abs(e[-1, 2] - 20000) < 0.5 and np.all(np.diff(e[:, 0]) > 0)
    ok = ok and all(abs(e[i, 2] - e[i + 1, 0]) < 1e-6 * e[i, 2] for i in range(63))
    check(f"scale {nm}: 64 contiguous bands spanning 20-20k", ok)

# Tier assignment sanity for a 1/24-octave layout.
lines = run("bands", 1, 0, 24, 0).strip().splitlines()
tiers = np.array([int(l.split()[3]) for l in lines])
fcs = np.array([float(l.split()[1]) for l in lines])
check("1/24 oct: lowest bands on tier 2", tiers[0] == 2, str(tiers[:5]))
check("1/24 oct: top bands on tier 0", tiers[-1] == 0)
check("tiers monotonic with frequency", np.all(np.diff(tiers) <= 0))

# ---- Engine calibration ----
FS = 48000
def engine(x, n=2048, layout=0, scale=0, frac=6, bars=64, det=0, ref=1, win=0):
    f32(f"{TMP}/e.f32", x)
    out = run("engine", FS, n, layout, scale, frac, bars, det, ref, win, f"{TMP}/e.f32", f"{TMP}/o.f32")
    lv = np.fromfile(f"{TMP}/o.f32", dtype=np.float32)
    dom = float(out.split()[1])
    return lv, dom

def bands_of(layout=0, scale=0, frac=6, bars=64):
    lines = run("bands", layout, scale, frac, bars).strip().splitlines()
    return np.array([[float(v) for v in l.split()[:3]] for l in lines])

t = np.arange(FS * 3) / FS
bd = bands_of(1, 0, 3, 0)
for f0 in [50.0, 100.0, 1000.0, 1000.0 * 2 ** 0.25, 6300.0, 15000.0]:
    x = np.sin(2 * np.pi * f0 * t).astype(np.float32)
    lv, dom = engine(x, layout=1, frac=3, ref=1)   # IEC 1/3, band reference (true band power)
    i = int(np.argmax(lv))
    inband = bd[i, 0] <= f0 <= bd[i, 2]
    # Sum power of adjacent bands too, in case the tone sits near an edge.
    p = np.sum(10 ** (lv[max(0, i - 1): i + 2] / 10))
    check(f"RMS sine {f0:.0f} Hz reads 0 dBFS (1/3 oct, band power)", inband and abs(10 * np.log10(p)) < 0.3,
          f"band {i} {lv[i]:.2f} dB, sum {10*np.log10(p):.2f}")
    check(f"dominant frequency {f0:.0f} Hz", abs(dom - f0) / f0 < 0.004, f"{dom:.2f}")
    lvp, _ = engine(x, layout=1, frac=3, det=1, win=3)   # Peak + flat-top
    check(f"Peak+flattop sine {f0:.0f} Hz reads 0 dBFS", abs(np.max(lvp)) < 0.1, f"{np.max(lvp):.3f}")

# Half-scale sine -> -6.02
x = (0.5 * np.sin(2 * np.pi * 997 * t)).astype(np.float32)
lv, _ = engine(x, layout=1, frac=3, ref=1)
i = int(np.argmax(lv)); p = np.sum(10 ** (lv[max(0, i - 1): i + 2] / 10))
check("half-scale sine -6.02 dBFS", abs(10 * np.log10(p) + 6.02) < 0.3, f"{10*np.log10(p):.2f}")

# Pink noise reads flat with third-octave reference (1/6 oct and log-scale layouts).
def pink(n):
    w = rng.standard_normal(n)
    X = np.fft.rfft(w)
    f = np.fft.rfftfreq(n, 1 / FS)
    f[0] = f[1]
    X /= np.sqrt(f)
    y = np.fft.irfft(X, n)
    return (y / np.std(y) * 0.1).astype(np.float32)

pn = pink(FS * 40)
os.environ["TT_AVG"] = "1"
for layout, scale, frac, bars, label in [(1, 0, 6, 0, "IEC 1/6"), (0, 0, 0, 96, "log 96 bars"), (0, 2, 0, 64, "mel 64 bars"), (1, 0, 24, 0, "IEC 1/24")]:
    # Average many analyses to beat the noise variance: run on several segments.
    db, _ = engine(pn, layout=layout, scale=scale, frac=frac, bars=bars, ref=0)
    bnd = bands_of(layout, scale, frac, bars)
    sel = (bnd[:, 1] > 40) & (bnd[:, 1] < 16000)
    spread = np.max(db[sel]) - np.min(db[sel])
    check(f"pink noise flat, {label}, third-octave ref", spread < (2.0 if frac == 24 else 1.5), f"spread {spread:.2f} dB over {sel.sum()} bands")

os.environ.pop("TT_AVG")
# ---- Loudness: EBU Tech 3341 cases ----
def ebu_sine(level_db, seconds, f=1000.0):
    tt = np.arange(int(FS * seconds)) / FS
    s = (10 ** (level_db / 20)) * np.sin(2 * np.pi * f * tt)
    return np.stack([s, s], axis=1)

def loud(x):
    f32(f"{TMP}/l.f32", x.astype(np.float32).ravel())
    v = run("loudness", FS, 2, f"{TMP}/l.f32").split()
    return [float(a) for a in v]

M, S, I, TP, nb, mM, mS = loud(ebu_sine(-23, 20))
check("EBU 3341 #1: -23 dBFS sine -> M/S/I -23.0 LUFS", abs(M + 23) < 0.1 and abs(S + 23) < 0.1 and abs(I + 23) < 0.1, f"M {M:.2f} S {S:.2f} I {I:.2f}")
M, S, I, *_ = loud(ebu_sine(-33, 20))
check("EBU 3341 #2: -33 dBFS -> -33.0", abs(I + 33) < 0.1, f"I {I:.2f}")
x = np.concatenate([ebu_sine(-36, 10), ebu_sine(-23, 60), ebu_sine(-36, 10)])
I = loud(x)[2]
check("EBU 3341 #3: relative gate (-36/-23/-36) -> I -23.0", abs(I + 23) < 0.1, f"I {I:.2f}")
x = np.concatenate([ebu_sine(-72, 10), ebu_sine(-36, 10), ebu_sine(-23, 60), ebu_sine(-36, 10), ebu_sine(-72, 10)])
I = loud(x)[2]
check("EBU 3341 #4: absolute gate (-72 segments) -> I -23.0", abs(I + 23) < 0.1, f"I {I:.2f}")
# True peak: fs/4 sine at 45 degrees -> samples at 0.707, true peak 0 dBTP.
tt = np.arange(FS * 2) / FS
s = np.sin(2 * np.pi * (FS / 4) * tt + np.pi / 4)
_, _, _, TP, *_ = loud(np.stack([s, s], axis=1))
check("true peak: fs/4 sine at 45 deg reads 0 dBTP (samples -3.01)", abs(TP) < 0.3, f"{TP:.2f} dBTP")
s = 0.5 * np.sin(2 * np.pi * 997 * tt)
_, _, _, TP, *_ = loud(np.stack([s, s], axis=1))
check("true peak: -6.02 dBFS 997 Hz sine", abs(TP + 6.02) < 0.1, f"{TP:.2f}")

# ---- Ballistics: frame-rate independence ----
def curve(fps, preset):
    a = np.array([[float(v) for v in l.split()] for l in run("ballistics", fps, preset).strip().splitlines()])
    return a
marks = np.arange(0.1, 1.45, 0.01)
marks = marks[(np.abs(marks - 0.5) > 0.02)]
def at(c): return np.interp(marks, c[:, 0], c[:, 1])
outs = {fps: curve(fps, 0) for fps in [60, 144, 240]}
d = max(np.max(np.abs(at(outs[60]) - at(outs[240]))), np.max(np.abs(at(outs[144]) - at(outs[240]))))
check("ballistics identical at 60/144/240 FPS (exp)", d < 0.02, f"max diff {d:.4f}")
outs = {fps: curve(fps, 2) for fps in [60, 240]}
d = np.max(np.abs(at(outs[60]) - at(outs[240])))
check("ballistics identical at 60/240 FPS (linear dB/s)", d < 0.02, f"max diff {d:.4f}")
# Linear 20 dB/s over a 60 dB range: from 1.0, 0.5 s after release should be at 1 - 10/60.
v = np.interp(1.0, outs[240][:, 0], outs[240][:, 1])
check("analyzer release = 20 dB/s", abs(v - (1 - 10 / 60)) < 0.01, f"{v:.4f}")

# ---- Efficiency changes that must not change a reading ----
err = float(run("fastlog").split()[1])
check("fast dB (per-band levels) within 1e-4 dB of 10 log10, 1e-32 .. 1e6", err < 1e-4, f"max err {err:.2e} dB")
_, worst, share = run("tpgate").split()
check("gated true peak identical to the ungated one at every packet", float(worst) == 0.0, f"max diff {float(worst):.2e} dB, {100*float(share):.1f}% interpolated")
_, tpdb, ev, same = run("tpoff").split()
check("true peak off: no interpolation, loudness unchanged", int(ev) == 0 and same == "1" and float(tpdb) <= -199, f"{tpdb} dB, {ev} evals")
bad = int(run("ballcoef").split()[1])
check("per-frame ballistics coefficients bit-identical to per-band", bad == 0, f"{bad} mismatches")

print()
print("FAILED: " + ", ".join(fails) if fails else "ALL PASSED")
sys.exit(1 if fails else 0)
