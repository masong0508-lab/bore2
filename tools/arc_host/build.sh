#!/bin/bash
# build.sh: the arcade games on a PC (tools/arc_host). Then: run.sh GAME TICKS EVERY  -> /tmp/ah/GAME.png (a strip of shots)
H=$(cd "$(dirname "$0")" && pwd); S=$H/../../src; mkdir -p /tmp/ah/src; cp $H/arc_i.h /tmp/ah/src/
for f in ag_old ag_new1 ag_new2; do cp $S/$f.c /tmp/ah/src/; done
gcc -O1 -w -DHOSTBOT -fsanitize=address,undefined -I/tmp/ah/src -o /tmp/ah/arcsim $H/arcsim.c /tmp/ah/src/ag_old.c /tmp/ah/src/ag_new1.c /tmp/ah/src/ag_new2.c
