"""Additional living-world score: 54 location/time-specific arrangements.

Every arrangement quotes a phrase from an existing original BORE sequence.
Independent rhythms, bass movement, voicing, counterpoint and section dynamics
provide a new arrangement rather than merely renaming the original.
"""
# Order is an ABI with src/living_calendar.h. Do not reorder.
MORNING = (
 ("FIRST LIGHT ON ELM",104),("COMMUTER EXPRESS",118),("OPEN THE SHOPS",128),
 ("LOW TIDE SUNRISE",86),("UNPACKING THE STALLS",120),("TOOLS IN THE SUN",96),
 ("WHEELS BEFORE SCHOOL",144),("COCONUT BREAKFAST",110),("ESPRESSO AT DAWN",108),
 ("THE READING ROOM",82),("WARM UP CIRCUIT",142),("SUN ON THE LANES",112),
 ("EARLY SCREENING",92),("WATER THE SEEDLINGS",118),("MORNING BIRDS",94),
 ("FIRST HIGH SCORE",148),("OPEN THE GALLERY",88),("BREAKFAST RECEPTION",102),
 ("WALK TO SCHOOL",126),("FIRST LIGHT TELESCOPE",90),
)
EVENING = (
 ("PORCH LIGHTS",78),("THE MIDNIGHT ROUTE",96),("STREET LAMPS",106),
 ("TIDE AFTER DARK",72),("CLOSING THE MARKET",96),("WORKSHOP LAMPLIGHT",82),
 ("AFTER HOURS SKATE",132),("CAMPFIRE ABOVE THE SEA",80),("LAST CUP",84),
 ("BOOKS AT CLOSING",70),("LAST REP",122),("THE QUIET POOL",88),
 ("LATE SHOW",76),("FARMHOUSE SUPPER",94),("FIREFLIES ON THE PATH",72),
 ("ARCADE AFTER DARK",136),("AFTER THE EXHIBITION",74),("HOTEL LOBBY AT NIGHT",84),
 ("EMPTY CLASSROOMS",90),("CONSTELLATIONS",76),
)
WEEKEND = (
 ("SATURDAY ON ELM",112),("SUNDAY STREET FAIR",124),("BRUNCH TABLE",116),
 ("WEEKEND BOOK CLUB",90),("THE FITNESS MEET",140),("FAMILY SWIM",116),
 ("FESTIVAL MATINEE",102),("THE PRODUCE FAIR",126),("PICNIC IN THE PARK",106),
 ("SCOREBOARD SATURDAY",150),("OPEN STUDIO",100),("RESORT WEEKEND",110),
 ("COMMUNITY SCHOOL DAY",118),("SCIENCE OPEN HOUSE",108),
)
ROLES = tuple(range(20))*2+(0,2)+tuple(range(8,20))
DESIGNS = MORNING+EVENING+WEEKEND
assert len(DESIGNS)==len(ROLES)==54
assert len({x[0] for x in DESIGNS})==54

def fit(n, lo, hi):
    while n<lo: n+=12
    while n>hi: n-=12
    return n

def tracks(original):
    result=[]
    for i, (title,bpm) in enumerate(DESIGNS):
        role=ROLES[i]
        _,_,root,prog,motif,fx=original[role]
        result.append((title,bpm,root,prog,motif,fx))
    return tuple(result)

def arrange(i, riff, track):
    """48-bar A/bridge/A'/outro arrangement, ten hardware channels."""
    role=ROLES[i]; phase=0 if i<20 else 1 if i<40 else 2
    _,_,root,prog,motif,_=track
    rows=[[] for _ in range(48*16)]
    style=(role+phase*3)%6
    kicks=((0,6,10),(0,8),(0,4,8,12),(0,7,11),(0,5,12),(0,10,14))[style]
    bass_grid=((0,4,7,10,14),(0,6,12),(0,4,8,12),(0,3,8,11),(0,5,9,13),(0,7,12))[style]
    lead_grid=((2,7,11),(1,6,13),(3,6,10,14),(2,5,10),(1,7,12),(3,9,14))[style]
    def hit(bar,step,ch,inst,n,vol,pan=3):
        row=rows[bar*16+step]
        assert 0<=n<96 and all(e[0]!=ch for e in row)
        row.append((ch,inst,n,vol,pan))
    for bar in range(48):
        section=bar//16
        phrase_bar=bar%4
        chroma=riff["roots"][phrase_bar]
        # Quote the source bass in the outer sections; the middle eight
        # bars move through the original venue's contrasting chord cycle.
        chord=root+((chroma-root+6)%12-6)
        if section==1 and 20<=bar<28: chord=root+prog[(bar-20+role//6)%len(prog)]
        third=3 if (role+phase)%3==0 else 4
        if section!=1:
            classes={(n-chroma)%12 for k,(n,v) in riff["phrase"].items() if k//16==phrase_bar}
            third=3 if 3 in classes and 4 not in classes else 4 if 4 in classes and 3 not in classes else 12
        notes=[fit(chord,60,71),fit(chord+third,60,75),fit(chord+7,60,76)]
        quiet=phase==1 or (section==1 and bar<20)
        velocity=0 if not quiet else -13
        if bar>=44: velocity-=min(18,(bar-43)*4)
        for step in (kicks if not (section==1 and bar<20) else (0,8)):
            hit(bar,step,0,0,60,96+velocity-(step!=0)*12)
        for step in ((4,12) if not quiet else (8,)):
            hit(bar,step,1,1,60,79+velocity)
        for step in range(0,16,4 if quiet else 2):
            hit(bar,step,2,2,60,39+velocity+(step%4==0)*8,4)
        for step in ((3,11) if style%2 else (7,15)):
            hit(bar,step,3,3,60,42+velocity,1)
        for j,step in enumerate(bass_grid if not quiet else (0,8,13)):
            degree=(0,7,12,7,0)[j%5]
            if j==2 and bar%4==3: degree=(2,5,7,10)[role//6]
            hit(bar,step,4,4,fit(chord+degree,36,53),83+velocity-(j>0)*11)
        for j,step in enumerate((0,4,8,12) if style%2==0 else (0,5,10,13)):
            hit(bar,step,5,5,notes[(j+bar+phase)%3],56+velocity-(j>0)*7,2)
        hit(bar,0,6,6,notes[0],47+velocity,1)
        hit(bar,0,7,6,notes[2],43+velocity,5)
        if section!=1:
            for k,(nt,v) in sorted(riff["phrase"].items()):
                if k//16==phrase_bar: hit(bar,k%16,8,10,nt,min(100,max(39,v+velocity)),3)
        else:
            for j,step in enumerate(lead_grid):
                n=fit(chord+motif[(bar+j+phase+role//6)%len(motif)],61,83)
                hit(bar,step,8,7 if (j+bar)%2 else 8,n,63+velocity-(j>0)*8,4)
        for step in ((5,13) if bar%4!=3 else (5,9,13,15)):
            hit(bar,step,9,9,60,45+velocity,5)
    for start in (0,32*16):
        for k,(nt,v) in riff["phrase"].items():
            assert any(e[1]==10 and e[2]==nt for e in rows[start+k])
    assert {e[1] for row in rows for e in row}==set(range(11))
    return rows
