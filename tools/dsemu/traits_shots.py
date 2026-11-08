#!/usr/bin/env python3
"""traits_shots.py rom elf outdir [name ...] (D73): shots of the trait picker, the wish picker, the locked slots, the cheat tab, a housemate's traits page and the move-in toast,
both screens (top over bottom, 256 x 384), by touch scripts (Create-a-Sim) and the test scenes 110-112 (Live).
   rom: a -DDSTEST build (add -DDS_CHEATDEF for the cheat shots: cheat traits on, two extra slots open)."""
import os, subprocess, sys
from PIL import Image
START = "30-33:0:128,90;60-63:0:212,140;100-103:0:128,130"   # the title's New sim slot, its OK, the welcome pop-up's OK
def tap(f, x, y): return ";%d-%d:0:%d,%d" % (f, f + 3, x, y)
TAB_TRAITS = tap(130, 20, 140)
SLOT1 = tap(150, 96, 52)
def chip(f, k): return tap(f, 59 + 69 * k, 64)            # the category chip k of the three on view
def scroll(f, n): return "".join(tap(f + 6 * i, 244, 64) for i in range(n))
def view(f, v, nv=5): w = 256 // nv; return tap(f, v * w + 1 + (w - 2) // 2, 38)
def row(f, r): return tap(f, 110, 89 + 24 * r)
BACK, RANDOM, CLEAR, AZ, FILTER = (23, 12), (106, 12), (152, 12), (193, 12), (234, 12)
CATS = ["body", "mind", "social", "romance", "work", "money", "food", "home", "outdoors", "fun", "nightday", "pets", "luck", "fight", "earned"]
SC = {}   # name: (script, last frame, pokes)
def add(name, taps, fr, poke=None): SC[name] = (START + taps, fr, poke)
add("traits_01_slots_locked", TAB_TRAITS, 190)
add("traits_02_picker_all", TAB_TRAITS + SLOT1, 200)
add("traits_03_trait_selected", TAB_TRAITS + SLOT1 + row(170, 0), 210)
add("traits_04_picked", TAB_TRAITS + SLOT1 + row(170, 0) + row(185, 0) + row(200, 1) + row(215, 1) + row(230, 2), 260)
for j, c in enumerate(CATS, 1):
    n = min(j - 1, 13)
    add("traits_cat_%02d_%s" % (j, c), TAB_TRAITS + SLOT1 + scroll(165, n) + chip(165 + 6 * n + 6, j - n) + row(165 + 6 * n + 20, 0), 165 + 6 * n + 45)
add("traits_view_picked", TAB_TRAITS + SLOT1 + row(170, 0) + row(185, 0) + row(200, 1) + row(215, 1) + view(235, 1), 260)
add("traits_view_for_you", TAB_TRAITS + SLOT1 + view(170, 2) + row(190, 0), 225)
add("traits_view_recent", TAB_TRAITS + SLOT1 + view(170, 3), 210)
add("traits_az", TAB_TRAITS + SLOT1 + tap(170, *AZ), 210)
add("traits_az_jump_m", TAB_TRAITS + SLOT1 + tap(170, *AZ) + tap(190, 177, 113), 225)
add("traits_filter", TAB_TRAITS + SLOT1 + tap(170, *FILTER), 210)
add("traits_filter_mood", TAB_TRAITS + SLOT1 + tap(170, *FILTER) + tap(190, 197, 89) + row(205, 0), 235)
add("traits_clash", TAB_TRAITS + SLOT1 + tap(170, *RANDOM) + view(190, 0) + row(205, 1), 235)
add("traits_random", TAB_TRAITS + SLOT1 + tap(170, *RANDOM) + view(190, 1), 215)
add("traits_slots_filled", TAB_TRAITS + SLOT1 + tap(170, *RANDOM) + tap(190, *BACK), 220)
add("traits_wish_page", TAB_TRAITS + tap(150, 198, 30), 200)
add("traits_wish_picker", TAB_TRAITS + tap(150, 198, 30) + tap(170, 148, 130), 220)
add("traits_wish_selected", TAB_TRAITS + tap(150, 198, 30) + tap(170, 148, 130) + tap(190, 128, 64) + row(205, 1), 240)
add("traits_wish_chosen", TAB_TRAITS + tap(150, 198, 30) + tap(170, 148, 130) + row(190, 1) + row(205, 1) + tap(220, *BACK), 245)
add("traits_cheats_tab", TAB_TRAITS + SLOT1 + view(170, 4) + row(190, 0), 225)
add("traits_cheats_confirm", TAB_TRAITS + SLOT1 + view(170, 4) + row(190, 0) + row(205, 0), 235)
add("traits_cheats_picked", TAB_TRAITS + SLOT1 + view(170, 4) + row(190, 0) + row(205, 0) + tap(220, 74, 130), 250)
add("traits_cheat_slots", TAB_TRAITS + SLOT1 + view(170, 4) + row(190, 0) + row(205, 0) + tap(220, 74, 130) + tap(235, *BACK), 260)
LIVE = "100-103:0:212,140;200-203:1:-1,-1"   # the title's OK (a new game) then A on the Map: Live
for nm, key, fr in (("traits_housemate_2", 0x4, 700), ("traits_housemate_1", 0x400, 700), ("traits_move_in_toast", 0x800, 520)):   # (a ROM built with -DDSTEST -DAUTO_SCENE=140: SELECT 140, X 141, Y 142; AUTOROM=that ROM)
    SC[nm] = (LIVE + ";385-388:%x:-1,-1" % key, fr, "auto")
def shot(rom, syms, out, name):
    script, fr, poke = SC[name]; ppm = os.path.join(out, name + ".ppm"); env = dict(os.environ)
    if poke == "auto": rom = os.environ["AUTOROM"]
    subprocess.run(["/tmp/dsrun", rom, str(fr + 1), ppm, script, str(fr)], capture_output=True, text=True, env=env)
    sp = ppm[:-4] + "_%d.ppm" % fr
    if os.path.exists(sp): Image.open(sp).save(os.path.join(out, name + ".png")); os.remove(sp)
    if os.path.exists(ppm): os.remove(ppm)
if __name__ == "__main__":
    rom, elf, out = sys.argv[1], sys.argv[2], sys.argv[3]; os.makedirs(out, exist_ok=True)
    syms = {}
    for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
        p = ln.split()
        if len(p) == 3: syms[p[2]] = p[0]
    for n in (sys.argv[4:] or list(SC)): shot(rom, syms, out, n)
