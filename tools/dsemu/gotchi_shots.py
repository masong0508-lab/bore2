#!/usr/bin/env python3
"""gotchi_shots.py rom outdir base scene name frame [name frame ...]: D69 shots of the Boreagotchi first-run screens and eggs (both screens, top over bottom).
The ROM is a -DDSTEST -DAUTO_SCENE=<base> build; the scene's low four bits (X=1 Y=2 L=4 R=8) are held at frame 385. Frames are counted from the start of the run
(the scene opens at about frame 395; its clock runs 60 ticks a second, a frame is about 1/60 s at 60 fps). Example:
  gotchi_shots.py rom.nds out 96 98 unbox_a 400 unbox_b 440
Also: --sheet outdir out.png name... pastes shots side by side."""
import os, subprocess, sys
from PIL import Image
def main():
    rom, out, base, sc = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]); rest = sys.argv[5:]
    os.makedirs(out, exist_ok=True); d = sc - base
    keys = (0x400 if d & 1 else 0) | (0x800 if d & 2 else 0) | (0x200 if d & 4 else 0) | (0x100 if d & 8 else 0)
    names = rest[0::2]; frames = [int(x) for x in rest[1::2]]
    script = "100-103:0:212,140;200-203:1:-1,-1" + (";385-396:%x:-1,-1" % keys if keys else "") + os.environ.get("GS", "")
    ppm = os.path.join(out, "_t.ppm")
    subprocess.run(["/tmp/dsrun", rom, str(max(frames) + 1), ppm, script, ",".join(map(str, sorted(frames)))], capture_output=True, text=True)
    for n, f in zip(names, frames):
        p = os.path.join(out, "_t_%d.ppm" % f)
        if os.path.exists(p): Image.open(p).save(os.path.join(out, n + ".png")); os.remove(p)
    if os.path.exists(ppm): os.remove(ppm)
def sheet():
    d, o, names = sys.argv[2], sys.argv[3], sys.argv[4:]
    ims = [Image.open(os.path.join(d, n + ".png")) for n in names]
    W = sum(i.width for i in ims) + 4 * (len(ims) - 1); S = Image.new("RGB", (W, max(i.height for i in ims)), (255, 0, 255)); x = 0
    for i in ims: S.paste(i, (x, 0)); x += i.width + 4
    S.save(o)
if __name__ == "__main__":
    if sys.argv[1] == "--sheet": sheet()
    else: main()
