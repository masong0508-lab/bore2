#!/bin/bash
# ab.sh <cue> <frames> <script> <prefix>: like run.sh but for any disc (A/B against an older build); log in /tmp/lr_<prefix>.log
C=$1; F=${2:-600}; S=${3:-title.txt}; P=${4:-ab}
rm -f /tmp/${P}_*.ppm /home/claude/bore2/test/${P}_*.png
timeout 900 /home/claude/tools/lrrun /home/claude/tools/pcsx_rearmed_libretro.so $C $F /tmp/$P $(pwd)/$S 2>/tmp/lr_$P.log
cd /home/claude/bore2/test && for f in /tmp/${P}_*.ppm; do python3 -c "from PIL import Image; Image.open('$f').save('$f'.replace('/tmp/','/home/claude/bore2/test/').replace('.ppm','.png'))"; done
