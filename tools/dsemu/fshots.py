#!/usr/bin/env python3
"""fshots.py rom outdir [name ...]: screenshots of the UI screens (top over bottom, 256 x 384) for the font and alignment checks (D65).
Run it on the old ROM and the new one, then `fshots.py --pair beforedir afterdir outdir` pastes each name side by side (before | after).
Scenes that need a test build (-DDSTEST -DAUTO_SCENE=72) are the g_* ones of bench.py; the rest are plain navigation with the touch script below."""
import os, subprocess, sys
from PIL import Image
OK = "100-103:0:212,140"; LIVE = OK + ";200-203:1:-1,-1"; MENU = ";400-403:0:128,185"
def chipm(i): return ";440-443:0:%d,%d" % (14 + (i % 4) * 58 + 27, 60 + (i // 4) * 36 + 15)   # a chip of the Live menu (Buy 0, Build 1, Map 2, Save 3, Options 4, Title 5, Job 6, Songs 7, Life 8, Travel 9, Games 10, Heist 11)
SC = {   # name: (script, frame)
    "title": ("", 60),
    "title_opt": ("30-33:0:144,141", 80),
    "title_opt2": ("30-33:0:144,141;60-63:0:100,29", 90),
    "cas_hair": ("30-33:0:128,90;60-63:0:212,140", 180),
    "cas_pad": ("30-33:0:128,90;60-63:0:212,140;120-123:0:29,129", 180),
    "cas_paint": ("30-33:0:128,90;60-63:0:212,140;120-123:0:29,149", 180),
    "cas_more": ("30-33:0:128,90;60-63:0:212,140;120-123:0:29,169", 180),
    "map": (OK, 150),
    "live": (LIVE, 300),
    "menu": (LIVE + MENU, 450),
    "buy": (LIVE + MENU + chipm(0), 600),
    "build": (LIVE + MENU + chipm(1), 600),
    "options": (LIVE + MENU + chipm(4), 600),
    "job": (LIVE + MENU + chipm(6), 600),
    "songs": (LIVE + MENU + chipm(7), 600),
    "life": (LIVE + MENU + chipm(8), 600),
    "travel": (LIVE + MENU + chipm(9), 600),
}
SC40 = {   # needs a -DDSTEST -DAUTO_SCENE=40 build: the keys held at frame 420 pick the scene (X+Y 43 = the first game, X+Y+L 47 = the prize counter)
    "arc_intro": (LIVE + ";420-440:c00:-1,-1", 470),
    "arc_play": (LIVE + ";420-440:c00:-1,-1;500-503:1:-1,-1;560-640:10:-1,-1", 700),
    "arc_prizes": (LIVE + ";420-440:e00:-1,-1", 500),
    "skate": (LIVE + ";420-440:4:-1,-1", 520),        # SELECT held: scene 40 (a -DAUTO_SCENE=40 build)
    "heist": (LIVE + ";420-440:4:-1,-1", 520),        # the same with a -DAUTO_SCENE=48 build: scene 48, the Underworld
}
SC.update(SC40)
def shot(rom, out, name):
    script, fr = SC[name]
    ppm = os.path.join(out, name + ".ppm")
    subprocess.run(["/tmp/dsrun", rom, str(fr + 1), ppm, script, str(fr)], capture_output=True, text=True)
    sp = ppm[:-4] + "_%d.ppm" % fr
    if os.path.exists(sp): Image.open(sp).save(os.path.join(out, name + ".png")); os.remove(sp)
    if os.path.exists(ppm): os.remove(ppm)
if __name__ == "__main__":
    if sys.argv[1] == "--pair":
        a, b, out = sys.argv[2:5]; os.makedirs(out, exist_ok=True)
        for f in sorted(os.listdir(b)):
            if not f.endswith(".png") or not os.path.exists(os.path.join(a, f)): continue
            x, y = Image.open(os.path.join(a, f)).convert("RGB"), Image.open(os.path.join(b, f)).convert("RGB")
            s = Image.new("RGB", (x.width + y.width + 4, x.height), (255, 0, 255)); s.paste(x, (0, 0)); s.paste(y, (x.width + 4, 0))
            s.save(os.path.join(out, "fonts_" + f))
    else:
        rom, out = sys.argv[1], sys.argv[2]; os.makedirs(out, exist_ok=True)
        for n in (sys.argv[3:] or list(SC)): shot(rom, out, n)
