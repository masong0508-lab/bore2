#!/bin/bash
# film_to_mp4.sh [OUT.mp4]: the opening film "TV SHOW & TELL" (CUT.OVL) as an MP4 (H.264 + AAC), 640x480 (the emulator's full 320x240 picture, 2x nearest
# neighbour), 60 fps, picture and sound as the PS1 test emulator makes them. Also writes the storyboard (one still per shot, film_storyboard.png, film_storyboard.txt).
#   WORK=dir      scratch (default /tmp/film_work; ~100 MB of frames, kept unless CLEAN=1)
#   DISC=x.cue    a disc built with -DAUTO_CUT=1 (default: built here in $WORK/build)
#   FRAMES=5700   emulator frames to run (the film is at about 1568..5220)
#   SB=dir        where the storyboard goes (default: a film_storyboard/ folder beside the mp4)
# How: DuckStation regtest dumps EVERY emulator frame (-dumpinterval 1, FULL=1: the whole 320x240) and the SPU's output (DSWAV). One emulator frame is one 60 Hz
# film step (the film's clock counts vblanks: a slow frame repeats a picture, never moves the sound), so the frames ARE the film's clock. tools/film_verify.py finds
# every voice line in the dump by matched filtering; the sound is moved by what it finds (the picture trails the game's clock by about 7 frames) so voice and subtitle meet.
set -e
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=$(cd "$HERE/.." && pwd)
OUT=${1:-/tmp/BORE2_missy_film.mp4}; WORK=${WORK:-/tmp/film_work}; FRAMES=${FRAMES:-5700}; SB=${SB:-$(dirname "$OUT")/film_storyboard}
FPS=53693175/897619            # the PS1's NTSC 240-line frame: 263 lines of 3413 clocks = 59.817 Hz (the emulator's frame rate; the SPU's 44100 Hz is locked to it)
mkdir -p "$WORK/dsr" "$WORK/dsd"
if [ -z "$DISC" ]; then
  [ -f "$WORK/build/build.ninja" ] || cmake -S "$ROOT" -B "$WORK/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake -DPSN00BSDK_TC=/opt/psn00bsdk -DCMAKE_C_FLAGS=-DAUTO_CUT=1 >/dev/null
  ninja -C "$WORK/build" >/dev/null; DISC=$WORK/build/bore2.cue
fi
rm -rf "$WORK/dsd/film"
( cd "$ROOT" && FULL=1 DI=1 DSD=$WORK/dsd DSR=$WORK/dsr DSL=$WORK DSWAV=$WORK/film.wav bash test/ds.sh "$DISC" film $FRAMES >/dev/null )
read FIRST LAST < <(python3 "$HERE/film_frames.py" "$WORK/dsd/film")
N=$((LAST - FIRST + 1)); echo "film: emulator frames $FIRST..$LAST ($N)"
python3 "$HERE/film_verify.py" "$WORK/film.wav" $FIRST | tee "$WORK/verify.txt"
LAG=$(grep MEDIAN_MS "$WORK/verify.txt" | awk '{print $2}')
A0=$(python3 -c "print(max(0, $FIRST / ($FPS) + $LAG / 1000.0))"); DUR=$(python3 -c "print($N / ($FPS))")
mkdir -p "$(dirname "$OUT")"
ffmpeg -y -loglevel error -framerate $FPS -start_number $FIRST -i "$WORK/dsd/film/frame_%05d.png" -ss $A0 -t $DUR -i "$WORK/film.wav" \
  -vf "scale=640:480:flags=neighbor,fps=60,format=yuv420p" -c:v libx264 -preset slow -crf 14 -tune animation -r 60 \
  -af "apad" -c:a aac -b:a 192k -t $DUR -movflags +faststart "$OUT"
python3 "$HERE/film_storyboard.py" "$WORK/dsd/film" $FIRST "$SB"
[ -n "$CLEAN" ] && rm -rf "$WORK/dsd/film"
ls -l "$OUT"; ffprobe -v error -show_entries format=duration -of csv=p=0 "$OUT"
