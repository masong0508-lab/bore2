#!/bin/bash
# ds.sh <cue> <name> [frames] [script]: run a disc in DuckStation's regression runner (OpenBIOS).
# Script times count from the game's start; "shot" frames saved, then tiled to $DSD/<name>.png. RAM at end: $DSL/ds_<name>_ram.bin
DSD=${DSD:-/tmp/dsd}; DSR=${DSR:-/tmp/dsr}; DSL=${DSL:-/tmp}
C=$1; N=$2; F=${3:-900}; S=$4
mkdir -p $DSD $DSR; rm -rf $DSD/$N
cp -f $C $DSR/$N.cue; cp -f $(dirname $C)/$(grep -o '"[^"]*"' $C | tr -d '"') $DSR/$N.bin
sed -i "s/FILE \"[^\"]*\"/FILE \"$N.bin\"/" $DSR/$N.cue
[ -n "$FULL" ] && export DSCROP=None DSRAW=1   # FULL=1: the whole 320x240 frame (the default crop is 292x224: 8 rows lost top and bottom)
[ -n "$S" ] && export DSSCRIPT=$(realpath $S)
DSRAM=$DSL/ds_${N}_ram.bin timeout 900 /home/claude/tools/ds/b/bin/duckstation-regtest -log info -frames $F -dumpdir $DSD -dumpinterval ${DI:-100000} $DSR/$N.cue 2>&1 | head -c 30000000 > $DSL/ds_$N.log
python3 -c "
import glob
from PIL import Image
fs=sorted(glob.glob('$DSD/$N/frame_*.png'))
if fs:
  c=Image.new('RGB',(320*len(fs),240))
  for i,f in enumerate(fs): c.paste(Image.open(f).convert('RGB').resize((320,240)),(320*i,0))
  c.save('$DSD/$N.png')
print(len(fs),'frames')"
grep -a "GAMESTART" $DSL/ds_$N.log; grep -a PCLOG $DSL/ds_$N.log | tail -2
