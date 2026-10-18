#!/usr/bin/env python3
"""The creator's FACE tab, made bigger: more eyes, mouths, brows, noses, cheeks and marks, glasses.
Writes src/face_more.inc. The GBA game's own face art (look_port.inc) stays first in every list; these come after it.
Art keys: k dark, w white, r red, s lid shadow (the skin, darker), i iris, b blush, g glasses frame, h brows (the hair, darker),
l dark lenses, y gold, p lips (LIPS), c face paint (blue), n a mole (dark brown), e eye shadow (MAKEUP), t a tear, m metal, a a plaster."""
import sys, os
KEYS = "kwrsibghlypcnetma"
OUT = os.path.join(os.path.dirname(__file__), "..", "src", "face_more.inc")
B9 = "........."

def c(w, rows, n):   # centre each row in w columns; pad to n rows (blank rows split above / below unless the art says)
    out = []
    for r in rows:
        assert len(r) <= w, (r, w)
        l = (w - len(r)) // 2
        out.append("." * l + r + "." * (w - len(r) - l))
    while len(out) < n: out.append("." * w)
    return out
def rowsat(w, n, top, rows):   # rows placed from row `top`
    return c(w, [""] * top + rows, n)

# ---- eyes: 9 x 8, one eye (the other is its mirror); x 0 is the outer corner, x 8 by the nose ----
EYES = [
 ("STAR",     [B9,"....i....","...iii...",".iiiiiii.","..iiiii..","..ii.ii..",".i.....i.",B9]),
 ("HEART",    [B9,".ii...ii.","iiii.iiii","iiiiiiiii",".iiiiiii.","..iiiii..","...iii...","....i...."]),
 ("DIZZY",    [".kkkkkk..","k......k.","k.kkkk.k.","k.k..k.k.","k.k.kk.k.","k.k....k.","k.kkkkkk.",B9]),
 ("KNOCKED OUT",[B9,".k.....k.","..k...k..","...k.k...","....k....","...k.k...","..k...k..",".k.....k."]),
 ("SPARKLY",  ["..kkkkk..",".kiiiiik.","kiwwiiiik","kiwwiiiik","kiiiiiiik","kiiiiwiik",".kiiiiik.","..kkkkk.."]),
 ("CLOSED",   [B9,B9,B9,".k.....k.","..kkkkk..",B9,B9,B9]),
 ("SQUINT",   [B9,B9,".kk......","...kkk...",".....kkk.","...kkk...",".kk......",B9]),
 ("SNAKE",    [B9,"..kkkkk..",".kiikiik.","kiiikiiik","kiiikiiik",".kiikiik.","..kkkkk..",B9]),
 ("GOAT",     [B9,"..kkkkk..",".kiiiiik.","kikkkkkik",".kiiiiik.","..kkkkk..",B9,B9]),
 ("ROBOT",    [B9,"kkkkkkkkk","kiiiiiiik","kiwwiiiik","kiiiiiiik","kiiiiiiik","kkkkkkkkk",B9]),
 ("BUG",      ["..kkkkk..",".kkkkkkk.","kkwwkkkkk","kkwkkkkkk","kkkkkkkkk","kkkkkkkkk",".kkkkkkk.","..kkkkk.."]),
 ("GLOWING",  [B9,"..wwwww..",".wwwwwww.",".wwwwwww.",".wwwwwww.","..wwwww..",B9,B9]),
 ("CRYING",   [B9,"..kkkkk..",".kwwiwwk.",".kwikiwk.",".kwwwwwk.","..kkkkk..","......t..","......t.."]),
 ("SAD",      [B9,"......kk.","....kkk..",".kkkwwwk.",".kwwikwk.","..kwiiwk.","...kkkk..",B9]),
 ("SHOCKED",  [".kkkkkkk.","kwwwwwwwk","kwwwwwwwk","kwwwkwwwk","kwwwwwwwk","kwwwwwwwk",".kkkkkkk.",B9]),
 ("LOOK UP",  [B9,"..kkkkk..",".kwiiiwk.",".kwikiwk.",".kwwwwwk.",".kwwwwwk.","..kkkkk..",B9]),
 ("DREAMY",   [B9,B9,".sssssss.",".kkkkkkk.",".kwiiiwk.","..kiiik..","...kkk...",B9]),
 ("BEADY",    [B9,B9,"...kkk...","..kkwkk..","..kkkkk..","...kkk...",B9,B9]),
 ("PLUS",     [B9,"....i....","....i....","..iiiii..","....i....","....i....",B9,B9]),
 ("DIAMOND",  [B9,"....k....","...kik...","..kiwik..","..kiiik..","...kik...","....k....",B9]),
 ("MOON",     [B9,"...iii...","..ii.....",".ii......",".ii......","..ii.....","...iii...",B9]),
 ("FLOWER",   ["....b....","..b.b.b..","...bbb...",".bbbybbb.","...bbb...","..b.b.b..","....b....",B9]),
 ("TIRED",    [B9,B9,".kkkkkkk.",".kwwiwwk.","..kwiwk..","...kkk...",".sssssss.","..sssss.."]),
 ("WISE",     [B9,"..kkkkk..","skwwiwwk.",".kwikiwk.","skwwwwwk.","..kkkkk..","s........",B9]),
 ("WINGED",   [B9,"kk.......",".kkkkkkk.",".kwwiwwk.",".kwikiwk.","..kwwwk..","...kkk...",B9]),
 ("STARRY",   ["..kkkkk..",".kiiwiik.","kiwwwwwik","kiiwwwiik","kiwiiiwik",".kiiiiik.","..kkkkk..",B9]),
 ("BLANK",    [B9,"..kkkkk..",".kwwwwwk.",".kwwwwwk.",".kwwwwwk.","..kkkkk..",B9,B9]),
 ("HYPNO",    ["..kkkkk..",".kwwwwwk.","kwkkkkkwk","kwkwwwkwk","kwkwkwkwk","kwkwwwkwk",".kwkkkwk.","..kkkkk.."]),
 ("GOLD",     [B9,"..kkkkk..",".kyyyyyk.",".kyykyyk.",".kyykyyk.","..kkkkk..",B9,B9]),
 ("HAPPY TEARS",[B9,B9,"...kkk...","..k...k..",".k.....k.",B9,".t.......",".t......."]),
 ("ANIME",    ["kkkkkkkk.",".kiiiiik.",".kwiiiik.",".kiiiiik.",".kiikiik.",".kiiiiik.","..kkkkk..",B9]),
 ("SLY",      [B9,B9,"kkkkkkkkk",".kwwwiik.","..kkkkk..",B9,B9,B9]),
]
# ---- mouths: 19 x 8 ----
def m(rows, top=2): return rowsat(19, 8, top, rows)
MOUTHS = [
 ("TEETH",     m(["kkkkkkkkkkk","kwwwwwwwwwk",".kkkkkkkkk."])),
 ("BIG SMILE", m(["k.............k",".k...........k.","..kkkkkkkkkkk.."], 1)),
 ("LAUGH",     m(["kkkkkkkkkkk","kwwwwwwwwwk","krrrrrrrrrk",".krrrrrrrk.","..kkkkkkk.."])),
 ("WAVY",      m([".kk...kk...kk.","k..k.k..k.k..k"], 3)),
 ("GRIMACE",   m(["kkkkkkkkkkkkk","kwkwkwkwkwkwk","kkkkkkkkkkkkk"])),
 ("KISS",      m([".kk.","krrk",".kk."])),
 ("WHISTLE",   m([".kkk.","krrrk",".kkk."])),
 ("BITE LIP",  m(["kkkkkkkkkkk","..wwww.....","..rrrr....."])),
 ("GAP TOOTH", m(["kkkkkkkkkkkkk","kwwwww.wwwwwk",".kkkkkkkkkkk."])),
 ("BRACES",    m(["kkkkkkkkkkkkk","kwmwmwmwmwmwk",".kkkkkkkkkkk."])),
 ("VAMPIRE",   m(["kkkkkkkkkkk",".w.......w.",".w.......w."])),
 ("SHOCKED",   m([".kkkkk.","krrrrrk","krrrrrk","krrrrrk",".kkkkk."], 1)),
 ("YELL",      m(["kkkkkkkkkkk","kwwwwwwwwwk","krrrrrrrrrk","krrrrrrrrrk",".kkkkkkkkk."], 1)),
 ("LOPSIDED",  m(["k..........",".k.........","..kkkkkkkkk"])),
 ("POUT",      m(["...kkkkk...","..krrrrrk..","...kkkkk..."])),
 ("DROOL",     m([".kkkkkkkkk.","........t..","........t.."], 3)),
 ("TONGUE SIDE",m(["kkkkkkkkkkk","......krrk.",".......kk.."])),
 ("SMALL SMILE",m(["k.....k",".kkkkk."], 3)),
 ("TINY O",    m([".kk.","k..k",".kk."])),
 ("CHEESY",    m(["k...............k","kkkkkkkkkkkkkkkkk",".kwwwwwwwwwwwwwk.","..kkkkkkkkkkkkk.."], 1)),
 ("NERVOUS",   m([".k.k.k.k.k.","k.k.k.k.k.k"], 3)),
 ("OPEN SMILE",m(["kkkkkkkkkkk",".krrrrrrrk.","..krrrrrk..","...kkkkk..."])),
 ("BLEH",      m(["kkkkkkkkk","...krrk..","...krrk..","....kk..."])),
 ("DIMPLES",   m(["s.k.........k.s","...k.......k...","....kkkkkkk...."])),
 ("ONE TOOTH", m(["kkkkkkkkkkk",".....w.....",".....w....."], 3)),
 ("MONSTER",   m(["kkkkkkkkkkkkk","kw.w.w.w.w.wk",".k.........k.","..kkkkkkkkk.."])),
 ("BUCK TEETH",m(["kkkkkkkkkkk","....kwk....","....kwk....","....kkk...."])),
 ("SNEER",     m(["........kkk","kkkkkkkkwwk","..........."])),
 ("MEOW",      m(["k...k...k",".kkk.kkk."], 3)),
 ("SING",      m(["..kkkkk..",".krrrrrk.","krrrrrrrk",".krrrrrk.","..kkkkk.."], 1)),
]
# ---- brows: 9 x 2, over the eye's top two rows ----
BROWS = [
 ("ARCHED",  ["..hhhhh..",".h.....h."]),
 ("BUSHY",   [".hhhhhhh.","hhhhhhhh."]),
 ("SHORT",   ["...hhh...",B9]),
 ("DOTS",    ["......hh.","......hh."]),
 ("BOLD",    ["..hhhhh..","..hhhhh.."]),
 ("SPLIT",   [".hh.hhhh.",".hh.hhhh."]),
 ("FIERCE",  ["hhhh.....","..hhhhhhh"]),
 ("SAD THICK",[".....hhhh","hhhhhhh.."]),
 ("WAVY",    ["..hh...hh",".h..hhh.."]),
 ("LONG",    ["hhhhhhhhh",B9]),
 ("PIERCED", [".hhhhhmh.","......m.."]),
 ("TAPERED", ["..hhhhhhh",".hhh....."]),
 ("RAISED",  [".hhhhhh..",B9]),
]
# ---- noses: 19 x 2, the top two rows over the mouth ----
def n(a, b=""): return c(19, [a, b], 2)
NOSES = [
 ("BIG",      n("sssss", "sssssss")),
 ("LONG",     n("ss", "sss")),
 ("CLOWN",    n("rwr", "rrrrr")),
 ("TINY",     n("s")),
 ("NOSTRILS", n("", "k...k")),
 ("HOOKED",   n("sss.", "s..s")),
 ("FRECKLED", n("s.sss.s", ".s...s.")),
 ("RING",     n("sss", "...m.")),
 ("SEPTUM",   n("sss", "m")),
 ("ROUND",    n("sssss", "sssss")),
 ("SNUB",     n("", "sk.ks")),
 ("KITTY",    n("bbb", "k")),
 ("DOG",      n("kkkkk", "kwkkk")),
 ("SUNBURN",  n("bbbsssbbb", "bb...bb")),
]
# ---- cheeks and marks: 19 x 4 over the mouth (only where the mouth leaves skin) ----
B19 = "." * 19
CHEEKS = [
 ("BIG BLUSH",   ["bbbb...........bbbb","bbbb...........bbbb",".bb.............bb.",B19]),
 ("HEARTS",      ["b.b.............b.b","bbb.............bbb",".b...............b.",B19]),
 ("DIMPLES",     [B19,B19,"..s.............s..",B19]),
 ("MOLE",        [B19,"................n..",B19,B19]),
 ("BEAUTY MARK", [B19,"..n................",B19,B19]),
 ("STITCHES",    ["..k.k.k............","..kkkkk............","..k.k.k............",B19]),
 ("PLASTER",     ["...............aaa.","..............aaaa.","...............aa..",B19]),
 ("TEAR MARK",   ["..k................",".kk................",B19,B19]),
 ("STARS",       ["..y.............y..",".yyy...........yyy.","..y.............y..",B19]),
 ("SMILE LINES", [B19,"..s.............s..","...s...........s...",B19]),
 ("LOTS OF FRECKLES",[".s.s.s.......s.s.s.","s.s.s.........s.s.s",".s.s...........s.s.",B19]),
 ("WAR PAINT",   ["cccc...........cccc",B19,"cccc...........cccc",B19]),
 ("WHISKER DOTS",[B19,".k.k...........k.k.","..k.............k..",B19]),
 ("SWEAT",       ["t..................","tt.................","tt.................",B19]),
 ("GOLD STRIPES",["yyyyy.........yyyyy",B19,B19,B19]),
 ("CLOWN DOTS",  ["rrr.............rrr","rrr.............rrr",B19,B19]),
 ("GLITTER",     ["y..m...........m..y",".m..y.........y..m.","..y.............y..",B19]),
 ("ROSY",        ["bbbbb.........bbbbb",".bbb...........bbb.",B19,B19]),
]
# ---- glasses: 9 x 8 over the eye ----
GLASSES = [
 ("TINY ROUND", [B9,B9,"..ggggg..",".g.....g.","..ggggg..",B9,B9,B9]),
 ("BIG ROUND",  ["..ggggg..",".g.....g.","g.......g","g.......g","g.......g",".g.....g.","..ggggg..",B9]),
 ("CAT-EYE",    ["g........","ggggggggg","g.......g",".g.....g.","..ggggg..",B9,B9,B9]),
 ("HALF RIM",   [B9,"ggggggggg","g.......g",B9,B9,B9,B9,B9]),
 ("READING",    [B9,B9,B9,"g.......g",".g.....g.","..ggggg..",B9,B9]),
 ("THICK",      [B9,"ggggggggg","ggggggggg","gg.....gg","gg.....gg","ggggggggg",B9,B9]),
 ("AVIATORS",   [B9,"ggggggggg","glllllllg","glllllllg",".gllllll.","..gllll..",B9,B9]),
 ("HEART SHADES",[B9,".rr...rr.","rrrr.rrrr","rrrrrrrrr",".rrrrrrr.","..rrrrr..","...rrr...","....r...."]),
 ("GOGGLES",    [B9,".mmmmmmm.","mm.....mm","m.......m","m.......m","mm.....mm",".mmmmmmm.",B9]),
 ("SPORT",      [B9,"ggggggggg","lllllllll","lllllllll","ggggggggg",B9,B9,B9]),
 ("VISOR",      [B9,B9,"ccccccccc","ccccccccc",B9,B9,B9,B9]),
 ("GOLD RIMS",  [B9,"..yyyyy..",".y.....y.","yy.....yy",".y.....y.","..yyyyy..",B9,B9]),
 ("STAR SHADES",[B9,"....y....","...yly...","yyylllyyy",".ylllly..","..yl.ly..",".y.....y.",B9]),
]
MAKEUP = ["NONE","LINER","WINGED LINER","BIG LASHES","BLUE SHADOW","PURPLE SHADOW","GOLD SHADOW","PINK SHADOW","GREEN SHADOW","SMOKY","GLITTER"]
LIPS = ["NONE","RED","PINK","CORAL","BERRY","PURPLE","BLACK","GOLD","BLUE","NUDE","BROWN"]

