#!/usr/bin/env python3
# Painted, shaded models for the garden/pool/beach, farm and space things, plus the park bench (writes src/items_home.inc, items_farm.inc,
# items_space.inc). Every face gets its own small painted texture (palette picked from the pixels), '.' is see-through.
import hashlib, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from item_fit import scale, shift, top as box_top   # (the size fixes: docs/OBJECT_SCALE.md)
def C(r,g,b): return (r,g,b)
def sh(c,k): return tuple(max(0,min(31,int(v*k+0.5))) for v in c)
WHITE=C(29,29,28); CREAM=C(30,27,20); RED=C(26,5,5); BLUE=C(7,13,26); YEL=C(31,26,6); ORG=C(30,16,4); GRN=C(7,21,7); LEAF=C(6,22,6)
WOOD=C(18,12,6); DWOOD=C(12,8,4); SOIL=C(11,7,4); GREY=C(15,15,17); DARK=C(5,4,8); PINK=C(30,14,22); CHROME=C(26,26,28); GLASS=C(17,25,29)
BRICK=C(22,9,6); STONE=C(19,19,18); WATER=C(10,20,30); SKIN=C(29,21,16); BLACK=C(2,2,3); SAND=C(29,25,16)
class Cv:
    def __init__(s,w,h,fill=None): s.w=w; s.h=h; s.p=[[fill]*w for _ in range(h)]
    def px(s,x,y,c):
        if 0<=x<s.w and 0<=y<s.h: s.p[y][x]=c
    def rect(s,x0,y0,x1,y1,c):
        for y in range(y0,y1):
            for x in range(x0,x1): s.px(x,y,c)
    def hl(s,y,x0,x1,c): s.rect(x0,y,x1,y+1,c)
    def vl(s,x,y0,y1,c): s.rect(x,y0,x+1,y1,c)
    def box(s,x0,y0,x1,y1,c,edge=None):   # filled with a light top/left and dark bottom/right edge
        s.rect(x0,y0,x1,y1,c)
        if edge!=False:
            s.hl(y0,x0,x1,sh(c,1.18)); s.vl(x0,y0,y1,sh(c,1.1)); s.hl(y1-1,x0,x1,sh(c,0.72)); s.vl(x1-1,y0,y1,sh(c,0.82))
    def disc(s,cx,cy,r,c):
        for y in range(s.h):
            for x in range(s.w):
                if (x+0.5-cx)**2+(y+0.5-cy)**2<=r*r: s.px(x,y,c)
    def fill_noise(s,c,k=0.1,seed=1,x0=0,y0=0,x1=None,y1=None):
        x1=x1 or s.w; y1=y1 or s.h; n=seed
        for y in range(y0,y1):
            for x in range(x0,x1):
                n=(n*1103515245+12345)&0x7fffffff; v=(n>>16)%3
                s.px(x,y,sh(c,1.0+k*(v-1)))
mats={}; order=[]; out_cur=None
def mat(cv):
    key=hashlib.md5(repr(cv.p).encode()).hexdigest()[:8]
    if key in mats: return mats[key][0]
    pal=[]
    for row in cv.p:
        for c in row:
            if c is not None and c not in pal: pal.append(c)
    if len(pal)>26:   # too many colours: merge the rarest into nearest
        raise SystemExit('too many colours %d' % len(pal))
    rows=[''.join('.' if c is None else chr(97+pal.index(c)) for c in row) for row in cv.p]
    nm='n_%s'%key
    mats[key]=(nm,pal,cv.w,cv.h,rows,False); order.append(key)
    return nm
def emit_mats(keys):
    o=[]
    for k in keys:
        nm,pal,w,h,rows,_=mats[k]
        o.append('static const u16 p_%s[%d]={%s};'%(nm,max(1,len(pal)),','.join('RGB(%d,%d,%d)'%c for c in pal) or '0'))
        o.append('MAT(%s,p_%s,%d,%d,%s)'%(nm,nm,w,h,','.join('"%s"'%r for r in rows)))
    return o
def solid(c,w=2,h=2):
    cv=Cv(w,h,c)
    for y in range(h):
        for x in range(w):
            if (x+y)&1: cv.px(x,y,sh(c,0.93))
    return mat(cv)
# ---- textures ----
def planks(w,h,c,vertical=False,seed=3):
    cv=Cv(w,h,c); n=seed
    for y in range(h):
        for x in range(w):
            n=(n*1103515245+12345)&0x7fffffff; cv.px(x,y,sh(c,[0.92,1.0,1.06][(n>>16)%3]))
    for i in range(0,(w if vertical else h),2):
        if vertical: cv.vl(i,0,h,sh(c,0.6))
        else: cv.hl(i,0,w,sh(c,0.6))
    return cv
def t_picket():
    cv=Cv(8,8,None)
    for x0 in (0,3,6):
        for y in range(1,8): cv.hl(y,x0,x0+2,WHITE); cv.px(x0+1,y,sh(WHITE,0.85))
        cv.px(x0,0,sh(WHITE,1.0)) if x0<6 else None
    cv.px(0,0,WHITE); cv.px(3,0,WHITE); cv.px(6,0,WHITE)
    cv.hl(2,0,8,sh(WHITE,0.7)); cv.hl(5,0,8,sh(WHITE,0.7))
    for x0 in (0,3,6): cv.px(x0+1,2,sh(WHITE,0.6)); cv.px(x0+1,5,sh(WHITE,0.6))
    return mat(cv)
def t_leaf(top=False):
    cv=Cv(8,8,sh(LEAF,0.55)); n=7
    for _ in range(26):
        n=(n*1103515245+12345)&0x7fffffff; x=(n>>10)%7; n=(n*1103515245+12345)&0x7fffffff; y=(n>>10)%7
        k=1.0 if top else (0.8+0.07*y/1.0)
        cv.rect(x,y,x+2,y+1,sh(LEAF,min(1.5,k*1.1))); cv.px(x+1,y,sh(LEAF,1.35 if top else 1.15)); cv.px(x,y+1,sh(LEAF,0.7))
    return mat(cv)
def t_bloom(c):
    cv=Cv(4,4,sh(c,0.8)); cv.rect(1,0,3,4,c); cv.rect(0,1,4,3,c); cv.rect(1,1,3,3,YEL if c!=YEL else ORG); cv.px(1,1,sh(c,1.2)); return mat(cv)
def t_stem():
    cv=Cv(2,4,GRN); cv.vl(0,0,4,sh(GRN,1.2)); cv.vl(1,0,4,sh(GRN,0.7)); return mat(cv)
def t_soil_flowers():
    cv=Cv(8,8,SOIL); cv.fill_noise(SOIL,0.25,5)
    for (x,y,c) in ((1,1,RED),(5,1,YEL),(6,5,PINK),(2,5,WHITE),(4,3,sh(LEAF,1))):
        cv.px(x,y,c); cv.px(x+1,y,sh(c,0.8)); cv.px(x,y+1,sh(c,0.7))
    return mat(cv)
