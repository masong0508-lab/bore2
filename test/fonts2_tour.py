#!/usr/bin/env python3 -I
"""fonts2_tour.py: the text tour. Steps through every overlay and screen that still has its own text layout so they can be shot again
(docs/fonts2_*.png, docs/FONTS.md). One slot per screen; the game side is the AUTO_SCREENS hook in src/main.c (scr_tick), which opens the
n-th screen of test/fonts2_order.h by itself every SLOT vblanks; this file writes that order and the pad script that turns the pages and shoots.

    python3 -I test/fonts2_tour.py gen            writes test/fonts2_order.h and /tmp/f2/tour.txt (the DuckStation script) and /tmp/f2/tour.json
    bash test/fonts2_tour.sh BUILD NAME           builds nothing: runs BUILD/bore2_lite.cue, shoots, and files the frames in /tmp/f2/dsd/NAME/
    python3 -I test/fonts2_tour.py pair BEFORE AFTER   makes the before | after sheets docs/fonts2_<group>.png from two such runs

Build: cmake -DCMAKE_C_FLAGS="-DAUTO_SCREENS=420 -DCIT_KNOWN=3" (Release, PSn00bSDK), ninja iso_lite (see test/fonts2_tour.sh).
The slot start is B0 + 240 k frames after the game starts (B0 was measured: the life starts, then slot 0, the loading screen).
Token language of a slot: a number waits that many frames; s shoots (a label may follow: s:name); b = X, a = O, x = TRIANGLE, y = SQUARE,
u d l r = the pad (up down left right), L = L1, R = R1, t = START, e = SELECT; every button is held 8 frames and followed by 14 frames of quiet."""
import sys, os, json, glob

SLOT = 420
B0 = 1365           # game-start relative frame at which slot 0 begins (measured with test/fonts2_tour.sh cal)
HERE = os.path.dirname(os.path.abspath(__file__))

# (case id in scr_tick, group, name, tokens)
SLOTS = [
    (0, 'loading', 'loading', '60 s'),
    (1, 'party', 'party', '40 s d d d d r 30 s d 24 s a 10'),
    (2, 'super', 'super_a', '40 s d d d d b 30 s b 30 s a a d b 30 s a'),
    (2, 'super', 'super_b', '40 d d d d d d b 60 s a'),
    (2, 'super', 'super_c', '40 d d d d d d d d b 30 s b 60 s'),
    (3, 'dog', 'dog', '40 s R 20 s R 20 s R 20 s R 20 s'),
    (4, 'prison', 'underworld', '40 s d d 20 s'),
    (18, 'prison', 'prison', '40 s b 30 s b 30 s b 30 s b 40 s b 40 s b 40 s b 40 s'),
    (5, 'space', 'space', '40 s b 30 s a d b 30 s a d d b 30 s'),
    (6, 'paradise', 'paradise', '40 s R 24 s R 24 s d d 20 s'),
    (8, 'farm', 'farm', '40 s R 24 s R 24 s R 24 s R 24 s'),
    (10, 'skysea', 'skysea', '40 s d b 30 s'),
    (14, 'dj', 'dj', '40 s'),
    (15, 'gotchi', 'pet_day1', '40 s b 40 s b 40 s b 100 s'),
    (16, 'gotchi', 'pet_intro', '40 s'),
    (48, 'build', 'catalogue', '130 s m 24 s m 24 s d d d d 20 s'),
    (49, 'build', 'floors', '130 s'),
    (50, 'pie', 'pie_people', '40 s'),
    (51, 'pie', 'pie_things', '40 s'),
    (20, 'menus', 'pause', '40 s'),
    (21, 'menus', 'help', '40 s R 20 s R 20 s R 20 s R 20 s R 20 s'),
    (23, 'menus', 'travel', '40 s'),
    (24, 'menus', 'goals', '40 s'),
    (25, 'menus', 'panel', '40 s R 24 s R 24 s R 24 s R 24 s R 24 s'),
    (26, 'maps', 'map', '50 s'),
    (27, 'maps', 'people', '40 s b 30 s'),
    (28, 'menus', 'jobs', '40 s d d 20 s'),
    (29, 'shops', 'homes', '40 s R 24 s d 14 s'),
    (30, 'menus', 'craft', '40 s'),
    (31, 'menus', 'slots', '40 s'),
    (32, 'worlds', 'nhood', '50 s'),
    (33, 'worlds', 'worlds', '50 s'),
    (34, 'shops', 'expansions', '50 s d d 20 s'),
    (35, 'menus', 'name', '40 s'),
    (36, 'worlds', 'stories', '40 s'),
    (37, 'worlds', 'wmake', '50 s'),
    (38, 'worlds', 'mode', '40 s'),
    (39, 'worlds', 'nhed', '50 s'),
    (40, 'menus', 'confirm', '40 s'),
    (41, 'menus', 'switch', '40 s'),
    (42, 'menus', 'juke', '40 s'),
    (43, 'menus', 'over', '40 s'),
    (44, 'menus', 'clock', '40 s'),
    (45, 'arcade', 'arcade', '60 s'),
    (47, 'arcade', 'prizes', '60 s'),
    (46, 'realm', 'realm', '60 s'),
]
BTN = {'b': 'b', 'a': 'a', 'x': 'x', 'y': 'y', 'u': 'up', 'd': 'down', 'l': 'left', 'r': 'right', 'L': 'l', 'R': 'r', 't': 'start', 'e': 'select', 'm': 'r3'}


