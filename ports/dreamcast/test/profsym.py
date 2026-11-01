#!/usr/bin/env python3
# profsym.py <elf> <log>: the profiler's "P addr count" lines (dc_prof.c) as functions, busiest first, summed per function
import subprocess, sys, bisect, collections
syms = []
for l in subprocess.run(['sh-elf-nm', '-n', sys.argv[1]], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) == 3 and p[1] in 'tTwW': syms.append((int(p[0], 16), p[2]))
addrs = [a for a, n in syms]
tot = collections.Counter(); n_all = 0
for l in open(sys.argv[2], errors='replace'):
    if l.startswith('PROF'): n_all = int(l.split()[1])
    if not l.startswith('P '): continue
    a, c = int(l.split()[1], 16), int(l.split()[2])
    i = bisect.bisect_right(addrs, a) - 1
    tot[syms[i][1] if i >= 0 else '?'] += c
print('samples', n_all)
for f, c in tot.most_common(40): print('%5.1f%%  %s' % (100.0 * c / max(1, n_all), f))
