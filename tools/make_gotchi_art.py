#!/usr/bin/env python3
# make_gotchi_art.py: GOTCHI.ART, the pack guide's screenshots. Real frames from the game, made to pop (a little saturation and contrast, a
# crisp downscale, a touch of sharpening, a soft vignette), then packed as 72 x 54 pixel 15-bit pictures, dithered so skies do not band.
# usage: make_gotchi_art.py out.ART shot0 shot1 ... (8 PNGs, in the order the guide's pages use them)
import sys
from PIL import Image, ImageEnhance, ImageFilter, ImageDraw
W, H = 72, 54
def pop(im):
    im = im.convert('RGB')
    w, h = im.size
    im = im.crop((0, 0, w, h))
    big = im.resize((W * 4, H * 4), Image.LANCZOS)
    big = ImageEnhance.Color(big).enhance(1.28)
    big = ImageEnhance.Contrast(big).enhance(1.12)
    big = ImageEnhance.Brightness(big).enhance(1.04)
    # a soft vignette
    v = Image.new('L', big.size, 0); d = ImageDraw.Draw(v)
    d.ellipse((-big.size[0] * 0.18, -big.size[1] * 0.18, big.size[0] * 1.18, big.size[1] * 1.18), fill=255)
    v = v.filter(ImageFilter.GaussianBlur(big.size[0] * 0.12))
    dark = Image.eval(big, lambda p: int(p * 0.72))
    big = Image.composite(big, dark, v)
    small = big.resize((W, H), Image.LANCZOS)
    return small.filter(ImageFilter.UnsharpMask(radius=0.8, percent=70, threshold=2))
def pack(im):
    px = im.load(); out = bytearray()
    err = [[(0, 0, 0)] * (W + 2) for _ in range(2)]
    cur = [[0.0, 0.0, 0.0] for _ in range(W + 2)]; nxt = [[0.0, 0.0, 0.0] for _ in range(W + 2)]
    for y in range(H):
        for x in range(W):
            r, g, b = px[x, y]
            c = [r + cur[x][0], g + cur[x][1], b + cur[x][2]]
            q = [max(0, min(31, int(round(v / 255 * 31)))) for v in c]
            e = [c[i] - q[i] * 255 / 31 for i in range(3)]
            for i in range(3):
                if x + 1 < W: cur[x + 1][i] += e[i] * 7 / 16
                if x > 0: nxt[x - 1][i] += e[i] * 3 / 16
                nxt[x][i] += e[i] * 5 / 16
                if x + 1 < W: nxt[x + 1][i] += e[i] * 1 / 16
            v = q[0] | (q[1] << 5) | (q[2] << 10)
            if v == 0: v = 0x0421          # (black is transparent on the GPU)
            out += v.to_bytes(2, 'little')
        cur, nxt = nxt, [[0.0, 0.0, 0.0] for _ in range(W + 2)]
    return bytes(out)
if __name__ == '__main__':
    out = sys.argv[1]; shots = sys.argv[2:]
    assert len(shots) == 8, 'need 8 pictures'
    with open(out, 'wb') as f:
        for s in shots: f.write(pack(pop(Image.open(s))))
    print(out, 8 * W * H * 2, 'bytes')
