#!/usr/bin/env python3
"""film_storyboard.py FRAMEDIR FIRST OUTDIR [PICLAG]: one still per shot of the film (the shot list is read from src/cutscene.c: the camera keys with a hard cut
and the authored comment above each), shot_NN.png (640x480, nearest neighbour), film_storyboard.png (a contact sheet) and film_storyboard.txt (each shot's time,
camera, who is in it, what is said). FRAMEDIR: the frame-by-frame dump (frame_NNNNN.png); FIRST: the emulator frame where the film opens (film_frames.py).
PICLAG (default 0): frames to add if the picture trails the film's clock (it does not: a cut at step 630 is on screen at FIRST + 630)."""
import sys, os, re, math
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = open(os.path.join(HERE, '..', 'src', 'cutscene.c')).read()
NAMES = {1: 'CHAD (the host)', 2: 'MISSY (the judge)', 3: 'the INTERN'}
# what the geometry cannot say: things in the picture that are not one of the three people (written by hand from the stills; keyed by shot number)
EXTRA = {1: 'the whole studio, the stage, the wall of lights (nobody on stage yet), the title card TV SHOW & TELL / Starring Missy Teller', 2: 'the audience at the edge of the frame', 3: 'the audience (front row)',
         5: 'the judge\'s desk', 6: 'the judge\'s desk, the stage', 7: 'the tray with the drink glass', 8: 'the tray, the glass', 9: 'the glass (it falls and breaks)',
         10: 'the audience reacts', 12: 'the whole stage, then the title card MISSY TELLER'}


# the geometry test is a guess: where the stills show otherwise (checked by eye), the list is written here
WHO = {1: [], 7: [NAMES[2], NAMES[3] + ' (coming on at the left edge)'], 8: [NAMES[2] + ' (close)', NAMES[3] + ' (only the tray in hand)'], 9: [NAMES[2] + ' (close, face and hand)']}


def parse():
    # the keys in order, each with the comment of the shot it belongs to
    body = re.search(r'cam_k\[\] = \{(.*?)\};', SRC, re.S).group(1)
    shots = []
    for line in body.split('\n'):
        m = re.match(r'\s*//\s*(\d+)\.\s*(.*)', line)
        if m: shots.append({'n': int(m.group(1)), 'say': m.group(2).strip(), 'keys': []}); continue
        for t, ex, ey, ez, ax, ay, az, c in re.findall(r'CK_\((\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(\d)\)', line):
            shots[-1]['keys'].append((int(t) * 6, int(ex), int(ey), int(ez), int(ax), int(ay), int(az), int(c)))
    end = int(re.search(r'#define FILM_END T\((\d+)\)', SRC).group(1)) * 6
    for i, s in enumerate(shots):
        s['t0'] = s['keys'][0][0]; s['t1'] = shots[i + 1]['keys'][0][0] if i + 1 < len(shots) else end
    return shots


def subs():
    m = re.search(r'static const Sub subs\[\] = \{(.*?)\} \};', SRC, re.S)
    return [(int(a) * 6, int(b) * 6, int(w), t) for a, b, w, t in re.findall(r'\{ T\((\d+)\),\s*T\((\d+)\),\s*(\d),\s*"([^"]*)"', m.group(1))]


def walks():
    out = {}
    for who, name in ((1, 'chad_w'), (2, 'missy_w'), (3, 'intern_w')):
        body = re.search(name + r'\[\] = \{(.*?)\};', SRC, re.S).group(1)
        out[who] = [(int(t) * 6, int(x), int(z)) for t, x, z, y in re.findall(r'WK_\((\d+),(-?\d+),(-?\d+),(\d+)\)', body)]
    return out


def lerp_path(p, t):
    if t <= p[0][0]: return p[0][1:]
    for a, b in zip(p, p[1:]):
        if a[0] <= t <= b[0]:
            u = (t - a[0]) / max(1, b[0] - a[0]); return (a[1] + (b[1] - a[1]) * u, a[2] + (b[2] - a[2]) * u)
    return p[-1][1:]


def cam_at(keys, t):
    ks = [k for k in keys]
    if t <= ks[0][0]: return ks[0][1:7]
    for a, b in zip(ks, ks[1:]):
        if a[0] <= t <= b[0]:
            u = (t - a[0]) / max(1, b[0] - a[0]); return tuple(a[i] + (b[i] - a[i]) * u for i in range(1, 7))
    return ks[-1][1:7]


