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

def self_test():
    a = Image.new("RGB", (32, 32), (50, 100, 150))
    b = Image.new("RGB", (32, 32), (240, 240, 240))
    assert audit([a, b, a])["flash_candidates"], "isolated bright flash missed"
    assert audit([a]*4 + [b]*4 + [a]*4)["flash_candidates"], "four-vblank flash missed"
    assert not audit([a]*9)["flash_candidates"], "static scene rejected"
    ramp = [Image.new("RGB", (32, 32), (v, v, v)) for v in range(0, 200, 10)]
    assert not audit(ramp)["flash_candidates"], "gradual change rejected"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--route", type=Path, required=True)
    args = parser.parse_args()
    self_test()
    args.route.write_text("".join(f"{t} shot\n" for t in TICKS))
    print(f"Flash-audit self-test PASS; requesting {len(TICKS)} screenshots")
