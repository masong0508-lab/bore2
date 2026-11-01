#!/bin/bash
# runbuild.sh BUILDDIR NAME [frames] [script]: copy a built test disc aside and run the city script on it (no rebuild), print the readout
# (BUILDDIR must have been configured with -DAUTO_CITY=1 and built; the copy means the build can be changed while this runs)
HERE=$(cd "$(dirname "$0")" && pwd)
B=$1; N=$2; F=${3:-3700}; S=${4:-$HERE/city.txt}
D=/tmp/ab/$N; mkdir -p $D
cp -f $B/bore2.elf $B/bore2_lite.cue $B/bore2_lite.bin $D/
cd /home/claude/bore2 && DI=100000 DSPROF=$D/pc.txt DSPROFFROM=2200 bash test/ds.sh $D/bore2_lite.cue ab_$N $F $S > $D/run.log 2>&1
python3 $HERE/ramprof.py /tmp/ds_ab_${N}_ram.bin $D/bore2.elf > $D/out.txt 2>&1
echo "== $N"; grep -E "WHOLE|far pass|mean shed|share of frames" $D/out.txt
