"""Conservative temporal smoke test for large isolated render flashes.

A and C must be close while B is dramatically different from both.
This avoids treating ordinary camera motion as a flash. It is not a
pixel-perfect check and does not detect every type of flicker.
"""
from PIL import Image, ImageChops, ImageStat
from pathlib import Path
import argparse

ANCHORS = (2600, 2800, 3000, 3200, 3400)
BURSTS = tuple(tuple(range(t, t + 25)) for t in (2800, 3200))
TICKS = tuple(sorted(set(ANCHORS).union(*BURSTS)))
CROP = (16, 12, 304, 180)

def diff(a, b):
    return sum(ImageStat.Stat(ImageChops.difference(a, b)).mean) / 3

def audit(images):
    distances = []
    failures = []
    for i in range(len(images)):
        for lag in (1, 2, 4, 8):
            if i < lag or i + lag >= len(images):
                continue
            a, b, c = images[i-lag], images[i], images[i+lag]
            ac, ab, bc = diff(a, c), diff(a, b), diff(b, c)
            jump = min(ab, bc)
            distances.append(jump)
            if ac <= 8 and jump >= 24 and jump >= 4 * max(1, ac):
                failures.append(dict(index=i, lag=lag, before_after=round(ac, 2),
                                     flash=round(jump, 2)))
    return dict(samples=len(images), max_isolated_jump=round(max(distances, default=0), 2),
                flash_candidates=failures)

def local_audit(images, patch=24):
    """Report large local A/B/A changes hidden by whole-screen averaging.
    Two adjacent patches must return to their original appearance. Report
    separately for inspection: animated objects and HUD can also qualify.
    """
    hits = []
    held = 0
    for i in range(1, len(images)):
        held += ImageChops.difference(images[i-1], images[i]).getbbox() is None
    for i in range(len(images)):
        for lag in (1, 2, 4, 8):
            if i < lag or i + lag >= len(images):
                continue
            a,b,c = images[i-lag],images[i],images[i+lag]
            w,h = a.size
            if diff(a,c) > 8:
                continue
            ac,ab,bc = (ImageChops.difference(x,y) for x,y in ((a,c),(a,b),(b,c)))
            changes = {}
            # Exclude the top and outer edges, where HUD changes dominate.
            for y in range(patch, h-patch+1, patch):
                for x in range(0, w-patch+1, patch):
                    box = (x,y,x+patch,y+patch)
                    d0,d1,d2 = (sum(ImageStat.Stat(im.crop(box)).mean)/3 for im in (ac,ab,bc))
                    jump = min(d1,d2)
                    if d0 <= 4 and jump >= 30 and jump >= 6*max(1,d0):
                        changes[(x,y)] = round(jump,2)
            pairs = [(x,y,j) for (x,y),j in changes.items()
                     if (x+patch,y) in changes or (x,y+patch) in changes]
            if pairs:
                hits.append(dict(index=i,lag=lag,patches=pairs))
    return dict(local_switch_candidates=hits, identical_adjacent_frames=held)

def self_test():
    a = Image.new("RGB", (32, 32), (50, 100, 150))
    b = Image.new("RGB", (32, 32), (240, 240, 240))
    assert audit([a, b, a])["flash_candidates"], "isolated bright flash missed"
    assert audit([a]*4 + [b]*4 + [a]*4)["flash_candidates"], "four-vblank flash missed"
    assert not audit([a]*9)["flash_candidates"], "static scene rejected"
    ramp = [Image.new("RGB", (32, 32), (v, v, v)) for v in range(0, 200, 10)]
    assert not audit(ramp)["flash_candidates"], "gradual change rejected"
    a=Image.new("RGB",(144,96),(50,100,150))
    b=a.copy(); b.paste((240,240,240),(24,24,72,48))
    assert not audit([a,b,a])["flash_candidates"], "local synthetic switch must be below global threshold"
    assert local_audit([a,b,a])["local_switch_candidates"], "local geometry switch missed"
    assert not local_audit([a,a,a])["local_switch_candidates"], "static local region rejected"
    assert local_audit([a,a,a])["identical_adjacent_frames"]==2

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--route", type=Path, required=True)
    args = parser.parse_args()
    self_test()
    args.route.write_text("".join(f"{t} shot\n" for t in TICKS))
    print(f"Flash-audit self-test PASS; requesting {len(TICKS)} screenshots")
