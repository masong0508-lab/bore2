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

# ---- the DS's own additions (2 players on one DS: pad_in, the opponent row on the pick screen; see ds_fight.c) ----
def patch(old, new):
    global t
    if t.count(old) != 1: raise SystemExit("patch target not found once: " + old[:60])
    t = t.replace(old, new)
patch("static void step_fight(int held, int pressed) {\n    FIn a, b; read_input(held, pressed, &a);",
 "static void pad_in(const Fz *f, const Fz *o, int hd, int pr, FIn *in) {   // (DS, 2 players on one DS) a fighter from the shared buttons: left, right, crouch, jump, two attacks (both together: a throw); holding back guards\n"
 "    memset(in, 0, sizeof *in);\n"
 "    int ux = (hd & FB_R ? 1 : 0) - (hd & FB_L ? 1 : 0), mx = ux * fcos(cyaw), mz = -ux * fsin(cyaw), dx = (int)(o->x - f->x), dz = (int)(o->z - f->z), back = ux && (mx * (dx >> 4) + mz * (dz >> 4)) < 0;\n"
 "    in->down = !!(hd & FB_D); if (back) in->guard = 1; else { in->mx = mx; in->mz = mz; }\n"
 "    in->a = !!(hd & FB_A1); in->b = !!(hd & FB_A2); in->pa = !!(pr & FB_A1); in->pb = !!(pr & FB_A2); in->pj = !!(pr & FB_J);\n"
 "}\n"
 "static int fg_first;\n"
 "static void step_fight(int held, int pressed) {\n    FIn a, b; if (fight_2p) { pad_in(&F[0], &F[1], fg_h1, fg_first ? fg_p1 : 0, &a); pad_in(&F[1], &F[0], fg_h2, fg_first ? fg_p2 : 0, &b); } else read_input(held, pressed, &a);")
patch("#ifdef BORE_DC\n    { int h2, p2; if (dc_pad2(&h2, &p2)) read_input_p2(h2, p2, &b); else ai(&F[1], &F[0], &b); }\n#else\n    ai(&F[1], &F[0], &b);\n#endif",
 "#ifdef BORE_DC\n    { int h2, p2; if (dc_pad2(&h2, &p2)) read_input_p2(h2, p2, &b); else ai(&F[1], &F[0], &b); }\n#else\n    if (!fight_2p) ai(&F[1], &F[0], &b);\n#endif")
patch("if (style == 3) { if (rnd8() < 40 && b.guard) b.guard = 0; }", "if (style == 3 && !fight_2p) { if (rnd8() < 40 && b.guard) b.guard = 0; }")
patch("step_fight(held, s == 0 ? pressed : 0);", "fg_first = s == 0; step_fight(held, s == 0 ? pressed : 0);")
patch("pick_row = (pick_row + 2) % 3;", "pick_row = (pick_row + 3) % 4;")
patch("pick_row = (pick_row + 1) % 3;", "pick_row = (pick_row + 1) % 4;")
patch("if (dir && pick_row == 2) cam_mode = (cam_mode + dir + CAM_N) % CAM_N;", "if (dir && pick_row == 2) cam_mode = (cam_mode + dir + CAM_N) % CAM_N;\n        if (dir && pick_row == 3) opp_step(dir);\n        if (dir && pick_row == 0 && opp_i == you_i) opp_i = -1;")
patch("if (F[0].wins >= 2) { fight_last_wins++;", "if (F[0].wins >= 2 && !fight_2p) { fight_last_wins++;")
patch("            if (F[0].wins >= 2) { if (stage == 8)", "            if (fight_2p) match_begin();\n            else if (F[0].wins >= 2) { if (stage == 8)")
patch("F[1].d = MIN(9, w->d + (style == 3 ? 0 : 0));", "F[1].d = MIN(9, w->d + (style == 3 ? 0 : 0));\n    if (fight_2p) { F[0].pw = F[1].pw = 100; F[0].sp = F[1].sp = 16; F[0].d = F[1].d = 0; }")
patch("stage = 0; style = 0; you_i = ctl;", "stage = 0; style = 0; opp_i = -1; you_i = ctl;")
patch("static void match_begin(void) { fight_matches++;", "static void match_begin(void) { if (fight_2p) cam_mode = CAM_SIDE; fight_matches++;")
hdr = ("// ds_fight_logic.h: made by tools/dsbuild/mk_dsfight.py from the PS1's src/fight.c (do not edit): BORTUA FIGHTER's rules, line for line.\n"
       "// The ring is in millimetres, 60 Hz steps, angles 0..255. The drawing and the DS's screens are in ds_fight.c.\n")
open(os.path.join(R, "ports", "dsi", "source", "ds_fight_logic.h"), "w").write(hdr + t + "\n")
print("ds_fight_logic.h: %d lines" % (t.count("\n") + 3))