def check(name, rows, w, h):
    assert len(rows) == h, (name, len(rows))
    for r in rows:
        assert len(r) == w, (name, r, len(r))
        for ch in r: assert ch == '.' or ch in KEYS, (name, ch)

o = ["// GENERATED by tools/make_face.py: the FACE tab's own art after the GBA game's (look_port.inc)", ""]
def sym(p, nm): return p + "".join(ch if ch.isalnum() else "_" for ch in nm.lower())
def emit(prefix, items, w, h):
    for nm, rows in items:
        check(nm, rows, w, h)
        o.append("static const char *const %s[%d] = { %s };" % (sym(prefix, nm), h, ", ".join('"%s"' % r for r in rows)))
emit("fe_", EYES, 9, 8); emit("fm_", MOUTHS, 19, 8); emit("fb_", BROWS, 9, 2); emit("fn_", NOSES, 19, 2); emit("fc_", CHEEKS, 19, 4); emit("fg_", GLASSES, 9, 8)
base_eyes = [("SLEEPY","aHalf"),("ROUND","aRound"),("HAPPY","aHappy"),("WIDE","aWide"),("ANGRY","aAngry"),("CUTE","aCute"),("CAT","aCat"),("DOT","aDot"),("LASHES","aLash")]
base_mouths = [("FLAT","mFlat"),("SMILE","mSmile"),("OH","mOh"),("GRIN","mGrin"),("SMIRK","mSmirk"),("FROWN","mFrown"),("TONGUE","mTongue"),("FANGS","mFangs"),("KITTY","mCat")]
def table(kind, cnt, names, arts):
    o.append("#define %s %d" % (cnt, len(names)))
    o.append("static const char *const %sNmAll[%s] = { %s };" % (kind, cnt, ",".join('"%s"' % x for x in names)))
    o.append("static const char *const *const %sArtAll[%s] = { %s };" % (kind, cnt, ", ".join(arts)))
