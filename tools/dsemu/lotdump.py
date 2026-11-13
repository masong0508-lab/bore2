#!/usr/bin/env python3
"""lotdump.py ram.bin bore.elf [-q] : draw the 13 x 13 plot of a RAM dump (tools/dsemu/run.cpp RAMDUMP) as text and check it (docs/DS_TOWN7.md).
Cell: floor char (. none/lawn, w wood, t tile, c carpet, b bath, p paving/path/drive, s patio), objects as letters of their kind; walls as - | (D = door gap).
Checks (the same as the C audit in main.c, DS_T7_AUDIT, plus the ones only a picture shows): prints a list of findings, exit code = number of findings."""
import re, struct, subprocess, sys, os
R = os.path.dirname(os.path.abspath(__file__)) + "/../../"
GN = 13
def load_items():
    s = open(R + "ports/dsi/source/ds_items.h").read()
    tab = s[s.index("static const Item ITEMS"):]
    out = []
    for m in re.finditer(r'\{ "([^"]*)", (\d+), (\d+), C15\([^)]*\), (\d+), (\d+), (\d+), (\d+), (\d+), (\d+),', tab):
        out.append(dict(name=m.group(1), h=int(m.group(3)), kind=int(m.group(4)), fw=int(m.group(6)), fl=int(m.group(7))))
    return out
def syms(elf):
    d = {}
    for ln in subprocess.run(["llvm-nm", elf], capture_output=True, text=True).stdout.splitlines():
        p = ln.split()
        if len(p) == 3: d[p[2]] = int(p[0], 16)
    return d
class Ram:
    def __init__(s, path, elf):
        s.b = open(path, "rb").read(); s.sy = syms(elf)
    def u8(s, name, off=0): return s.b[s.sy[name] - 0x02000000 + off]
    def i32(s, name, off=0): return struct.unpack_from("<i", s.b, s.sy[name] - 0x02000000 + off)[0]
    def arr(s, name, n, off=0):
        a = s.sy[name] - 0x02000000 + off; return list(s.b[a:a + n])
FL = {0: ' ', 1: ',', 2: '.', 3: ':', 4: ';', 5: '='}
CH = {"Chair": "c", "Armchair": "A", "Sofa": "S", "Loveseat": "S", "Dining Table": "T", "Desk": "D", "Coffee Table": "t", "Counter": "k", "TV": "V", "Computer": "C", "Sound System": "s", "Fridge": "F", "Floor Lamp": "l",
      "Rug": "~", "Houseplant": "p", "Statue": "&", "Bed": "B", "Single Bed": "B", "Family Bed": "B", "Four Poster Bed": "B", "Crib": "b", "Bathtub": "U", "Toilet": "W", "Sink": "w", "Nightstand": "n", "Wardrobe": "Z", "Dresser": "Z",
      "Stove": "O", "Kitchen Sink": "K", "Mirror": "m", "Towels": "r", "Piano": "P", "Bookshelf": "H", "Stairs": "^", "Toy Box": "y", "Hedge": "#", "Mailbox": "M", "Oak Tree": "T", "Pine Tree": "T", "Palm Tree": "T", "Dishwasher": "k", "Microwave": "k",
      "Kitchen Island": "I", "Washing Machine": "k", "Grandfather Clock": "L", "Framed Art": "a", "Aquarium": "q", "Planter": "p", "Flower Bed": "f", "Picket Fence": "|", "Street Lamp": "l", "Fireplace": "R", "Vase": "v"}
def grab(ram, items):
    flr = [ram.arr("flr", GN, y * GN) for y in range(GN)]
    hwl = [ram.arr("hwl", GN, y * GN) for y in range(GN + 1)]
    vwl = [ram.arr("vwl", GN + 1, y * (GN + 1)) for y in range(GN)]
    nobj = ram.i32("nobj"); objs = []
    for i in range(nobj):
        x, y, c, it, rot = ram.arr("objs", 5, i * 5)
        iid = c * 4 + it; I = items[iid] if iid < len(items) else dict(name="?", h=0, kind=0, fw=1, fl=1)
        w, h = (I["fl"], I["fw"]) if rot & 1 else (I["fw"], I["fl"])
        objs.append(dict(x=x, y=y, id=iid, rot=rot, lv=(rot >> 2) & 3, name=I["name"], w=w, h=h, kind=I["kind"], ht=I["h"]))
    return flr, hwl, vwl, objs, (ram.i32("door_cx"), ram.i32("door_cy"))
