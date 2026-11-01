# citprobe.py RAM ELF: the townsfolk over a run (a CIT_PROBE build): a snapshot every 512 logic steps, and the run's counters
import struct,sys,subprocess
ram,elf=sys.argv[1],sys.argv[2]
d=open(ram,'rb').read(); S={}
for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm',elf]).decode().splitlines():
    p=l.split()
    if len(p)==3: S[p[2]]=int(p[0],16)&0x1fffff
def rd(n,k=1,fmt='i',sz=4): a=S[n]; return struct.unpack('<%d%s'%(k,fmt),d[a:a+sz*k])
cc=rd('dbg_cc',16); n=rd('dbg_cpr_n')[0]; r=rd('dbg_cpr',24*24,'H',2)
names=['calls','steps lived','route plans','chores','visits','decide changes','snaps','moved','known calls','lod builds','greets','max calls/frame','plans deferred','known taken','evictions','steps dropped']
print('counters: '+', '.join('%s %d'%(names[i],cc[i]) for i in range(16) if cc[i] or i<10))
md=['IDLE','WALK','CHAT','CHASE','KO','GONE','SLEEP','GOHOME','LEAVE','CHORE','VISIT','PARTY','STOP','AWAY']
if 'dbg_cm' in S:
    cm=rd('dbg_cm',32); md0=['IDLE','WALK','CHAT','CHASE','KO','GONE','SLEEP','GOHOME','LEAVE','CHORE','VISIT','PARTY','STOP','AWAY','GREET','?']
    tot=sum(cm[0::2]) or 1
    print('steps lived by mode (share, ground per step): '+'  '.join('%s %.0f%% %.2f'%(md0[m],100*cm[2*m]/tot,cm[2*m+1]/max(1,cm[2*m])) for m in range(16) if cm[2*m]))
if 'dbg_pm' in S:
    pm=rd('dbg_pm',32); print('route searches by mode (found no way): '+'  '.join('%s %d (%d)'%(md0[m],pm[2*m],pm[2*m+1]) for m in range(16) if pm[2*m]))
if 'dbg_wk' in S:
    w=rd('dbg_wk',4); print('walks: begun %d, ended by the time %d, ended held up %d, steps held up %d (%.1f a walk)'%(w[0],w[1],w[2],w[3],w[3]/max(1,w[0])))
print('  min day(+256: you indoors) live in  bed  rel+ known chores decide moved/256 | '+' '.join(m[:5] for m in md))
for k in range(n):
    x=r[k*24:(k+1)*24]
    print('%5d %3d %4d %3d %4d %5d %5d %6d %6d %8d | '%(x[0],x[23],x[1],x[16],x[17],x[18],x[22],x[19],x[20],x[21])+' '.join('%5d'%x[2+i] for i in range(14)))
