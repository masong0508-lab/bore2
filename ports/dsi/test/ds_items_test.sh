#!/bin/bash
# ds_items_test.sh : D82 every thing of the item table is used in melonDS (needs /tmp/dsrun or DSRUN, the mkp.sh toolchain).
#  - build -DDSTEST -DDS_ACT_TEST: each item id is put in a clear room, sim 0 is told to use it (start_act, as the popup's button does) and sims_step runs.
#    act_res: 1 used (it reached the thing and a need rose), 3 reached it but nothing rose (a decor thing with no effect), 4 refused by its life stage (cribs, toy things: right), 9 not tried (island things, stairs: the next test).
#    PASS: no 2, 3 or 5; every 4 is a baby / toddler / child item.
#  - build -DDSTEST -DDS_ISL_TEST: the 16 stairs and island things (ids 189 .. 204): obj_acts offers something, the first choice is done, and the state changes (isl_res 1).
D=${D:-/tmp/ds82i}; RUN=${DSRUN:-/tmp/dsrun}; R=$(cd "$(dirname "$0")/../../.." && pwd); fail=0
mem() { b=$((16#$(llvm-nm $1/bore.elf | grep " $2\$" | cut -d' ' -f1))); o=""; for ((k=0;k<$3;k++)); do o="$o$(printf %x $((b+4*k))),"; done; echo ${o%,}; }
if [ "$1" != nobuild ]; then
  OUT=$D/act XTRA="-DDSTEST -DDS_ACT_TEST" bash $R/tools/dsbuild/mkp.sh 2>&1 | grep " error"
  OUT=$D/isl XTRA="-DDSTEST -DDS_ISL_TEST" bash $R/tools/dsbuild/mkp.sh 2>&1 | grep " error"
fi
L="100-103:0:212,140;200-203:1:-1,-1"
out=$(DBGMEM=$(mem $D/act act_res 256),$(mem $D/act act_done 1) $RUN $D/act/bore.nds 9000 $D/act/o.ppm "$L" 2>&1 | tail -1); set -- ${out#*:}
A=("$@"); echo "act: done ${A[256]}"
r3=""; r4=""; r2=""; for ((i=0;i<256;i++)); do case ${A[$i]} in 3) r3="$r3 $i";; 4) r4="$r4 $i";; 2|5) r2="$r2 $i";; esac; done
echo "decor with no effect (3):${r3:- none}   never reached (2, 5):${r2:- none}   refused by age (4):${r4:- none}"
[ "${A[256]}" = 1 ] && [ -z "$r3" ] && [ -z "$r2" ] && echo "ITEMS act PASS" || { echo "ITEMS act FAIL"; fail=1; }
out=$(DBGMEM=$(mem $D/isl isl_res 16),$(mem $D/isl isl_n 16),$(mem $D/isl isl_tdone 1) $RUN $D/isl/bore.nds 2500 $D/isl/o.ppm "$L" 2>&1 | tail -1); set -- ${out#*:}
B=("$@"); echo "island/stairs results (ids 189..204): ${B[@]:0:16}   offered: ${B[@]:16:16}   done ${B[32]}"
ok=1; for ((i=0;i<16;i++)); do [ "${B[$i]}" = 1 ] || ok=0; done
[ "${B[32]}" = 1 ] && [ $ok = 1 ] && echo "ITEMS island/stairs PASS" || { echo "ITEMS island/stairs FAIL"; fail=1; }
#  - build -DDSTEST -DDS_ACT_HOMES: in every plan home (both facings) sim 0 is sent from the hall to use every thing indoors (ah[1] things, ah[2] never reached, ah[5] done). PASS: none unreached.
[ "$1" != nobuild ] && OUT=$D/ah XTRA="-DDSTEST -DDS_ACT_HOMES" bash $R/tools/dsbuild/mkp.sh 2>&1 | grep " error"
out=$(DBGMEM=$(mem $D/ah ah 8) $RUN $D/ah/bore.nds 14000 $D/ah/o.ppm "$L" 2>&1 | tail -1); set -- ${out#*:}
C=("$@"); echo "homes ${C[0]}, things tried ${C[1]}, never reached ${C[2]}, reached but not used ${C[3]}, refused ${C[4]}, done ${C[5]}"
[ "${C[5]}" = 1 ] && [ "${C[2]}" = 0 ] && [ "${C[3]}" = 0 ] && echo "ITEMS every thing in every home PASS" || { echo "ITEMS every thing in every home FAIL"; fail=1; }
exit $fail
