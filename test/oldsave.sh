#!/bin/bash
# oldsave.sh BUILD: old saves still load in today's game. For each old commit (save version 40: 9a7e273, version 41: ad56fbc, the last before
# the properties) a worktree in /tmp gets test/oldhook.py's AUTO_OLD hook and is built and run on a memory card kept in a file (DSCARD: the
# regtest runner patched with tools/perf/regtest_card_wav.patch): it makes a life and saves it in slot 1. Then BUILD (a plain Release build
# directory of today's game, with bore2_lite.cue) boots on that card and CONTINUE loads slot 1 with the pad (oldsave_load.txt).
# Printed: what was saved, and what came back (version, money, home, properties, family, farm, job level, the options, the gnome on the home's lot).
# The cards are kept (/tmp/oldsave_card*.mcd): delete one to make that save again.
HERE=$(cd "$(dirname "$0")" && pwd); B=$(realpath "$1")
for p in "40 9a7e273" "41 ad56fbc"; do set -- $p; V=$1; C=$2; W=/tmp/oldsave_w$V; O=/tmp/oldsave_b$V; CARD=/tmp/oldsave_card$V.mcd
  if [ ! -f $CARD ]; then
    [ -d $W ] || { git -C "$HERE/.." worktree add -f --detach $W $C > /dev/null && python3 "$HERE/oldhook.py" $W/src/main.c $V; }
    [ -f $O/build.ninja ] || cmake -S $W -B $O -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake \
                               -DPSN00BSDK_TC=/opt/psn00bsdk "-DCMAKE_C_FLAGS=-DAUTO_CITY=1 -DAUTO_OLD=1" > /dev/null 2>&1
    ninja -C $O iso_lite > /dev/null || exit 1
    echo "2500 shot" > /tmp/oldsave_s.txt; DSCARD=$CARD bash "$HERE/ds.sh" $O/bore2_lite.cue oldsave_m$V 2600 /tmp/oldsave_s.txt > /dev/null
    echo "== version $V saved:"; ELF=$O/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_m${V}_ram.bin dbg_old:12 opt:46:B
  fi
  DSCARD=$CARD bash "$HERE/ds.sh" "$B/bore2_lite.cue" oldsave_l$V 2900 "$HERE/oldsave_load.txt" > /dev/null
  echo "== version $V loaded (the screens: /tmp/dsd/oldsave_l$V.png):"
  ELF=$B/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_l${V}_ram.bin save_ver cash home_city home_bx home_by nprops nsims fm_hens:1:B fm_n:4:B fm_stock:4:H job_level opt:46:B
  W0=$(ELF=$B/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_l${V}_ram.bin world_wbx world_wby home_bx home_by | tr -d '[]' | awk '{ print $2 }' | tr '\n' ' ')
  set -- $W0; ELF=$B/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_l${V}_ram.bin lifeMap+$(( ((($4 - $2) * 18 + 11) * 81) + ($3 - $1) * 18 + 11 )):1:B | sed 's/$/  (78: the gnome is there)/'
done
