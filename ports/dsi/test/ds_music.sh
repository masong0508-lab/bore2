#!/bin/bash
# D77: the DS music regression test (melonDS, tools/dsemu/run.cpp built with SPULOG/DBG7: the sound channel registers, change by change, and AUDOUT).
# It was written for the d73-d76 bug: those ROMs shipped with an old ARM7 (no song player) so sample music was silent. Checks, on the real ROM:
#   - the ARM7 answers the 'MUS1' handshake (else the ARM9 falls back to the synth tunes: never silence)
#   - music on the title (sample mode), in the city (after entering the game), and after 6 song switches (o_song poked at frames 150..900)
#   - at each switch every sample voice goes off at once, music returns, no loud burst, no voice held longer than the 6 s guard
#   - synth mode (o_msrc = 1): audio is heard, no sample voice is used
# usage: OUT=/tmp/ds77 tools/dsbuild/mkp.sh; bash ports/dsi/test/ds_music.sh [rom] [elf] [dsrun]      (the dsrun needs SPULOG: relink with the g++ line in tools/dsbuild/README.md)
ROM=${1:-/tmp/ds77/bore.nds}; ELF=${2:-${ROM%.nds}.elf}; RUN=${3:-/tmp/dsrun}; D=$(dirname "$ROM"); H=$(dirname "$0")
sym() { llvm-nm "$ELF" | grep " $1\$" | cut -d' ' -f1; }
OS=$(sym o_song); OM=$(sym o_msrc); MH=$(sym mus_hello)
[ -z "$OS" ] && { echo "no o_song symbol in $ELF"; exit 2; }
CH=0,1,2,3,4,5,6,7,14,15
GO="100-103:0:212,140;200-203:1:-1,-1;300-303:1:-1,-1;400-403:1:-1,-1"
SW=150,300,450,600,750,900
P=""; n=5; for f in ${SW//,/ }; do P="$P,$f:$OS:$(printf %x $n)"; n=$((n+7)); done; P=${P#,}
rm -f $D/m.sav
BORE_SAV=$D/m.sav POKE=$P DBGMEM=$MH SPULOG=1 AUDOUT=$D/m_s.wav $RUN $ROM 1100 $D/m.ppm "$GO" > $D/m_s.log 2>&1
python3 -I $H/ds_music_check.py $D/m_s.log $D/m_s.wav $SW $CH sample 1100 || FAIL=1
rm -f $D/m.sav
BORE_SAV=$D/m.sav POKE=20:$OM:1 DBGMEM=$MH SPULOG=1 AUDOUT=$D/m_y.wav $RUN $ROM 400 $D/m.ppm "" > $D/m_y.log 2>&1
python3 -I $H/ds_music_check.py $D/m_y.log $D/m_y.wav "" $CH synth 400 || FAIL=1
exit ${FAIL:-0}
