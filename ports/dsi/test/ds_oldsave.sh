#!/bin/bash
# ds_oldsave.sh : D82 an OLD ROM's save (opt[38]=0, gen 3 town) is read by this build.
#   OLD: a build of an old commit (git worktree add /tmp/old <commit>; OUT=/tmp/dsold R=/tmp/old XTRA="-DDSTEST -DAUTO_SCENE=140" tools/dsbuild/mkp.sh)
#   NEW: this commit, same XTRA.   AUTO_SCENE=140: X+Y held = scene 143 (traits + wishes set, save written), L = 144 (save read back).
#   usage: OLD=/tmp/dsold NEW=/tmp/dsnew DSRUN=/tmp/dsrun bash ports/dsi/test/ds_oldsave.sh
#   PASS: the new build reads it (test_sres 401) with the same traits checksum (trg_sig) and the town is the generation the old build wrote (bm_gen; since D84 new lives are gen 8, so it was a fixed 3 and failed against any d84-or-later OLD).
O=${OLD:-/tmp/dsold}; N=${NEW:-/tmp/dsnew}; RUN=${DSRUN:-/tmp/dsrun}; S=$N/old.sav
sym() { llvm-nm "$1/bore.elf" | grep " $2\$" | cut -d' ' -f1; }
L="100-103:0:212,140;200-203:1:-1,-1"
rm -f $S
a=$(BORE_SAV=$S DBGMEM=$(sym $O test_sres),$(sym $O trg_sig),$(sym $O bm_gen) $RUN $O/bore.nds 700 $N/o.ppm "$L;385-388:c00:-1,-1" 2>&1 | tail -1)
b=$(BORE_SAV=$S DBGMEM=$(sym $N test_sres),$(sym $N trg_sig),$(sym $N bm_gen) $RUN $N/bore.nds 700 $N/o.ppm "$L;385-388:200:-1,-1" 2>&1 | tail -1)
echo "old writes: $a"; echo "new reads:  $b"
set -- ${a#*:}; sa=$2; ga=$3; set -- ${b#*:}; rb=$1; sb=$2; gb=$3
if [ "$rb" = 401 ] && [ "$sa" = "$sb" ] && [ "$gb" = "$ga" ]; then echo "OLDSAVE PASS"; else echo "OLDSAVE FAIL"; exit 1; fi
