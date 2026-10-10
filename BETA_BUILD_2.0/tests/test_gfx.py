"""Runs the mod's HLSL on the CPU (test_gfx) and checks it against an independent
re-implementation of the 1.5 Direct2D geometry, colours and the ttdsp analysis."""
import os, subprocess, sys, tempfile, math
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "test_gfx")
TMP = tempfile.mkdtemp()
fails = []
SS = 16  # supersampling per axis for the reference


def check(name, ok, detail=""):
    print(("PASS " if ok else "FAIL ") + name + ("  " + detail if detail else ""))
    if not ok:
        fails.append(name)


def write_cfg(path, d):
    with open(path, "w") as f:
        for k, v in d.items():
            vals = v if isinstance(v, (list, tuple, np.ndarray)) else [v]
            f.write(k + " " + " ".join(repr(float(x)) for x in vals) + " ;\n")


def raster(cfg):
    write_cfg(f"{TMP}/c.txt", cfg)
    subprocess.run([EXE, "raster", f"{TMP}/c.txt", f"{TMP}/o.f32"], check=True)
    W, H = int(cfg["vw"]), int(cfg["vh"])
    return np.fromfile(f"{TMP}/o.f32", dtype=np.float32).reshape(H, W, 4)


# ---- Reference: exact shapes, supersampled -------------------------------------
def rr_inside(px, py, l, t, r, b, radii):
    """Rounded rect with per-corner radii (TL TR BR BL), D2D clamping."""
    w, h = r - l, b - t
    if w <= 0 or h <= 0:
        return np.zeros_like(px, dtype=bool)
    m = min(w, h) / 2
    rtl, rtr, rbr, rbl = [min(max(x, 0), m) for x in radii]
    ins = (px >= l) & (px <= r) & (py >= t) & (py <= b)
    for (cx, cy, rad, sx, sy) in [(l + rtl, t + rtl, rtl, -1, -1), (r - rtr, t + rtr, rtr, 1, -1),
                                  (r - rbr, b - rbr, rbr, 1, 1), (l + rbl, b - rbl, rbl, -1, 1)]:
        if rad <= 0:
            continue
        corner = ((px - cx) * sx > 0) & ((py - cy) * sy > 0)
        ins &= ~corner | ((px - cx) ** 2 + (py - cy) ** 2 <= rad * rad)
    return ins


def capsule_inside(px, py, ax, ay, bx, by, rad):
    bax, bay = bx - ax, by - ay
    bb = bax * bax + bay * bay
    h = np.clip(((px - ax) * bax + (py - ay) * bay) / bb, 0, 1) if bb > 1e-12 else 0
    dx, dy = px - ax - bax * h, py - ay - bay * h
    return dx * dx + dy * dy <= rad * rad


def ref_render(W, H, prims):
    """prims: list of (kind, params, rgba straight). Returns premultiplied RGBA."""
    img = np.zeros((H, W, 4))
    o = (np.arange(SS) + 0.5) / SS
    for kind, p, col in prims:
        if kind == "rr":
            l, t, r, b, radii = p
            x0, x1, y0, y1 = l - 1, r + 1, t - 1, b + 1
        else:
            ax, ay, bx, by, rad = p
            x0, x1 = min(ax, bx) - rad - 1, max(ax, bx) + rad + 1
            y0, y1 = min(ay, by) - rad - 1, max(ay, by) + rad + 1
        X0, X1 = max(0, int(math.floor(x0))), min(W, int(math.ceil(x1)) + 1)
        Y0, Y1 = max(0, int(math.floor(y0))), min(H, int(math.ceil(y1)) + 1)
        if X1 <= X0 or Y1 <= Y0:
            continue
        xs = (np.arange(X0, X1)[:, None] + o[None, :]).ravel()
        ys = (np.arange(Y0, Y1)[:, None] + o[None, :]).ravel()
        PX, PY = np.meshgrid(xs, ys)
        ins = rr_inside(PX, PY, l, t, r, b, radii) if kind == "rr" else capsule_inside(PX, PY, *p)
        cov = ins.reshape(Y1 - Y0, SS, X1 - X0, SS).mean(axis=(1, 3))
        a = cov * col[3]
        src = np.stack([col[0] * a, col[1] * a, col[2] * a, a], axis=-1)
        dst = img[Y0:Y1, X0:X1]
        img[Y0:Y1, X0:X1] = src + dst * (1 - a[..., None])
    return img


