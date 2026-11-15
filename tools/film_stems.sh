#!/bin/bash
# film_stems.sh [WORKDIR]: the film's sound as the test emulator's SPU makes it, the whole mix and each bus alone (stems): WORKDIR/mix.wav, voice.wav, music.wav, crowd.wav, foley.wav, cue.wav
# (44.1 kHz stereo, the same run each time, the same clock: the stems add up to the mix). A stem is a build with -DMIX_SOLO=n (src/audio.c: bit 0 voice 1 score 2 crowd 3 foley 4 shouts); the ducks, fades and
# panning are all still computed, only the other buses are turned off at the voices' volumes. AUTO_CUT=1 starts the film at the title screen; the run takes a few seconds, a build about 30 s.
# Then: python3 tools/film_mix_check.py WORKDIR/mix.wav --stems WORKDIR --png WORKDIR/png
set -e
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=$(cd "$HERE/.." && pwd)
W=${1:-/tmp/film_stems}; FRAMES=${FRAMES:-5700}; mkdir -p "$W"
B=$W/build
for s in "mix:31" "voice:1" "music:2" "crowd:4" "foley:8" "cue:16"; do
  name=${s%%:*}; mask=${s##*:}
  cmake -S "$ROOT" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake -DPSN00BSDK_TC=/opt/psn00bsdk "-DCMAKE_C_FLAGS=-DAUTO_CUT=1 -DMIX_SOLO=$mask" >/dev/null 2>&1
  ninja -C "$B" >/dev/null 2>&1
  ( cd "$ROOT" && DSD=$W/dsd DSR=$W/dsr DSL=$W DSWAV=$W/$name.wav bash test/ds.sh "$B/bore2.cue" film $FRAMES >/dev/null )
  echo "$name: $(ls -l "$W/$name.wav" | awk '{print $5}') bytes"
done
rm -rf "$B" "$W/dsd" "$W/dsr"
