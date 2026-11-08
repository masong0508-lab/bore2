#!/bin/bash
# ds_traits_test.sh : D73 trait checks.
# 1) host: every supported hook and every offered trait moves a DS number (ds_traits_test.c).
# 2) emulator (needs /tmp/dsrun = tools/dsemu/run.cpp built, and mkp.sh's toolchain): a new save reloads the same.
#    Builds with -DDSTEST -DAUTO_SCENE=140: SELECT 140, X+Y 143 (gives traits, writes the save), L 144 (reads it back).
#    (An old d70/d71/d72 save: build that commit with -DAUTO_SCENE=70, X = scene 71 writes the save, then run 144 on the new ROM with the same BORE_SAV file: test_sres 401.)
R=$(cd "$(dirname "$0")/../../.." && pwd); T=${T:-/tmp/dstt}; mkdir -p "$T"
gcc -O1 -w -I"$R/src" -o "$T/dstt" "$R/ports/dsi/test/ds_traits_test.c" || exit 1
"$T/dstt" | tail -30
[ -x /tmp/dsrun ] || { echo "no /tmp/dsrun: host part only"; exit 0; }
OUT=$T/b XTRA="-DDSTEST -DAUTO_SCENE=140" bash "$R/tools/dsbuild/mkp.sh" >/dev/null 2>&1 || { echo "build failed"; exit 1; }
sym() { llvm-nm "$T/b/bore.elf" | grep " $1\$" | cut -d' ' -f1; }
L="100-103:0:212,140;200-203:1:-1,-1"
rm -f "$T/n.sav"
M=$(sym test_sres),$(sym trg_sig)
a=$(BORE_SAV=$T/n.sav DBGMEM=$M /tmp/dsrun "$T/b/bore.nds" 700 "$T/o.ppm" "$L;385-388:c00:-1,-1" 2>&1 | tail -1)
b=$(BORE_SAV=$T/n.sav DBGMEM=$M /tmp/dsrun "$T/b/bore.nds" 700 "$T/o.ppm" "$L;385-388:200:-1,-1" 2>&1 | tail -1)
echo "write: $a"
echo "read : $b"
[ "${a#*: 301 }" = "${b#*: 401 }" ] && echo "SAVE ROUNDTRIP PASS" || echo "SAVE ROUNDTRIP FAIL"