# ---- Reference geometry: ported from the 1.5 RenderVisualizer -------------------
def ref_bars(cfg, caps=False):
    prims = []
    n = int(cfg["bars"]); w = cfg["barW"]; gap = cfg["gap"]; mx = cfg["maxSize"]; idle = cfg["idle"]
    bx, by = cfg["bx"], cfg["by"]; anchor = int(cfg["anchor"]); vert = int(cfg["vertical"])
    radii = cfg.get("radii", [0, 0, 0, 0])
    for i in range(n):
        fac = max(0.0, cfg["levels"][i])
        size = idle + fac * max(0.0, mx - idle)
        if not vert:
            x = bx + i * (w + gap)
            if anchor == 0: y, yb = by, by + size
            elif anchor == 1: y = by + (mx - size) / 2; yb = y + size
            else: y, yb = by + (mx - size), by + mx
            prims.append(("rr", (x, y, x + w, yb, radii), cfg["col"](i, fac)))
        else:
            y = by + i * (w + gap)
            if anchor == 0: xr = bx + mx; x = xr - size
            elif anchor == 1: cx = bx + mx / 2; x = cx - size / 2; xr = cx + size / 2
            else: x = bx; xr = bx + size
            prims.append(("rr", (x, y, xr, y + w, radii), cfg["col"](i, fac)))
    if caps:
        ct = cfg["capT"]
        for i in range(n):
            hold = idle + cfg["peaks"][i] * max(0.0, mx - idle)
            if hold <= 0.5:
                continue
            if not vert:
                x = bx + i * (w + gap)
                capY = by + hold if anchor == 0 else (by + (mx - hold) / 2 if anchor == 1 else by + (mx - hold))
                prims.append(("rr", (x, capY - ct / 2, x + w, capY + ct / 2, [0] * 4), cfg["peakColor"]))
            else:
                y = by + i * (w + gap)
                capX = bx + mx - hold if anchor == 0 else (bx + mx / 2 - hold / 2 if anchor == 1 else bx + hold)
                prims.append(("rr", (capX - ct / 2, y, capX + ct / 2, y + w, [0] * 4), cfg["peakColor"]))
    return prims


def ref_dots(cfg):
    prims = []
    n = int(cfg["bars"]); w = cfg["barW"]; gap = cfg["gap"]; mx = cfg["maxSize"]; idle = cfg["idle"]
    bx, by = cfg["bx"], cfg["by"]; anchor = int(cfg["anchor"]); vert = int(cfg["vertical"])
    dr = cfg.get("dotRadii", [0, 0, 0, 0])
    r = w / 2; step = w + gap
    for i in range(n):
        fac = max(0.0, cfg["levels"][i])
        cs = idle + fac * max(0.0, mx - idle)
        if cs < 0.5:
            continue
        nd = max(1, int(cs / step) if step > 0.5 else 1)
        col = cfg["col"](i, fac)
        def dot(cx, cy):
            prims.append(("rr", (cx - r, cy - r, cx + r, cy + r, dr), col))
        for d in range(nd):
            if not vert:
                cx = bx + i * step + r
                if anchor == 0:
                    cy = by + d * step + r
                    if cy - r > by + mx: break
                    dot(cx, cy)
                elif anchor == 1:
                    c = by + mx / 2
                    if d == 0: dot(cx, c)
                    else:
                        up, dn = c - d * step, c + d * step
                        upok, dnok = up - r >= by, dn + r <= by + mx
                        if upok: dot(cx, up)
                        if dnok: dot(cx, dn)
                        if not upok and not dnok: break
                else:
                    cy = by + mx - d * step - r
                    if cy + r < by: break
                    dot(cx, cy)
            else:
                cy = by + i * step + r
                if anchor == 0:
                    cx = bx + mx - d * step - r
                    if cx + r < bx: break
                    dot(cx, cy)
                elif anchor == 1:
                    c = bx + mx / 2
                    if d == 0: dot(c, cy)
                    else:
                        rr_, ll = c + d * step, c - d * step
                        rok, lok = rr_ + r <= bx + mx, ll - r >= bx
                        if rok: dot(rr_, cy)
                        if lok: dot(ll, cy)
                        if not rok and not lok: break
                else:
                    cx = bx + d * step + r
                    if cx - r > bx + mx: break
                    dot(cx, cy)
    return prims