def who_in(keys, t, W):
    ex, ey, ez, ax, ay, az = cam_at(keys, t)
    fx, fz = ax - ex, az - ez; fl = math.hypot(fx, fz) or 1
    res = []
    for w, path in W.items():
        if w == 3 and t < 285 * 6: continue   # (the intern is not on until 28.5 s)
        x, z = lerp_path(path, t); dx, dz = x - ex, z - ez; d = math.hypot(dx, dz) or 1
        ang = math.degrees(math.acos(max(-1, min(1, (dx * fx + dz * fz) / (d * fl)))))
        if ang < 26 and d < 3200 and not (w == 1 and t < 50 * 6): res.append((w, d))
    return [NAMES[w] for w, d in sorted(res, key=lambda r: r[1])]


def ts(step): return '%d:%04.1f' % (int(step / 60) // 60, step / 60 - 60 * (int(step / 60) // 60))


def main():
    fd, first, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]; lag = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    os.makedirs(out, exist_ok=True)
    S = subs(); W = walks(); shots = parse(); tiles = []; lines = []
    lines.append('TV SHOW & TELL - Missy Teller\'s opening film: %d shots, %s long (3600 steps of the 60 Hz film clock). The base for an art-drawn FMV.' % (len(shots), ts(shots[-1]['t1'])))
    lines.append('Times are the film\'s clock; "key" lines are the camera keys (eye x,y,z -> aim x,y,z, in the film\'s units: a person is 384 tall; a Catmull-Rom path between keys, hard cut at each shot start).')
    lines.append('Voice: CHAD (host, warm and fast), MISSY (judge, dry and sultry), INTERN (stammers). Sound: the crowd cheers on the beats, shouts, a shriek and a win cheer (the cues), the jukebox is stopped for the film.\n')
    for s in shots:
        said0 = [(a, b) for a, b, w, t in S if a < s['t1'] and b > s['t0']]
        # the still: when the first line of the shot is almost all typed in (else 45% into the shot)
        step = int(min(s['t1'] - 8, max(s['t0'] + 14, said0[0][0] + 0.8 * (said0[0][1] - said0[0][0])))) if said0 else int(s['t0'] + max(14, 0.45 * (s['t1'] - s['t0'])))
        fr = first + lag + step; src = os.path.join(fd, 'frame_%05d.png' % fr)
        im = Image.open(src).convert('RGB'); big = im.resize((640, 480), Image.NEAREST); big.save(os.path.join(out, 'shot_%02d.png' % s['n']))
        tiles.append((s, im))
        said = [(a, w, t) for a, b, w, t in S if a < s['t1'] and b > s['t0']]
        who = WHO[s['n']] if s['n'] in WHO else who_in(s['keys'], step, W)
        k0, k1 = s['keys'][0], s['keys'][-1]
        lines.append('SHOT %d   %s - %s (%.1f s)   still: %s  (emulator frame %d)' % (s['n'], ts(s['t0']), ts(s['t1']), (s['t1'] - s['t0']) / 60, ts(step), fr))
        lines.append('  what: %s' % s['say'])
        lines.append('  camera: eye (%d,%d,%d) aim (%d,%d,%d)%s' % (k0[1], k0[2], k0[3], k0[4], k0[5], k0[6],
                     '' if len(s['keys']) == 1 else '  ->  eye (%d,%d,%d) aim (%d,%d,%d) at %s' % (k1[1], k1[2], k1[3], k1[4], k1[5], k1[6], ts(k1[0]))))
        lines.append('  in frame: %s' % ('; '.join(who + ([EXTRA[s['n']]] if s['n'] in EXTRA else [])) or 'nobody'))
        lines.append('  said: %s' % (' / '.join('%s: "%s" (%s)' % ({1: 'CHAD', 2: 'MISSY', 3: 'INTERN'}[w], t, ts(a)) for a, w, t in said) or '(nothing)'))
        lines.append('')
    open(os.path.join(out, 'film_storyboard.txt'), 'w').write('\n'.join(lines))
    cols = 4; rows = (len(tiles) + cols - 1) // cols; cw, ch, lab = 320, 240, 18
    sheet = Image.new('RGB', (cols * (cw + 4) + 4, rows * (ch + lab + 4) + 4), (24, 24, 28)); d = ImageDraw.Draw(sheet)
    for i, (s, im) in enumerate(tiles):
        x = 4 + (i % cols) * (cw + 4); y = 4 + (i // cols) * (ch + lab + 4)
        sheet.paste(im, (x, y + lab)); d.text((x + 2, y + 3), 'SHOT %d  %s - %s' % (s['n'], ts(s['t0']), ts(s['t1'])), fill=(255, 255, 255))
    sheet.save(os.path.join(out, 'film_storyboard.png'))
    print('storyboard: %d shots in %s' % (len(tiles), out))


main()
