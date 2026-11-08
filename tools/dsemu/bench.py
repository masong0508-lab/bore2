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
    "toybox": (LIVE, {400: 52}, 500, 800, 780),   # (D53) the Toy Box
    "guide5": (LIVE, {400: 53}, 500, 800, 780),
    "guide3": (LIVE, {400: 54}, 500, 800, 780),
    "livebasket": (LIVE, {400: 55}, 500, 800, 780),
    "family": (LIVE, {400: 56}, 500, 800, 780),   # (D54) a family in the basket
    "tree":   (LIVE, {400: 57}, 500, 800, 780),
    "more":   (LIVE, {400: 58}, 500, 800, 780),
    "home":   (LIVE, {400: 59}, 500, 800, 780),   # (D54) its family tree
    "shop":   (LIVE, {400: 51}, 500, 800, 780),
    "g_hand": (LIVE + ";420-440:400:-1,-1", {}, 500, 800, 780),   # (D64) Boreagotchi v48: the handheld, feed, game picker, training, goals, stats, games, cards
    "g_feed": (LIVE + ";420-440:800:-1,-1", {}, 500, 800, 780), "g_pick": (LIVE + ";420-440:c00:-1,-1", {}, 500, 800, 780), "g_train": (LIVE + ";420-440:200:-1,-1", {}, 500, 800, 780), "g_goals": (LIVE + ";420-440:600:-1,-1", {}, 500, 800, 780),
    "g_st0": (LIVE + ";420-440:a00:-1,-1", {}, 500, 800, 780), "g_st2": (LIVE + ";420-440:e00:-1,-1", {}, 500, 800, 780), "g_st1": (LIVE + ";420-440:400:-1,-1", {}, 500, 800, 780), "g_st3": (LIVE + ";420-440:800:-1,-1", {}, 500, 800, 780),
    "g_beat": (LIVE + ";420-440:c00:-1,-1", {}, 500, 800, 780), "g_hop": (LIVE + ";420-440:200:-1,-1", {}, 500, 800, 780), "g_stack": (LIVE + ";420-440:600:-1,-1", {}, 500, 800, 780), "g_whack": (LIVE + ";420-440:a00:-1,-1", {}, 500, 800, 780),
    "g_intro0": (LIVE + ";420-440:e00:-1,-1", {}, 500, 800, 780), "g_intro2": (LIVE + ";420-440:100:-1,-1", {}, 500, 800, 780), "g_tut0": (LIVE + ";420-440:500:-1,-1", {}, 500, 800, 780), "g_event": (LIVE + ";420-440:900:-1,-1", {}, 500, 800, 780),
    "gtseq":  (LIVE + ";520-523:1:-1,-1", {400: 50}, 450, 800, 780),   # (D62) the toy: title, hatching, day summary
    "fight":  (LIVE + ";385-396:400:-1,-1;450-455:1:-1,-1;520-525:1:-1,-1" + "".join(";%d-%d:%x:-1,-1" % (800 + 40 * q, 819 + 40 * q, [0x1, 0x11, 0x2, 0x21, 0x4, 0x80][q % 6]) for q in range(30)), {}, 700, 1900, 1880),   # (D59) BORTUA FIGHTER vs the CPU: pick, VS, then punching and kicking (build with XTRA=-DAUTO_SCENE=88: X held at frame 385 opens scene 89)
    "fight2p": (LIVE + ";385-396:400:-1,-1;430-432:80:-1,-1;440-442:80:-1,-1;450-452:80:-1,-1;460-462:10:-1,-1;480-482:1:-1,-1;520-522:1:-1,-1;700-770:10:-1,-1;700-770:800:-1,-1" + "".join(";%d-%d:%x:-1,-1" % (780 + 40 * q, 790 + 40 * q, [0x200, 0x100, 0x1, 0x2, 0x400, 0x40][q % 6]) for q in range(30)), {}, 700, 1900, 1880),   # (D59) the same with two players on one DS
    "close":  (LIVE, {400: 6}, 700, 1000, 980),
    "horizon": (LIVE, {400: 5}, 700, 1300, 1280),
    "orbit":  (LIVE + ";500-1300:10:-1,-1", {}, 600, 1300, 1280),        # Live, the camera turning all the time (the still list is recorded again often)
    "skate":  (LIVE + "".join(";%d-%d:%x:-1,-1" % (500 + 50 * q, 549 + 50 * q, [0x50, 0x41, 0x20, 0x81][q % 4] | (0x800 if q % 3 == 1 else 0) | (0x400 if q % 5 == 2 else 0)) for q in range(20)), {400: 41}, 600, 1000, 980),   # (D51) riding about the plaza, jumping and flipping, staying inside the lot
    "farmorbit": (LIVE + ";500-1300:10:-1,-1", {400: 2}, 600, 1300, 1280),
}
# BENCH_LOOK=0/1/2 (Classic, Rich, Toon) and BENCH_DETAIL=0/1/2 (Auto, DS, DSi): poked at frame 300 and 330
EXTRA = {}
if os.environ.get("BENCH_LOOK"): EXTRA[300] = 10 + int(os.environ["BENCH_LOOK"])
if os.environ.get("BENCH_DETAIL"): EXTRA[330] = 20 + int(os.environ["BENCH_DETAIL"])
if os.environ.get("BENCH_ROOF"): EXTRA[360] = 30 + int(os.environ["BENCH_ROOF"])
want = sys.argv[4:] or list(SC)
mem = [syms["fps_now"], syms["polys_last"], "4000604"] + ([syms["ui_fps"], syms["st_rec"]] if "ui_fps" in syms else []) + (["%x" % (int(syms["prof_ms"], 16) + 4 * k) for k in range(12)] + [syms["sml_miss"]] if "prof_ms" in syms and os.environ.get("PROF") else [])
for name in want:
    script, pokes, a, b, shot = SC[name]
    if os.environ.get("BENCH_SHOT"): shot = int(os.environ["BENCH_SHOT"]); b = max(b, shot + 1)
    env = dict(os.environ, DBGMEM=",".join(mem))
    pokes = dict(pokes); pokes.update(EXTRA) if name != "map" else None
    if os.environ.get("BENCH_AUTO"):   # (D62) the POKE does not reach in this build: the ROM was built with -DAUTO_SCENE=<base> and the scene's low four bits are held as X Y L R at frame 385
        base = int(os.environ["BENCH_AUTO"]); sc_ = pokes.get(400, 0)
        if sc_ and (sc_ & ~15) != base: print("%-7s skipped (scene %d is not in the build's AUTO_SCENE=%d group)" % (name, sc_, base)); continue
        if sc_: script += ";385-396:%x:-1,-1" % ((0x400 if sc_ & 1 else 0) | (0x800 if sc_ & 2 else 0) | (0x200 if sc_ & 4 else 0) | (0x100 if sc_ & 8 else 0))
        pokes = {}
    if pokes: env["POKE"] = ",".join("%d:%s:%x" % (f, syms["test_scene"], v) for f, v in pokes.items())
    ppm = os.path.join(out, name + ".ppm")
    r = subprocess.run(["/tmp/dsrun", rom, str(b + 1), ppm, script, str(shot)], capture_output=True, text=True, env=env)
    fps, quads, ui, rec, prof, gpu = [], [], [], [], [], []
    for ln in r.stdout.splitlines():
        m = re.match(r"f(\d+):(.*)", ln)
        if not m or not (a <= int(m.group(1)) <= b): continue
        v = [int(x) for x in m.group(2).split()]
        gpu.append(v[2]); v = v[:2] + v[3:]
        fps.append(v[0]); quads.append(v[1] // 2)
        if len(v) > 3: ui.append(v[2]); rec.append(v[3])
        if len(v) > 4: prof.append(v[4:])
    sp = ppm[:-4] + "_%d.ppm" % shot
    if os.path.exists(sp): Image.open(sp).save(os.path.join(out, name + ".png")); os.remove(sp)
    if os.path.exists(ppm): os.remove(ppm)
    uis = " ui %d-%d  still lists recorded %d" % (min(ui), max(ui), rec[-1] - rec[0]) if ui else ""
    if prof: uis += "  ms/s steps %d ui %d moving %d still %d dma %d rest %d walls %d sims %d setup %d lvl %d pre %d - %d  sim misses %d" % tuple(sum(p[k] for p in prof) // len(prof) for k in range(13))
    if os.environ.get("TRACE") == "2": [print(f, "fps", x, "prof(steps ui moving still dma rest walls sims setup)", pr[:9]) for f, x, pr in zip(range(len(fps)), fps, prof)]
    if os.environ.get("TRACE"): print(" ".join("%d/%d" % (f, u) for f, u in zip(fps, ui or fps)))
    gp = [g & 0xFFF for g in gpu]; gv = [(g >> 16) & 0x1FFF for g in gpu]
    print("%-7s fps %2d-%2d avg %4.1f  quads sent %4d-%4d, GPU drew %4d-%4d polygons %4d-%4d vertices%s" % (name, min(fps), max(fps), sum(fps) / len(fps), min(quads), max(quads), min(gp), max(gp), min(gv), max(gv), uis))
