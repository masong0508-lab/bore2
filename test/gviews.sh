#!/bin/bash
# gviews.sh frames DI (a full sweep: 9600 120, then SKIP=8 for the loading frames; the game runs 30 frames a second): every BOREAGOTCHI screen in turn (cmake with -DAUTO_GOTCHI=0 -DAUTO_GVIEWS into $GV, default /tmp/c_gv: CMakeLists.txt builds the overlay with -Os for it so the hook links; forces the slot files and disc to rebuild);
# tiles (12 a sheet) in ${DSD:-/tmp/dsd}/${GVN}t*.png. GVN (default gv) names the run so helpers do not clash; FULL=1 shows the whole 320x240 frame.
cd "$(dirname "$0")/.."
GV=${GV:-/tmp/c_gv}; N=${GVN:-gv}; SKIP=${SKIP:-12}
rm -f $GV/slot_full/KINGDOM.OVL $GV/bore2.bin $GV/bore2.cue; ninja -C $GV 2>&1 | grep -E "error|FAILED"
DI=$2 bash test/ds.sh $GV/bore2.cue $N $1 /dev/null >/dev/null
N=$N SKIP=$SKIP DSD=${DSD:-/tmp/dsd} python3 - <<'P'
import glob, os
from PIL import Image
N=os.environ['N']; S=int(os.environ['SKIP']); D=os.environ['DSD']
fs=sorted(glob.glob('%s/%s/frame_*.png'%(D,N)))[S:]
print(len(fs))
for part in range(0,len(fs),12):
    sel=fs[part:part+12]
    c=Image.new('RGB',(320*4,240*3))
    for i,f in enumerate(sel): c.paste(Image.open(f).convert('RGB').resize((320,240)),(320*(i%4),240*(i//4)))
    c.save('%s/%st%d.png'%(D,N,part//12))
P
