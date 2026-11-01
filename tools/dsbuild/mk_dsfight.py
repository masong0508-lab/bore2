#!/usr/bin/env python3
"""mk_dsfight.py: BORTUA FIGHTER's rules for the DS, taken line for line from the PS1's src/fight.c (the moves, the blows, the AI, the poses,
the camera, the match and its menus' state machine) into ports/dsi/source/ds_fight_logic.h. Only the names the DS does not have are changed
(PAD_ bits stay, audio_voice -> voice, cash -> funds, the PS1's look system and belt are left out) and the drawing is the DS's own (ds_fight.c).
Run it after src/fight.c changes; the DS build needs no Python.
   python3 tools/dsbuild/mk_dsfight.py"""
import os, re
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
L = open(os.path.join(R, "src", "fight.c")).read().split("\n")
def seg(a, b): return L[a - 1:b]          # 1-based, inclusive
def find(prefix, start=1):
    for i in range(start - 1, len(L)):
        if L[i].startswith(prefix): return i + 1
    raise SystemExit("not found: " + prefix)
a_maths = find("static int fframe;")
a_camap = find("static void cam_apply")
a_who = find("// ---- the opponents")
a_who_tbl = find("typedef struct { const char *nm; u16 seed;")
a_who_end = find("static Bore *them_p;") - 1          # the ladder table's last line
a_you = find("static int you_i;")
a_build = find("static void build_them")
a_match = find("// ---- the match ----")
a_pic = find("// ---- the picture ----")
out = seg(a_maths, a_camap - 1) + [""] + seg(a_who_tbl, a_who_end) + seg(a_you, a_build - 1) + [""] + seg(a_match, a_pic - 1)
t = "\n".join(out)
t = re.sub(r"audio_voice\(VOICE_(\w+)\)", r"voice(V_\1)", t)
t = t.replace("audio_voice(", "voice(")
t = re.sub(r"\bVOICE_", "V_", t)
t = re.sub(r"\bcash\b", "funds", t)
t = re.sub(r"\bnsims\b", "nsim", t)
t = re.sub(r"\bplayer\b", "ctl", t)
t = re.sub(r"\bBM_(?=[A-Z])", "FM_", t)   # (main.c has BM_N, the map size)
t = t.replace("int fight_last_wins, fight_matches;", "static int fight_last_wins, fight_matches;")
t = re.sub(r"RGB\(", "C15(", t)
t = t.replace("int belt = dojo_belt(s), body = wants_skill_level(s->skill_body);", "int belt = 0, body = 0; (void)s;   // (the DS has no dojo belts or skills)")
t = t.replace("sims[you_i].bore.stage < AG_CHILD", "stage_of(sims[you_i].age) < 2")
t = t.replace("{ u32 n; them_p = (Bore *)(((u32)crowd_scratch(&n) + 7) & ~7u); } them_built = -1;", "")
t = t.replace("if (them_built >= 0) { look_release(&them); them_built = -1; } return 0;", "return 0;")
t = re.sub(r"static const Bore \*you\(void\)[^\n]*\n", "", t)
t = t.replace("void fight_open(int cr)", "static void fight_open(int cr)").replace("int fight_update(int held", "static int fight_update(int held")
hdr = ("// ds_fight_logic.h: made by tools/dsbuild/mk_dsfight.py from the PS1's src/fight.c (do not edit): BORTUA FIGHTER's rules, line for line.\n"
       "// The ring is in millimetres, 60 Hz steps, angles 0..255. The drawing and the DS's screens are in ds_fight.c.\n")
open(os.path.join(R, "ports", "dsi", "source", "ds_fight_logic.h"), "w").write(hdr + t + "\n")
print("ds_fight_logic.h: %d lines" % (t.count("\n") + 3))
