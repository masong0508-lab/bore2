#!/usr/bin/env python3
"""Frame-rate bench for the DS port in melonDS (tools/dsemu/run.cpp built as /tmp/dsrun).
   bench.py test_rom.nds test_rom.elf outdir [scene ...]
The ROM must be a -DDSTEST build (the test_scene hook). Prints, per scene, the 3D frame rate the game counts
(fps_now: main-loop passes a second; with the D39 split loop, 3D frames a second) min/avg over the window, the UI rate
(ui_fps, D39 on) and the quads sent, and writes outdir/<scene>.png (top screen over bottom screen)."""
import os, re, subprocess, sys
from PIL import Image

rom, elf, out = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(out, exist_ok=True)
syms = {}
for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
    p = ln.split()
    if len(p) == 3: syms[p[2]] = p[0]
OK = "100-103:0:212,140"            # title: OK (new game) -> the Map
LIVE = OK + ";200-203:1:-1,-1"      # Map: A -> Live on the picked lot
MENU = ";400-403:0:128,185"         # Live: the "..." menu
SC = {   # name: (script, pokes {frame: scene}, measure from, to, screenshot frame)
    "map":    (OK, {}, 300, 900, 880),
    "live":   (LIVE, {}, 500, 1100, 1080),
    "buy":    (LIVE + MENU + ";440-443:0:43,75", {}, 600, 1200, 1180),
    "build":  (LIVE + MENU + ";440-443:0:101,75", {}, 600, 1200, 1180),
    "island": (LIVE, {400: 1}, 700, 1300, 1280),
    "farm":   (LIVE, {400: 2}, 700, 1300, 1280),
    "street": (LIVE, {400: 3}, 700, 1300, 1280),
    "town":   (LIVE, {400: 4}, 700, 1300, 1280),
}
want = sys.argv[4:] or list(SC)
mem = [syms["fps_now"], syms["polys_last"]] + ([syms["ui_fps"], syms["st_rec"]] if "ui_fps" in syms else []) + (["%x" % (int(syms["prof_ms"], 16) + 4 * k) for k in range(12)] + [syms["sml_miss"]] if "prof_ms" in syms and os.environ.get("PROF") else [])
for name in want:
    script, pokes, a, b, shot = SC[name]
    env = dict(os.environ, DBGMEM=",".join(mem))
    if pokes: env["POKE"] = ",".join("%d:%s:%x" % (f, syms["test_scene"], v) for f, v in pokes.items())
    ppm = os.path.join(out, name + ".ppm")
    r = subprocess.run(["/tmp/dsrun", rom, str(b + 1), ppm, script, str(shot)], capture_output=True, text=True, env=env)
    fps, quads, ui, rec, prof = [], [], [], [], []
    for ln in r.stdout.splitlines():
        m = re.match(r"f(\d+):(.*)", ln)
        if not m or not (a <= int(m.group(1)) <= b): continue
        v = [int(x) for x in m.group(2).split()]
        fps.append(v[0]); quads.append(v[1] // 2)
        if len(v) > 3: ui.append(v[2]); rec.append(v[3])
        if len(v) > 4: prof.append(v[4:])
    sp = ppm[:-4] + "_%d.ppm" % shot
    if os.path.exists(sp): Image.open(sp).save(os.path.join(out, name + ".png")); os.remove(sp)
    if os.path.exists(ppm): os.remove(ppm)
    uis = " ui %d-%d  still lists recorded %d" % (min(ui), max(ui), rec[-1] - rec[0]) if ui else ""
    if prof: uis += "  ms/s steps %d ui %d moving %d still %d dma %d rest %d walls %d sims %d setup %d lvl %d pre %d - %d  sim misses %d" % tuple(sum(p[k] for p in prof) // len(prof) for k in range(13))
    if os.environ.get("TRACE"): print(" ".join("%d/%d" % (f, u) for f, u in zip(fps, ui or fps)))
    print("%-7s fps %2d-%2d avg %4.1f  quads %4d-%4d%s" % (name, min(fps), max(fps), sum(fps) / len(fps), min(quads), max(quads), uis))
