#!/usr/bin/env python3
# views_script.py PLAN.h OUT.txt [T0]: the DuckStation script for a views_plan.h: start + menu presses, then a shot 55 frames after each 'p' / 'g' / 'v' entry
# (T0: the frame the life starts at, about 1950 for a Release build).
import re, sys
t0 = int(sys.argv[3]) if len(sys.argv) > 3 else 1950
lines = ["1100 start", "1340 b", "1500 b"]
for m in re.finditer(r"\{\s*(\d+),\s*'(\w)',", open(sys.argv[1]).read()):
    if m.group(2) in 'pgvH': lines.append("%d shot" % (t0 + int(m.group(1)) + 55))
open(sys.argv[2], 'w').write("\n".join(lines) + "\n")
print(len(lines) - 3, 'shots, last frame', t0 + 200 + int(re.findall(r"\{\s*(\d+),\s*'", open(sys.argv[1]).read())[-1]))
