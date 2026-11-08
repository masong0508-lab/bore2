#!/bin/bash
# D76: a d75 save loads in d76. Needs /tmp/dsrun and two builds with -DDSTEST -DAUTO_SCENE=140:
#   the d75 commit (R=<its checkout> OUT=/tmp/ds75t ...) and this one (OUT=/tmp/ds76q ...). Keys X+Y = scene 143 (gives traits, writes the save), L = 144 (reads it back).
O=${1:-/tmp/ds75t}; N=${2:-/tmp/ds76q}
sym() { llvm-nm "$N/bore.elf" | grep " $1\$" | cut -d' ' -f1; }
symo() { llvm-nm "$O/bore.elf" | grep " $1\$" | cut -d' ' -f1; }
L="100-103:0:212,140;200-203:1:-1,-1"
rm -f /tmp/ds76q/old.sav
a=$(BORE_SAV=/tmp/ds76q/old.sav DBGMEM=$(symo test_sres),$(symo trg_sig) /tmp/dsrun $O/bore.nds 700 /tmp/ds76q/o.ppm "$L;385-388:c00:-1,-1" 2>&1 | tail -1)
b=$(BORE_SAV=/tmp/ds76q/old.sav DBGMEM=$(sym test_sres),$(sym trg_sig) /tmp/dsrun $N/bore.nds 700 /tmp/ds76q/o.ppm "$L;385-388:200:-1,-1" 2>&1 | tail -1)
echo "d75 writes: $a"
echo "d76 reads : $b"
[ "${a#*: 301 }" = "${b#*: 401 }" ] && echo "D75 SAVE LOADS IN D76 (same trait checksum)" || echo "D75 SAVE FAIL"
