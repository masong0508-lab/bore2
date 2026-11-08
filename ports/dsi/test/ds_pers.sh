#!/bin/bash
# D76: a townie's personality comes from the world seed and their unique id, not their name.
# Host part: ds_traits_test.c (persona checks). melonDS part (needs /tmp/dsrun and a build with -DDSTEST -DAUTO_SCENE=155, keys: X 156 (the player's traits page), Y 157 (a housemate's wish page), X+Y 158 (with wishes done), L 159, L+X 160, L+Y 161):
#   159 (L): 24 townies with ONE name (ids 5 + 96 z): pers_dist personalities, pers_dup same-name count, pers_sig checksum
#   160 (L+X): a townie moves in: test_sres 701 (the Sim's bars equal the town card's), pers_sig, save written
#   161 (L+Y): the save read back (test_sres 800 + result): pers_sig is the same
#   145/146 are the older D75 scenes (AUTO_SCENE=144): see ds_town.sh
# usage: OUT=/tmp/ds76t XTRA="-DDSTEST -DAUTO_SCENE=155" bash tools/dsbuild/mkp.sh; bash ports/dsi/test/ds_pers.sh
ROM=${1:-/tmp/ds76t/bore.nds}; ELF=${2:-/tmp/ds76t/bore.elf}; D=$(dirname $ROM)
sym() { llvm-nm "$ELF" | grep " $1\$" | cut -d' ' -f1; }
M=$(sym test_sres),$(sym pers_sig),$(sym pers_dist),$(sym pers_dup)
L="100-103:0:212,140;200-203:1:-1,-1"
rm -f $D/n.sav
run() { BORE_SAV=$D/n.sav DBGMEM=$M /tmp/dsrun $ROM 700 $D/o.ppm "$L;385-388:$1:-1,-1" 2>&1 | tail -1; }
a=$(run 200); echo "24 same-name townies (L): $a"
b=$(run 600); echo "move in + write (L+X):    $b"
c=$(run a00); echo "reload (L+Y):             $c"
d=$(run 600); echo "move in again (L+X):    $d"
echo "$a" | grep -q " 24 " || echo "NOTE: distinct count not 24 in line 1"
# the 2nd and 3rd lines: pers_sig is printed (third value); they must be equal
pb=$(echo "$b" | awk '{print $(NF-2)}'); pc=$(echo "$c" | awk '{print $(NF-2)}')
echo "$b" | grep -q ": 701 " && echo "MOVE-IN MATCHES CARD" || echo "MOVE-IN DIFFERS FROM CARD"
[ "$pb" = "$pc" ] && echo "PERSONALITY STABLE ACROSS SAVE/RELOAD" || echo "RELOAD DIFFERS ($pb vs $pc)"
