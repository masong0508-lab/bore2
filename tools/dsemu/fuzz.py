#!/usr/bin/env python3
"""fuzz.py rom elf seed frames [lead] : random buttons and taps into the DS game under melonDS, to find hangs and crashes.
Reads vbl_n (the vblank counter) and mode every 30 frames (DBGMEM): a vbl_n that stops while the emulator runs is a hang or a crash.
Starts with the title flow (touch New game) then random input. Prints the last readings, any stall, and saves a shot of both screens at the end (PNG next to the seed).
usage: DSRUN=/tmp/ds80/dsrun python3 tools/dsemu/fuzz.py /tmp/ds80b/bore.nds /tmp/ds80b/bore.elf 1 6000"""
import os, random, subprocess, sys
rom, elf, seed, frames = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
lead = sys.argv[5] if len(sys.argv) > 5 else "100-103:0:212,140;200-203:1:-1,-1"
syms = {}
for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
    p = ln.split()
    if len(p) == 3: syms[p[2]] = int(p[0], 16)
KEYS = [0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800]
rnd = random.Random(seed); ev = [lead]; f = 300
while f < frames - 20:
    r = rnd.random()
    if r < 0.45: ev.append("%d-%d:%x:-1,-1" % (f, f + 2, rnd.choice(KEYS)))
    elif r < 0.95: ev.append("%d-%d:0:%d,%d" % (f, f + 2, rnd.randrange(256), rnd.randrange(192)))
    else: ev.append("%d-%d:%x:-1,-1" % (f, f + 40, rnd.choice(KEYS)))   # a long hold
    f += rnd.choice([4, 6, 8, 12, 20])
names = [n for n in ("vbl_n", "mode") if n in syms]
env = dict(os.environ, DBGMEM=",".join("%x" % syms[n] for n in names))
out = "/tmp/fuzz_%d" % seed
r = subprocess.run([os.environ.get("DSRUN", "/tmp/dsrun"), rom, str(frames), out + ".ppm", ";".join(ev)], capture_output=True, text=True, env=env)
rows = [l for l in r.stdout.splitlines() if l.startswith("f")]
prev = None; stall = 0; last = ""
for l in rows:
    p = l.split(); v = int(p[1]); m = int(p[2]) if len(p) > 2 else -1
    if prev is not None and v == prev: stall += 1
    else: stall = 0
    prev = v; last = l
    if stall == 3: print("STALL at", p[0], "vbl_n", v, "mode", m)
print("seed", seed, "last:", last, "stall rows", stall)
try:
    from PIL import Image
    Image.open(out + ".ppm").save(out + ".png")
except Exception as e: print("no shot", e)
