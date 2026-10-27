import re,sys
src=open('poster.py').read()
def variant(name,W,H,P,scenes,smoke,G,DX,DY,OUT):
    s=src
    s=s.replace("SS=3; W,H=1100,1560",f"SS=3; W,H={W},{H}; G={G}; DX={DX}; DY={DY}")
    s=re.sub(r"\nP=\{.*?\n\}\n","\nP="+repr(P)+"\n",s,flags=re.S)
    s=re.sub(r"scenes=\[.*?\]\n(?=def inset)","scenes="+re.sub(r"'(toasters|vacuum|duck|fridge|speech|smoking)'",r"\1",repr(scenes))+"\n",s,flags=re.S)
    s=re.sub(r"SMOKE=\[.*?\]\n","SMOKE="+repr(smoke)+"\n",s)
    s=s.replace("ttf',108)","ttf',int(108*G))")
    s=s.replace("gx,gy=int(700/U),int(585/U)","gx,gy=int((36+(700-36)*G+DX)/U),int((782+(585-782)*G+DY)/U)")
    s=s.replace("tx=int(640); ty=int(770-twoW.height/2)","tx=int(36+(640-36)*G+DX); ty=int(782+(770-782)*G+DY-twoW.height/2)")
    s=s.replace("lw=840;","lw=int(840*G);")
    s=s.replace("(int(36*SS),int((782-lh/2)*SS))","(int((36+DX)*SS),int((782+DY-lh/2)*SS))")
    s=s.replace("d.text((60*SS,905*SS),","d.text(((36+(60-36)*G+DX)*SS,(782+(905-782)*G+DY)*SS),")
    s=s.replace("ttf',34*SS)","ttf',int(34*G*SS))")
    s=s.replace("im=im.resize((W*2,H*2),Image.LANCZOS)",f"im=im.resize({OUT},Image.LANCZOS)")
    s=s.replace("/tmp/art/stage1.png",f"/tmp/art/{name}.png")
    open(f'/tmp/art/{name}.py','w').write(s)
def sm(f,ox,oy):
    return [(915+(x-915)*f+(ox-915),oy+(y-1500)*f,r*f,a) for x,y,r,a in [(984,1288,9,200),(992,1258,12,190),(978,1222,16,175),(996,1182,20,150),(974,1136,25,125),(1000,1084,30,95)]]
# PS1
def top2(x): return 832-30*(x-20)/1060
P={'p1':[(20,20),(430,20),(380,430),(20,430)],'p2':[(446,20),(720,20),(760,430),(396,430)],'p3':[(736,20),(1080,20),(1080,430),(776,430)],
 'logo':[(20,478),(1080,446),(1080,786),(20,818)],
 'q1':[(20,top2(20)),(330,top2(330)),(380,1268),(20,1268)],'q2':[(346,top2(346)),(760,top2(760)),(720,1268),(396,1268)],'q3':[(776,top2(776)),(1080,top2(1080)),(1080,1268),(736,1268)]}
sc=[('p1','toasters',(225,405,65)),('p2','vacuum',(575,405,68)),('p3','duck',(930,405,52)),('q1','fridge',(165,1248,57)),('q2','speech',(570,1248,84)),('q3','smoking',(915,1228,79))]
variant('ps1',1100,1288,P,sc,sm(0.86,915,1228),0.9,0,-150,"(2200,2576)")
# DSi
W,H=1431,1007
bx=[20,483,946,1411]
def row(y0,y1,sl,names):
    out={}
    for i,n in enumerate(names):
        a=bx[i]+(0 if i==0 else 8); b=bx[i+1]-(0 if i==2 else 8)
        la=sl if i>0 else 0; rb=sl if i<2 else 0
        out[n]=[(a+la,y0),(b+rb,y0),(b-rb,y1),(a-la,y1)]
    return out
P={'logo':[(20,40),(1411,20),(1411,265),(20,285)]}
P.update(row(305,635,18,['p1','p2','p3'])); P.update(row(655,987,-18,['q1','q2','q3']))
cx=[(a+b)//2 for a,b in zip(bx,bx[1:])]
sc=[('p1','toasters',(cx[0],615,50)),('p2','vacuum',(cx[1],615,62)),('p3','duck',(cx[2],615,52)),('q1','fridge',(cx[0],967,57)),('q2','speech',(cx[1],967,78)),('q3','smoking',(cx[2],950,66))]
variant('dsi',W,H,P,sc,sm(66/92,cx[2],950),0.72,332,-625,"(1431,1007)")
