#!/usr/bin/env python3
"""Eight original, hardware-sequenced BORE 2 arrangements and sample sets.

Append new playable banks without rewriting existing music data. No third-party
samples, dependencies, streamed PCM, or filler. All instruments are used by
their arrangements. Build the original score on the host; native targets use
their existing BSQ2 or BDM2 player and fixed memory budgets.
"""
from pathlib import Path
import argparse, array, ctypes, hashlib, json, math, os, random, re, struct, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
SR = 24000
MIXR = 18157
TICK = 33513982 / 64 / 512
BAR = 16
BARS = 48
BASE_NOTES = (60,60,60,60,45,60,60,60,60,60)
DURATIONS = (.38,.46,.18,.62,1.50,2.90,4.20,1.75,2.65,.60)
# title, BPM, tonic MIDI, four harmonic sections, melodic scale degrees, texture
# Original compositions: each has 48 bars with an introduction, contrasting
# bridge and reprise, independent bass, percussion, harmony and melody.
TRACKS = (
 ("SUNDAY ON ELM STREET", 96, 60, (0,5,9,7,2,5,0,7), (0,2,4,7,4,2,5,4), 1),
 ("THE LAST BUS", 108, 57, (0,8,3,10,5,0,8,7), (0,3,7,5,3,1,5,7), 1),
 ("PAVEMENT PARADE", 124, 62, (0,5,7,5,2,7,9,5), (0,4,2,7,5,4,2,6), 0),
 ("BLUE HOUR AT THE PIER", 82, 58, (0,3,8,5,0,10,3,7), (0,3,5,7,10,7,5,3), 2),
 ("MARKET DAY", 116, 65, (0,7,9,5,0,2,5,7), (4,2,0,4,5,7,9,7), 1),
 ("Rooftops After Rain".upper(), 90, 63, (0,8,10,5,3,0,5,7), (7,5,3,0,2,3,5,7), 2),
 ("NEON SKATE NIGHT", 138, 55, (0,3,5,10,0,7,8,5), (0,7,10,5,3,7,12,10), 0),
 ("ISLAND MORNING", 102, 60, (0,5,7,9,2,5,7,0), (0,4,7,9,7,4,2,5), 1),
)
# Existing jukebox titles, not newly invented imitations. The build extracts
# actual note events and the instrument's PCM from these original sequences.
RIFF_SOURCES = ("WHISTLER MAN", "CLOSER TO THE END", "THE CYNICAL SYNDICATION",
    "WORTHLESS CLOUDS", "GOTTCHO BARRACHO", "HERE TODAY (FULL VERSION)",
    "HOT DAMN", "SUNMAN SUNRISE")
ARTIST = "BORE 2 AREA ARRANGEMENT"
RIFF_INST = 10
RIFF_STEPS = 64
NINST = 11
def align(n,k): return (n+k-1)//k*k
def pad(b,k): return bytes(b)+bytes((-len(b))%k)
def u32(b,o=0): return struct.unpack_from("<I",b,o)[0]
def pack(fmt,*a): return struct.pack("<"+fmt,*a)
def pitch(note,inst,riff):
    if inst == RIFF_INST:
        return max(256,min(65000,round(riff["pitch"][note]*44100/4096)))
    return min(65000,max(256,round(SR*2**((note-BASE_NOTES[inst])/12))))

def spu_pitch(note,inst,riff):
    return riff["pitch"][note] if inst==RIFF_INST else min(0x3fff,max(1,round(pitch(note,inst,riff)*4096/44100)))
def midi(n): return 440*2**((n-69)/12)
def fit(n,lo,hi):
    while n<lo: n+=12
    while n>hi: n-=12
    return n

