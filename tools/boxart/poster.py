import sys, random
from bore import *
from PIL import ImageFont, ImageFilter, ImageChops
SS=3; W,H=1100,1560
CREAM=(244,236,214); INK=(24,14,28)
def jag(poly,step=34,amp=7,seed=1):
    rnd=random.Random(seed); out=[]; n=len(poly)
    for i in range(n):
        (x0,y0),(x1,y1)=poly[i],poly[(i+1)%n]; L=math.hypot(x1-x0,y1-y0); k=max(2,int(L/step)); nx,ny=-(y1-y0)/L,(x1-x0)/L
        for j in range(k):
            t=j/k; off=amp*(1 if j%2 else -1)*(0.75+0.5*rnd.random()) if j else 0
            out.append((x0+(x1-x0)*t+nx*off,y0+(y1-y0)*t+ny*off))
    return out
def top(x): return 1031-30*(x-20)/1060
P={
 'p1':[(20,20),(430,20),(380,550),(20,550)],
 'p2':[(446,20),(720,20),(760,550),(396,550)],
 'p3':[(736,20),(1080,20),(1080,550),(776,550)],
 'logo':[(20,600),(1080,568),(1080,985),(20,1017)],
 'q1':[(20,top(20)),(330,top(330)),(380,1540),(20,1540)],
 'q2':[(346,top(346)),(760,top(760)),(720,1540),(396,1540)],
 'q3':[(776,top(776)),(1080,top(1080)),(1080,1540),(736,1540)],
}
COL={'p1':'#3f7fbf','p2':'#e3ac2f','p3':'#c4563f','logo':'#2a1b3b','q1':'#58ad84','q2':'#8d5fab','q3':'#e27ba3'}
im=Image.new('RGB',(W*SS,H*SS),CREAM); d=ImageDraw.Draw(im)
def S2(p): return [(x*SS,y*SS) for x,y in p]
for k,poly in P.items():
    jp=S2(jag(poly,seed=hash(k)%97)); d.polygon(jp,fill=hexc(COL[k])); d.line(jp+[jp[0]],fill=INK,width=9*SS,joint='curve')
def shade(c,f): return tuple(int(v*f) for v in hexc(c))
def shadow(cx,cy,rw,rh,k):
    d.ellipse([(cx-rw)*SS,(cy-rh)*SS,(cx+rw)*SS,(cy+rh)*SS],fill=shade(COL[k],0.72))
def render(sc,ox,oy,s): sc.render(d,ox*SS,oy*SS,s*SS)
from scenes import *
scenes=[('p1',toasters,(225,525,84)),('p2',vacuum,(575,525,88)),('p3',duck,(930,525,68)),
        ('q1',fridge,(165,1520,66)),('q2',speech,(570,1520,98)),('q3',smoking,(915,1500,92))]
def inset(poly,m=3):
    cx=sum(x for x,_ in poly)/len(poly); cy=sum(y for _,y in poly)/len(poly)
    return [(x+(cx-x)*0.0,y) for x,y in poly]