def ref_radial(cfg):
    prims = []
    n = int(cfg["bars"]); mx = cfg["maxSize"]; idle = cfg["idle"]
    cx, cy = cfg["cx"], cfg["cy"]; inner = mx * 0.15; sw = max(1.0, cfg["barW"])
    for i in range(n):
        fac = max(0.0, cfg["levels"][i])
        ln = idle + fac * max(0.0, mx - idle)
        if ln < 0.5:
            continue
        a = i / n * 2 * math.pi - math.pi / 2
        dx, dy = math.cos(a), math.sin(a)
        prims.append(("cap", (cx + dx * inner, cy + dy * inner, cx + dx * (inner + ln), cy + dy * (inner + ln), sw / 2),
                      cfg["col"](i, fac, True)))
    return prims


def compare(name, got, ref, tol_max=0.12, tol_area=0.006):
    ga, ra = got[..., 3], ref[..., 3]
    diff = np.abs(ga - ra)
    area = abs(ga.sum() - ra.sum()) / max(1.0, ra.sum())
    rgb = np.max(np.abs(got[..., :3] - ref[..., :3]))
    ok = diff.max() <= tol_max and area <= tol_area and rgb <= tol_max + 0.02
    check(name, ok, f"max px err {diff.max():.3f}, area err {area*100:.3f}%, rgb err {rgb:.3f}")
    return ok


white = lambda i, fac, radial=False: (1, 1, 1, 1)
rng = np.random.default_rng(5)

base = dict(vw=420, vh=180, bx=10.37, by=12.61, barW=6, gap=4, maxSize=140, idle=4, bars=32,
            colorMode=0, flags=0, capT=2, col=white)
for vert in [0, 1]:
    for anchor in [0, 1, 2]:
        for radii in [[0, 0, 0, 0], [3, 3, 3, 3], [5, 5, 0, 0], [0, 7, 2, 9]]:
            cfg = dict(base, vertical=vert, anchor=anchor, radii=radii, levels=rng.random(32).tolist())
            if vert:
                cfg.update(vw=180, vh=420)
            got = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[1, 32]))
            ref = ref_render(int(cfg["vw"]), int(cfg["vh"]), ref_bars(cfg))
            compare(f"bars vert={vert} anchor={anchor} radii={radii}", got, ref)

# Thin and fractional bars, 1 px wide.
cfg = dict(base, barW=1, gap=1, bars=120, vw=300, levels=rng.random(120).tolist(), vertical=0, anchor=2, radii=[0, 0, 0, 0])
got = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[1, 120]))
compare("1 px bars at a fractional origin", got, ref_render(300, 180, ref_bars(cfg)))
cfg = dict(base, barW=2.5, gap=0.75, bars=64, vw=260, levels=rng.random(64).tolist(), vertical=0, anchor=1, radii=[1, 1, 1, 1])
got = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[1, 64]))
compare("2.5 px bars (150% DPI), middle anchor", got, ref_render(260, 180, ref_bars(cfg)))

# Peak caps.
for vert in [0, 1]:
    for anchor in [0, 1, 2]:
        lv = rng.random(32) * 0.8
        pk = np.minimum(1, lv + rng.random(32) * 0.3)
        cfg = dict(base, vertical=vert, anchor=anchor, radii=[3, 3, 3, 3], levels=lv.tolist(), peaks=pk.tolist(),
                   flags=1, peakColor=(1, 0.2, 0.2, 1))
        if vert:
            cfg.update(vw=180, vh=420)
        got = raster(dict({k: v for k, v in cfg.items() if k not in ("col",)}, passes=[1, 32, 2, 32]))
        ref = ref_render(int(cfg["vw"]), int(cfg["vh"]), ref_bars(cfg, caps=True))
        compare(f"bars + peak caps vert={vert} anchor={anchor}", got, ref)

# Dots.
for vert in [0, 1]:
    for anchor in [0, 1, 2]:
        cfg = dict(base, vertical=vert, anchor=anchor, dotRadii=[3, 3, 3, 3], levels=rng.random(32).tolist(), radii=[0, 0, 0, 0])
        if vert:
            cfg.update(vw=180, vh=420)
        maxDots = math.ceil(cfg["maxSize"] / (cfg["barW"] + cfg["gap"])) + 1
        slots = maxDots * 2 + 1 if anchor == 1 else maxDots
        got = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[3, 32 * slots]))
        ref = ref_render(int(cfg["vw"]), int(cfg["vh"]), ref_dots(cfg))
        compare(f"dots vert={vert} anchor={anchor}", got, ref, tol_area=0.012)

