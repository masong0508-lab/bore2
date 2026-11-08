#!/bin/bash
# DS stable townies: A writes a save, B reads it back (same ids), C old-save ids (legacy), D another seed, A again (same as A)
ROM=${1:-/tmp/ds75t/bore.nds}; ELF=${2:-/tmp/ds75t/bore.elf}; D=$(dirname $ROM)
sym() { llvm-nm "$ELF" | grep " $1\$" | cut -d' ' -f1; }
M=$(sym test_sres),$(sym town_sig)
L="100-103:0:212,140;200-203:1:-1,-1"
rm -f $D/n.sav
run() { BORE_SAV=$D/n.sav DBGMEM=$M /tmp/dsrun $ROM 700 $D/o.ppm "$L;385-388:$1:-1,-1" 2>&1 | tail -1; }
echo "A write (X):        $(run 400)"
echo "B reload (Y):       $(run 800)"
echo "C old save (X+Y):   $(run c00)"
echo "D other seed (L):   $(run 200)"
echo "A again (X):        $(run 400)"
