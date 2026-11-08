#!/usr/bin/env python3
"""cas_shots.py rom outdir [name ...] (D67): a shot of every Create-a-Sim step, both screens (top over bottom, 256 x 384), by touch scripts only (a normal build).
Title -> New sim -> OK, the welcome pop-up's OK, then the taps of each scene. Touch x,y are bottom-screen pixels."""
import os, subprocess, sys
from PIL import Image
START = "30-33:0:128,90;60-63:0:212,140;100-103:0:128,130"   # the title's New sim slot, its OK, the welcome pop-up's OK
def tap(f, x, y): return ";%d-%d:0:%d,%d" % (f, f + 3, x, y)
TAB = {"basics": (20, 16), "looks": (20, 47), "hair": (20, 78), "outfit": (20, 109), "traits": (20, 140), "done": (20, 171)}
def tab(f, n): return tap(f, *TAB[n])
def sub(f, i, n):   # the sub-tab i of n (row y 30)
    w = (213 - 8 - (n - 1) * 2) // n; return tap(f, 45 + i * (w + 2) + w // 2, 30)
NEXT = (219, 177); BACK = (76, 177)
SC = {}
def add(name, taps, fr): SC[name] = (START + taps, fr)
add("cas_01_welcome", "", 96)
add("cas_02_basics", "", 160)
add("cas_03_first_name", tap(130, 91, 33), 180)
add("cas_04_last_name", tap(130, 91, 33) + tap(145, 224, 156) , 200)
add("cas_05_skin_mix", tap(130, 230, 108) + tap(150, 80, 100), 190)
add("cas_06_looks_face", tab(130, "looks") + tap(150, 130, 70), 220)
add("cas_07_looks_features", tab(130, "looks") + sub(150, 1, 3) + tap(170, 60, 56) + tap(190, 130, 70), 250)
add("cas_08_looks_body", tab(130, "looks") + sub(150, 1, 3) + tap(170, 60, 130), 260)
add("cas_09_looks_sliders", tab(130, "looks") + sub(150, 2, 3), 220)
add("cas_10_hair_style", tab(130, "hair") + tap(150, 130, 90), 220)
add("cas_11_hair_style_p4", tab(130, "hair") + tap(150, 232, 11) + tap(165, 232, 11) + tap(180, 232, 11), 240)
add("cas_12_hair_colour", tab(130, "hair") + sub(150, 1, 4) + tap(170, 150, 82), 230)
add("cas_13_hair_beard", tab(130, "hair") + sub(150, 2, 4) + tap(170, 130, 100), 230)
add("cas_14_hair_hat", tab(130, "hair") + sub(150, 3, 4) + tap(170, 200, 100), 240)
add("cas_15_outfit_top", tab(130, "outfit") + tap(150, 130, 100), 240)
add("cas_16_outfit_bottom", tab(130, "outfit") + sub(150, 1, 3) + tap(170, 60, 100), 250)
add("cas_17_outfit_mix", tab(130, "outfit") + sub(150, 2, 3) + tap(170, 90, 106), 250)
add("cas_18_traits", tab(130, "traits") + tap(150, 80, 74) + tap(160, 80, 89) + tap(170, 80, 134) + tap(180, 80, 104), 230)
add("cas_19_wish", tab(130, "traits") + sub(150, 1, 2) + tap(170, 120, 111), 230)
add("cas_20_summary", tab(130, "traits") + tap(140, 80, 74) + tap(150, 80, 89) + tap(160, 190, 134) + sub(170, 1, 2) + tap(180, 120, 111) + tab(200, "hair") + sub(210, 3, 4) + tap(220, 200, 100) + tab(235, "done"), 280)
add("cas_21_ready", tab(130, "done") + tap(160, *NEXT), 200)
add("cas_22_leave", tap(130, *BACK), 160)
def shot(rom, out, name):
    script, fr = SC[name]; ppm = os.path.join(out, name + ".ppm")
    subprocess.run(["/tmp/dsrun", rom, str(fr + 1), ppm, script, str(fr)], capture_output=True, text=True)
    sp = ppm[:-4] + "_%d.ppm" % fr
    if os.path.exists(sp): Image.open(sp).save(os.path.join(out, name + ".png")); os.remove(sp)
    if os.path.exists(ppm): os.remove(ppm)
if __name__ == "__main__":
    rom, out = sys.argv[1], sys.argv[2]; os.makedirs(out, exist_ok=True)
    for n in (sys.argv[3:] or list(SC)): shot(rom, out, n)
