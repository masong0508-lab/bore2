#!/usr/bin/env python3
"""peek.py rom elf frames "script" var[+off] ... : print the named variables (32-bit words) every 30 frames (DBGMEM)."""
import os, subprocess, sys
rom, elf, frames, script = sys.argv[1:5]
syms = {}
for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
    p = ln.split()
    if len(p) == 3: syms[p[2]] = int(p[0], 16)
addrs = []
for v in sys.argv[5:]:
    n, _, off = v.partition("+")
    addrs.append("%x" % ((syms[n] if n in syms else int(n, 16)) + (int(off) if off else 0)))
env = dict(os.environ, DBGMEM=",".join(addrs))
r = subprocess.run(["/tmp/dsrun", rom, frames, "/tmp/peek.ppm", script], capture_output=True, text=True, env=env)
print("        " + " ".join(sys.argv[5:]))
print(r.stdout[-3000:])
