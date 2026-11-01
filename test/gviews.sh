#!/bin/bash
# gviews.sh frames DI: every BOREAGOTCHI screen in turn (cmake with -DAUTO_GOTCHI=0 -DAUTO_GVIEWS into /tmp/c_gv; forces the slot files and disc to rebuild); tiles in /tmp/dsd/gvt*.png
cd /home/claude/bore2
rm -f /tmp/c_gv/slot_full/KINGDOM.OVL /tmp/c_gv/bore2.bin /tmp/c_gv/bore2.cue; ninja -C /tmp/c_gv 2>&1 | grep -E "error|FAILED"
DI=$2 bash test/ds.sh /tmp/c_gv/bore2.cue gv $1 /dev/null >/dev/null
python3 - <<'P'
import glob
from PIL import Image
fs=sorted(glob.glob('/tmp/dsd/gv/frame_*.png'))[12:]
print(len(fs))
for part in range(0,len(fs),12):
    sel=fs[part:part+12]
    c=Image.new('RGB',(320*4,240*3))
    for i,f in enumerate(sel): c.paste(Image.open(f).convert('RGB').resize((320,240)),(320*(i%4),240*(i//4)))
    c.save('/tmp/dsd/gvt%d.png'%(part//12))
P
