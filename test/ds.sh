#!/bin/bash
# ds.sh <cue> <name> [frames] [script]: run a disc in DuckStation's regression runner (OpenBIOS).
# Script times count from the game's start; "shot" frames saved, then tiled to /tmp/dsd/<name>.png. RAM at end: /tmp/ds_<name>_ram.bin
C=$1; N=$2; F=${3:-900}; S=$4
mkdir -p /tmp/dsd /tmp/dsr; rm -rf /tmp/dsd/$N
cp -f $C /tmp/dsr/$N.cue; cp -f $(dirname $C)/$(grep -o '"[^"]*"' $C | tr -d '"') /tmp/dsr/$N.bin
sed -i "s/FILE \"[^\"]*\"/FILE \"$N.bin\"/" /tmp/dsr/$N.cue
[ -n "$FULL" ] && export DSCROP=None DSRAW=1   # FULL=1: the whole 320x240 frame (the default crop is 292x224: 8 rows lost top and bottom)
[ -n "$S" ] && export DSSCRIPT=$(realpath $S)
DSRAM=/tmp/ds_${N}_ram.bin timeout 900 /home/claude/tools/ds/b/bin/duckstation-regtest -log info -frames $F -dumpdir /tmp/dsd -dumpinterval ${DI:-100000} /tmp/dsr/$N.cue 2>&1 | head -c 30000000 > /tmp/ds_$N.log
python3 -c "
import glob
from PIL import Image
fs=sorted(glob.glob('/tmp/dsd/$N/frame_*.png'))
if fs:
  c=Image.new('RGB',(320*len(fs),240))
  for i,f in enumerate(fs): c.paste(Image.open(f).convert('RGB').resize((320,240)),(320*i,0))
  c.save('/tmp/dsd/$N.png')
print(len(fs),'frames')"
grep -a "GAMESTART" /tmp/ds_$N.log; grep -a PCLOG /tmp/ds_$N.log | tail -2
