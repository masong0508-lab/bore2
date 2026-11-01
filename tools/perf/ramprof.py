import struct,sys,subprocess
ram,elf=sys.argv[1],sys.argv[2]
d=open(ram,'rb').read()
S={}
for l in subprocess.check_output(['/opt/mipsel/bin/mipsel-none-elf-nm',elf]).decode().splitlines():
    p=l.split()
    if len(p)==3: S[p[2]]=int(p[0],16)&0x1fffff
def rd(n,k=1,off=0):
    a=S[n]+off; return struct.unpack('<%di'%k,d[a:a+4*k])
print('avg cpu hb %.0f  world %.0f  sims(prof4) %.0f'%(rd('avg_cpu16')[0]/16,rd('avg_world16')[0]/16,rd('avg_hh16')[0]/16))
print('prof [floor,walls,items,roofs/far,sims,citfull,citlod,x]',[round(x/16) for x in rd('avg_prof16',8)])
h=rd('hist_cpu',5); t=sum(h) or 1
print('frames by cost (<1vb,<2,<3,<4,>=4 vb): ',h,' mean fps ~ %.1f'%(60.0*t/sum((i+1 if i<4 else 5)*c for i,c in enumerate(h))))
n=rd('dbg_slow_n')[0]; print('slow n',n)
nn=rd('dbg_life_n')[0]
if nn:
    vb=rd('dbg_life_vb')[0]; sc=rd('dbg_sum_cpu')[0]; sp=rd('dbg_sum_prof',8)
    print('WHOLE RUN: %d frames in %d vblanks = %.2f fps; mean cpu %.0f hb; mean prof %s'%(nn,vb,60.0*nn/max(vb,1),sc/nn,[round(x/nn) for x in sp]))
if nn:
    pt=rd('dbg_sum_part',6); print('mean parts hb: update(all game logic+) %.0f  of which logic %.0f | world %.0f | sims %.0f | bubbles %.0f | hud %.0f'%tuple(x/nn for x in pt))
if nn:
    print('residual (sky, audio, pad, packet setup, flip prep): %.0f hb'%((sc-pt[0]-pt[2]-pt[3]-pt[4]-pt[5])/nn))
if nn:
    lg=rd('dbg_sum_lg',4); print('logic per frame hb: sim_step %.0f  citizens %.0f  traffic %.0f   (avg steps/frame %.2f)'%(lg[0]/nn,lg[1]/nn,lg[2]/nn,lg[3]/nn))
if nn:
    w=rd('dbg_sum_w',4); print('world parts hb: layer0 tiles %.0f | upper storeys %.0f | roofs %.0f | skyline boxes (cumulative from prof start) %.0f'%tuple(x/nn for x in w))
if nn:
    w=rd('dbg_sum_w',7); print('far pass hb: land %.0f | far buildings %.0f | trees %.0f'%tuple(x/nn for x in w[4:7]))
if nn:
    pn=rd('dbg_sum_pn',8); print('prims per frame [floor,walls,items,roofs/far,sims,citfull,citlod,x]',[round(x/nn) for x in pn], 'total',round(sum(pn)/nn))
if nn:
    print('mean shed level %.2f (0 none .. 4 most), mean detail radius %.1f tiles'%(rd('dbg_sum_shed')[0]/nn, rd('dbg_sum_r')[0]/nn))
if 'dbg_end_hist' not in S: sys.exit(0)
h=rd('dbg_end_hist',24)
tot=sum(h) or 1
print('frame cost WITHOUT the spare-time sprite fill (the real need), 50-hb bins; 786 = 3 vblanks (20 fps), 524 = 2 vb (30 fps):')
acc=0
for i,c in enumerate(h):
    acc+=c
    if c: print('   %4d-%4d hb %4d frames  cum %.0f%%'%(i*50,i*50+49,c,100*acc/tot))
under=sum(h[:16]); print('   share of frames that fit 3 vblanks (<=785): %.0f%%'%(100*under/tot))

if nn:
    f=rd('dbg_sum_far',6); print('far pass per frame: quads %.0f (clipped %.0f) cone tests %.0f | prims land %.0f buildings %.0f trees %.0f'%tuple(x/nn for x in f))
