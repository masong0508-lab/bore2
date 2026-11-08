#!/bin/bash
# glyph_check.sh [WORKDIR]: the glyph chart on the PS1 (DuckStation) and the Dreamcast (flycast), compared cell by cell (test/glyph_check.py).
# Every printable ASCII character in every font style (BODY READ HEAD NUM FUN BIG) in every TEXT LOOK (SIMS READABLE CAPS), alone and after a Z, and a page of shipped strings.
# Builds (AUTO_GLYPHS=1) into WORKDIR (default /tmp/glyph): c_ps1 (cmake) and obj_dc (make). Needs the Dreamcast toolchain (ports/dreamcast/toolchain/restore.sh).
# Prints MISMATCH lines and exits 1 if the two platforms draw any cell differently; WORKDIR/diff_diff.png shows them. The result images: docs/glyph_check_*.png.
cd "$(dirname "$0")/.." || exit 2
ROOT=$PWD; W=${1:-/tmp/glyph}; mkdir -p $W
[ -f $W/c_ps1/build.ninja ] || cmake -G Ninja -S . -B $W/c_ps1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake -DCMAKE_C_FLAGS=-DAUTO_GLYPHS=1 > /dev/null || exit 2
ninja -C $W/c_ps1 2>&1 | grep -E "error|FAILED"
DSD=$W/dsd DSR=$W/dsr DSL=$W FULL=1 DI=10 bash test/ds.sh $W/c_ps1/bore2.cue ps1 6000 /dev/null > /dev/null   # (a page is 10 vblanks, about 50 emulator frames on the PS1)
( . /tmp/dc/environ.sh; cd ports/dreamcast && make -j2 O=$W/obj_dc AUTO=AUTO_GLYPHS=1 disc 2>&1 | grep -E "error|\*\*\*" )
rm -rf $W/dcs; mkdir -p $W/dcs $W/dcsys
python3 -I -c "print('\n'.join('%d shot' % f for f in range(40, 1900, 4)))" > $W/dcshots.txt
DCRUN_SYS=$W/dcsys timeout 900 /tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so $W/obj_dc/bore2_dc.gdi 1950 $W/dcs/s $W/dcshots.txt > $W/dc.log 2>&1
python3 -I test/glyph_check.py $W/dsd/ps1 $W/dcs $W/diff
R=$?
[ -n "$KEEP" ] || rm -rf $W/dcs $W/dsd $W/dsr    # (the frames are big; KEEP=1 keeps them for test/glyph_shots.py)
exit $R
