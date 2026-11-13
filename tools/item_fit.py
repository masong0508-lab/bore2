#!/usr/bin/env python3
# item_fit.py: the size fixes the item generators (make_items_q.py, make_items2.py) apply, so a model matches its real size (docs/OBJECT_SCALE.md).
# A box is (a0, b0, a1, b1, z0, z1, mats...): a across, b along (8 units = one tile = 1.13 m), z in pixels (1 px = 10.6 cm; a person is 16).
def scale(boxes, sa=8, sb=8, oa=0, ob=0, zn=0, zd=0):
    """a and b stretched by sa/8 and sb/8 (then moved oa, ob units); z squeezed from zd to zn (a thing zd px tall made zn px tall)."""
    out = []
    for bx in boxes:
        a0, b0, a1, b1, z0, z1 = bx[:6]
        na0, na1 = (a0 * sa + 4) // 8 + oa, (a1 * sa + 4) // 8 + oa
        nb0, nb1 = (b0 * sb + 4) // 8 + ob, (b1 * sb + 4) // 8 + ob
        if na1 <= na0: na1 = na0 + 1
        if nb1 <= nb0: nb1 = nb0 + 1
        if zd:
            z0, z1 = (z0 * zn + zd // 2) // zd, (z1 * zn + zd // 2) // zd
            if z1 <= z0: z1 = z0 + 1
        out.append((na0, nb0, na1, nb1, z0, z1) + tuple(bx[6:]))
    return out
def shift(boxes, oa=0, ob=0): return scale(boxes, 8, 8, oa, ob)
def top(boxes): return max(b[5] for b in boxes)

# by name (make_items2.py): (sa, sb, oa, ob, height px or 0 for as it is)
FIT2 = {
    'BUNK BED':          (8, 8, 0, 0, 15),      # 1.6 m high (it was 1.4 m); one tile, the houses are built with it
    'FOUR POSTER BED':   (12, 15, 2, 0, 0),     # 1.7 x 2.1 m on a 2 x 2 tile footprint, centred (it was one tile)
    'NIGHTSTAND':        (8, 8, 0, 0, 8),       # a table with a lamp: 0.85 m (it was 1.17 m)
    'WARDROBE':          (8, 8, 0, 0, 18),      # 1.9 m (it was 2.1 m: nearly the ceiling)
    'GRANDFATHER CLOCK': (8, 8, 0, 0, 18),      # 1.9 m (it was 2.3 m: through the ceiling)
}
def fit2(name, boxes):
    f = FIT2.get(name)
    if not f: return boxes
    sa, sb, oa, ob, h = f
    return scale(boxes, sa, sb, oa, ob, h, top(boxes) if h else 0)