def gen():
    lines, shots = [], []
    for k, (cid, grp, name, toks) in enumerate(SLOTS):
        t = B0 + SLOT * k
        n = 0
        for tok in toks.split():
            if tok.isdigit():
                t += int(tok)
            elif tok.split(':')[0] == 's':
                lab = tok.split(':')[1] if ':' in tok else (name if n == 0 else '%s%d' % (name, n + 1))
                lines.append('%d shot' % t)
                shots.append({'t': t, 'group': grp, 'name': lab, 'slot': k})
                n += 1
                t += 2
            else:
                lines.append('%d %s' % (t, BTN[tok]))
                t += 14
        assert t < B0 + SLOT * (k + 1) - 5, 'slot %s too long (%d)' % (name, t - B0 - SLOT * k)
    os.makedirs('/tmp/f2', exist_ok=True)
    open('/tmp/f2/tour.txt', 'w').write('\n'.join(sorted(lines, key=lambda l: int(l.split()[0]))) + '\n')
    json.dump({'shots': shots, 'frames': B0 + SLOT * len(SLOTS) + 60}, open('/tmp/f2/tour.json', 'w'))
    open(os.path.join(HERE, 'fonts2_order.h'), 'w').write(
        '// written by test/fonts2_tour.py: the screen of each AUTO_SCREENS slot\nstatic const unsigned char scr_order[] = { %s };\n' % ', '.join(str(s[0]) for s in SLOTS))
    print(len(SLOTS), 'slots,', len(shots), 'shots,', B0 + SLOT * len(SLOTS) + 60, 'frames')


def frames(name):
    return sorted(glob.glob('/tmp/f2/dsd/%s/frame_*.png' % name))


def pair(before, after):
    from PIL import Image, ImageDraw
    meta = json.load(open('/tmp/f2/tour.json'))['shots']
    fb, fa = frames(before), frames(after)
    assert len(fb) == len(fa) == len(meta), (len(fb), len(fa), len(meta))
    groups = {}
    for m, b, a in zip(meta, fb, fa):
        groups.setdefault(m['group'], []).append((m['name'], b, a))
    outd = os.path.join(HERE, '..', 'docs')
    for g, items in groups.items():
        # up to 3 pairs stacked, 2x scale, a slim label strip over each half (as docs/fonts_*.png)
        for part in range(0, len(items), 3):
            sel = items[part:part + 3]
            W, H = 292 * 2, 224 * 2
            im = Image.new('RGB', (W * 2 + 8, (H + 16) * len(sel)), (28, 28, 32))
            d = ImageDraw.Draw(im)
            for i, (nm, b, a) in enumerate(sel):
                y = i * (H + 16)
                d.text((6, y + 3), 'BEFORE  ' + nm, fill=(235, 235, 235))
                d.text((W + 14, y + 3), 'AFTER  ' + nm, fill=(235, 235, 235))
                im.paste(Image.open(b).convert('RGB').resize((W, H), Image.NEAREST), (0, y + 16))
                im.paste(Image.open(a).convert('RGB').resize((W, H), Image.NEAREST), (W + 8, y + 16))
            fn = os.path.join(outd, 'fonts2_%s%s.png' % (g, '' if part == 0 else str(part // 3 + 1)))
            im.save(fn, optimize=True)
            print(fn, im.size)


if __name__ == '__main__':
    if sys.argv[1] == 'gen':
        gen()
    elif sys.argv[1] == 'pair':
        pair(sys.argv[2], sys.argv[3])
