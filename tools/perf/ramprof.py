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
if nn and 'dbg_lg_max' in S:
    mx=rd('dbg_lg_max',2); h=rd('dbg_cit_hist',12); t=sum(h) or 1
    print('logic worst frame hb: citizens %d  sim_step %d | citizens a frame, 20-hb bins: %s'%(mx[0],mx[1],' '.join('%d'%x for x in h)))
    acc=0; p95=0
    for i,c in enumerate(h):
        acc+=c
        if acc>=0.95*t: p95=(i+1)*20; break
    print('   citizens: 95%% of frames under %d hb'%p95)
if nn and 'dbg_ctier' in S:
    c=rd('dbg_ctier',8); print('townsfolk a frame: busy with you %.1f | know you %.1f | strangers near %.1f | far %.1f | thinking calls %.1f | frames spaced out %d, put off by the clock %d'%(c[0]/nn,c[1]/nn,c[2]/nn,c[3]/nn,c[4]/nn,c[6],c[7]))
if nn:
    w=rd('dbg_sum_w',4); print('world parts hb: layer0 tiles %.0f | upper storeys %.0f | roofs %.0f | skyline boxes (cumulative from prof start) %.0f'%tuple(x/nn for x in w))
if nn:
    w=rd('dbg_sum_w',7); print('far pass hb: land %.0f | far buildings %.0f | trees %.0f'%tuple(x/nn for x in w[4:7]))
if nn:
    pn=rd('dbg_sum_pn',8); print('prims per frame [floor,walls,items,roofs/far,sims,citfull,citlod,x]',[round(x/nn) for x in pn], 'total',round(sum(pn)/nn))
if nn:
    print('mean shed level %.2f (0 none .. 4 most), mean detail radius %.1f tiles'%(rd('dbg_sum_shed')[0]/nn, rd('dbg_sum_r')[0]/nn))
if nn and 'dbg_hl' in S:
    h=rd('dbg_hl',6); print('speed helper: steps given up %d, given back %d | aiming at 30 in %.0f%% of frames | mean ladder step %.2f, crowd step %.2f'%(h[0],h[1],100.0*h[2]/nn,h[3]/nn,h[4]/nn))
if 'dbg_per_hist' in S:
    p=rd('dbg_per_hist',6); t=sum(p) or 1
    print('frame periods (vblanks each life frame really took) 1..5+: %s  | took 4 or more: %.1f%%  2 or fewer: %.1f%%'%(' '.join(str(x) for x in p[1:]),100*sum(p[4:])/t,100*sum(p[:3])/t))
    e=rd('dbg_edge',12); print('   latest flip (hb) seen with 1..5 vblanks: %s | earliest: %s'%(e[1:6],e[7:12]))
    if 'dbg_vbs' in S:
        v=rd('dbg_vbs',4)
        if v[0] and v[1]: print('   by the clock (like for like under AB_LOCK4): fps from the need %.2f, from the flip (with the spare-time work) %.2f; spare-time work pushed %d frames past their vblank'%(60.0*t/v[0],60.0*t/v[1],v[2]))
    if 'dbg_gw' in S:
        g=rd('dbg_gw',3); print('   the flip waited for the GPU: mean %.1f hb, in %d frames (over 2 hb), at most %d hb'%(g[0]/t,g[1],g[2]))
if 'dbg_slow' in S:
    n=min(16,rd('dbg_slow_n')[0]); sl=[rd('dbg_slow',8,32*i) for i in range(n)]
    for s in sorted(sl,key=lambda s:s[7]): print('   slow frame %6d: cpu %d need %d logic %d world %d people %d hud+bub %d roofs/far %d'%(s[7],s[0],s[1],s[2],s[3],s[4],s[5],s[6]))
if nn and 'dbg_sim_hist' in S:
    h=rd('dbg_sim_hist',12); print('household (sim_step) a frame, 40-hb bins: %s'%' '.join(str(x) for x in h))
    p=rd('dbg_sum_ppl',4); print('people a frame hb: the crowd %.0f (of it spare-time work %.0f) | the household %.0f | traffic %.0f'%(p[0]/nn,p[1]/nn,p[2]/nn,p[3]/nn))
    if 'dbg_rn' in S: r=rd('dbg_rn',3); print('crowd sprite renders: %d in spare time, forced (no time left): %d first sprites, %d stale'%r)
if 'dbg_end_hist' not in S: sys.exit(0)
h=rd('dbg_end_hist',24)
tot=sum(h) or 1
print('frame cost WITHOUT the work done only in its spare time (the real need), 50-hb bins; 786 = 3 vblanks (20 fps), 524 = 2 vb (30 fps):')
acc=0
for i,c in enumerate(h):
    acc+=c
    if c: print('   %4d-%4d hb %4d frames  cum %.0f%%'%(i*50,i*50+49,c,100*acc/tot))
under=sum(h[:16]); print('   share of frames that fit 3 vblanks (<=785): %.0f%%   (mean need %.0f hb)'%(100*under/tot, sum((i*50+25)*c for i,c in enumerate(h))/tot))

if nn:
    f=rd('dbg_sum_far',6); print('far pass per frame: quads %.0f (clipped %.0f) cone tests %.0f | prims land %.0f buildings %.0f trees %.0f'%tuple(x/nn for x in f))

if nn:
    f=rd('dbg_sum_miss',4); print('per frame: height-corner misses %.0f | block hill misses %.0f | bm_terrain calls %.0f | far ring hb %.0f'%tuple(x/nn for x in f))

if nn:
    print('far roads hb per frame %.0f'%(rd('dbg_sum_roads')[0]/nn))

if 'dbg_ws' in S:
    w=rd('dbg_ws',8); print('window shift worst hb: keep+maps %d | ao+roofs move %d | new blocks %d | their shading %d | keep alone %d, ground maps %d, storeys %d (storey slides %d)'%w)
    print('window moves: %d, in a frame\'s spare time %d, worst %d hb'%(rd('dbg_shifts')[0], rd('dbg_ws_spare')[0], rd('dbg_shift_hb')[0]))
if 'gfx_peak_bytes' in S: print('packet buffer peak bytes this run: %d (of %d)'%(rd('gfx_peak_bytes')[0], 58*1024))
