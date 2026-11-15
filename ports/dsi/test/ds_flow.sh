#!/bin/bash
# ds_flow.sh : D82 scripted-input flow tests in melonDS (stylus and keys through the real screens; needs a dsrun and any mkp.sh build with its bore.elf next to bore.nds).
#   usage: OUT=/tmp/ds82f tools/dsbuild/mkp.sh; DSRUN=/tmp/dsrun bash ports/dsi/test/ds_flow.sh [builddir]
# 1. BUY + SAVE + LOAD: Live, menu, Buy, the Chair tile, Place (funds 3500 -> 3410, one more thing), back to Live, menu > Save ("Game saved"), menu > Title, Continue, the Map, Live:
#    the funds and the things are what they were saved with.
# 2. JOB: menu > Job, pick the first job (hired), the menu again, Go to work: "Good shift", paid ($80), the shift took six hours of the clock.
# Prints the numbers and PASS / FAIL.
D=${1:-/tmp/ds82f}; RUN=${DSRUN:-/tmp/dsrun}; ELF=$D/bore.elf
sym() { llvm-nm "$ELF" | grep " $1\$" | cut -d' ' -f1; }
M="$(sym funds),$(sym nobj),$(sym minutes)"
last() { DBGMEM=$M BORE_SAV=$D/flow.sav $RUN $D/bore.nds $1 $D/flow.ppm "$2" 2>&1 | grep "^f" | tail -1; }
LIVE="100-103:0:212,140;200-203:1:-1,-1"; MENU="128,185"
fail=0
# 1
rm -f $D/flow.sav
S1="$LIVE;400-403:0:$MENU;440-443:0:43,75;500-503:0:115,88;540-542:1:-1,-1;600-603:0:88,8;640-643:0:$MENU;680-683:0:215,75"
a=$(last 760 "$S1"); echo "after buy and save: $a"
S2="$S1;760-763:0:$MENU;800-803:0:99,111;880-883:0:212,140;960-963:1:-1,-1"
b=$(last 1100 "$S2"); echo "after title > continue: $b"
set -- ${a#*:}; fa=$1; na=$2; set -- ${b#*:}; fb=$1; nb=$2
if [ "$fa" = 3410 ] && [ "$fb" = 3410 ] && [ "$na" = "$nb" ] && [ "$na" -gt 0 ]; then echo "FLOW buy/save/load PASS (funds $fb, things $nb)"; else echo "FLOW buy/save/load FAIL"; fail=1; fi
# 2
rm -f $D/flow.sav
S3="$LIVE;400-403:0:$MENU;440-443:0:157,111;480-483:0:70,70;540-543:0:$MENU;580-583:0:157,111;620-623:0:70,122"
c=$(last 700 "$S3"); echo "after job: $c"
set -- ${c#*:}; fc=$1; mc=$3
if [ "$fc" = 3580 ] && [ "$mc" -gt 1000 ]; then echo "FLOW job PASS (paid $((fc - 3500)), clock $mc)"; else echo "FLOW job FAIL"; fail=1; fi
exit $fail
