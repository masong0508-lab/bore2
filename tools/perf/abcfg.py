#!/usr/bin/env python3
# abcfg.py BIN CAM VIEW [HELPER_OFF [PIN]]: set an AB_CFG test build's camera (1 default, 2 zoomed in, 3 zoomed out, 4 first person),
# VIEW DISTANCE (0..5), speed helper (1 = off) and, with the helper on, optionally the far view's step it is held at in place in its disc image, so one build runs every camera and view distance.
import sys
p, cam, view = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]); off = int(sys.argv[4]) if len(sys.argv) > 4 else 0; pin = int(sys.argv[5]) if len(sys.argv) > 5 else 0
d = bytearray(open(p, 'rb').read())
i = d.find(b'ABCF')
assert i >= 0 and d.find(b'ABCF', i + 1) < 0, 'magic not found once'
d[i + 4], d[i + 5], d[i + 6], d[i + 7] = cam, view, off, pin   # (pin: hold the far view's helper step: 1 as chosen, 2 to 4 shorter, 5 lite, 6 none; 0 the helper decides)
open(p, 'wb').write(d)
import os
par = os.environ.get('AB_PAR')
if par:   # AB_PAR="0=5,1=96": tuning bytes of an AB_CFG build (src/world.c and others read them with ab_par(k, default))
    d = bytearray(open(p_ := sys.argv[1], 'rb').read()); j = d.find(b'ABPR'); assert j >= 0
    for kv in par.split(','):
        k, v = kv.split('='); d[j + 4 + int(k)] = int(v)
    open(sys.argv[1], 'wb').write(d)
