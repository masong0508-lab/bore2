#!/bin/bash
# run.sh GAME TICKS EVERY [players]: bot-plays one game, then tiles the shots (with their texts) into /tmp/ah/GAME.png
mkdir -p /tmp/ah/o; rm -f /tmp/ah/o/$1_*
/tmp/ah/arcsim $1 $2 $3 /tmp/ah/o/$1 $4 2>&1 | tail -${LINES_MAX:-12}
python3 - "$1" <<'P'
import sys,glob
from PIL import Image,ImageDraw,ImageFont
g=sys.argv[1]; fs=sorted(glob.glob('/tmp/ah/o/%s_*.ppm'%g))
fs=fs[:8] if len(fs)<=8 else [fs[i*(len(fs)-1)//7] for i in range(8)]
ft=ImageFont.load_default(); tiles=[]
for f in fs:
    im=Image.open(f).convert('RGB'); d=ImageDraw.Draw(im)
    for l in open(f[:-4]+'.txt'):
        p=l.rstrip('\n').split(' ',4); x,y,sz,c,s=int(p[0]),int(p[1]),int(p[2]),int(p[3]),p[4]
        d.text((x,y),s,fill=(c&255,(c>>8)&255,(c>>16)&255),font=ft)
    tiles.append(im)
if tiles:
    W=min(4,len(tiles)); H=(len(tiles)+W-1)//W; c=Image.new('RGB',(320*W,240*H))
    for i,t in enumerate(tiles): c.paste(t,(320*(i%W),240*(i//W)))
    c.save('/tmp/ah/%s.png'%g)
P
