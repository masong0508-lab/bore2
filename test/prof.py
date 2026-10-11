# prof.py <ram> <elf>: the sampling profiler's histogram, by function
import struct,subprocess,sys,collections
ram=open(sys.argv[1],'rb').read()
out=subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm','-n',sys.argv[2]],capture_output=True,text=True).stdout
syms=[];sy={}
for l in out.split('\n'):
    p=l.split()
    if len(p)==3:
        a=int(p[0][-8:],16); sy[p[2]]=a
        if p[1] in 'tTwW': syms.append((a,p[2]))
syms.sort()
o=sy['prof_hist']-0x80000000
h=struct.unpack('<1800H',ram[o:o+3600])
import bisect
keys=[a for a,_ in syms]
c=collections.Counter()
for i,n in enumerate(h):
    if not n: continue
    a=0x80010000+i*256+128
    j=bisect.bisect_right(keys,a)-1
    c[syms[j][1] if j>=0 else '?']+=n
tot=sum(c.values()); print('total',tot)
for f,n in c.most_common(30): print('%5.1f%% %s'%(100*n/tot,f))
