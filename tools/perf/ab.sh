#!/bin/bash
# ab.sh NAME [frames]: (needs an AUTO_CITY=1 Release build in /tmp/c_cut: see README.md here)
# build the test disc (unless NOBUILD=1: re-run the saved one), run the city script, print the whole-run readout and the top PC samples
HERE=$(cd "$(dirname "$0")" && pwd)
N=$1; F=${2:-3700}; R=${RUN:-}
D=/tmp/ab/$N; mkdir -p $D
if [ -z "$NOBUILD" ]; then
  cd /tmp/c_cut && ninja 2>&1 | grep -E "error|FAILED" | head
  cp -f /tmp/c_cut/bore2.elf $D/bore2.elf; cp -f /tmp/c_cut/bore2_lite.cue /tmp/c_cut/bore2_lite.bin $D/
fi
cd /home/claude/bore2 && DSPROF=$D/pc$R.txt DSPROFFROM=2200 DI=100000 bash test/ds.sh $D/bore2_lite.cue ab_$N$R $F ${SCRIPT:-$HERE/city.txt} 2>&1 | grep -E "GAMESTART"
python3 $HERE/ramprof.py /tmp/ds_ab_$N${R}_ram.bin $D/bore2.elf > $D/out$R.txt; grep -E "${GREP:-.}" $D/out$R.txt
/opt/mipsel/bin/mipsel-none-elf-nm $D/bore2.elf | grep -E " _end$"
[ -n "$TOP" ] && python3 $HERE/pcsym.py $D/bore2.elf $D/pc$R.txt $TOP
