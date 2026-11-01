import subprocess,collections,sys
elf,pcf=sys.argv[1],sys.argv[2]; fns=sys.argv[3:]
pcs=[(int(l.split()[0],16),int(l.split()[1])) for l in open(pcf)]
nm=[l.split() for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm','-n','-S','--defined-only',elf]).decode().splitlines()]
rng={}
for x in nm:
    if len(x)==4 and x[2] in 'tT': rng[x[3]]=(int(x[0],16)&0x1fffffff,(int(x[0],16)&0x1fffffff)+int(x[1],16))
for fn in fns:
    lo,hi=rng[fn]; sel=[(p,n) for p,n in pcs if lo<=p<hi]
    if not sel: print(fn,'none'); continue
    out=subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-addr2line','-e',elf]+[hex(p|0x80000000) for p,_ in sel],capture_output=True,text=True).stdout.splitlines()
    c=collections.Counter()
    for (p,n),l in zip(sel,out): c[l.split('/')[-1]]+=n
    tot=sum(c.values()); print(fn,tot)
    for l,n in c.most_common(6): print('   %5.1f%% %s'%(100*n/tot,l))
