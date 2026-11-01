#!/usr/bin/env python3
# abcfg.py BIN CAM VIEW [HELPER_OFF]: set an AB_CFG test build's camera (1 default, 2 zoomed in, 3 zoomed out, 4 first person),
# VIEW DISTANCE (0..5) and speed helper (1 = off) in place in its disc image, so one build runs every camera and view distance.
import sys
p, cam, view = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]); off = int(sys.argv[4]) if len(sys.argv) > 4 else 0
d = bytearray(open(p, 'rb').read())
i = d.find(b'ABCF')
assert i >= 0 and d.find(b'ABCF', i + 1) < 0, 'magic not found once'
d[i + 4], d[i + 5], d[i + 6] = cam, view, off
open(p, 'wb').write(d)
