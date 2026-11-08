#!/usr/bin/env python3
"""uibench.py rom.nds rom.elf [scene ...]   (D70) the menu responsiveness meter, for -DDSTEST builds (main.c: ui_seen ui_proc ui_lat_* ui_stp).
Plays a script of fast taps / key presses in melonDS and prints, per scene:
  seen     presses the vblank latch saw      proc  presses the main loop acted on (seen - proc = presses merged into one = dropped)
  lat      vblanks (1/60 s) from the press to the end of the pass that drew the answer: average and worst
  hitch    passes that took 2 or more vblanks (a frame that did not make 60) out of all passes in the scene
  fps      the 3D frames a second and the UI passes a second in the last 30 frames
Same scripts for any ROM, so run it on the old ROM and the new one:  tools/dsemu/tb.sh builds a test ROM (B=dir; XTRA=...)."""
import re, subprocess, sys, os
rom, elf = sys.argv[1], sys.argv[2]
syms = {}
for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
    p = ln.split()
    if len(p) == 3: syms[p[2]] = p[0]
OK = "100-103:0:212,140"; LIVE = OK + ";200-203:1:-1,-1"
def chipm(i): return ";440-443:0:%d,%d" % (14 + (i % 4) * 58 + 27, 60 + (i // 4) * 36 + 15)
def taps(f0, n, gap, pts, hold=2):
    return "".join(";%d-%d:0:%d,%d" % (f0 + k * gap, f0 + k * gap + hold - 1, pts[k % len(pts)][0], pts[k % len(pts)][1]) for k in range(n))
def keys(f0, n, gap, mask, hold=2):
    return "".join(";%d-%d:%x:-1,-1" % (f0 + k * gap, f0 + k * gap + hold - 1, mask[k % len(mask)] if isinstance(mask, list) else mask) for k in range(n))
ROWS = [(150, 60), (60, 80), (200, 100), (150, 120), (60, 140), (200, 60)]
SC = {   # name: (script, frames to run, what)
    "opt_mash": ("30-33:0:144,141" + taps(80, 60, 4, ROWS), 400, "title > Options, 60 fast taps on rows and tabs"),
    "opt_dpad": ("30-33:0:144,141" + keys(80, 60, 4, [0x40, 0x10, 0x80, 0x20, 0x10, 0x10]), 400, "Options, 60 d-pad presses (needs the new Options)"),
    "opt_hold": ("30-33:0:144,141;80-200:10:-1,-1", 300, "Options, RIGHT held 2 s (hold-to-repeat; new Options only)"),
    "menu_open": (LIVE + keys(420, 30, 6, 0x8), 700, "Live, START (open/close the menu) 30 times"),
    "menu_tap": (LIVE + ";400-403:0:128,185" + taps(440, 40, 4, [(41, 75), (101, 75), (160, 75), (222, 75), (41, 111), (101, 111)]), 700, "Live menu, 40 fast taps on its buttons"),
    "buy_tap": (LIVE + ";400-403:0:128,185;440-443:0:43,75" + taps(500, 50, 4, [(40, 60), (110, 60), (180, 60), (40, 110), (110, 110), (180, 110)]), 800, "Buy catalogue, 50 fast taps"),
    "popup": (LIVE + taps(420, 30, 8, [(128, 100), (200, 40), (60, 120)]), 700, "Live, 30 taps on the lot (action menus open and close)"),
}
START = {"opt_mash": 60, "opt_dpad": 60, "opt_hold": 60, "menu_open": 410, "menu_tap": 430, "buy_tap": 490, "popup": 410}   # the counters are zeroed at these frames (POKE), so the boot and the walk to the lot do not count
want = sys.argv[3:] or list(SC)
names = ["ui_seen", "ui_proc", "ui_lat_sum", "ui_lat_max", "ui_lat_n"] + ["ui_stp"] * 0
mem = [syms[n] for n in names] + ["%x" % (int(syms["ui_stp"], 16) + 4 * k) for k in range(1, 5)] + [syms["fps_now"], syms["ui_fps"]]
print("%-10s %5s %5s %5s %7s %5s %8s %9s  %s" % ("scene", "seen", "proc", "drop", "lat avg", "worst", "hitch", "fps 3D/UI", ""))
for n in want:
    script, fr, what = SC[n]
    r = subprocess.run(["/tmp/dsrun", rom, str(fr), "/tmp/ui/_u.ppm", script], capture_output=True, text=True, env=dict(os.environ, DBGMEM=",".join(mem), POKE=",".join("%d:%s:0" % (START[n], a) for a in mem[:9])))
    last = None
    for ln in r.stdout.splitlines():
        m = re.match(r"f(\d+):(.*)", ln)
        if m: last = [int(x) for x in m.group(2).split()]
    if not last: print(n, "no data"); continue
    seen, proc, ls, lm, ln_, s1, s2, s3, s4, fps, uif = last
    tot = s1 + s2 + s3 + s4
    print("%-10s %5d %5d %5d %7.2f %5d %7.1f%% %4d/%-4d  %s" % (n, seen, proc, seen - proc, ls / max(ln_, 1), lm, 100.0 * (s2 + s3 + s4) / max(tot, 1), fps, uif, what))
try: os.remove("/tmp/ui/_u.ppm")
except OSError: pass
