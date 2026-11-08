#!/bin/bash
# gseq.sh TAG: shots of the BOREAGOTCHI! first run (title, shop card, unboxing, egg card, hatching, day), one tile per step from test/gseq_tab.h
# (a build with -DAUTO_GOTCHI=0 -DAUTO_GSEQ in $GV, default /tmp/c_gseq; the slot files and disc are forced to rebuild). Sheets of 12 (292x224 tiles, the real crop,
# labelled above) go to ${OUT:-docs}/${PFX:-gotchi_first}_TAG_N.png. TABF names another step table (default test/gseq_tab.h; the build needs -DAUTO_GEGG for gegg_tab.h):
# test/gegg_tab.h is every egg on the screens (test/gegg.sh), where the optional x and a columns pick the slot, the egg pattern and its age. DSD/DSR/DSL name the scratch folders (default /tmp/dsd...); GVN names the run.
cd "$(dirname "$0")/.."
GV=${GV:-/tmp/c_gseq}; N=${GVN:-gseq}; TAG=${1:-after}; OUT=${OUT:-docs}; TABF=${TABF:-test/gseq_tab.h}; PFX=${PFX:-gotchi_first}; export DSD=${DSD:-/tmp/dsd}
[ -n "$NOBUILD" ] || { rm -f $GV/slot_full/KINGDOM.OVL $GV/bore2.bin $GV/bore2.cue; ninja -C $GV 2>&1 | grep -E "error|FAILED"; }
NT=$(grep -c '^{' $TABF)
DI=6 bash test/ds.sh $GV/bore2.cue $N $(( ${FR:-1500} + NT * 20 )) /dev/null >/dev/null
N=$N TAG=$TAG OUT=$OUT TABF=$TABF PFX=$PFX python3 - <<'P'
import glob, os, re
from PIL import Image, ImageDraw
N=os.environ['N']; D=os.environ['DSD']; TAG=os.environ['TAG']; OUT=os.environ['OUT']; TABF=os.environ['TABF']; PFX=os.environ['PFX']
fs=sorted(glob.glob('%s/%s/frame_*.png'%(D,N)))
labs=[re.search(r'//\s*(.*)$',l).group(1) for l in open(TABF) if l.startswith('{')]
first={}
for f in fs:
    im=Image.open(f).convert('RGB')
    if im.size!=(292,224): continue
    p=im.getpixel((13,1)); k=int(p[0]/8+.5)+(int(p[1]/8+.5)<<5)-1
    if 3<=p[2]<=16 and k>=0 and k not in first and k<len(labs): first[k]=f
print(len(fs),'frames,',len(first),'of',len(labs),'steps')
sel=[first[k] for k in sorted(first)]; labs=[labs[k] for k in sorted(first)]
W,H=Image.open(sel[0]).size
for part in range(0,len(sel),12):
    c=Image.new('RGB',(W*4,(H+14)*3),(40,40,40)); d=ImageDraw.Draw(c)
    for i,f in enumerate(sel[part:part+12]):
        x=W*(i%4); y=(H+14)*(i//4)
        im=Image.open(f).convert('RGB'); im.paste(im.getpixel((8,8)),(9,0,18,5)); c.paste(im,(x,y+14)); d.text((x+4,y+1),labs[part+i],fill=(255,255,255))
    c.save('%s/%s_%s_%d.png'%(OUT,PFX,TAG,part//12+1))
P
# packet bytes each step used (a buffer is 59,392; the hook keeps the largest): ELF=$GV/GOTCHI.OVL.elf python3 test/ramsym.py $DSL/ds_${N}_ram.bin gseq_pk:30:h
