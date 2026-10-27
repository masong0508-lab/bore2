from eng import *
SKIN=[hexc(x) for x in('f0c8a0','dba070','b97a4c','8a5632','633a20','f4d4b8')]
def bore(sc,pos=(0,0,0),yaw=0.0,skin=1,shirt='#c0392b',pants='#2c3e6b',hair='#2a1a10',style='short',face='smile',
         armL=(0,0),armR=(0,0),elbL=0,elbR=0,legL=(0,0),legR=(0,0),kneeL=0,kneeR=0,lean=0.0,head=(0,0,0),pelvis=1.3,scale=1.0,shoe='#222222',sit=False,propL=None,propR=None,headprop=None,butt=0.0):
    sk=SKIN[skin]; sh=hexc(shirt); pa=hexc(pants); ha=hexc(hair); so=hexc(shoe)
    sc.push(ry(yaw)@np.eye(3),pos); sc.push(None,(0,0,0))
    # legs
    for side,(a,zz),kn in((-1,legL,kneeL),(1,legR,kneeR)):
        sc.push(rx(a)@rz(zz),(side*0.28,pelvis,0))
        sc.box((0,-0.33,0),(0.38,0.7,0.4),pa)
        sc.push(rx(kn),(0,-0.68,0))
        sc.box((0,-0.33,0),(0.34,0.68,0.36),pa); sc.box((0,-0.72,0.1),(0.38,0.18,0.62),so); sc.box((0,-0.82,0.1),(0.4,0.05,0.64),(236,236,230))
        sc.pop(); sc.pop()
    # torso
    sc.push(rx(lean),(0,pelvis,0))
    sc.box((0,0.52,0),(1.0,1.05,0.56),sh)
    sc.box((0,1.0,0.0),(0.46,0.1,0.58),tuple(min(255,c+40) for c in sh))
    sc.box((0,0.02,0),(0.98,0.2,0.54),pa)
    if butt:   # a blocky rear to match the rest of the model: two box cheeks under a boxer band
        sc.box((0,0.17,0),(1.02,0.12,0.58),(238,238,244)); sc.box((0,0.17,0),(1.03,0.04,0.59),(215,60,80))
        for sx in(-1,1): sc.box((sx*0.25,-0.14,-0.16*butt),(0.48,0.5,0.3+0.12*butt),pa)
    # arms
    for side,(a,zz),el in((-1,armL,elbL),(1,armR,elbR)):
        sc.push(rx(a)@rz(zz),(side*0.66,0.98,0))
        sc.box((0,-0.28,0),(0.3,0.62,0.32),sh)
        sc.push(rx(el),(0,-0.58,0))
        sc.box((0,-0.27,0),(0.27,0.56,0.29),sk); sc.box((0,-0.6,0),(0.3,0.2,0.3),sk)
        pr=propL if side<0 else propR
        if pr: sc.push(None,(0,-0.66,0.05)); pr(sc); sc.pop()
        sc.pop(); sc.pop()
    # head
    sc.box((0,1.08,0),(0.24,0.14,0.24),sk)
    sc.push(rx(head[0])@ry(head[1])@rz(head[2]),(0,1.2,0))
    sc.box((0,0.42,0),(0.86,0.84,0.8),sk)
    hy=0.42; fz=0.402
    def eye(x,lid=0.0,pup=0.0):
        sc.decal([(x-0.11,hy+0.12+0.0,fz),(x+0.11,hy+0.12,fz),(x+0.11,hy-0.04-lid*0.1,fz),(x-0.11,hy-0.04-lid*0.1,fz)],(250,250,250))
        sc.decal([(x-0.045+pup,hy+0.08,fz),(x+0.045+pup,hy+0.08,fz),(x+0.045+pup,hy-0.03-lid*0.1,fz),(x-0.045+pup,hy-0.03-lid*0.1,fz)],(25,20,30),bias=0.2)
    if face in('squint','chill'):
        for x in(-0.2,0.2):
            sc.decal([(x-0.11,hy+0.1,fz),(x+0.11,hy+0.1,fz),(x+0.11,hy+0.0,fz),(x-0.11,hy+0.0,fz)],tuple(int(c*0.75) for c in sk))
            sc.decal([(x-0.1,hy+0.02,fz),(x+0.1,hy+0.02,fz),(x+0.1,hy-0.02,fz),(x-0.1,hy-0.02,fz)],(25,20,30),bias=0.2)
    else:
        eye(-0.2,pup=0.03 if face!='dizzy' else 0); eye(0.2,pup=0.03 if face!='dizzy' else 0)
    for x,t in((-0.2,0.0),(0.2,0.0)):
        ty=hy+0.2+(0.05 if face in('shock','open') else 0)
        sc.decal([(x-0.13,ty,fz),(x+0.13,ty+(0.02 if face=='smirk' and x>0 else 0.0),fz),(x+0.13,ty+0.06,fz),(x-0.13,ty+0.06,fz)],tuple(int(c*0.5) for c in ha))
    m=hy-0.3
    if face in('smile','chill','squint'):
        sc.decal([(-0.17,m+0.07,fz),(0.17,m+0.07,fz),(0.12,m,fz),(-0.12,m,fz)],(120,40,40))
    elif face in('open','shock'):
        sc.decal([(-0.1,m+0.09,fz),(0.1,m+0.09,fz),(0.1,m-0.09,fz),(-0.1,m-0.09,fz)],(80,20,30))
    elif face=='smirk':
        sc.decal([(-0.12,m+0.02,fz),(0.17,m+0.08,fz),(0.17,m+0.04,fz),(-0.12,m-0.02,fz)],(120,40,40))
    else:
        sc.decal([(-0.14,m+0.03,fz),(0.14,m+0.03,fz),(0.14,m-0.02,fz),(-0.14,m-0.02,fz)],(120,50,50))
    sc.decal([(-0.05,hy-0.05,fz),(0.05,hy-0.05,fz),(0.04,hy-0.2,fz),(-0.04,hy-0.2,fz)],tuple(int(c*0.88) for c in sk))
    for x in(-0.3,0.3): sc.decal([(x-0.07,hy-0.12,fz),(x+0.07,hy-0.12,fz),(x+0.07,hy-0.2,fz),(x-0.07,hy-0.2,fz)],tuple(min(255,int(c*1.0+25)) if i==0 else int(c*0.88) for i,c in enumerate(sk)))
    # hair
    if style=='short': sc.box((0,0.88,-0.02),(0.92,0.22,0.86),ha); sc.box((0,0.62,-0.36),(0.9,0.5,0.16),ha)
    elif style=='bob': sc.box((0,0.88,-0.02),(0.96,0.22,0.9),ha); sc.box((-0.47,0.5,0),(0.12,0.7,0.8),ha); sc.box((0.47,0.5,0),(0.12,0.7,0.8),ha); sc.box((0,0.5,-0.4),(0.9,0.8,0.14),ha)
    elif style=='afro': sc.ell((0,0.98,-0.02),(0.72,0.6,0.66),ha,nl=6,nn=10)
    elif style=='mohawk': sc.box((0,1.0,0),(0.16,0.3,0.74),ha)
    elif style=='bald': pass
    elif style=='long': sc.box((0,0.88,-0.02),(0.92,0.22,0.86),ha); sc.box((-0.47,0.3,-0.05),(0.12,1.1,0.7),ha); sc.box((0.47,0.3,-0.05),(0.12,1.1,0.7),ha); sc.box((0,0.3,-0.4),(0.9,1.1,0.14),ha)
    elif style=='pony': sc.box((0,0.88,-0.02),(0.92,0.22,0.86),ha); sc.box((0,0.6,-0.5),(0.2,0.5,0.18),ha)
    if headprop: headprop(sc)
    sc.pop()
    sc.pop(); sc.pop(); sc.pop()