table("eye", "NEYE_ALL", [a for a, _ in base_eyes] + [a for a, _ in EYES], [b for _, b in base_eyes] + [sym("fe_", a) for a, _ in EYES])
table("mouth", "NMOUTH_ALL", [a for a, _ in base_mouths] + [a for a, _ in MOUTHS], [b for _, b in base_mouths] + [sym("fm_", a) for a, _ in MOUTHS])
# (0 is NONE in these: the art table starts at 1)
table("brow", "NBROW_ALL", ["NONE","THIN","THICK","ANGRY","WORRIED","UNIBROW"] + [a for a, _ in BROWS], ["0"] + ["brArt[%d]" % i for i in range(5)] + [sym("fb_", a) for a, _ in BROWS])
table("nose", "NNOSE_ALL", ["NONE","BUTTON","POINTY","WIDE","PIG","ANIMAL"] + [a for a, _ in NOSES], ["0"] + ["noArt[%d]" % i for i in range(5)] + [sym("fn_", a) for a, _ in NOSES])
table("cheek", "NCHEEK_ALL", ["NONE","BLUSH","FRECKLES","WHISKERS","SCAR"] + [a for a, _ in CHEEKS], ["0"] + ["chArt[%d]" % i for i in range(4)] + [sym("fc_", a) for a, _ in CHEEKS])
table("glass", "NGLASS_ALL", ["NONE","ROUND","SQUARE","SHADES"] + [a for a, _ in GLASSES], ["0"] + ["glArt[%d]" % i for i in range(3)] + [sym("fg_", a) for a, _ in GLASSES])
o.append("#define NMAKEUP %d" % len(MAKEUP)); o.append("static const char *const makeupNm[NMAKEUP] = { %s };" % ",".join('"%s"' % x for x in MAKEUP))
o.append("#define NLIPS %d" % len(LIPS)); o.append("static const char *const lipsNm[NLIPS] = { %s };" % ",".join('"%s"' % x for x in LIPS))
o.append('#define FACE_KEYS "%s"' % KEYS)
open(OUT, "w").write("\n".join(o) + "\n")
print("face: %d eyes, %d mouths, %d brows, %d noses, %d cheeks, %d glasses, %d makeup, %d lips" % (9 + len(EYES), 9 + len(MOUTHS), 6 + len(BROWS), 6 + len(NOSES), 5 + len(CHEEKS), 4 + len(GLASSES), len(MAKEUP), len(LIPS)))