# Radial.
cfg = dict(base, shape=6, bars=48, vw=320, vh=320, cx=160.3, cy=159.7, maxSize=140, levels=rng.random(48).tolist(),
           col=lambda i, f, radial=True: (1, 1, 1, 1))
got = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[4, 48]))
compare("radial capsules", got, ref_render(320, 320, ref_radial(cfg)), tol_max=0.15, tol_area=0.01)

# ---- Terminal glyph pass ----------------------------------------------------------
# The renderer uploads only the cells with a glyph, each packed as
# char | colour << 8 | grid index << 16, and draws that many instances.
def term_pack(cells):
    return [(c & 0xFFFF) | (i << 16) for i, c in enumerate(cells) if (c & 127) > 32]

def term_ref(cells, cols, cw, chh, tx, ty, size=80):
    ref = np.zeros((size, size, 4))
    for i, cell in enumerate(cells):
        ch, ci = cell & 127, (cell >> 8) & 7
        if ch <= 32:
            continue
        g = ch - 32
        col = (0.2 * (ci + 1), 1.0 - 0.15 * ci, 0.5, 1.0)
        x0, y0 = tx + (i % cols) * cw, ty + (i // cols) * chh
        for y in range(chh):
            for x in range(cw):
                a = ((g * 7 + x * 3 + y * 5) % 11) / 10.0
                ref[y0 + y, x0 + x] = [col[0] * a, col[1] * a, col[2] * a, a]
    return ref

cw, chh, cols, rows, tx, ty = 7, 12, 9, 5, 3, 5
cells = []
for i in range(cols * rows):
    ch = int(rng.integers(32, 127))
    cells.append(ch | (int(rng.integers(0, 5)) << 8))
packed = term_pack(cells)
img = raster(dict(vw=80, vh=80, termCols=cols, termRows=rows, cellW=cw, cellH=chh, tx=tx, ty=ty, sceneAlpha=1,
                  cells=packed, passes=[8, len(packed)]))
err = np.max(np.abs(img - term_ref(cells, cols, cw, chh, tx, ty)))
check("terminal glyphs: every cell, UV and colour exact", err < 1e-5, f"max err {err:.2e}")

# Mostly blank (a Columns grid): the compact list draws the same picture with
# a fraction of the instances.
sparse = [32] * (cols * rows)
for c in range(cols):
    h = int(rng.integers(0, rows + 1))
    for k in range(h):
        sparse[(rows - 1 - k) * cols + c] = ord('|') | ((2 if k > 2 else 1) << 8)
packed = term_pack(sparse)
img = raster(dict(vw=80, vh=80, termCols=cols, termRows=rows, cellW=cw, cellH=chh, tx=tx, ty=ty, sceneAlpha=1,
                  cells=packed, passes=[8, len(packed)]))
err = np.max(np.abs(img - term_ref(sparse, cols, cw, chh, tx, ty)))
check(f"terminal compact list: {len(packed)} of {cols * rows} cells drawn, picture exact", err < 1e-5,
      f"max err {err:.2e}")

# ---- Colours: Color Mode rules vs the 1.5 code ------------------------------------
def hsv(h, s, v, a):
    h = h % 360
    c = v * s; x = c * (1 - abs((h / 60) % 2 - 1)); m = v - c
    r, g, b = [(c, x, 0), (x, c, 0), (0, c, x), (0, x, c), (x, 0, c), (c, 0, x)][int(h // 60) % 6]
    return (r + m, g + m, b + m, a)

def lerp4(a, b, t): return tuple(a[k] + (b[k] - a[k]) * t for k in range(4))

c1, g1, c2 = (0.9, 0.3, 0.2, 1), (0.1, 0.8, 0.4, 1), (0.2, 0.3, 0.95, 1)
n = 16
levels = np.linspace(0.2, 1.0, n)
for mode, fn in [
    (1, lambda i, f: lerp4(g1, c2, i / (n - 1))),
    (2, lambda i, f: lerp4(g1, c2, f)),
    (5, lambda i, f: lerp4(g1, c2, min(1, i / (n - 1) * 0.6 + f * 0.4))),
    (6, lambda i, f: (c1[0], c1[1], c1[2], min(180, int(180 * f)) / 255)),
    (7, lambda i, f: hsv(37.0 + i / (n - 1) * 360, 0.85, 1, 1)),
]:
    cfg = dict(vw=200, vh=120, bx=4, by=4, barW=8, gap=4, maxSize=100, idle=4, bars=n, vertical=0, anchor=2,
               radii=[0, 0, 0, 0], colorMode=mode, c1=c1, grad1=g1, c2=c2, rainbowBase=37.0, levels=levels.tolist())
    img = raster(dict(cfg, passes=[1, n]))
    worst = 0
    for i in range(n):
        x = int(4 + i * 12 + 4); y = 4 + 100 - 2
        px = img[y, x]
        want = fn(i, levels[i])
        a = want[3]
        exp = np.array([want[0] * a, want[1] * a, want[2] * a, a])
        worst = max(worst, np.max(np.abs(px - exp)))
    check(f"colour mode {mode} matches the Direct2D rules", worst < 2e-3, f"worst {worst:.4f}")

# ---- Compute: GPU kernels vs ttdsp ---------------------------------------------
FS = 48000
t = np.arange(FS * 2) / FS
sig = (0.5 * np.sin(2 * np.pi * 997 * t) + 0.25 * np.sin(2 * np.pi * 63 * t) + 0.05 * rng.standard_normal(len(t))).astype(np.float32)
sig.tofile(f"{TMP}/s.f32")
for n_, layout, frac, det in [(2048, 1, 6, 0), (4096, 1, 24, 0), (1024, 0, 0, 0), (2048, 1, 3, 1)]:
    write_cfg(f"{TMP}/cc.txt", dict(n=n_, layout=layout, frac=frac, ebars=96, det=det))
    out = subprocess.run([EXE, "compute", f"{TMP}/cc.txt", f"{TMP}/s.f32", f"{TMP}/r"], check=True, capture_output=True, text=True).stdout
    gp = np.fromfile(f"{TMP}/r.gpu_power", dtype=np.float32)
    cp = np.fromfile(f"{TMP}/r.cpu_power", dtype=np.float32)
    rel = np.max(np.abs(gp - cp)) / np.max(cp)
    check(f"CsFft power == ttdsp RealFft (N={n_})", rel < 1e-4, f"rel err {rel:.2e}")
    gdb = np.fromfile(f"{TMP}/r.gpu_db", dtype=np.float32)
    cdb = np.fromfile(f"{TMP}/r.cpu_db", dtype=np.float32)
    sel = cdb > -120
    err = np.max(np.abs(gdb[sel] - cdb[sel]))
    check(f"CsBands dB == ttdsp band dB (N={n_}, layout={layout}, 1/{frac}, det={det})", err < 0.05, f"max {err:.4f} dB")
    lv = np.fromfile(f"{TMP}/r.gpu_level", dtype=np.float32)
    want = np.clip((cdb + 72) / 60, 0, 1)
    err = np.max(np.abs(lv - want))
    check(f"CsShape bar levels == display mapping (N={n_})", err < 0.01, f"max {err:.4f}")
    parts = out.split()
    g, c = float(parts[2]), float(parts[4])
    check(f"CsReduce dominant Hz == ttdsp (N={n_})", abs(g - c) < 0.5 and abs(g - 997) < 4, f"gpu {g:.2f} cpu {c:.2f}")

print()
# Outline and Shadow (FX): one wide bar, outline just inside its edge, the
# shadow down and to the right, nothing further out.
cfg = dict(base, bars=1, barW=40, gap=0, vw=120, vh=180, bx=20, by=10, vertical=0, anchor=2, radii=[0, 0, 0, 0],
           levels=[0.5], lineW=2, lineColor=(1, 0, 0, 1), shadow=(0, 0, 0, 0.8), shadowX=4, shadowY=4, shadowSoft=0)
img = raster(dict({k: v for k, v in cfg.items() if k != "col"}, passes=[1, 1]))
ys = np.where(img[:, 40, 3] > 0.5)[0]
top, bot = (int(ys.min()), int(ys.max())) if len(ys) else (0, 0)
mid = (top + bot) // 2
edge, body = img[mid, 20], img[mid, 40]   # leftmost pixel column of the bar, its middle
shade, far = img[mid, 62], img[mid, 70]   # 2 px right of the bar (inside the 4 px shadow), well past it
check("outline: edge pixel takes the outline colour", edge[0] > 0.8 and edge[1] < 0.2, f"edge {edge}")
check("outline: inside the bar keeps its own colour", body[1] > 0.8, f"body {body}")
check("shadow: dark and opaque beside the bar", shade[3] > 0.6 and shade[0] < 0.1, f"shade {shade}")
check("shadow: nothing past the offset", far[3] < 0.01, f"far {far}")

print("FAILED: " + ", ".join(fails) if fails else "ALL PASSED")
sys.exit(1 if fails else 0)
