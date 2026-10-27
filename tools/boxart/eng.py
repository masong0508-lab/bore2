import numpy as np, math
from PIL import Image, ImageDraw
def rx(a): c,s=math.cos(a),math.sin(a); return np.array([[1,0,0],[0,c,-s],[0,s,c]])
def ry(a): c,s=math.cos(a),math.sin(a); return np.array([[c,0,s],[0,1,0],[-s,0,c]])
def rz(a): c,s=math.cos(a),math.sin(a); return np.array([[c,-s,0],[s,c,0],[0,0,1]])
def hexc(h): h=h.lstrip('#'); return tuple(int(h[i:i+2],16) for i in (0,2,4))
class Scene:
    def __init__(s): s.faces=[]; s.stack=[(np.eye(3),np.zeros(3))]
    def push(s,R=None,t=(0,0,0)):
        R0,t0=s.stack[-1]; R=np.eye(3) if R is None else R
        s.stack.append((R0@R, R0@np.array(t,float)+t0))
    def pop(s): s.stack.pop()
    def W(s,p): R,t=s.stack[-1]; return (R@np.array(p,float).T).T+t if np.ndim(p)>1 else R@np.array(p,float)+t
    def add(s,quads,color,cen,outline=True,bias=0.0):
        cen=s.W(cen)
        for q in quads: s.faces.append((s.W(np.array(q,float)),color,cen,outline,bias))
    def box(s,c,sz,color,**k):
        x,y,z=c; a,b,d=[v/2 for v in sz]
        V=[(x+i*a,y+j*b,z+l*d) for i in(-1,1) for j in(-1,1) for l in(-1,1)]
        idx=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
        s.add([[V[i] for i in f] for f in idx],color,c,**k)
    def ell(s,c,r,color,nl=6,nn=10,**k):
        x,y,z=c; P=[]
        for i in range(nl+1):
            th=math.pi*i/nl
            P.append([(x+r[0]*math.sin(th)*math.cos(2*math.pi*j/nn),y+r[1]*math.cos(th),z+r[2]*math.sin(th)*math.sin(2*math.pi*j/nn)) for j in range(nn)])
        q=[]
        for i in range(nl):
            for j in range(nn):
                j2=(j+1)%nn; q.append([P[i][j],P[i][j2],P[i+1][j2],P[i+1][j]])
        s.add(q,color,c,**k)
    def cyl(s,c,r,h,color,n=10,axis='y',**k):
        x,y,z=c; q=[]
        def pt(a,t):
            ca,sa=math.cos(a)*r,math.sin(a)*r
            return (x+ca,y+t,z+sa) if axis=='y' else (x+t,y+ca,z+sa) if axis=='x' else (x+ca,y+sa,z+t)
        for j in range(n):
            a0,a1=2*math.pi*j/n,2*math.pi*(j+1)/n
            q.append([pt(a0,-h/2),pt(a1,-h/2),pt(a1,h/2),pt(a0,h/2)])
        q.append([pt(2*math.pi*j/n,h/2) for j in range(n)]); q.append([pt(2*math.pi*j/n,-h/2) for j in range(n)][::-1])
        s.add(q,color,c,**k)
    def decal(s,quad,color,bias=0.15):   # flat paint on a surface, no outline, drawn on top
        c=np.mean(np.array(quad,float),axis=0); s.add([quad],color,c,outline=False,bias=bias)
    def render(s,d,ox,oy,scale,light=(-0.45,0.75,0.5),D=14.0,out=(28,16,32),ow=2):
        L2=np.array((0.6,0.1,0.5)); L2=L2/np.linalg.norm(L2)
        L=np.array(light); L/=np.linalg.norm(L); items=[]
        for v,col,cen,outl,bias in s.faces:
            n=np.cross(v[1]-v[0],v[2]-v[0]); ln=np.linalg.norm(n)
            if ln<1e-9: continue
            n/=ln; fc=v.mean(axis=0)
            if outl:
                if np.dot(n,fc-cen)<0: n=-n
                if n[2]<=0.02: continue
            sh=0.42+0.5*max(0,float(np.dot(n,L)))+0.18*max(0,float(np.dot(n,L2)))
            items.append((fc[2]+bias, v, tuple(int(min(255,c*(0.5+0.6*sh))) for c in col) if outl else col, outl))
        items.sort(key=lambda t:t[0])
        for z,v,col,outl in items:
            pts=[(ox+scale*p[0]*D/(D-p[2]), oy-scale*p[1]*D/(D-p[2])) for p in v]
            d.polygon(pts,fill=col)
            if outl: d.line(pts+[pts[0]],fill=out,width=ow,joint='curve')