SMOKE=[(968,1272,9,200),(979,1242,12,190),(962,1208,16,175),(982,1168,20,150),(960,1122,25,125),(986,1072,30,95)]
for k,fn,(ox,oy,s) in scenes:
    layer=Image.new('RGBA',im.size,(0,0,0,0)); ld=ImageDraw.Draw(layer)
    ld.ellipse([(ox-150)*SS,(oy-20)*SS,(ox+150)*SS,(oy+12)*SS],fill=shade(COL[k],0.72)+(255,))
    sc=fn(); sc.render(ld,ox*SS,(oy-8)*SS,s*SS,ow=3*SS//2+1)
    if k=='q3':
        for x,y,r,a_ in SMOKE: ld.ellipse([(x-r)*SS,(y-r)*SS,(x+r)*SS,(y+r)*SS],fill=(238,232,244,a_),outline=(150,140,165,a_//2),width=2)
    mask=Image.new('L',im.size,0); ImageDraw.Draw(mask).polygon(S2(P[k]),fill=255)
    layer.putalpha(ImageChops.multiply(layer.getchannel('A'),mask)); im.paste(layer,(0,0),layer)
# re-stroke the jagged outlines over the scenes so nothing spills
for k,poly in P.items():
    jp=S2(jag(poly,seed=hash(k)%97)); d.line(jp+[jp[0]],fill=INK,width=9*SS,joint='curve')
# ---- logo panel
gold=Image.open('/mnt/user-data/attachments/2483a6af-7ed4-56bf-9a29-66f3f6dd76d4/dcaf7cf3-image.png').convert('RGBA')
U=6   # one pixel of the logo art = 6 poster px
big=Image.new('RGBA',(W//U+2,H//U+2),(0,0,0,0)); bd=ImageDraw.Draw(big)
fnt=ImageFont.truetype('/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf',108)
gx,gy=int(700/U),int(585/U)
bd.text((gx,gy),"2",font=fnt,fill=(255,255,255,255))
a=big.getchannel('A').point(lambda v:255 if v>110 else 0)
bb=a.getbbox(); a=a.crop(bb); gw,gh=a.size
body=Image.new('RGBA',a.size,(0,0,0,0)); px=body.load(); ap=a.load()
for y in range(gh):
    t=y/max(1,gh-1); c=(int(255-60*t),int(92-70*t),int(139-40*t))
    for x in range(gw):
        if ap[x,y]:
            hl=1 if (x<=2 and y>3) or y<2 else 0
            px[x,y]=tuple(min(255,v+50) for v in c)+(255,) if hl else c+(255,)
outl=a.filter(ImageFilter.MaxFilter(5)); ol=Image.new('RGBA',a.size,(0,0,0,0))
pad=Image.new('RGBA',(gw+8,gh+8),(0,0,0,0)); 
o2=Image.new('L',(gw+8,gh+8),0); o2.paste(a,(4,4)); o2=o2.filter(ImageFilter.MaxFilter(5)); sh=Image.new('L',(gw+8,gh+8),0); sh.paste(o2,(2,2))
pad.paste((22,10,30,255),(0,0),sh); pad.paste((22,10,30,255),(0,0),o2); pad.paste(body,(4,4),body)
twoW=pad.resize((pad.width*U,pad.height*U),Image.NEAREST)
tx=int(640); ty=int(770-twoW.height/2)
layer=Image.new('RGBA',im.size,(0,0,0,0)); layer.paste(twoW.resize((twoW.width*SS//1,twoW.height*SS//1),Image.NEAREST),(tx*SS,ty*SS),twoW.resize((twoW.width*SS,twoW.height*SS),Image.NEAREST))
lw=840; lh=int(gold.height*lw/gold.width); logo=gold.resize((lw*SS,lh*SS),Image.NEAREST)
layer.paste(logo,(int(36*SS),int((782-lh/2)*SS)),logo)
mask=Image.new('L',im.size,0); ImageDraw.Draw(mask).polygon(S2(P['logo']),fill=255)
layer.putalpha(ImageChops.multiply(layer.getchannel('A'),mask)); im.paste(layer,(0,0),layer)
jp=S2(jag(P['logo'],seed=hash('logo')%97)); d.line(jp+[jp[0]],fill=INK,width=9*SS,joint='curve')
tf=ImageFont.truetype('/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf',34*SS)
d.text((60*SS,905*SS),"BOREDOM. SIMULATED.",font=tf,fill=(244,236,214))
tf2=ImageFont.truetype('/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf',22*SS)
d.text((62*SS,950*SS),"A WHOLE CITY OF BORES  -  PLAYSTATION",font=tf2,fill=(255,92,139))
im=im.resize((W*2,H*2),Image.LANCZOS)
im.save('/tmp/art/stage1.png')
print('ok')
