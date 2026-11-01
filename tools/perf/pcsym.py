import sys,subprocess,bisect,collections
elf,pcf=sys.argv[1],sys.argv[2]
nm=[l.split() for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm','-n','--defined-only',elf]).decode().splitlines()]
syms=[(int(a,16)&0x1fffffff,n) for a,t,n in [x for x in nm if len(x)==3] if t in 'tTwW']
addrs=[a for a,_ in syms]
c=collections.Counter(); tot=0
for l in open(pcf):
    pc,n=l.split(); pc=int(pc,16); n=int(n); tot+=n
    i=bisect.bisect_right(addrs,pc)-1
    c[syms[i][1] if i>=0 else '?']+=n
top=int(sys.argv[3]) if len(sys.argv)>3 else 40
print('total samples',tot)
for k,v in c.most_common(top): print('%6.2f%% %7d %s'%(100*v/tot,v,k))