def t_brick(w,h):
    cv=Cv(w,h,BRICK); mort=sh(STONE,1.05)
    for y in range(0,h,3):
        cv.hl(y,0,w,mort)
        off=(y//3)%2*2
        for x in range(off,w,4): cv.vl(x,y,min(h,y+3),mort)
    for y in range(h):
        for x in range(w):
            if cv.p[y][x]==BRICK and (x*3+y*5)%7==0: cv.px(x,y,sh(BRICK,0.85))
    return mat(cv)
def t_mail_front():
    cv=Cv(8,5,BLUE); cv.box(0,0,8,5,BLUE); cv.rect(1,1,7,4,sh(BLUE,0.9)); cv.hl(1,1,7,sh(BLUE,0.6)); cv.vl(1,1,4,sh(BLUE,0.6)); cv.vl(6,1,4,sh(BLUE,0.6)); cv.hl(3,1,7,sh(BLUE,0.6))
    cv.px(5,2,CHROME); cv.px(6,2,GREY); return mat(cv)
def t_mail_side():
    cv=Cv(6,5,BLUE); cv.box(0,0,6,5,BLUE); cv.px(1,1,CHROME); cv.px(4,1,CHROME); cv.px(1,3,CHROME); cv.px(4,3,CHROME); cv.rect(2,2,4,3,sh(BLUE,0.75)); return mat(cv)
def t_mail_top():
    cv=Cv(6,6,BLUE); cv.box(0,0,6,6,BLUE); cv.hl(1,1,5,sh(BLUE,1.5)); cv.hl(2,1,5,sh(BLUE,1.3)); return mat(cv)
def t_mail_arch():
    cv=Cv(6,2,BLUE); cv.hl(0,0,6,sh(BLUE,1.6)); cv.hl(1,0,6,sh(BLUE,1.1)); return mat(cv)
def t_flag():
    cv=Cv(2,6,RED); cv.vl(0,0,6,sh(RED,1.25)); cv.vl(1,0,6,sh(RED,0.75)); return mat(cv)
def t_post(c=WOOD,w=2,h=8):
    cv=Cv(w,h,c)
    for y in range(h):
        cv.px(0,y,sh(c,1.15)); cv.px(w-1,y,sh(c,0.7))
        if y%3==1: cv.px(min(1,w-1),y,sh(c,0.85))
    return mat(cv)
def t_stone(w=4,h=4,c=STONE):
    cv=Cv(w,h,c); cv.fill_noise(c,0.12,9)
    cv.hl(h-1,0,w,sh(c,0.72)); cv.vl(w-1,0,h,sh(c,0.82)); cv.hl(0,0,w,sh(c,1.15)); return mat(cv)
def t_bath_top():
    cv=Cv(8,8,STONE); cv.fill_noise(STONE,0.1,4); cv.rect(1,1,7,7,sh(STONE,0.7)); cv.rect(2,2,6,6,WATER)
    cv.px(3,3,sh(WATER,1.35)); cv.px(4,3,sh(WATER,1.25)); cv.px(4,4,sh(WATER,0.85)); cv.px(3,4,sh(WATER,1.1)); cv.px(5,5,sh(WATER,1.3))
    return mat(cv)
def t_bird(c):
    cv=Cv(2,2,c); cv.px(1,1,sh(c,0.7)); cv.px(0,0,sh(c,1.2)); return mat(cv)
def t_gnome_front():
    cv=Cv(4,6,BLUE); cv.vl(0,0,6,sh(BLUE,1.2)); cv.vl(3,0,6,sh(BLUE,0.75)); cv.hl(4,0,4,DWOOD); cv.px(1,4,YEL); cv.px(2,4,YEL); cv.px(1,1,YEL); cv.px(1,2,YEL)
    cv.rect(0,5,4,6,DARK); return mat(cv)
def t_gnome_face():
    cv=Cv(4,3,SKIN); cv.px(1,0,BLACK); cv.px(2,0,BLACK); cv.rect(1,1,3,2,sh(SKIN,0.95)); cv.px(1,1,C(29,12,10)); cv.px(2,1,C(29,12,10)); cv.rect(0,2,4,3,sh(SKIN,0.8)); return mat(cv)
def t_beard():
    cv=Cv(4,4,WHITE)
    for y in range(4):
        for x in range(4):
            if (x+y)&1: cv.px(x,y,sh(WHITE,0.88))
    cv.px(0,3,None); cv.px(3,3,None); cv.px(1,3,sh(WHITE,0.8)); return mat(cv)
def t_hat():
    cv=Cv(4,6,RED); cv.vl(0,0,6,sh(RED,1.3)); cv.vl(3,0,6,sh(RED,0.7)); cv.hl(5,0,4,sh(RED,0.6)); return mat(cv)
def t_diveboard_top():
    cv=Cv(8,4,BLUE); cv.hl(0,0,8,sh(BLUE,1.3)); cv.hl(3,0,8,sh(BLUE,0.7))
    for x in range(1,8,2): cv.px(x,1,sh(BLUE,1.2)); cv.px(x-1,2,sh(BLUE,1.2))
    cv.rect(0,0,2,4,WHITE); return mat(cv)
def t_conc(w=4,h=4):
    cv=Cv(w,h,C(18,18,19)); cv.fill_noise(C(18,18,19),0.1,6); cv.hl(h-1,0,w,C(12,12,13)); return mat(cv)
def t_chrome(w=2,h=4):
    cv=Cv(w,h,CHROME); cv.vl(0,0,h,C(30,30,31)); cv.vl(w-1,0,h,C(16,16,19)); return mat(cv)
def t_rung():
    cv=Cv(2,2,CHROME); cv.hl(0,0,2,C(30,30,31)); cv.hl(1,0,2,C(17,17,20)); return mat(cv)
def t_slat(c,w=8,h=6,vertical=False):
    cv=Cv(w,h,c)
    n=(h if not vertical else w)
    for i in range(n):
        if i%2==0:
            if vertical: cv.vl(i,0,h,sh(c,1.12))
            else: cv.hl(i,0,w,sh(c,1.12))
        else:
            if vertical: cv.vl(i,0,h,sh(c,0.7))
            else: cv.hl(i,0,w,sh(c,0.7))
    return mat(cv)
def t_stripes(c1,c2,w=8,h=2):
    cv=Cv(w,h,c1)
    for x in range(w):
        for y in range(h):
            if (x//2)%2: cv.px(x,y,c2)
    cv.hl(h-1,0,w,sh(c1,0.7)) ; return mat(cv)
def t_umb_top():
    cv=Cv(8,8,RED)
    for y in range(8):
        for x in range(8):
            dx=x-3.5; dy=y-3.5
            import math
            a=math.atan2(dy,dx); s=int((a+math.pi)/(2*math.pi)*8)%8
            cv.px(x,y,RED if s%2==0 else WHITE)
    cv.rect(3,3,5,5,YEL); return mat(cv)
def t_towel_top():
    cv=Cv(8,8,BLUE)
    for y in range(8):
        for x in range(8):
            if y in (1,2,5,6): cv.px(x,y,WHITE if y in (1,5) else sh(WHITE,0.85))
    for x in range(8): cv.px(x,0,sh(BLUE,0.8)); cv.px(x,7,sh(BLUE,0.8))
    return mat(cv)
def t_bottle():
    cv=Cv(2,3,ORG); cv.hl(0,0,2,WHITE); cv.vl(1,1,3,sh(ORG,0.7)); return mat(cv)
def t_grate():
    cv=Cv(8,8,DARK)
    for y in range(8):
        for x in range(8):
            if x%2==0: cv.px(x,y,sh(GREY,0.9))
            elif (x*7+y*3)%5==0: cv.px(x,y,C(28,9,2))
            else: cv.px(x,y,C(9,4,2) if y%2 else C(14,5,2))
    cv.rect(0,0,8,1,sh(GREY,0.75)); cv.rect(0,7,8,8,sh(GREY,0.75)); return mat(cv)
def t_bbq_side():
    cv=Cv(8,4,C(8,8,10)); cv.box(0,0,8,4,C(8,8,10)); 
    for x in range(1,7,2): cv.vl(x,1,3,BLACK)
    return mat(cv)
def t_lid_in():
    cv=Cv(8,4,C(10,10,12)); cv.box(0,0,8,4,C(10,10,12)); cv.rect(2,1,6,3,sh(CHROME,0.6)); return mat(cv)
def t_handle():
    cv=Cv(2,2,CHROME); cv.hl(1,0,2,C(14,14,17)); return mat(cv)
# ---- farm ----
def t_furrow():
    cv=Cv(8,8,SOIL); cv.fill_noise(SOIL,0.2,8)
    for y in (1,4,7 if False else 6): cv.hl(y,0,8,sh(SOIL,1.3)); cv.hl(y+1,0,8,sh(SOIL,0.65))
    return mat(cv)
def t_bed_side():
    cv=planks(8,3,WOOD); cv.px(1,1,sh(WOOD,0.5)); cv.px(6,1,sh(WOOD,0.5)); cv.hl(0,0,8,sh(WOOD,1.2)); return mat(cv)
def t_crop():
    cv=Cv(4,4,GRN); cv.fill_noise(GRN,0.3,2); cv.vl(1,0,4,sh(LEAF,1.5)); cv.hl(2,0,4,sh(LEAF,0.6)); cv.px(3,0,sh(LEAF,1.4)); return mat(cv)
def t_produce(c):
    cv=Cv(2,2,c); cv.px(0,0,sh(c,1.35)); cv.px(1,1,sh(c,0.65)); return mat(cv)
def t_coop_front():
    cv=planks(8,6,C(22,6,5),True,5)
    cv.rect(3,2,5,6,BLACK); cv.rect(3,1,5,2,sh(C(22,6,5),0.5)); cv.px(1,1,CREAM); cv.px(6,1,CREAM); return mat(cv)
def t_coop_side():
    cv=planks(6,6,C(22,6,5),True,6); cv.rect(2,2,4,4,CREAM); cv.px(2,2,GLASS); cv.px(3,2,GLASS); cv.px(2,3,GLASS); cv.px(3,3,sh(GLASS,0.8)); return mat(cv)
def t_roof(w=8,h=3,c=C(14,10,9)):
    cv=Cv(w,h,c)
    for y in range(h):
        for x in range(w):
            cv.px(x,y,sh(c,1.15 if (x+y*2)%4<2 else 0.8))
    cv.hl(h-1,0,w,sh(c,0.55)); return mat(cv)
def t_roof_top(w=8,h=8,c=C(14,10,9)):
    cv=Cv(w,h,c)
    for y in range(h):
        for x in range(w):
            cv.px(x,y,sh(c,1.15 if ((x+(y%2)*2)%4)<2 else 0.85))
        cv.hl(y,0,w,sh(c,1.2 if y%2==0 else 0.7)) if False else None
    for y in range(0,h,2): cv.hl(y,0,w,sh(c,0.65))
    return mat(cv)
def t_ramp():
    cv=Cv(4,4,WOOD); cv.hl(0,0,4,sh(WOOD,1.2)); cv.hl(2,0,4,sh(WOOD,0.6)); cv.vl(0,0,4,sh(WOOD,0.8)); return mat(cv)
def t_hen_body():
    cv=Cv(4,4,WHITE); cv.fill_noise(WHITE,0.07,3); cv.rect(0,3,4,4,sh(WHITE,0.75)); cv.px(2,1,sh(WHITE,0.7)); return mat(cv)
def t_hen_head():
    cv=Cv(2,2,WHITE); cv.px(0,0,BLACK); cv.px(1,1,ORG); return mat(cv)
def t_cow_side():
    cv=Cv(8,5,WHITE); cv.hl(0,0,8,WHITE)
    for (x,y,w,h) in ((0,0,3,2),(3,2,3,3),(6,0,2,2),(1,3,2,2)): cv.rect(x,y,x+w,y+h,C(4,3,5))
    cv.hl(4,0,8,sh(WHITE,0.78)); return mat(cv)
def t_cow_top():
    cv=Cv(8,4,WHITE)
    for (x,y,w,h) in ((0,0,3,2),(4,1,3,3),(6,0,2,1)): cv.rect(x,y,x+w,y+h,C(4,3,5))
    return mat(cv)
def t_cow_back():
    cv=Cv(4,5,WHITE); cv.rect(0,0,2,2,C(4,3,5)); cv.hl(4,0,4,sh(WHITE,0.8)); cv.rect(1,2,3,3,PINK); return mat(cv)
def t_cow_face():
    cv=Cv(4,6,WHITE); cv.rect(0,0,4,2,C(4,3,5)); cv.px(0,2,BLACK); cv.px(3,2,BLACK); cv.px(0,2,BLACK)
    cv.rect(0,3,4,6,PINK); cv.px(1,4,sh(PINK,0.5)); cv.px(2,4,sh(PINK,0.5)); cv.rect(1,0,3,1,sh(WHITE,0.9)); return mat(cv)
def t_cow_leg():
    cv=Cv(2,3,WHITE); cv.hl(2,0,2,DARK); cv.vl(1,0,2,sh(WHITE,0.78)); return mat(cv)
def t_cow_ear():
    cv=Cv(2,2,C(4,3,5)); cv.px(0,0,PINK); return mat(cv)
def t_horn():
    cv=Cv(2,2,CREAM); cv.px(1,1,sh(CREAM,0.7)); return mat(cv)
def t_udder():
    cv=Cv(4,2,PINK); cv.px(0,1,sh(PINK,0.7)); cv.px(2,1,sh(PINK,0.7)); cv.px(1,1,sh(PINK,0.85)); return mat(cv)
def t_counter_front():
    cv=planks(8,4,WOOD,True,7); cv.hl(0,0,8,sh(WOOD,1.25)); cv.rect(1,1,7,3,sh(WOOD,0.78)); cv.hl(1,1,7,sh(WOOD,0.55)); return mat(cv)
def t_basket_top(a,b):
    cv=Cv(4,6,sh(WOOD,0.8)); 
    for y in range(1,5):
        for x in range(1,3):
            cv.px(x,y,a if (x+y)%2==0 else b)
    cv.px(1,1,sh(a,1.4)); cv.px(2,3,sh(b,1.4)); return mat(cv)
def t_awning_top():
    cv=Cv(8,4,RED)
    for x in range(8):
        if (x//2)%2: cv.rect(x,0,x+1,4,WHITE)
    return mat(cv)
def t_valance():
    cv=Cv(8,2,RED)
    for x in range(8):
        if (x//2)%2: cv.rect(x,0,x+1,2,WHITE)
    for x in (1,3,5,7): cv.px(x,1,None)
    cv.px(0,1,None) if False else None; return mat(cv)
# ---- space ----
def t_tube(c,w=8,h=4):
    cv=Cv(w,h,c); cv.hl(0,0,w,sh(c,1.35)); cv.hl(1,0,w,sh(c,1.15)); cv.hl(h-1,0,w,sh(c,0.6))
    cv.vl(1,0,h,C(24,19,5)); cv.vl(w-2,0,h,C(24,19,5)); return mat(cv)
def t_lens(c=GLASS):
    cv=Cv(4,4,DARK); cv.rect(1,1,3,3,c); cv.px(1,1,sh(c,1.3)); return mat(cv)
def t_metal(w=2,h=4,c=C(12,12,14)):
    cv=Cv(w,h,c); cv.vl(0,0,h,sh(c,1.3)); cv.vl(w-1,0,h,sh(c,0.7)); return mat(cv)
def t_dome(c=WHITE):
    cv=Cv(8,4,c); cv.hl(0,0,8,sh(c,1.15)); cv.hl(3,0,8,sh(c,0.7))
    for x in (2,5): cv.vl(x,0,4,sh(c,0.8))
    return mat(cv)
def t_dome_slit():
    cv=Cv(8,4,WHITE); cv.hl(0,0,8,sh(WHITE,1.1)); cv.hl(3,0,8,sh(WHITE,0.7)); cv.vl(2,0,4,sh(WHITE,0.8)); cv.vl(5,0,4,sh(WHITE,0.8)); cv.rect(3,0,5,4,C(4,6,12)); cv.px(3,1,C(10,12,22)); return mat(cv)
def t_dome_door():
    cv=Cv(8,6,WHITE); cv.hl(0,0,8,sh(WHITE,1.1)); cv.hl(5,0,8,sh(WHITE,0.7)); cv.rect(3,2,5,6,C(7,8,14)); cv.vl(3,2,6,GREY); cv.vl(4,2,6,sh(GREY,0.7)); cv.px(1,2,GLASS); cv.px(6,2,GLASS); return mat(cv)
def t_dome_top():
    cv=Cv(4,4,WHITE); cv.fill_noise(WHITE,0.06,2); cv.px(1,1,GREY); return mat(cv)
def t_dish_front():
    cv=Cv(8,8,None); cv.disc(4,4,4,C(27,27,28)); cv.disc(4,4,3.1,C(22,22,24)); cv.disc(4,4,2.1,C(18,18,20)); cv.disc(4,4,1.1,RED); cv.px(3,3,C(30,12,10))
    cv.px(1,1,C(29,29,30)); cv.px(2,0,C(29,29,30)); return mat(cv)
def t_dish_back():
    cv=Cv(8,8,None); cv.disc(4,4,4,C(20,20,22))
    for (x,y) in ((4,0),(4,1),(4,2),(4,5),(4,6),(4,7),(0,4),(1,4),(2,4),(5,4),(6,4),(7,4)): cv.px(x,y,C(13,13,15))
    cv.px(4,4,C(10,10,12)); return mat(cv)
def t_cabinet():
    cv=Cv(4,4,C(10,11,14)); cv.box(0,0,4,4,C(10,11,14)); cv.px(1,1,C(5,28,6)); cv.px(2,1,RED); cv.px(1,2,YEL); cv.px(2,2,C(5,28,6)); return mat(cv)
def t_pad():
    cv=Cv(8,8,C(14,14,16)); cv.fill_noise(C(14,14,16),0.1,2)
    for i in range(8):
        for (x,y) in ((i,0),(0,i),(i,7),(7,i)):
            if (i//2)%2==0: cv.px(x,y,YEL)
            else: cv.px(x,y,DARK)
    cv.disc(4,4,2.2,sh(C(14,14,16),0.6)); return mat(cv)
def t_hull():
    cv=Cv(8,16,WHITE)
    for y in range(16):
        for x in range(8): cv.px(x,y,sh(WHITE,1.0 if x<5 else 0.85 if x<7 else 0.7))
    cv.rect(0,5,8,7,RED); cv.rect(0,10,8,11,BLUE)
    cv.disc(3.5,13,1.5,GLASS); cv.px(3,12,C(28,29,30)); cv.hl(2,0,8,sh(WHITE,0.7)); cv.vl(0,0,16,sh(WHITE,1.2))
    cv.rect(2,7,6,8,BLACK); return mat(cv)
def t_hull_side():
    cv=Cv(8,16,WHITE)
    for y in range(16):
        for x in range(8): cv.px(x,y,sh(WHITE,0.95 if x<3 else 0.8))
    cv.rect(0,5,8,7,sh(RED,0.85)); cv.rect(0,10,8,11,sh(BLUE,0.85)); cv.hl(2,0,8,sh(WHITE,0.65)); return mat(cv)
def t_nose():
    cv=Cv(8,6,RED); cv.vl(0,0,6,sh(RED,1.3)); cv.vl(1,0,6,sh(RED,1.15)); cv.vl(7,0,6,sh(RED,0.65)); cv.vl(6,0,6,sh(RED,0.8)); return mat(cv)
def t_fin():
    cv=Cv(8,8,None)
    for y in range(8):
        for x in range(8):
            if x<2 or x>5: 
                top=8-y
                if y>=(7-x if x<2 else x-0)-2: cv.px(x,y,sh(RED,1.1 if x<2 else 0.8))
    for y in range(8): cv.px(3,y,sh(RED,0.9)); cv.px(4,y,sh(RED,0.9))
    cv.hl(0,3,5,None) if False else None
    return mat(cv)
def t_truss(w=2,h=12):
    cv=Cv(w,h,None); cv.vl(0,0,h,C(16,16,18)); cv.vl(w-1,0,h,C(12,12,14))
    for y in range(0,h,2): cv.hl(y,0,w,C(18,18,20))
    return mat(cv)
def t_rocket_fin_end():
    cv=Cv(2,2,sh(RED,0.9)); return mat(cv)
def t_console_front():
    cv=Cv(8,4,C(8,9,12)); cv.box(0,0,8,4,C(8,9,12)); cv.rect(1,1,3,3,sh(GREY,0.8)); cv.rect(5,1,7,2,sh(GREY,0.8)); cv.px(6,3,GRN); cv.px(5,3,RED); return mat(cv)
def t_screen(kind):
    cv=Cv(8,6,BLACK); cv.box(0,0,8,6,C(8,8,10)); cv.rect(1,1,7,5,C(2,8,4))
    if kind==0:
        for i in range(1,7): cv.px(i,4-((i*3)%3),C(8,31,10))
        cv.px(2,2,C(8,31,10))
    elif kind==1:
        cv.disc(4,3,2,C(3,14,5)); cv.px(4,3,C(8,31,10)); cv.px(5,2,C(8,31,10)); cv.hl(3,1,7,C(3,18,6)); cv.vl(4,1,5,C(3,18,6))
    else:
        cv.hl(1,1,6,C(8,31,10)); cv.hl(2,1,5,C(6,24,8)); cv.hl(3,1,6,C(8,31,10)); cv.hl(4,1,4,C(6,24,8))
    return mat(cv)
def t_screen_side(): return t_metal(2,5,C(8,8,10))
def t_keyboard():
    cv=Cv(8,4,C(9,9,11))
    for y in range(1,4):
        for x in range(1,7): cv.px(x,y,C(22,22,23) if (x+y)%2 else C(17,17,19))
    return mat(cv)
def t_desk_top():
    cv=Cv(8,6,C(10,11,14)); cv.fill_noise(C(10,11,14),0.1,3); cv.hl(0,0,8,C(16,17,20)); return mat(cv)
def t_btn():
    cv=Cv(4,4,C(9,9,12)); cv.box(0,0,4,4,C(9,9,12)); cv.px(1,1,RED); cv.px(2,1,YEL); cv.px(1,2,GRN); cv.px(2,2,BLUE); return mat(cv)
def t_canvas_art():
    cv=Cv(8,8,C(28,26,19)); cv.rect(1,1,7,7,C(15,23,30)); cv.rect(1,4,7,7,C(8,19,7)); cv.rect(2,5,5,7,C(6,15,6)); cv.disc(5,2.5,1,YEL); cv.rect(3,3,4,4,C(11,18,8)); cv.px(2,4,C(10,20,8))
    cv.px(5,5,RED); cv.px(6,6,PINK); cv.vl(0,0,8,WOOD); cv.vl(7,0,8,sh(WOOD,0.7)); cv.hl(0,0,8,WOOD); cv.hl(7,0,8,sh(WOOD,0.7)); return mat(cv)
def t_canvas_back():
    cv=Cv(4,4,C(26,23,17)); cv.hl(0,0,4,C(18,13,7)); cv.hl(3,0,4,C(18,13,7)); cv.vl(0,0,4,C(18,13,7)); cv.vl(3,0,4,C(18,13,7)); cv.hl(2,1,3,C(18,13,7)); return mat(cv)
def t_bench_seat():
    cv=Cv(8,4,WOOD)
    for y in range(4):
        c=sh(WOOD,1.12) if y%2==0 else sh(WOOD,0.82)
        cv.hl(y,0,8,c)
    cv.px(1,0,sh(WOOD,0.55)); cv.px(6,2,sh(WOOD,0.55)); return mat(cv)
def t_bench_front():
    cv=Cv(8,3,WOOD); cv.hl(0,0,8,sh(WOOD,1.15)); cv.hl(1,0,8,sh(WOOD,0.85)); cv.hl(2,0,8,sh(WOOD,0.6)); return mat(cv)
def t_bench_back():
    cv=Cv(8,6,WOOD)
    for y in range(6):
        cv.hl(y,0,8,sh(WOOD,1.12) if y in (0,3) else sh(WOOD,0.8) if y in (1,4) else sh(WOOD,0.55))
    return mat(cv)
def t_iron(w=2,h=8):
    cv=Cv(w,h,C(6,7,9)); cv.vl(0,0,h,C(13,14,17)); cv.vl(w-1,0,h,C(3,3,4)); return mat(cv)
def t_iron_cap():
    cv=Cv(2,2,C(13,14,17)); cv.px(1,1,C(5,5,7)); return mat(cv)
def t_iron_arm():
    cv=Cv(2,4,C(8,9,11)); cv.hl(0,0,2,C(16,17,20)); cv.hl(3,0,2,C(4,4,5)); return mat(cv)
def t_sky_plate():
    cv=Cv(2,2,C(14,14,16)); return mat(cv)
# ---- box builder ----
def Bx(x0,y0,x1,y1,z0,z1,m,top=None):
    ms=list(m) if isinstance(m,(list,tuple)) else [m]*4
    ms=[mat(q) if isinstance(q,Cv) else q for q in ms]; top=mat(top) if isinstance(top,Cv) else top
    t=top or ms[0]
    return (x0,y0,x1,y1,z0,z1,[ms[0],ms[1],ms[2],ms[3],t])
def fmt(b):
    x0,y0,x1,y1,z0,z1,m=b
    return '{%d,%d,%d,%d,%d,%d,{%s}}'%(x0,y0,x1,y1,z0,z1,','.join('&'+q for q in [m[0],m[1],m[2],m[3],m[4]]))

emitted=set()
def write(path,header,items,prefix,start,macro):
    boxes=[]
    for it in items:
        for grp in (it[3],) + ((it[4],) if len(it)>4 and it[4] else ()):
            boxes+=grp
    used=[]
    for b in boxes:
        for m in b[6]:
            if m not in used: used.append(m)
    keys=[k for k in order if mats[k][0] in used and k not in emitted]
    emitted.update(keys)
    o=[header]+emit_mats(keys)
    defs=[]
    for i,it in enumerate(items):
        nm,fl,cost,bx=it[:4]; bv=it[4] if len(it)>4 else None; letter=it[5] if len(it)>5 else None
        o.append('static const IBox bx%s%d[%d]={%s};'%(prefix,i,len(bx),','.join(fmt(b) for b in bx)))
        if bv: o.append('static const IBox bx%sV%d[%d]={%s};'%(prefix,i,len(bv),','.join(fmt(b) for b in bv)))
        if letter=='N' or letter=='skip': continue
        defs.append('    { (char)0x%X, "%s", bx%s%d, %d, %s, %d, %s, %d },   \\'%(start+i,nm,prefix,i,len(bx),('bx%sV%d'%(prefix,i)) if bv else '0',len(bv) if bv else 0,fl,cost))
    o.append('#define '+macro+' \\')
    o+=defs; o.append('')
    open(path,'w').write('\n'.join(o)+'\n')
    return len(boxes)

# ======== HOME: garden, pool, beach (letters 0xDF..0xEA) + the park bench ========
pk=t_picket(); wt=solid(WHITE); rail=solid(sh(WHITE,0.8))
post=t_post(WHITE,2,9); postcap=solid(sh(WHITE,1.1))
fence=[Bx(0,3,8,4,0,8,[pk,rail,pk,rail],wt), Bx(0,2,1,5,0,9,[post]*4,postcap), Bx(7,2,8,5,0,9,[post]*4,postcap)]
mp=t_post(WOOD,2,9); mf=t_mail_front(); ms=t_mail_side(); mt=t_mail_top(); ma=t_mail_arch(); mb=solid(sh(BLUE,0.6)); mfl=t_flag(); mback=solid(sh(BLUE,0.7))
mailbox=[Bx(3,3,5,5,0,9,[mp]*4,solid(sh(WOOD,1.2))), Bx(2,3,6,5,8,9,[t_post(WOOD,4,2)]*4), Bx(1,2,7,6,9,10,[mb,mb,mb,mb],mb), Bx(1,2,7,6,10,13,[mf,ms,mback,ms],mt),
         Bx(2,2,6,6,13,14,[ma,ma,ma,ma],mt), Bx(7,3,8,4,10,16,[mfl]*4,solid(sh(RED,1.3)))]
hl=t_leaf(False); hlt=t_leaf(True)
hedge=[Bx(0,1,8,7,0,8,hl,hlt), Bx(1,2,7,6,8,9,hl,hlt)]
brk8=t_brick(8,3); soilT=t_soil_flowers(); stem=t_stem()
def flower(x,y,c,h): return [Bx(x,y,x+1,y+1,2,h,[stem]*4), Bx(x-1 if x>0 else x,y-1 if y>0 else y,x+2,y+2,h,h+2,[solid(c)]*4,t_bloom(c))]
bed=[Bx(0,0,8,8,0,2,brk8,soilT)]+flower(2,2,RED,6)+flower(5,2,YEL,7)+flower(2,5,PINK,5)+flower(5,5,WHITE,6)
gg=t_gnome_front(); gf=t_gnome_face(); gb=t_beard(); gh=t_hat()
gnome=[Bx(3,3,5,5,0,1,solid(DARK)), Bx(3,3,5,5,1,6,[gg,t_metal(2,5,BLUE),solid(sh(BLUE,0.7)),t_metal(2,5,BLUE)],solid(sh(BLUE,1.1))),
       Bx(3,2,5,3,2,6,[gb,gb,gb,gb],gb), Bx(3,3,5,5,6,8,[gf,solid(SKIN),solid(sh(SKIN,0.8)),solid(SKIN)],solid(SKIN)),
       Bx(3,3,5,5,8,10,[gh,t_hat(),t_hat(),t_hat()],solid(sh(RED,1.1))), Bx(4,4,5,5,10,11,[t_hat()]*4,solid(sh(RED,1.1)))]
st=t_stone(4,4); stp=t_stone(2,4)
bath=[Bx(2,2,6,6,0,1,[t_stone(4,2)]*4,t_stone()), Bx(3,3,5,5,1,6,[stp]*4,t_stone()), Bx(1,1,7,7,6,8,[t_stone(8,2)]*4,t_bath_top()),
      Bx(3,3,5,4,8,9,[t_bird(BLUE)]*4,t_bird(sh(BLUE,1.3))), Bx(3,4,4,5,8,10,[t_bird(YEL)]*4,t_bird(YEL))]
cc=t_conc(8,4); dt=t_diveboard_top()
dive=[Bx(0,2,3,6,0,4,[cc,t_conc(4,4),cc,t_conc(4,4)],t_conc(4,4)), Bx(0,3,8,5,4,5,[solid(WHITE,8,1),solid(sh(BLUE,0.8)),solid(WHITE,8,1),solid(sh(WHITE,0.8))],dt), Bx(1,3,3,5,5,6,[solid(CHROME)]*4,solid(CHROME))]
cr=t_chrome(2,8); rg=t_rung()
ladder=[Bx(1,2,2,3,0,9,[cr]*4,rg), Bx(1,5,2,6,0,9,[cr]*4,rg), Bx(2,2,3,3,7,9,[cr]*4,rg), Bx(2,5,3,6,7,9,[cr]*4,rg),
        Bx(1,3,2,5,2,3,[rg]*4,rg), Bx(1,3,2,5,4,5,[rg]*4,rg), Bx(1,3,2,5,6,7,[rg]*4,rg)]
lw=t_post(WHITE,2,12); lsl=t_slat(WHITE,8,6,True); lsr=t_slat(sh(WHITE,0.9),8,3)
lifeg=[Bx(1,1,2,2,0,12,[lw]*4,wt), Bx(6,1,7,2,0,12,[lw]*4,wt), Bx(1,6,2,7,0,12,[lw]*4,wt), Bx(6,6,7,7,0,12,[lw]*4,wt),
       Bx(1,1,7,2,5,6,[solid(sh(WHITE,0.8),8,1)]*4,wt), Bx(1,6,7,7,5,6,[solid(sh(WHITE,0.8),8,1)]*4,wt),
       Bx(0,0,8,8,12,13,[t_slat(WHITE,8,2)]*4,t_slat(WHITE,8,8)), Bx(0,0,8,1,13,19,[lsl,solid(WHITE),lsl,solid(WHITE)],wt),
       Bx(0,0,1,8,13,16,[solid(RED),solid(sh(RED,1.1)),solid(RED),solid(sh(RED,0.8))],solid(RED)), Bx(0,0,8,5,19,20,[t_stripes(RED,WHITE)]*4,t_awning_top())]
pole=t_post(WOOD,2,16); ust=t_stripes(RED,WHITE,8,3); ust2=t_stripes(BLUE,WHITE,8,3)
umb=[Bx(3,3,5,5,0,15,[pole]*4,solid(WOOD)), Bx(0,0,8,8,14,16,[ust]*4,t_umb_top()), Bx(2,2,6,6,16,18,[t_stripes(RED,WHITE,4,2)]*4,t_umb_top()), Bx(3,3,5,5,18,19,[solid(YEL)]*4,solid(YEL))]
towel=[Bx(1,0,7,8,0,1,[t_stripes(BLUE,WHITE,6,1)]*4,t_towel_top()), Bx(5,5,6,6,1,3,[t_bottle()]*4,solid(WHITE)), Bx(2,2,4,4,1,2,[solid(YEL)]*4,solid(sh(YEL,1.1)))]
gr=t_grate(); gs=t_bbq_side(); lid=t_lid_in()
bbq=[Bx(1,1,2,2,0,5,[t_metal(2,5,C(7,7,9))]*4,solid(DARK)), Bx(6,1,7,2,0,5,[t_metal(2,5,C(7,7,9))]*4,solid(DARK)), Bx(1,5,2,6,0,5,[t_metal(2,5,C(7,7,9))]*4,solid(DARK)), Bx(6,5,7,6,0,5,[t_metal(2,5,C(7,7,9))]*4,solid(DARK)),
     Bx(1,1,7,6,5,8,[gs,t_metal(5,3,C(8,8,10)),gs,t_metal(5,3,C(8,8,10))],gr), Bx(1,5,7,6,8,12,[lid,solid(sh(C(8,8,10),0.8)),solid(C(9,9,11),8,4),solid(sh(C(8,8,10),0.8))],solid(C(9,9,11))),
     Bx(7,2,8,5,5,6,[solid(WOOD)]*4,t_slat(WOOD,2,4,False)), Bx(3,5,5,6,12,13,[t_handle()]*4,t_handle())]
bs=t_bench_seat(); bf=t_bench_front(); bk=t_bench_back(); ir=t_iron(2,8); ia=t_iron_arm(); ic=t_iron_cap()
def bench_u():   # runs along a (front +b): iron ends, slatted seat and back
    return [Bx(0,2,1,6,0,8,[ir,ir,ir,ir],ic), Bx(7,2,8,6,0,8,[ir,ir,ir,ir],ic), Bx(0,2,8,6,4,5,[bf,t_bench_front(),bf,t_bench_front()],bs),
            Bx(0,5,8,6,5,9,[bk,solid(sh(WOOD,0.9)),bk,solid(sh(WOOD,0.7))],t_bench_front()), Bx(0,2,1,5,6,7,[ia]*4,ic), Bx(7,2,8,5,6,7,[ia]*4,ic)]
def bench_v():
    return [Bx(2,0,6,1,0,8,[ir]*4,ic), Bx(2,7,6,8,0,8,[ir]*4,ic), Bx(2,0,6,8,4,5,[bf,t_bench_front(),bf,t_bench_front()],bs),
            Bx(5,0,6,8,5,9,[solid(sh(WOOD,0.9)),bk,solid(sh(WOOD,0.7)),bk],t_bench_front()), Bx(2,0,5,1,6,7,[ia]*4,ic), Bx(2,7,5,8,6,7,[ia]*4,ic)]
home=[('PICKET FENCE','0',150,fence),('MAILBOX','0',60,mailbox),('HEDGE','0',40,hedge),('FLOWER BED','0',50,bed),('GARDEN GNOME','0',45,gnome),('BIRD BATH','0',110,bath),
      ('DIVING BOARD','0',400,dive),('POOL LADDER','0',120,ladder),('LIFEGUARD CHAIR','0',260,lifeg),('BEACH UMBRELLA','0',80,umb),('BEACH TOWEL','0',20,towel),('BBQ GRILL','0',180,bbq),
      ('BENCH','F_ALONG',120,bench_u(),bench_v(),'N')]
nb_home=write('../src/items_home.inc','// GENERATED by tools/make_items_q.py: garden, pool, beach things and the park bench (included by items.c)',home,'H',0xDF,'ITEMS_HOME')

# ======== FARM (0xDB..0xDE) ========
fr=t_furrow(); bsd=t_bed_side(); cp=t_crop()
def plant(x,y,c): return [Bx(x,y,x+2,y+2,3,6,[cp]*4,cp), Bx(x,y,x+1,y+1,6,7,[t_produce(c)]*4,t_produce(sh(c,1.2)))]
crop=[Bx(0,0,8,8,0,3,[bsd]*4,fr)]+plant(1,1,ORG)+plant(5,1,RED)+plant(1,5,YEL)+plant(5,5,ORG)
cf=t_coop_front(); cs=t_coop_side(); rf=t_roof(8,3); rt=t_roof_top(8,8)
coop=[Bx(1,2,2,7,0,2,[t_post()]*4,solid(WOOD)), Bx(6,2,7,7,0,2,[t_post()]*4,solid(WOOD)),
      Bx(1,2,7,7,2,8,[cf,cs,planks(8,6,C(22,6,5),True,9),cs],solid(DWOOD)), Bx(0,1,8,8,8,9,[rf]*4,rt), Bx(1,2,7,7,9,10,[t_roof(6,2)]*4,t_roof_top(6,6)),
      Bx(3,0,5,2,0,1,[t_ramp()]*4,t_ramp()), Bx(3,1,5,2,1,2,[t_ramp()]*4,t_ramp()),
      Bx(5,0,7,2,0,2,[t_hen_body()]*4,t_hen_body()), Bx(5,0,6,1,2,3,[t_hen_head()]*4,solid(sh(WHITE,1.1))), Bx(5,0,6,1,3,4,[solid(RED)]*4,solid(sh(RED,1.2)))]
cside=t_cow_side(); ctop=t_cow_top()
cow=[Bx(2,1,6,6,3,8,[t_cow_back(),cside,t_cow_back(),cside],ctop), Bx(3,6,5,8,4,8,[t_cow_face(),t_cow_ear(),t_cow_back(),t_cow_ear()],solid(WHITE)),
     Bx(2,6,3,7,6,7,[t_cow_ear()]*4,t_cow_ear()), Bx(5,6,6,7,6,7,[t_cow_ear()]*4,t_cow_ear()), Bx(3,7,4,8,8,9,[t_horn()]*4,t_horn()), Bx(4,7,5,8,8,9,[t_horn()]*4,t_horn()),
     Bx(2,1,3,2,0,3,[t_cow_leg()]*4,solid(WHITE)), Bx(5,1,6,2,0,3,[t_cow_leg()]*4,solid(WHITE)), Bx(2,5,3,6,0,3,[t_cow_leg()]*4,solid(WHITE)), Bx(5,5,6,6,0,3,[t_cow_leg()]*4,solid(WHITE)),
     Bx(3,3,5,5,2,3,[t_udder()]*4,solid(PINK)), Bx(3,0,4,1,3,7,[t_cow_leg()]*4,solid(DARK))]
cc2=t_counter_front()
stand=[Bx(0,1,1,2,0,9,[t_post()]*4,solid(WOOD)), Bx(7,1,8,2,0,9,[t_post()]*4,solid(WOOD)), Bx(1,1,7,2,0,7,[planks(8,7,WOOD,True,4)]*4,solid(WOOD)),
       Bx(0,3,8,6,0,4,[cc2,t_metal(3,4,WOOD),planks(8,4,WOOD),t_metal(3,4,WOOD)],planks(8,3,WOOD)),
       Bx(1,3,4,6,4,5,[solid(sh(WOOD,0.8))]*4,t_basket_top(RED,YEL)), Bx(5,3,7,6,4,5,[solid(sh(WOOD,0.8))]*4,t_basket_top(ORG,GRN)),
       Bx(0,0,8,4,9,10,[t_awning_top()]*4,t_awning_top()), Bx(0,0,8,1,8,9,[t_valance()]*4,solid(RED))]
farm=[('CROP BED','F_ROT',40,crop),('HEN COOP','F_ROT',90,coop),('DAIRY COW','F_ROT',300,cow),('FARM STAND','F_ROT',150,stand)]
nb_farm=write('../src/items_farm.inc','// GENERATED by tools/make_items_q.py: FARM LIFE things (included by items.c)',farm,'F',0xDB,'ITEMS_FARM')

# ======== SPACE (0xD3..0xDA) ========
tb=t_tube(WHITE); tbb=t_tube(BLUE); lns=t_lens(); mt_=t_metal(2,4); mt6=t_metal(2,6)
leg=[Bx(1,1,7,7,0,1,[t_metal(6,1,C(8,8,10))]*4,solid(C(10,10,12))), Bx(2,2,6,6,1,2,[t_metal(4,1,C(8,8,10))]*4,solid(C(10,10,12)))]
small=leg+[Bx(3,3,5,5,2,7,[mt6]*4,solid(DARK)), Bx(1,3,7,5,7,9,[tb,lns,tb,t_lens(C(8,12,20))],tb), Bx(6,3,7,5,9,10,[tbb]*4,solid(BLUE)), Bx(0,3,1,5,8,9,[mt_]*4,solid(DARK))]
hob=leg+[Bx(3,3,5,5,2,9,[mt6]*4,solid(DARK)), Bx(2,2,6,6,9,10,[solid(sh(GREY,0.8))]*4,solid(GREY)),
     Bx(0,2,7,6,10,14,[tb,lns,tb,t_lens(C(8,12,20))],tb), Bx(7,2,8,6,10,14,[t_metal(2,4,C(4,4,6))]*4,solid(DARK)), Bx(2,2,4,3,14,15,[tbb]*4,solid(BLUE)), Bx(6,3,7,5,6,9,[solid(sh(GREY,0.7))]*4,solid(GREY))]
conc=t_conc(8,8)
res=[Bx(2,2,6,6,0,8,[conc]*4,t_conc()), Bx(1,2,7,6,8,10,[t_metal(6,2,C(8,8,10))]*4,solid(DARK)), Bx(0,1,2,7,10,15,[tb,lns,tb,tbb],tb), Bx(2,1,6,7,10,15,[tbb,tb,tbb,tb],tbb), Bx(6,1,8,7,10,15,[tb,t_lens(C(15,26,30)),tb,tb],tb),
     Bx(3,0,5,1,13,15,[mt_]*4,solid(DARK)), Bx(2,7,3,8,2,5,[t_cabinet()]*4,solid(DARK))]
dm=t_dome(); dsl=t_dome_slit(); dd=t_dome_door()
obs=[Bx(0,0,8,8,0,6,[dd,t_dome(),t_dome(),t_dome()],solid(sh(WHITE,0.9))), Bx(0,0,8,8,6,7,[t_metal(8,1,GREY)]*4,solid(GREY)), Bx(1,1,7,7,7,10,[dsl,dm,dm,dm],t_dome_top()),
     Bx(2,2,6,6,10,12,[dsl,dm,dm,dm],t_dome_top()), Bx(3,3,5,5,12,13,[dm]*4,t_dome_top())]
dish=[Bx(2,2,6,6,0,2,[conc]*4,t_conc()), Bx(3,3,5,5,2,9,[t_metal(2,7,C(18,18,20))]*4,solid(GREY)), Bx(0,3,8,4,8,15,[t_dish_front(),solid(sh(GREY,0.8),1,1),t_dish_back(),solid(sh(GREY,0.8),1,1)],solid(GREY)),
      Bx(3,4,5,5,10,12,[t_metal(2,2,C(20,20,22))]*4,solid(GREY)), Bx(3,5,5,6,10,13,[t_lens(C(30,11,10))]*4,solid(RED)), Bx(6,5,8,7,0,4,[t_cabinet(),t_metal(2,4),t_cabinet(),t_metal(2,4)],solid(DARK))]
hull=t_hull(); hs=t_hull_side(); nose=t_nose(); fn=t_fin(); tr=t_truss(2,22)
rocket=[Bx(0,0,8,8,0,2,[conc]*4,t_pad()), Bx(1,3,7,5,2,9,[fn,solid(sh(RED,0.8)),fn,solid(sh(RED,0.8))],solid(sh(RED,0.9))), Bx(3,1,5,7,2,9,[solid(sh(RED,0.8)),fn,solid(sh(RED,0.8)),fn],solid(sh(RED,0.9))),
        Bx(2,2,6,6,2,18,[hull,hs,hs,hs],solid(sh(WHITE,0.9))), Bx(2,2,6,6,18,21,[nose]*4,solid(RED)), Bx(3,3,5,5,21,25,[nose]*4,solid(sh(RED,1.2))), Bx(3,3,5,5,25,26,[solid(WHITE)]*4,solid(WHITE)),
        Bx(0,0,2,2,2,24,[tr]*4,solid(GREY))]
cf2=t_console_front()
con=[Bx(0,1,8,6,0,4,[cf2,t_metal(5,4,C(8,9,12)),cf2,t_metal(5,4,C(8,9,12))],t_desk_top()), Bx(0,1,3,2,4,9,[t_screen(0),t_screen_side(),solid(DARK),t_screen_side()],solid(DARK)),
     Bx(3,1,6,2,4,10,[t_screen(1),t_screen_side(),solid(DARK),t_screen_side()],solid(DARK)), Bx(6,1,8,2,4,9,[t_screen(2),t_screen_side(),solid(DARK),t_screen_side()],solid(DARK)),
     Bx(1,3,5,5,4,5,[t_keyboard()]*4,t_keyboard()), Bx(6,3,7,5,4,5,[t_btn()]*4,t_btn())]
eas=[Bx(1,2,2,3,0,13,[t_post(WOOD,2,13)]*4,solid(WOOD)), Bx(6,2,7,3,0,13,[t_post(WOOD,2,13)]*4,solid(WOOD)), Bx(3,5,4,6,0,10,[t_post(WOOD,2,10)]*4,solid(WOOD)),
     Bx(1,3,7,4,5,12,[t_canvas_art(),solid(WOOD),t_canvas_back(),solid(WOOD)],solid(WHITE)), Bx(1,2,7,3,4,5,[planks(8,1,WOOD)]*4,planks(8,2,WOOD)), Bx(1,2,7,3,12,13,[planks(8,1,WOOD)]*4,planks(8,2,WOOD)),
     Bx(5,2,6,3,5,7,[t_produce(RED)]*4,t_produce(RED)), Bx(2,2,4,3,5,6,[solid(CREAM)]*4,t_produce(YEL))]
space=[('SMALL TELESCOPE','F_ROT',150,small),('HOBBY TELESCOPE','F_ROT',700,hob),('RESEARCH TELESCOPE','F_ROT',3500,res),('OBSERVATORY DOME','F_ROT',12000,obs),
       ('RADIO DISH ARRAY','F_ROT',30000,dish),('LAUNCH ROCKET','F_ROT',5000,rocket),('MISSION CONSOLE','F_ROT',800,con),('EASEL','F_ROT',120,eas)]
nb_space=write('../src/items_space.inc','// GENERATED by tools/make_items_q.py: the SPACE AGE telescopes, rocket, console and easel (included by items.c)',space,'S',0xD3,'ITEMS_SPACE')

# ======== BEDS, CRIBS, BEDROOM and BABY things (0xEB..0xF5) ========
CRIMSON=C(21,4,7); GOLD=C(30,24,6); NAVY=C(6,9,22); MINT=C(14,26,18); LILAC=C(22,16,28); TAN=C(25,20,12); BEAR=C(20,12,6); DEEPW=C(14,8,4); MAPLE=C(24,17,9)
def quilt(c1,c2,c3,kind):
    cv=Cv(8,8,c1)
    for y in range(8):
        for x in range(8):
            if kind==0:   # stripes with a border
                cv.px(x,y,c2 if y%4==1 else c1)
            elif kind==1: # patchwork squares
                cv.px(x,y,c1 if ((x//2)+(y//2))%2==0 else c2)
                if x%2==1 and y%2==1: cv.px(x,y,c3)
            elif kind==2: # diamonds
                cv.px(x,y,c2 if abs(x-3.5)+abs(y-3.5)<=2.5 else c1)
                if (x+y)%4==0 and abs(x-3.5)+abs(y-3.5)>2.5: cv.px(x,y,c3)
            else:        # stars on a field
                cv.px(x,y,c1)
    if kind==3:
        for (x,y) in ((1,1),(5,1),(3,4),(1,6),(6,6)): cv.px(x,y,c3); cv.px(x-1,y,c2); cv.px(x+1,y,c2); cv.px(x,y-1,c2); cv.px(x,y+1,c2)
    cv.hl(0,0,8,sh(c1,0.7)); cv.hl(7,0,8,sh(c1,0.7)); cv.vl(0,0,8,sh(c1,0.7)); cv.vl(7,0,8,sh(c1,0.7))
    return mat(cv)
def drape(c,w=8,h=3,trim=None):
    cv=Cv(w,h,c)
    for x in range(w): cv.px(x,0,sh(c,1.25))
    for y in range(h):
        for x in range(0,w,2): cv.px(x,y,sh(c,0.88))
    cv.hl(h-1,0,w,trim or sh(c,0.65)); return mat(cv)
def pillow_top(c=WHITE):
    cv=Cv(4,4,c); cv.rect(1,1,3,3,sh(c,0.9)); cv.hl(0,0,4,sh(c,1.1)); cv.vl(0,0,4,sh(c,1.05)); cv.hl(3,0,4,sh(c,0.8)); cv.vl(3,0,4,sh(c,0.82)); return mat(cv)
def pillow_side(c=WHITE): cv=Cv(4,2,c); cv.hl(0,0,4,sh(c,1.1)); cv.hl(1,0,4,sh(c,0.82)); return mat(cv)
def headboard(c,style):
    cv=Cv(8,12,c)
    for y in range(12): cv.px(0,y,sh(c,1.2)); cv.px(7,y,sh(c,0.7))
    cv.hl(0,0,8,sh(c,1.25)); cv.hl(11,0,8,sh(c,0.65))
    if style==0: cv.box(1,2,7,10,sh(c,0.85)); cv.rect(2,3,6,9,sh(c,1.0)); cv.hl(3,2,6,sh(c,1.2))
    elif style==1:
        for x in (2,4,6): cv.vl(x,3,11,sh(c,0.72)); cv.vl(x-1,3,11,sh(c,1.15))
        cv.hl(2,1,7,sh(c,1.1)); cv.hl(3,1,7,sh(c,0.7))
    else:
        cv.box(1,1,7,5,sh(c,0.9)); cv.px(4,3,GOLD)
        for x in (2,4,6): cv.vl(x,6,11,sh(c,0.75))
    return mat(cv)
def footboard(c):
    cv=Cv(8,4,c); cv.hl(0,0,8,sh(c,1.25)); cv.hl(3,0,8,sh(c,0.65)); cv.rect(1,1,7,3,sh(c,0.88)); cv.hl(1,1,7,sh(c,1.1)); return mat(cv)
def turned(c,h=12):
    cv=Cv(2,h,c); cv.vl(0,0,h,sh(c,1.25)); cv.vl(1,0,h,sh(c,0.7))
    for y in range(2,h,4): cv.hl(y,0,2,sh(c,0.55))
    return mat(cv)
def finial(c=GOLD): cv=Cv(2,2,c); cv.px(0,0,sh(c,1.3)); cv.px(1,1,sh(c,0.7)); return mat(cv)
def wicker(c,w=8,h=4):
    cv=Cv(w,h,c)
    for y in range(h):
        for x in range(w):
            cv.px(x,y,sh(c,1.15) if (x+y)%2==0 else sh(c,0.75))
    return mat(cv)
def wicker_open(c,w=8,h=4):
    cv=Cv(w,h,None)
    for y in range(h):
        for x in range(w):
            if (x+y)%2==0 or y in (0,h-1): cv.px(x,y,sh(c,1.1 if (x+y)%2==0 else 0.8))
    return mat(cv)
def bars(c,w=8,h=4):
    cv=Cv(w,h,None); cv.hl(0,0,w,sh(c,1.1)); cv.hl(h-1,0,w,sh(c,0.8))
    for x in range(1,w,2): cv.vl(x,0,h,c)
    return mat(cv)
def mesh(c,w=8,h=6):
    cv=Cv(w,h,None)
    for y in range(h):
        for x in range(w):
            if (x%2==0 and y%2==0) or (x%2==1 and y%2==1 and False): cv.px(x,y,c)
    cv.hl(0,0,w,sh(c,1.2)); cv.hl(h-1,0,w,sh(c,0.8)); return mat(cv)
def drawers(c):
    cv=Cv(8,7,c)
    for (y0,y1) in ((0,3),(3,7)):
        cv.box(0,y0,8,y1,c); cv.rect(1,y0+1,7,y1-1,sh(c,0.9)); cv.hl(y0+1,1,7,sh(c,0.65)); cv.px(3,(y0+y1)//2,GOLD); cv.px(4,(y0+y1)//2,GOLD)
    return mat(cv)
def doors(c):
    cv=Cv(8,12,c); cv.box(0,0,8,12,c); cv.vl(4,0,12,sh(c,0.5)); 
    for x0 in (0,4): cv.box(x0+1,1,x0+4 if x0==0 else 7,6,sh(c,0.9)); cv.box(x0+1,7,x0+4 if x0==0 else 7,11,sh(c,0.9))
    cv.px(3,6,GOLD); cv.px(5,6,GOLD); return mat(cv)
def chest_front():
    cv=planks(8,5,C(23,9,6),True,3); cv.hl(0,0,8,sh(C(23,9,6),1.2)); cv.hl(4,0,8,sh(C(23,9,6),0.6)); cv.disc(4,2.5,1.4,YEL); cv.px(4,2,ORG); cv.rect(0,1,1,4,GOLD); cv.rect(7,1,8,4,GOLD); return mat(cv)
def block(c): cv=Cv(2,2,c); cv.px(0,0,WHITE); cv.px(1,1,sh(c,0.65)); return mat(cv)
def bear_face():
    cv=Cv(2,2,BEAR); cv.px(0,0,BLACK); cv.px(1,0,BLACK); cv.px(0,1,sh(BEAR,1.4)); cv.px(1,1,sh(BEAR,1.4)); return mat(cv)
def horse_face():
    cv=Cv(4,4,TAN); cv.px(0,0,BLACK); cv.px(3,0,BLACK); cv.rect(1,2,3,4,sh(TAN,0.8)); cv.px(1,3,BLACK); cv.px(2,3,BLACK); cv.hl(0,1,3,DEEPW); return mat(cv)
def saddle(): cv=Cv(4,4,RED); cv.hl(0,0,4,sh(RED,1.3)); cv.vl(0,0,4,sh(RED,1.1)); cv.hl(3,0,4,sh(RED,0.6)); return mat(cv)
def mane(): cv=Cv(2,4,DEEPW); cv.vl(0,0,4,sh(DEEPW,1.3)); return mat(cv)
def play_pad():
    cv=Cv(8,8,C(18,25,29))
    for y in range(8):
        for x in range(8): cv.px(x,y,C(18,25,29) if (x//4+y//4)%2==0 else C(30,24,12))
    cv.rect(1,1,3,3,RED); cv.rect(5,5,7,7,BLUE); cv.px(5,1,YEL); cv.px(1,5,GRN); return mat(cv)
def lampshade(): cv=Cv(4,3,C(30,27,16)); cv.hl(0,0,4,C(31,29,20)); cv.hl(2,0,4,C(26,22,12)); cv.vl(0,0,3,C(31,29,19)); return mat(cv)
def shelf_front(c):
    cv=Cv(8,6,sh(c,0.5)); cv.hl(0,0,8,sh(c,1.2)); cv.hl(3,0,8,c); cv.rect(1,1,4,3,C(26,28,31)); cv.rect(4,1,7,3,PINK); cv.rect(1,4,7,6,C(30,29,22)); cv.hl(5,1,7,sh(C(30,29,22),0.8)); return mat(cv)
def changepad(): cv=Cv(8,8,C(14,22,29)); cv.box(0,0,8,8,C(14,22,29)); cv.rect(2,2,6,6,sh(C(14,22,29),1.15)); return mat(cv)
def foot_(c): return solid(c)
# a bed: head at b=0, foot at b=L, W wide (8 per tile), mattress top at z=mt (5), pillows to mt+1; the sleeper lies at the first tile's centre
def bed(frame,q,qs,pil=WHITE,hs=0,W=8,L=16,foot=True,mt=5,hh=11):
    hb=headboard(frame,hs); tn=turned(frame,12); fb=footboard(frame); fin=finial(); dk=sh(frame,0.8)
    b=[Bx(0,0,W,1,0,hh,[hb,solid(dk),solid(dk),solid(dk)],solid(sh(frame,1.15))), Bx(0,0,1,1,hh,hh+1,[fin]*4,fin), Bx(W-1,0,W,1,hh,hh+1,[fin]*4,fin)]
    b+=[Bx(0,L-1,1,L,0,4,[tn]*4,solid(sh(frame,1.15))), Bx(W-1,L-1,W,L,0,4,[tn]*4,solid(sh(frame,1.15))),
        Bx(0,1,1,L-1,2,4,[t_post(frame,2,3)]*4,solid(sh(frame,1.1))), Bx(W-1,1,W,L-1,2,4,[t_post(frame,2,3)]*4,solid(sh(frame,1.1)))]
    if foot: b+=[Bx(0,L-1,W,L,2,mt+1,[fb,t_post(frame,2,4),footboard(sh(frame,0.8)),t_post(frame,2,4)],solid(sh(frame,1.1)))]
    b+=[Bx(1,1,W-1,L-1,1,2,[solid(DARK)]*4,solid(DARK)), Bx(1,1,W-1,L-1,2,mt,[drape(qs),drape(qs),drape(qs),drape(qs)],q),
        Bx(2,1,6,3,mt,mt+1,[pillow_side(pil)]*4,pillow_top(pil))]
    if W>8: b+=[Bx(W-6,1,W-2,3,mt,mt+1,[pillow_side(pil)]*4,pillow_top(pil))]
    b+=[Bx(1,L-5,W-1,L-1,mt,mt+1,[drape(sh(qs,1.2),8,1)]*4,quilt(sh(qs,1.2),sh(qs,0.9),WHITE,0))]   # the folded cover at the foot
    return b
bedS=bed(MAPLE,quilt(C(8,14,27),C(14,20,30),WHITE,0),C(8,14,27),WHITE,0,8,8)           # (the one-tile bed the old houses are built with; new worlds use SINGLE BED 0xF6, two tiles long)
bedL=bed(MAPLE,quilt(C(8,14,27),C(14,20,30),WHITE,0),C(8,14,27),WHITE,0,7,15)          # a single bed, two tiles long
bedF=shift(bed(C(20,14,7),quilt(C(24,8,8),C(30,27,20),C(18,6,6),1),C(24,8,8),WHITE,1,12,15),2,0)   # a family bed: 1.5 tiles (1.7 m) wide, two long, on a 2 x 2 tile footprint, centred
# four-poster: tall posts and a rim of beams (open, so the sleeper shows), curtains in the corners
W4,L4=12,15
pt=t_post(DEEPW,2,20); cb=t_post(DEEPW,8,2); cbs=t_post(DEEPW,2,2); cbr=C(21,4,7)
bed4=[Bx(0,0,W4,1,0,12,[headboard(DEEPW,2),solid(sh(DEEPW,0.8)),solid(sh(DEEPW,0.8)),solid(sh(DEEPW,0.8))],solid(sh(DEEPW,1.15))),
      Bx(0,0,1,1,12,20,[pt]*4,finial()), Bx(W4-1,0,W4,1,12,20,[pt]*4,finial()), Bx(0,L4-1,1,L4,0,20,[pt]*4,finial()), Bx(W4-1,L4-1,W4,L4,0,20,[pt]*4,finial()),
      Bx(0,1,1,L4-1,19,20,[cbs]*4,solid(sh(DEEPW,1.1))), Bx(W4-1,1,W4,L4-1,19,20,[cbs]*4,solid(sh(DEEPW,1.1))), Bx(1,L4-1,W4-1,L4,19,20,[cb]*4,solid(sh(DEEPW,1.1))), Bx(1,0,W4-1,1,19,20,[cb]*4,solid(sh(DEEPW,1.1))),
      Bx(0,1,1,L4-1,2,4,[t_post(DEEPW,2,3)]*4,solid(sh(DEEPW,1.1))), Bx(W4-1,1,W4,L4-1,2,4,[t_post(DEEPW,2,3)]*4,solid(sh(DEEPW,1.1))), Bx(0,L4-1,W4,L4,2,5,[footboard(DEEPW),t_post(DEEPW,2,3),footboard(sh(DEEPW,0.8)),t_post(DEEPW,2,3)],solid(sh(DEEPW,1.1))),
      Bx(1,1,W4-1,L4-1,1,2,[solid(DARK)]*4,solid(DARK)), Bx(1,1,W4-1,L4-1,2,5,[drape(CRIMSON,8,3,GOLD)]*4,quilt(CRIMSON,sh(CRIMSON,1.3),GOLD,2)),
      Bx(2,1,6,3,5,6,[pillow_side(CREAM)]*4,pillow_top(CREAM)), Bx(W4-6,1,W4-2,3,5,6,[pillow_side(CREAM)]*4,pillow_top(CREAM)),
      Bx(0,1,1,2,13,19,[drape(sh(CRIMSON,1.1),2,6)]*4,solid(CRIMSON)), Bx(W4-1,1,W4,2,13,19,[drape(sh(CRIMSON,1.1),2,6)]*4,solid(CRIMSON))]
bed4=shift(bed4,2,0)
bedT=[Bx(0,0,8,1,0,8,[headboard(MINT,0),solid(sh(MINT,0.8)),solid(sh(MINT,0.8)),solid(sh(MINT,0.8))],solid(sh(MINT,1.15))),
      Bx(0,11,8,12,0,5,[footboard(MINT),t_post(MINT,2,3),footboard(sh(MINT,0.8)),t_post(MINT,2,3)],solid(sh(MINT,1.1))),
      Bx(0,1,1,11,3,6,[bars(MINT,8,3)]*4,solid(sh(MINT,1.1))), Bx(7,1,8,11,3,6,[bars(MINT,8,3)]*4,solid(sh(MINT,1.1))),
      Bx(0,1,1,11,0,3,[t_post(MINT,2,3)]*4,solid(MINT)), Bx(7,1,8,11,0,3,[t_post(MINT,2,3)]*4,solid(MINT)),
      Bx(1,1,7,11,1,3,[drape(NAVY,8,2)]*4,quilt(NAVY,C(10,14,28),YEL,3)), Bx(2,1,6,3,3,4,[pillow_side(WHITE)]*4,pillow_top(WHITE))]
# bassinet: a wicker basket on a stand (see-through weave: the baby shows), a hood at the head
wk=C(26,21,12); wo=wicker_open(wk,8,4); wo2=wicker_open(wk,6,4)
bass=[Bx(1,1,2,2,0,2,[t_post(DWOOD,2,2)]*4,solid(DWOOD)), Bx(6,1,7,2,0,2,[t_post(DWOOD,2,2)]*4,solid(DWOOD)), Bx(1,6,2,7,0,2,[t_post(DWOOD,2,2)]*4,solid(DWOOD)), Bx(6,6,7,7,0,2,[t_post(DWOOD,2,2)]*4,solid(DWOOD)),
      Bx(1,1,7,7,2,4,[solid(CREAM)]*4,quilt(CREAM,PINK,WHITE,1)),
      Bx(1,6,7,7,4,9,[wo]*4,solid(wk)), Bx(1,1,7,2,4,9,[wo]*4,solid(wk)), Bx(1,2,2,6,4,9,[wo2]*4,solid(wk)), Bx(6,2,7,6,4,9,[wo2]*4,solid(wk)),
      Bx(1,1,7,3,9,14,[drape(CREAM,8,5)]*4,solid(sh(CREAM,0.9)))]
crad=[Bx(0,1,8,2,0,1,[solid(WOOD)]*4,solid(WOOD)), Bx(0,6,8,7,0,1,[solid(WOOD)]*4,solid(WOOD)), Bx(1,1,2,7,1,2,[solid(DWOOD)]*4,solid(DWOOD)), Bx(6,1,7,7,1,2,[solid(DWOOD)]*4,solid(DWOOD)),
      Bx(2,2,6,6,2,4,[solid(CREAM)]*4,quilt(CREAM,sh(BLUE,1.6),WHITE,3)),
      Bx(1,1,7,2,2,8,[bars(WOOD,8,6)]*4,solid(WOOD)), Bx(1,6,7,7,2,8,[bars(WOOD,8,6)]*4,solid(WOOD)), Bx(1,2,2,6,2,6,[bars(WOOD,6,4)]*4,solid(WOOD)), Bx(6,2,7,6,2,6,[bars(WOOD,6,4)]*4,solid(WOOD))]
# bedroom and nursery things
dr=drawers(MAPLE); wd=doors(DEEPW)
nightst=[Bx(1,1,2,2,0,2,[t_post(MAPLE,2,2)]*4,solid(MAPLE)), Bx(6,1,7,2,0,2,[t_post(MAPLE,2,2)]*4,solid(MAPLE)), Bx(1,6,2,7,0,2,[t_post(MAPLE,2,2)]*4,solid(MAPLE)), Bx(6,6,7,7,0,2,[t_post(MAPLE,2,2)]*4,solid(MAPLE)),
         Bx(1,1,7,7,2,9,[dr,t_metal(6,7,MAPLE),solid(sh(MAPLE,0.7)),t_metal(6,7,MAPLE)],solid(sh(MAPLE,1.15))),
         Bx(3,3,5,5,9,10,[solid(GOLD)]*4,solid(GOLD)), Bx(3,3,5,5,10,12,[t_post(sh(GOLD,0.9),2,2)]*4,solid(GOLD)), Bx(2,2,6,6,12,15,[lampshade()]*4,solid(C(31,29,20)))]
toychest=[Bx(0,2,8,7,0,5,[chest_front(),solid(sh(C(23,9,6),0.85)),solid(sh(C(23,9,6),0.7)),solid(sh(C(23,9,6),0.85))],solid(sh(C(23,9,6),1.1))),
          Bx(0,2,8,7,5,6,[solid(GOLD)]*4,solid(sh(C(23,9,6),1.2))),
          Bx(1,3,2,4,6,7,[block(RED)]*4,block(RED)), Bx(2,5,3,6,6,7,[block(BLUE)]*4,block(BLUE)), Bx(1,5,2,6,6,8,[block(YEL)]*4,block(YEL)),
          Bx(4,3,7,6,6,9,[solid(BEAR)]*4,solid(sh(BEAR,1.2))), Bx(4,3,7,6,9,11,[bear_face(),solid(BEAR),solid(sh(BEAR,0.8)),solid(BEAR)],solid(sh(BEAR,1.1)))]
rocker=[Bx(1,0,2,8,0,1,[solid(WOOD)]*4,solid(WOOD)), Bx(6,0,7,8,0,1,[solid(WOOD)]*4,solid(WOOD)),
        Bx(2,1,3,2,1,5,[t_post(WOOD,2,4)]*4,solid(WOOD)), Bx(5,1,6,2,1,5,[t_post(WOOD,2,4)]*4,solid(WOOD)), Bx(2,6,3,7,1,5,[t_post(WOOD,2,4)]*4,solid(WOOD)), Bx(5,6,6,7,1,5,[t_post(WOOD,2,4)]*4,solid(WOOD)),
        Bx(2,1,6,7,5,9,[solid(TAN),solid(sh(TAN,0.8)),solid(sh(TAN,0.8)),solid(sh(TAN,0.8))],solid(sh(TAN,1.1))), Bx(3,3,5,5,9,10,[saddle()]*4,saddle()),
        Bx(3,6,5,8,8,12,[horse_face(),solid(TAN),solid(sh(TAN,0.8)),solid(TAN)],solid(sh(TAN,1.1))), Bx(3,5,5,6,9,13,[mane()]*4,solid(DEEPW)), Bx(3,0,5,1,5,9,[mane()]*4,solid(DEEPW))]
wardrobe=[Bx(0,1,8,7,1,22,[wd,t_metal(6,21,DEEPW),solid(sh(DEEPW,0.7)),t_metal(6,21,DEEPW)],solid(sh(DEEPW,1.1))), Bx(0,1,8,7,22,24,[footboard(DEEPW)]*4,solid(sh(DEEPW,1.25))),
          Bx(0,1,1,2,0,1,[solid(DEEPW)]*4,solid(DEEPW)), Bx(7,1,8,2,0,1,[solid(DEEPW)]*4,solid(DEEPW)), Bx(0,6,1,7,0,1,[solid(DEEPW)]*4,solid(DEEPW)), Bx(7,6,8,7,0,1,[solid(DEEPW)]*4,solid(DEEPW))]
nightst=scale(nightst,8,8,0,0,9,box_top(nightst))   # a table and a lamp, 0.95 m (it was 1.6 m)
wardrobe=scale(wardrobe,8,8,0,0,18,box_top(wardrobe))   # 1.9 m (it was 2.5 m, taller than the wall)
mshh=mesh(C(30,29,27),8,6)
playpen=[Bx(0,0,8,8,0,1,[solid(sh(BLUE,0.8))]*4,play_pad()), Bx(0,0,8,1,1,7,[mshh]*4,solid(C(30,29,27))), Bx(0,7,8,8,1,7,[mshh]*4,solid(C(30,29,27))), Bx(0,1,1,7,1,7,[mshh]*4,solid(C(30,29,27))), Bx(7,1,8,7,1,7,[mshh]*4,solid(C(30,29,27))),
         Bx(0,0,8,1,7,8,[solid(PINK)]*4,solid(sh(PINK,1.1))), Bx(0,7,8,8,7,8,[solid(PINK)]*4,solid(sh(PINK,1.1))), Bx(0,1,1,7,7,8,[solid(PINK)]*4,solid(sh(PINK,1.1))), Bx(7,1,8,7,7,8,[solid(PINK)]*4,solid(sh(PINK,1.1))),
         Bx(2,3,3,4,1,2,[block(RED)]*4,block(RED)), Bx(4,4,6,6,1,3,[block(YEL)]*4,block(YEL))]
changing=[Bx(0,1,8,7,0,8,[shelf_front(MAPLE),t_metal(6,8,MAPLE),solid(sh(MAPLE,0.7)),t_metal(6,8,MAPLE)],solid(sh(MAPLE,1.1))), Bx(1,2,7,6,8,9,[changepad()]*4,changepad()),
          Bx(0,1,1,7,9,11,[t_post(MAPLE,2,2)]*4,solid(MAPLE)), Bx(7,1,8,7,9,11,[t_post(MAPLE,2,2)]*4,solid(MAPLE)), Bx(1,1,7,2,9,11,[t_post(MAPLE,6,2)]*4,solid(MAPLE)), Bx(1,3,3,5,9,10,[block(PINK)]*4,block(PINK))]
bedl=[('BED','F_ROT|F_L2',700,bedS,None,'skip'),('FOUR POSTER BED','F_ROT|F_L2|F_W2',2200,bed4),('FAMILY BED','F_ROT|F_L2|F_W2',1500,bedF),('TODDLER BED','F_ROT|F_L2',450,bedT),('BASSINET','F_ROT',200,bass),('CRADLE','F_ROT',260,crad),
      ('NIGHTSTAND','F_ROT',80,nightst),('TOY CHEST','F_ROT',90,toychest),('ROCKING HORSE','F_ROT',140,rocker),('WARDROBE','F_ROT',320,wardrobe),('PLAYPEN','F_ROT',180,playpen),('CHANGING TABLE','F_ROT',160,changing),('SINGLE BED','F_ROT|F_L2',600,bedL)]
nb_bed=write('../src/items_bed.inc','// GENERATED by tools/make_items_q.py: the beds, cribs, bedroom and nursery things (included by items.c)',bedl,'B',0xEA,'ITEMS_BED')

print('boxes',nb_home,nb_farm,nb_space,'mats',len(mats),'px',sum(m[2]*m[3] for m in mats.values()))
print('bench boxes',len(bench_u()),len(bench_v()))
