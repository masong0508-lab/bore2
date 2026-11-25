import hashlib
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from living_score import ROLES, DESIGNS, tracks, arrange

# Exercise the actual arranger with a hand-written pitched phrase, rather
# than requiring the copyrighted/project-specific original bank on the host.
base = tuple(('old', 96, 52+i%12, (0,5,7,9), (0,4,7,2), i%4) for i in range(20))
riff = {'roots': (0,5,7,9), 'phrase': {0:(48,80), 5:(52,72), 16:(53,70), 35:(55,82), 63:(57,76)}}
assert len(DESIGNS) == 54
assert ROLES[:20] == tuple(range(20))
assert ROLES[20:40] == tuple(range(20))
assert ROLES[40:] == (0,2)+tuple(range(8,20))
assert len({title for title,bpm in DESIGNS}) == 54
fingerprints = set()
for i, track in enumerate(tracks(base)):
    rows = arrange(i, riff, track)
    assert len(rows) == 768
    assert {e[1] for row in rows for e in row} == set(range(11))
    for row in rows:
        assert len({e[0] for e in row}) == len(row)
        assert all(0<=e[0]<10 and 0<=e[2]<96 and 0<e[3]<=127 and 0<=e[4]<=6 for e in row)
    for start in (0, 512):
        for k,(n,v) in riff['phrase'].items():
            assert any(e[1]==10 and e[2]==n for e in rows[start+k])
    fingerprint = hashlib.sha256(json.dumps(rows).encode()).hexdigest()
    assert fingerprint not in fingerprints, (i, track[0])
    fingerprints.add(fingerprint)
print('Living score: 54 distinct arrangements, valid channels, 11 used instruments and exact source motifs')
