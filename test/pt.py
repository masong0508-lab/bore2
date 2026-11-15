#!/usr/bin/env python3
"""pt.py NAME 'tokens' [BUILD_DIR]: play the PS1 game with a pad script and look at the screens (DuckStation regtest).
Tokens (space separated; NEW and CAS are macros, see MACROS): b a x y (cross circle triangle square) up down left right l r l2 r2 l3 r3 start select
  wN = wait N frames, S = screenshot, up:90 = hold up 90 frames (up+left:90 both), busyN = wait N frames tapping square now and then, 'b*3' / 'down*5' = repeat, '#...' ignored.
Each press is held 8 frames and followed by PTGAP frames (default 22; the game runs about 20 fps).
START=frame (default 1300, the title is ready). PTB=build dir (default /tmp/pt_b, a plain Release `ninja iso_lite`).
Output: /tmp/pt_dsd/NAME_<n>.png contact sheets (3 per row, numbered), RAM at the end /tmp/pt_l/ds_NAME_ram.bin (read with test/ramsym.py).
"""
import sys, os, subprocess, glob
from PIL import Image, ImageDraw
MACROS = {   # named runs of presses (also usable inside other tokens' lists)
    'NEW': 'start w100 b w100 b w100 down b w300',   # title -> STORY MODE -> NEIGHBORHOODS -> set world MAPLE SHORES: the lot picker (the default Bore)
    'LOT': 'start w100 b w100 b w100 down b w300 b w600',  # ... and on into Maple Shores: the lot picker
    'HOME': 'start w100 b w100 b w100 down b w300 b w600 b w700',  # ... and move in to the first lot shown (a furnished cottage, $9600)
    'CAS': 'start w100 down*2 b w250 b w30',         # title -> CREATE A BORE (welcome card closed)
}
name = sys.argv[1]; toks = []
for w in sys.argv[2].split(): toks += MACROS.get(w, w).split()
bd = sys.argv[3] if len(sys.argv) > 3 else os.environ.get('PTB', '/tmp/pt_b')
gap = int(os.environ.get('PTGAP', 22)); t = int(os.environ.get('START', 1300)); out = []
for tk in toks:
    if tk.startswith('#'): continue
    n = 1
    if '+' in tk and ':' not in tk: tk += ':8'   # (l2+r2: both at once)
    if '*' in tk: tk, n = tk.split('*'); n = int(n)
    for _ in range(n):
        if tk in ('SHOT', 'S'): out.append('%d shot' % t); t += 6
        elif tk[0] == 'w' and tk[1:].isdigit(): t += int(tk[1:])
        elif tk[:4] == 'busy' and tk[4:].isdigit():   # busyN: N frames of waiting with a harmless SQUARE tap every 150 (the pad left alone for ~600 frames gives the Bore to its own AI: YOUR BORE ALONE)
            for k in range(0, int(tk[4:]), 150): out.append('%d y' % (t + k))
            t += int(tk[4:])
        elif ':' in tk:   # 'up:90' or 'up+left:90': hold for 90 frames
            bs, fr = tk.split(':')
            for k in range(0, int(fr), 6):
                for b_ in bs.split('+'): out.append('%d %s' % (t + k, b_))
            t += int(fr) + 8
        else: out.append('%d %s' % (t, tk)); t += 8 + gap
out.append('%d shot' % (t + 10)); t += 40
sf = '/tmp/pt_script_%s.txt' % name; open(sf, 'w').write('\n'.join(out) + '\n')
env = dict(os.environ, DSD='/tmp/pt_dsd', DSR='/tmp/pt_dsr', DSL='/tmp/pt_l')
here = os.path.dirname(os.path.abspath(__file__)); os.makedirs('/tmp/pt_l', exist_ok=True)
r = subprocess.run(['bash', here + '/ds.sh', bd + '/bore2_lite.cue', name, str(t + 60), sf], env=env, capture_output=True, text=True)
fs = sorted(glob.glob('/tmp/pt_dsd/%s/frame_*.png' % name))
cols = 3; W, H = 320, 240
for part in range(0, len(fs), 12):
    sub = fs[part:part + 12]; rows = (len(sub) + cols - 1) // cols
    c = Image.new('RGB', (W * cols, H * rows)); d = ImageDraw.Draw(c)
    for i, f in enumerate(sub):
        c.paste(Image.open(f).convert('RGB').resize((W, H)), (W * (i % cols), H * (i // cols)))
        d.text((W * (i % cols) + 3, H * (i // cols) + 3), str(part + i), fill=(255, 255, 0))
    c.save('/tmp/pt_dsd/%s_%d.png' % (name, part // 12))
print(len(fs), 'shots; last script time', t)
