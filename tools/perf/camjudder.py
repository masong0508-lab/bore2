#!/usr/bin/env python3
# camjudder.py RAM ELF: the follow camera's smoothness from a -DCAM_PROBE=1 build (main.c dbg_cam: vblank, eye x, eye z at each frame).
# Speed of each frame = distance moved / vblanks it took; judder = how much that speed changes from one frame to the next.
import struct,sys,subprocess,math
ram,elf=sys.argv[1],sys.argv[2]
d=open(ram,'rb').read(); S={}
for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm',elf]).decode().splitlines():
    p=l.split()
    if len(p)==3: S[p[2]]=int(p[0],16)&0x1fffff
n=struct.unpack('<i',d[S['dbg_cam_n']:S['dbg_cam_n']+4])[0]
rows=[struct.unpack('<3h',d[S['dbg_cam']+6*i:S['dbg_cam']+6*i+6]) for i in range(min(n,512))]
for i in range(1,len(rows)):   # (the vblank count is kept in 16 bits: unwrap)
    pass
rows=[r for r in rows if r[0]>0]
# the walking part: frames where the camera moves
sp=[];dts=[]
for a,b in zip(rows,rows[1:]):
    dt=b[0]-a[0]
    if dt<=0: continue
    dist=math.hypot(b[1]-a[1],b[2]-a[2]); sp.append(dist/dt); dts.append(dt)
# frames that moved more than 40 units a vblank are jumps (a taxi, a block edge): not counted, nor the frame after
ok=[v<=40 and v>2 for v in sp]
s=[];acc=[];t=[]
for i,v in enumerate(sp):
    if ok[i]: s.append(v); t.append(dts[i])
    if i and ok[i] and ok[i-1]: acc.append(abs(v-sp[i-1]))
if len(acc)<20: print('no walk'); sys.exit()
mean=sum(s)/len(s); sd=math.sqrt(sum((x-mean)**2 for x in s)/len(s))
acc.sort()
print('frames %d, walking %d; vblanks/frame mean %.2f hist %s'%(len(rows),len(s),sum(t)/len(t),{k:t.count(k) for k in sorted(set(t))}))
print('speed units/vblank: mean %.2f  sd %.2f | change from one frame to the next: mean %.2f  95%% %.2f  max %.2f'%(mean,sd,sum(acc)/len(acc),acc[int(len(acc)*.95)],acc[-1]))
