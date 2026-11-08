#!/bin/bash
# oldsave2.sh BUILD "VER COMMIT" ...: like oldsave.sh, for any saved versions (see test/oldhook2.py: more values are set and checked).
# Each old commit gets a worktree in /tmp/oldsave_w<VER>, a test build (AUTO_CITY + AUTO_OLD) that makes a life and saves it to slot 1 of a memory card
# file (DSCARD), then BUILD (a plain Release build directory of today's game, with bore2_lite.cue) boots on that card and CONTINUE loads slot 1.
# The old build and worktree are deleted after (disk). The cards stay in /tmp/oldsave_card<VER>.mcd: delete one to make that save again.
# Commits: v41 578dc62, v42 2e86539, v43 bcd8579, v44 b889bbe, v45 3c2d4f1, v46 06393a1.
HERE=$(cd "$(dirname "$0")" && pwd); B=$(realpath "$1"); shift
for p in "$@"; do set -- $p; V=$1; C=$2; W=/tmp/oldsave_w$V; O=/tmp/oldsave_b$V; CARD=/tmp/oldsave_card$V.mcd
  if [ ! -f $CARD ]; then
    [ -d $W ] || { git -C "$HERE/.." worktree add -f --detach $W $C > /dev/null && python3 "$HERE/oldhook2.py" $W/src/main.c $V; }
    cmake -S $W -B $O -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake \
          -DPSN00BSDK_TC=/opt/psn00bsdk "-DCMAKE_C_FLAGS=-DAUTO_CITY=1 -DAUTO_OLD=1" > /dev/null 2>&1
    ninja -C $O iso_lite 2>&1 | grep -E "error|FAILED" | head -5
    [ -f $O/bore2_lite.cue ] || { echo "version $V: build failed"; continue; }
    echo "2500 shot" > /tmp/oldsave_s.txt; DSCARD=$CARD bash "$HERE/ds.sh" $O/bore2_lite.cue oldsave_m$V 2600 /tmp/oldsave_s.txt > /dev/null
    echo "== version $V saved:"; ELF=$O/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_m${V}_ram.bin dbg_old:16 opt:46:B
    rm -rf $O; git -C "$HERE/.." worktree remove --force $W
  fi
  DSCARD=$CARD bash "$HERE/ds.sh" "$B/bore2_lite.cue" oldsave_l$V 2900 "$HERE/oldsave_load.txt" > /dev/null
  echo "== version $V loaded (the screens: /tmp/dsd/oldsave_l$V.png):"
  ELF=$B/bore2.elf python3 "$HERE/ramsym.py" /tmp/ds_oldsave_l${V}_ram.bin save_ver cash home_city home_bx home_by nprops nsims fm_hens:1:B fm_n:4:B fm_stock:4:H job_level opt:46:B arcade_tix arc+24:1:I arc+64:1:I arc+176:1:H gtoy:2:B gtoy+4:1:H
done