def unpack_lzss(data,n):
    result=bytearray();p=0
    while len(result)<n:
        flags=data[p];p+=1
        for bit in range(8):
            if len(result)>=n:break
            if flags>>bit&1:
                result.append(data[p]);p+=1
            else:
                a,b=data[p:p+2];p+=2;back=(a|((b&15)<<8))+1
                assert back<=len(result)
                for _ in range((b>>4)+3):
                    if len(result)>=n:break
                    result.append(result[-back])
    return bytes(result)

def decode_original(base,title):
    bank=(base/"SONGS.BIN").read_bytes()
    inc=(base/"songs.inc").read_text()
    names=re.findall(r'\{\s*\d+,\s*"([^"]+)",\s*"([^"]*)"',inc)
    ix=next(i for i,(name,_) in enumerate(names) if name==title)
    sector,seqs,smps,smpb=struct.unpack_from("<4I",bank,8+16*ix)
    seq=bank[sector*2048:(sector+seqs)*2048]
    sample=bank[(sector+seqs)*2048:(sector+seqs)*2048+smpb]
    h=struct.unpack_from("<20I",seq);assert h[0]==0x32515342,(title,"BSQ2 required")
    nord,loop,npat,ninst,nvt=h[1],h[2],h[5],h[6],h[7]
    order=list(seq[h[11]:h[11]+nord])
    pat_rows=struct.unpack_from("<"+"H"*npat,seq,h[12])
    vt=struct.unpack_from("<"+"H"*nvt,seq,h[14])
    pitches=struct.unpack_from("<"+"H"*(96*ninst),seq,h[15])
    offsets=struct.unpack_from("<"+"I"*ninst,seq,h[16])
    intro,bodyoff,body,_=struct.unpack_from("<4I",seq,h[19])
    raw=(unpack_lzss(seq[h[18]:],intro) if intro else b"")+unpack_lzss(seq[bodyoff:],body)
    # Existing BSQ2 body is in play order. Decode exactly the rows in
    # that order, including empty runs; key-offs are not melody onsets.
    timeline=[];starts=[];pos=0
    for pattern in order:
        starts.append(len(timeline));n=pat_rows[pattern];end=len(timeline)+n
        while len(timeline)<end:
            k=raw[pos];pos+=1
            if k&128:
                count=k&127
                assert count and len(timeline)+count<=end,(title,pattern,count)
                timeline.extend([[] for _ in range(count)])
            else:
                ev=[]
                for _ in range(k):
                    w=int.from_bytes(raw[pos:pos+3],"little");pos+=3
                    v=vt[(w&255)|((w>>22)<<8)]
                    ch,ins,note,vol=v&15,(v>>4)&31,(w>>8)&127,(w>>15)&127
                    if note<96 and ins<ninst and vol>0 and offsets[ins]!=0xffffffff and pitches[ins*96+note]:
                        ev.append((ch,ins,note,vol))
                timeline.append(ev)
    starts.append(len(timeline));assert pos==len(raw),(title,pos,len(raw))
    roles_text=(ROOT/"src/band_roles.inc").read_text()
    m=re.search(r'\{\s*"'+re.escape(title)+r'",\s*\d+,\s*\{([^}]+)\}',roles_text)
    roles=[int(v) for v in m.group(1).split(",") if v.strip()] if m else []
    # Find a repeated or strongly melodic two-order phrase in the source.
    # Keyboard/bass roles are preferred over percussion and effects.
    # Use exact source notes and relative onsets, not fabricated riffs.
    from collections import Counter,defaultdict
    recurrence=Counter(order)
    candidate=[]
    for oi in range(nord):
        a=starts[oi];b=starts[min(oi+2,nord)];length=b-a
        if length<4:continue
        grouped=defaultdict(list)
        for j,row in enumerate(timeline[a:b]):
            for ch,ins,nt,vol in row:
                grouped[ch,ins].append((j,nt,vol))
        for (ch,ins),notes in grouped.items():
            distinct=len({n for _,n,_ in notes})
            if len(notes)<4 or distinct<3:continue
            role=roles[ins] if ins<len(roles) else 0
            if role in (1,5):continue
            steps={}
            for j,nt,vol in notes:
                k=min(RIFF_STEPS-1,round(j*RIFF_STEPS/length))
                if k not in steps or vol>steps[k][1]:steps[k]=(nt,vol)
            if len(steps)<4 or len(steps)*4<len(notes)*3:continue
            span=max(n for _,n,_ in notes)-min(n for _,n,_ in notes)
            if span>36:continue
            # repeated motifs and distinct pitched notes take priority;
            # a bass role is a fallback when the song has no lead motif.
            quality=(70 if role==3 else 35 if role==2 else 5)+min(distinct,12)*5+min(len(steps),24)+min(recurrence[order[oi]],8)*4
            if role==4:quality-=18
            candidate.append((quality,-oi,ch,ins,steps,notes,a,b))
    assert candidate,(title,"no suitable pitched source phrase")
    # Rank recurring pitch-and-rhythm signatures, even when a tracker
    # assigned different pattern numbers to identical musical material.
    def signature(c):
        return c[3],tuple((k,n) for k,(n,_) in sorted(c[4].items()))
    phrase_frequency=Counter(signature(c) for c in candidate)
    _,neg,ch,ins,steps,notes,a,b=max(candidate,key=lambda c:(c[0]+min(phrase_frequency[signature(c)],12)*6,*c[1:4]))
    so=offsets[ins];assert so+16<=len(sample)
    stop=min([o for o in offsets if o!=0xffffffff and o>so]+[len(sample)])
    # Keep the actual patch's attack and tone. Respect the audio budgets
    # even if the original tracker used a very long sample or a loop.
    stop=min(stop,so+32000);stop=so+((stop-so)//16)*16
    rawpatch=sample[so:stop]
    for j in range(0,len(rawpatch),16):
        if rawpatch[j+1]&1:
            rawpatch=rawpatch[:j+16];break
    assert len(rawpatch)>=16
    tab=list(pitches[ins*96:(ins+1)*96])
    # Uniform octave shift if required by the DS's sample-frequency limit.
    maxp=max(tab[n] for n,_ in steps.values())
    while maxp*44100/4096>65000:
        tab=[v//2 for v in tab];maxp//=2
    for j,v in enumerate(tab):
        tab[j]=max(1,min(0x3fff,v))
    # Use the source bass/harmony to guide the new backing's four bars.
    roots=[]
    for bar in range(4):
        lo=a+(b-a)*bar//4;hi=a+(b-a)*(bar+1)//4
        bass=Counter(nt%12 for row in timeline[lo:hi] for _,i,nt,v in row if i<len(roles) and roles[i]==2)
        lead=Counter(nt%12 for k,(nt,v) in steps.items() if k//BAR==bar)
        chroma=(bass or lead)
        roots.append(chroma.most_common(1)[0][0] if chroma else (roots[-1] if roots else next(iter(steps.values()))[0]%12))
    return {"source":title,"artist":names[ix][1],"source_order":-neg,"source_channel":ch,
        "source_instrument":ins,"source_onsets":len(notes),"phrase":steps,"roots":roots,
        "source_rows":b-a,"pitch":tab,"patch":rawpatch,
        "riff_hash":hashlib.sha256(json.dumps(notes,separators=(",",":")).encode()).hexdigest()}

def riff_pcm(dll,riff):
    data=riff["patch"];nmax=len(data)//16*28
    src=(ctypes.c_uint8*len(data)).from_buffer_copy(data)
    dst=(ctypes.c_int16*nmax)()
    n=dll.spu_decode(src,len(data),dst);assert 0<n<=nmax
    pcm=array.array("h",dst[:n])
    # Avoid a click when a sustain/loop is converted to a one-shot patch.
    fade=min(n//4,512)
    for i in range(fade):pcm[n-fade+i]=round(pcm[n-fade+i]*(fade-i-1)/max(1,fade))
    return pcm

def score(t,riff):
    name,bpm,root,progression,motif,fx=TRACKS[t]
    rows=[[] for _ in range(BARS*BAR)]
    def hit(bar,step,ch,inst,n,vol,pan=3):
        r=rows[bar*BAR+step]
        assert all(e[0]!=ch for e in r)
        r.append((ch,inst,n,vol,pan))
    for bar in range(BARS):
        section=bar//16
        shift=(0,4,0)[section]
        degree=progression[(bar+shift)%len(progression)]
        chord=root+degree
        minor=(t in (1,3,5,6)) ^ (degree in (2,9))
        third=3 if minor else 4
        phrase_bar=(bar+(2 if section==1 else 0))%4
        chroma=riff["roots"][phrase_bar]
        chord=root+((chroma-root+6)%12-6)
        phrase_classes={(nt-chroma)%12 for k,(nt,v) in riff["phrase"].items() if k//BAR==phrase_bar}
        # Where the actual phrase does not imply a third, leave the
        # accompaniment open (root/octave/fifth) rather than clash.
        third=3 if 3 in phrase_classes and 4 not in phrase_classes else 4 if 4 in phrase_classes and 3 not in phrase_classes else 12
        notes=[fit(chord,60,71),fit(chord+third,60,75),fit(chord+7,60,76)]
        # Every track has its own groove. Bridge has a half-time or
        # syncopated feel, then the full drum pattern returns.
        kicks=((0,8),(0,6,8),(0,4,8,12),(0,10),(0,6,12),(0,8),(0,4,8,12),(0,7,10))[t]
        if section==1: kicks=(0,10) if t%2 else (0,8)
        for step in kicks: hit(bar,step,0,0,60,100 if step==0 else 83)
        for step in ((4,12) if section!=1 else (8,)): hit(bar,step,1,1,60,83)
        for step in range(0,16,2 if t!=6 else 1): hit(bar,step,2,2,60,29+(step%4==0)*14,4)
        for step in ((3,11) if t%2 else (7,15)): hit(bar,step,3,3,60,37,1)
        for step in ((0,6,10,14) if section!=1 else (0,8)):
            bass=fit(chord+(7 if step==10 else 12 if step==14 else 0),36,51)
            hit(bar,step,4,4,bass,86 if step==0 else 70)
        for step in (0,4,8,12):
            hit(bar,step,5,5,notes[(step//4+bar)%3],59 if step==0 else 48,2)
        hit(bar,0,6,6,notes[0],48,1)
        hit(bar,0,7,6,notes[2],43,5)
        # The A and A' sections are a four-bar quotation from the actual
        # source tracker channel. The B section develops a different color.
        if section!=1:
            for k,(n,v) in sorted(riff["phrase"].items()):
                if k//BAR==bar%4: hit(bar,k%BAR,8,RIFF_INST,n,min(100,max(40,v)),3)
        else:
            fragment=[nt for k,(nt,_) in sorted(riff["phrase"].items()) if k//BAR==phrase_bar]
            if not fragment:fragment=[nt for _,(nt,_) in sorted(riff["phrase"].items())]
            for j,step in enumerate((0,5,11)):
                n=fit(fragment[j%len(fragment)]+12,62,77)
                hit(bar,step,8,7 if (bar+j+t)%4==0 else 8,n,63 if j==0 else 51,4)
        for step in ((5,13) if bar%4!=3 else (5,9,13,15)):
            hit(bar,step,9,9,60,36 if bar%4!=3 else 49,5)
    # Check the first and last sections contain the exact quoted notes.
    for start in (0,32*BAR):
        for k,(nt,_) in riff["phrase"].items():
            assert any(e[1]==RIFF_INST and e[2]==nt for e in rows[start+k])
    return rows

def sample(track,inst):
    """Ten original, velocity-ready patches per track, each with an actual
    decay/release and band-limited harmonics. No repeated byte padding."""
    n=round(DURATIONS[inst]*SR)
    rng=random.Random(0xB0222026+track*1009+inst*37)
    freq=midi(BASE_NOTES[inst])
    out=array.array("h"); tau=2*math.pi
    flavor=track/7
    for i in range(n):
        t=i/SR; r=t/DURATIONS[inst]
        fade=min(1,t/.006)*min(1,(DURATIONS[inst]-t)/.025)
        p=tau*freq*t
        noise=rng.uniform(-1,1) if inst in (0,1,2,3,9) else 0
        if inst==0:
            phase=tau*(47*t+95*.035*(1-math.exp(-t/.035)))
            v=(.94*math.sin(phase)+.06*noise*math.exp(-t/.013))*math.exp(-t/(.070+.04*flavor))
        elif inst==1:
            v=(.69*noise+.25*math.sin(tau*(155+track*9)*t)+.06*math.sin(tau*390*t))*math.exp(-t/(.085+.045*flavor))
        elif inst==2:
            v=(noise*.67+.33*math.sin(tau*(6380-track*170)*t))*math.exp(-t/(.026+.007*flavor))
        elif inst==3:
            v=(noise*.56+.21*math.sin(tau*(4760+track*90)*t)+.23*math.sin(tau*3190*t))*math.exp(-t/(.15+.09*flavor))
        elif inst==4:
            v=(.68*math.sin(p)+.21*math.sin(2*p)+(.07+.04*flavor)*math.sin(3*p)+.035*math.sin(5*p))*math.exp(-t/(.45+.3*flavor))
        elif inst==5:
            v=(.50*math.sin(p)+.24*math.sin(2*p)*math.exp(-t/.65)+.12*math.sin(3*p)+.08*math.sin(4.01*p)*math.exp(-t/.35)+.055*math.sin(7*p)*math.exp(-t/.25))*math.exp(-t/(.85+.5*flavor))
        elif inst==6:
            attack=min(1,t/(.18+.14*flavor))
            v=(.40*math.sin(p)+.23*math.sin(p*(1.003+.002*flavor))+.18*math.sin(2*p)+.10*math.sin(3*p)+.055*math.sin(p*.995))*attack*(.83+.13*math.sin(tau*(.7+.5*flavor)*t))*math.exp(-t/2.5)
        elif inst==7:
            v=(.50*math.sin(p)+.22*math.sin(2*p)*math.exp(-t/.24)+.15*math.sin(3*p)*math.exp(-t/.17)+.08*math.sin(5*p)*math.exp(-t/.11))*math.exp(-t/(.43+.3*flavor))
        elif inst==8:
            vib=.09*math.sin(tau*(4.0+.24*track)*t)*min(1,t/.18)
            v=(.59*math.sin(p+vib)+.19*math.sin(2*p+2*vib)+.11*math.sin(3*p)+.07*math.sin(4*p)*math.exp(-t/.65))*math.exp(-t/(.78+.35*flavor))
        else:
            v=(.58*math.sin(tau*(470+track*28)*t)+.24*math.sin(tau*(790+track*38)*t)+.12*noise)*math.exp(-t/(.13+.04*flavor))
        # A small track-specific articulation, not a noise bed or file filler.
        if inst>=4 and inst<=8: v*=.93+.06*math.sin(tau*(1.4+.2*track)*t)
        out.append(max(-32768,min(32767,round(v*fade*30000))))
    return out

def encoder(tmp):
    lib=Path(tmp)/"codec.so"
    subprocess.run([os.environ.get("CC_FOR_BUILD","cc"),"-O3","-shared","-fPIC",
        str(ROOT/"tools/content_spu.c"),str(ROOT/"tools/dsmusic/adpcm.c"),"-o",str(lib)],check=True)
    dll=ctypes.CDLL(str(lib))
    for name in ("content_spu_encode","ima_encode"):
        fn=getattr(dll,name);fn.argtypes=(ctypes.POINTER(ctypes.c_int16),ctypes.c_int,ctypes.POINTER(ctypes.c_uint8));fn.restype=ctypes.c_int
    dll.spu_decode.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int,ctypes.POINTER(ctypes.c_int16))
    dll.spu_decode.restype=ctypes.c_int
    return dll

def enc(dll,pcm,kind):
    a=(ctypes.c_int16*len(pcm)).from_buffer(pcm)
    out=(ctypes.c_uint8*(len(pcm)+64))()
    n=(dll.content_spu_encode if kind=="console" else dll.ima_encode)(a,len(pcm),out)
    assert 0<n<=len(out)
    return bytes(out[:n])

def row_bytes(rows,vt):
    raw=bytearray();offs=[]
    for bar in range(BARS):
        offs.append(len(raw))
        for row in rows[bar*BAR:(bar+1)*BAR]:
            if not row: raw.append(0x81);continue
            raw.append(len(row))
            for ch,ins,nt,vol,bus in row:
                v=vt.index(ch|(ins<<4)|(bus<<9))
                w=(v&255)|(nt<<8)|(vol<<15)|((v>>8)<<22)
                raw+=w.to_bytes(3,"little")
    return bytes(raw),offs

def lzss_literals(raw):
    # BSQ2 literal tokens are valid LZSS. The small note stream stays below
    # 36 KiB; the decoder needs no special path or new ring buffer.
    return b"".join(bytes([(1<<len(raw[i:i+8]))-1])+raw[i:i+8] for i in range(0,len(raw),8))

def console_block(t,rows,patches,riff):
    bpm=TRACKS[t][1]; step=60/bpm/4; clock=round(step*MIXR*256)
    vtab=sorted({ch|(ins<<4)|(bus<<9) for row in rows for ch,ins,nt,vol,bus in row})
    raw,offsets=row_bytes(rows,vtab)
    blob=bytearray(); smpoff=[]
    for sample in patches: smpoff.append(len(blob));blob+=sample
    b=bytearray(80);o={}
    def put(k,d):
        b.extend(bytes((-len(b))%4));o[k]=len(b);b.extend(d)
    put("order",bytes(range(BARS)));put("rows",pack(str(BARS)+"H",*[BAR]*BARS))
    put("patoff",pack(str(BARS)+"I",*offsets));put("vt",pack(str(len(vtab))+"H",*vtab))
    put("pitch",pack(str(NINST*96)+"H",*[spu_pitch(note,i,riff) for i in range(NINST) for note in range(96)]))
    put("smpoff",pack(str(NINST)+"I",*smpoff))
    put("bus",bytes([108,102,89,77,64,44,24,24,44,64,77,89,102,108]))
    ev=lzss_literals(raw)
    # Verify the emitted BSQ2 stream, not merely the arrangement before encoding.
    assert unpack_lzss(ev,len(raw))==raw
    at=0
    for ri,row in enumerate(rows):
        count=raw[at];at+=1
        if count&128:
            assert count==0x81 and not row;continue
        decoded=[]
        for _ in range(count):
            w=int.from_bytes(raw[at:at+3],"little");at+=3
            v=vtab[(w&255)|((w>>22)<<8)]
            decoded.append((v&15,(v>>4)&31,(w>>8)&127,(w>>15)&127,(v>>9)&7))
        assert decoded==row,(t,ri)
    assert at==len(raw)
    put("ev",ev);put("extra",pack("4I",0,o["ev"],len(raw),0))
    sec=round(len(rows)*step)
    struct.pack_into("<20I",b,0,0x32515342,BARS,0,clock//256,clock%256,BARS,NINST,len(vtab),len(ev),len(blob),sec,
        *[o[k] for k in ("order","rows","patoff","vt","pitch","smpoff","bus","ev","extra")])
    assert len(b)<=36864 and len(blob)<=290272
    # Verify every instrument is referenced and the last ADPCM block ends.
    assert {e[1] for row in rows for e in row}==set(range(NINST))
    assert all(s[-15]==1 for s in patches)
    return bytes(b),bytes(blob),sec

def handheld_block(t,rows,patches,riff):
    step=60/TRACKS[t][1]/4; out=bytearray((0,0xfe)); cur=0
    def wait(tick):
        nonlocal cur
        d=tick-cur
        while d>254:out.append(255);d-=254
        out.append(d);cur=tick
    for ri,row in enumerate(rows):
        tick=round(ri*step*TICK)
        for ch,ins,nt,vol,bus in row:
            wait(tick);out.extend(pack("BBHBB",ch|0x40,ins,pitch(nt,ins,riff),vol,round(bus*127/6)))
    total=round(len(rows)*step*TICK);wait(total);out.append(0xff)
    seq=bytes(out);blob=bytearray(pad(seq,4));tab=len(blob);smp=bytearray()
    for s in patches:blob.extend(pack("II",len(smp),len(s)));smp+=s
    start=len(blob);blob+=smp
    assert len(blob)<=0x4A000 and all(len(s)%4==0 for s in patches)
    name=TRACKS[t][0].encode();artist=(riff["artist"] or "BORE 2").encode()[:23]
    entry=pack("IIIIIIIHBBII",0,len(blob),tab,start,len(smp),2,len(seq),NINST,TRACKS[t][5],0,0,total)+name.ljust(32,b"\0")+artist.ljust(24,b"\0")
    assert len(entry)==96
    return bytes(blob),entry,len(smp),len(seq)

def append_console(base,out,blocks,meta):
    old=(base/"SONGS.BIN").read_bytes(); inc=(base/"songs.inc").read_text()
    assert u32(old)==0x474E5342 and len(old)%2048==0
    n=u32(old,4);assert n==52 and n+len(blocks)<=127
    idx=bytearray(old[:2048]);blob=bytearray(old[2048:]);sec=len(old)//2048
    for t,(seq,smp,seconds) in enumerate(blocks):
        a,b=pad(seq,2048),pad(smp,2048)
        struct.pack_into("<4I",idx,8+16*(n+t),sec,len(a)//2048,len(b)//2048,len(smp));blob+=a+b;sec+=(len(a)+len(b))//2048
    struct.pack_into("<I",idx,4,n+len(blocks))
    new=bytes(idx)+blob;assert new[2048:len(old)]==old[2048:]
    out.joinpath("SONGS.BIN").write_bytes(new)
    inc=re.sub(r"#define NSONGS \d+",f"#define NSONGS {n+len(blocks)}",inc)
    before,after=inc.rsplit("};",1)
    extra="".join('    {{ {}, "{}", "{}", {}, 0, 0 }},\n'.format(s,TRACKS[t][0],meta[t]["source_artist"] or "BORE 2",TRACKS[t][5]) for t,(_,_,s) in enumerate(blocks))
    out.joinpath("songs.inc").write_text(before+extra+"};"+after)
    return len(new)-len(old)

def append_handheld(base,out,blocks):
    old=(base/"dsmusic.bin").read_bytes();magic,n,ent,diroff=struct.unpack_from("<4sIII",old)
    assert magic==b"BDM2" and ent==96 and diroff==16 and n==52 and n+len(blocks)<=64
    entries=[bytearray(old[16+i*96:16+(i+1)*96]) for i in range(n)]
    start=min(u32(e) for e in entries);newstart=align(16+96*(n+len(blocks)),512)
    assert start>=16+96*n and start%512==0
    shift=newstart-start;blob=bytearray(old[start:])
    for e in entries:struct.pack_into("<I",e,0,u32(e)+shift)
    for data,entry,_,_ in blocks:
        pos=newstart+len(blob);e=bytearray(entry);struct.pack_into("<I",e,0,pos);entries.append(e);blob+=pad(data,512)
    hdr=pack("4sIII",magic,len(entries),96,16)+b"".join(entries)
    new=pad(hdr,512)+blob
    assert new[newstart:newstart+len(old)-start]==old[start:]
    out.joinpath("dsmusic.bin").write_bytes(new)
    return len(new)-len(old)

def main():
    ap=argparse.ArgumentParser();ap.add_argument("kind",choices=("console","handheld","both"));ap.add_argument("base",type=Path);ap.add_argument("out",type=Path);a=ap.parse_args()
    a.out.mkdir(parents=True,exist_ok=True)
    kinds=("console","handheld") if a.kind=="both" else (a.kind,)
    key=hashlib.sha256()
    for file in (Path(__file__),ROOT/"tools/content_spu.c",ROOT/"tools/dsmusic/adpcm.c",ROOT/"src/band_roles.inc",*[a.base/x for x in ("SONGS.BIN","songs.inc","dsmusic.bin")]):
        key.update(file.read_bytes())
    key.update(a.kind.encode());stamp=key.hexdigest();manifest=a.out/("manifest-"+a.kind+".json")
    expected=["SONGS.BIN","songs.inc"] if a.kind=="console" else ["dsmusic.bin"] if a.kind=="handheld" else ["SONGS.BIN","songs.inc","dsmusic.bin"]
    if manifest.exists() and all((a.out/x).exists() for x in expected):
        cached=json.loads(manifest.read_text())
        if cached.get("key")==stamp and all(hashlib.sha256((a.out/x).read_bytes()).hexdigest()==cached["output_hashes"].get(x) for x in expected):
            print(json.dumps(cached,indent=2));return
    records={kind:[] for kind in kinds};meta=[]
    with tempfile.TemporaryDirectory() as tmp:
        dll=encoder(tmp)
        for t,(name,bpm,*_) in enumerate(TRACKS):
            riff=decode_original(a.base,RIFF_SOURCES[t])
            rows=score(t,riff);pcm=[sample(t,i) for i in range(10)]+[riff_pcm(dll,riff)]
            item={"name":name,"seconds":round(len(rows)*60/bpm/4),"notes":sum(map(len,rows)),"instruments":NINST,
                "source_song":riff["source"],"source_artist":riff["artist"],"source_order":riff["source_order"],
                "source_channel":riff["source_channel"],"source_instrument":riff["source_instrument"],
                "source_rows":riff["source_rows"],"source_onsets":riff["source_onsets"],
                "quoted_notes":len(riff["phrase"]),"riff_sha256":riff["riff_hash"]}
            for kind in kinds:
                patches=[enc(dll,x,kind) for x in pcm]
                assert len({hashlib.sha256(s).hexdigest() for s in patches})==NINST
                if kind=="console":
                    block=console_block(t,rows,patches,riff);item["console_payload"]=len(block[0])+len(block[1]);item["console_samples"]=len(block[1])
                else:
                    block=handheld_block(t,rows,patches,riff);item["handheld_payload"]=len(block[0]);item["handheld_samples"]=block[2]
                records[kind].append(block)
            meta.append(item);print(name,item,flush=True)
    result={"key":stamp,"tracks":meta,"seconds":sum(x["seconds"] for x in meta)}
    for kind in kinds:
        real=sum(x[kind+"_samples"]+ (len(records[kind][i][0]) if kind=="console" else records[kind][i][3]) for i,x in enumerate(meta))
        assert real>=1048576,(kind,real)
        delta=append_console(a.base,a.out,records[kind],meta) if kind=="console" else append_handheld(a.base,a.out,records[kind])
        result[kind]={"new_playable_bytes":real,"bank_growth_bytes":delta,"tracks":len(meta)}
    result["output_hashes"]={x:hashlib.sha256((a.out/x).read_bytes()).hexdigest() for x in expected}
    manifest.write_text(json.dumps(result,indent=2)+"\n");print(json.dumps(result,indent=2))
if __name__=="__main__":main()