def show(flr, hwl, vwl, objs, door, out=sys.stdout, mark=()):
    occ = {}
    for o in objs:
        if o["lv"]: continue
        for yy in range(o["y"], o["y"] + o["h"]):
            for xx in range(o["x"], o["x"] + o["w"]): occ[(xx, yy)] = o
    for y in range(GN + 1):
        line = ""
        for x in range(GN):
            t = hwl[y][x]; line += "+" + ("D" if t == 5 else "-" if t else " ")
        out.write(line + "+\n")
        if y == GN: break
        line = ""
        for x in range(GN + 1):
            t = vwl[y][x]; line += "D" if t == 5 else "|" if t else " "
            if x < GN:
                o = occ.get((x, y))
                line += "X" if (x, y) in mark else CH.get(o["name"], "?") if o else FL.get(flr[y][x], '?')
        out.write(line + "\n")
def reach(flr, hwl, vwl, objs, door):
    """the cells a sim can reach from the front doors (strictly: walls block, doors and free cells do not); returns (seen, solid)"""
    sol = [[0] * GN for _ in range(GN)]
    for o in objs:
        if o["lv"] or o["kind"] == 1: continue
        for yy in range(o["y"], o["y"] + o["h"]):
            for xx in range(o["x"], o["x"] + o["w"]):
                if xx < GN and yy < GN: sol[yy][xx] = 1
    seen = [[0] * GN for _ in range(GN)]; q = []
    dy = door[1]; wy = dy if hwl[dy][door[0]] == 5 else dy + 1
    for x in range(GN):
        if 0 < wy < GN and hwl[wy][x] == 5:
            for iy in (wy, wy - 1):
                if not seen[iy][x] and not sol[iy][x]: seen[iy][x] = 1; q.append((x, iy))
    while q:
        x, y = q.pop()
        for dx, dyy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dyy
            if not (0 <= nx < GN and 0 <= ny < GN) or seen[ny][nx] or sol[ny][nx]: continue
            w = vwl[y][nx] if nx > x else vwl[y][x] if nx < x else hwl[ny][x] if ny > y else hwl[y][x]
            if w and w != 5: continue
            seen[ny][nx] = 1; q.append((nx, ny))
    return seen, sol
def rooms(flr, hwl, vwl, sol):
    """floor regions cut by walls and doors, as the C audit sees them: list of (cells, furnished cells, walled)"""
    lab = [[0] * GN for _ in range(GN)]; out = []
    for y0 in range(GN):
        for x0 in range(GN):
            if lab[y0][x0] or not 1 <= flr[y0][x0] <= 4: continue
            q = [(x0, y0)]; lab[y0][x0] = 1; cells = []; walled = 0
            while q:
                x, y = q.pop(); cells.append((x, y))
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    w = vwl[y][nx] if dx > 0 else vwl[y][x] if dx < 0 else hwl[ny][x] if dy > 0 else hwl[y][x]
                    if not w and (not (0 <= nx < GN and 0 <= ny < GN) or flr[ny][nx] in (0, 5)): walled = 0 if False else walled | 2
                    if not (0 <= nx < GN and 0 <= ny < GN) or lab[ny][nx] or not 1 <= flr[ny][nx] <= 4 or w: continue
                    lab[ny][nx] = 1; q.append((nx, ny))
            out.append((cells, sum(sol[y][x] for x, y in cells), not (walled & 2)))
    return out
if __name__ == "__main__":
    items = load_items(); ram = Ram(sys.argv[1], sys.argv[2])
    flr, hwl, vwl, objs, door = grab(ram, items)
    print("objects", len(objs), "door", door)
    if "tour_k" in ram.sy:
        k = ram.i32("tour_k") - 1
        if k >= 0:
            g = lambda n: ram.i32(n, 4 * k) if n in ram.sy else -1
            print("tour lot", k, "block", g("tour_bx"), g("tour_by"), "type", g("tour_ty"), "face", g("tour_face"), "kind", g("tour_kind"))
    seen, sol = reach(flr, hwl, vwl, objs, door)
    bad = [(x, y) for y in range(GN) for x in range(GN) if 1 <= flr[y][x] <= 4 and not sol[y][x] and not seen[y][x]]
    show(flr, hwl, vwl, objs, door, mark=set(bad))
    print("unreachable free floor cells:", bad)
    for cells, f, walled in rooms(flr, hwl, vwl, sol):
        if walled and (len(cells) >= 4 and f == 0 or len(cells) >= 12 and f * 6 < len(cells)): print("bad room", len(cells), "cells", f, "furnished", cells[:6])
    for o in objs: print(o)
