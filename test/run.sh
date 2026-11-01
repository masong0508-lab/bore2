#!/bin/bash
# run.sh <frames> <script> <prefix>: boots build/bore2.cue headless, screenshots per the script, converts to PNG
F=${1:-600}; S=${2:-title.txt}; P=${3:-shot}
rm -f /tmp/${P}_*.ppm /home/claude/bore2/test/${P}_*.png
timeout 600 /home/claude/tools/lrrun /home/claude/tools/pcsx_rearmed_libretro.so ${CUE:-/home/claude/bore2/build/bore2.cue} $F /tmp/$P $(pwd)/$S 2>/tmp/lr.log
cd /home/claude/bore2/test && for f in /tmp/${P}_*.ppm; do python3 -c "from PIL import Image; Image.open('$f').save('$f'.replace('/tmp/','/home/claude/bore2/test/').replace('.ppm','.png'))"; done
ls /home/claude/bore2/test/${P}_*.png
