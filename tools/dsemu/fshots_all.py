#!/usr/bin/env python3
"""fshots_all.py: the D65 font before/after sheets. Needs the old ROM and the new one plus -DDSTEST test builds of each (see docs/DSI_ROADMAP.md, D65):
   /tmp/dsb/{b,a}{40,48,72,88}/bore.nds|elf   (b = d64 sources, a = new), bore2_dsi_d64.nds, bore2_dsi_d65.nds
   writes docs/ds_shots/fonts_<name>.png (before | after, each top over bottom)."""
import os, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
import fshots
from fshots import SC, shot
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(R, "docs", "ds_shots")
W = "/tmp/dsb/fw"
def run(tag, rom_plain, names_plain, base_names):
    d = os.path.join(W, tag); os.makedirs(d, exist_ok=True)
    for n in names_plain: shot(rom_plain, d, n)
    for base, names in base_names.items():
        for n in names: shot("/tmp/dsb/%s%d/bore.nds" % (tag[0], base), d, n)
    return d
plain = ["title", "title_opt", "title_opt2", "cas_hair", "cas_pad", "cas_paint", "cas_more", "map", "live", "menu", "buy", "build", "options", "job", "songs", "life", "travel"]
b = run("b", os.path.join(R, "bore2_dsi_d64.nds"), plain, {40: ["arc_intro", "arc_play", "arc_prizes", "skate"], 48: ["heist"]})
a = run("a", os.path.join(R, "bore2_dsi_d65.nds"), plain, {40: ["arc_intro", "arc_play", "arc_prizes", "skate"], 48: ["heist"]})
subprocess.run([sys.executable, os.path.join(R, "tools/dsemu/fshots.py"), "--pair", b, a, OUT])
