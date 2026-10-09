#!/usr/bin/env python3
"""BORE 2's wardrobe: every accessory of the creator's WEAR tab, as small voxel models (boxes), plus the clothes patterns.
Writes src/wear_names.inc (names, colours, counts: look.c) and src/wear_models.inc (the boxes: voxel.c).

A box is (x0, x1, y0, y1, z0, z1, colour) in the accessory's own units, around its anchor:
  x  across the body (left -, right +)        y  UP from the anchor            z  OUT from the surface it sits on
Units: HEAD and FACE items use head units (the head is 16 wide: x -8..8); NECK, BACK, HAND and WAIST items use body units
(48/8 = 6 world units; the torso is about 16 wide and 16 deep).
Anchors:  HEAD   the top centre of the head, on the hair          FACE  the front of the face, on the eye line
          NECK   the front of the torso, on the shoulder line      BACK  the back of the torso, on the shoulder line
          HAND   the bottom of the right hand (moves with the arm)   WAIST the front of the torso, on the hip line
Colours: A and B are the two the player picks (COLOUR 1 and 2); the rest are fixed."""
import os
A, B, GOLD, BLACK, WHITE, SILVER, RED, GREEN, YELLOW, BROWN, HAIR, SKIN, PINK, ORANGE = range(14)

def mirror(*boxes):   # a box and its mirror image across x = 0
    out = []
    for (x0, x1, y0, y1, z0, z1, c) in boxes: out += [(x0, x1, y0, y1, z0, z1, c), (-x1, -x0, y0, y1, z0, z1, c)]
    return out
def ring(x0, x1, z0, z1, y0, y1, t, c):   # a hollow rectangle (a crown, a halo, a hoop): four bars t thick
    return [(x0, x1, y0, y1, z0, z0 + t, c), (x0, x1, y0, y1, z1 - t, z1, c), (x0, x0 + t, y0, y1, z0 + t, z1 - t, c), (x1 - t, x1, y0, y1, z0 + t, z1 - t, c)]
def tiers(steps, c, y0=0, cx=0, cz=0):   # a cone of shrinking squares: steps = [(half width, height), ...]
    out = []; y = y0
    for (hw, h) in steps: out.append((cx - hw, cx + hw, y, y + h, cz - hw, cz + hw, c)); y += h
    return out

HEAD = [
    ('NONE', []),
    ('TOP HAT', [(-11, 11, 0, 1, -11, 11, A), (-7, 7, 1, 14, -7, 7, A), (-8, 8, 1, 3, -8, 8, B)]),
    ('COWBOY HAT', [(-13, 13, 0, 1, -12, 12, A), (-14, -11, 1, 3, -12, 12, A), (11, 14, 1, 3, -12, 12, A), (-7, 7, 1, 8, -6, 6, A), (-6, 6, 8, 9, -5, 5, A), (-8, 8, 1, 2, -7, 7, B)]),
    ('CROWN', ring(-8, 8, -8, 8, 0, 4, 1, GOLD) + mirror((-8, -6, 4, 7, 7, 8, GOLD), (-1, 1, 4, 8, 7, 8, GOLD)) + mirror((-8, -6, 4, 7, -8, -7, GOLD)) + [(-1, 1, 1, 3, 8, 9, A), (-1, 1, 4, 8, -8, -7, GOLD)]),
    ('CHEF HAT', [(-8, 8, 0, 3, -8, 8, WHITE), (-10, 10, 3, 10, -9, 9, WHITE), (-8, 8, 10, 12, -7, 7, WHITE)]),
    ('PARTY HAT', tiers([(6, 4), (4, 4), (2, 4), (1, 3)], A) + [(-4, 4, 4, 5, -4, 4, B), (-2, 2, 15, 17, -2, 2, WHITE)]),
    ('WIZARD HAT', [(-12, 12, 0, 1, -12, 12, A)] + tiers([(7, 5), (5, 5), (3, 5)], A, 1) + [(-1, 2, 16, 20, -2, 1, A), (-8, 8, 1, 2, -8, 8, B), (2, 4, 6, 8, 7, 8, YELLOW), (-5, -3, 3, 5, 7, 8, YELLOW)]),
    ('BIG BOW', [(-2, 2, -1, 3, -2, 2, A), (-9, -2, -2, 5, -2, 2, A), (2, 9, -2, 5, -2, 2, A), (-2, 2, 0, 2, 2, 3, B)]),
    ('FLOWER', [(3, 6, 0, 3, 2, 5, YELLOW), (0, 3, 0, 2, 2, 5, A), (6, 9, 0, 2, 2, 5, A), (3, 6, 0, 2, -1, 2, A), (3, 6, 0, 2, 5, 8, A)]),
    ('HEADPHONES', [(-9, 9, 0, 2, -1, 1, B)] + mirror((8, 10, -6, 0, -1, 1, B), (8, 11, -13, -5, -3, 3, A))),
    ('VIKING HELMET', [(-9, 9, -3, 4, -9, 9, SILVER), (-6, 6, 4, 6, -6, 6, SILVER), (-10, 10, -4, -2, -10, 10, BROWN)] + mirror((9, 12, 0, 3, -2, 2, WHITE), (11, 13, 3, 8, -2, 2, WHITE), (12, 13, 8, 11, -1, 1, WHITE))),
    ('HALO', ring(-7, 7, -7, 7, 5, 6, 2, GOLD)),
    ('BUNNY EARS', [(-9, 9, 0, 1, -1, 1, A)] + mirror((3, 6, 1, 15, -1, 1, A), (4, 5, 3, 13, 1, 2, PINK))),
    ('CAT EARS', [(-9, 9, 0, 1, -1, 1, A)] + mirror((3, 8, 1, 4, -1, 1, A), (4, 7, 4, 6, -1, 1, A), (5, 6, 6, 7, -1, 1, A), (4, 7, 1, 4, 1, 2, PINK))),
    ('BERET', [(-10, 9, 0, 3, -9, 9, A), (-7, 6, 3, 4, -6, 6, A), (-1, 1, 4, 5, -1, 1, A)]),
    ('HARD HAT', [(-9, 9, 0, 6, -9, 9, YELLOW), (-10, 10, 0, 1, -10, 13, YELLOW), (-1, 1, 6, 7, -8, 8, YELLOW)]),
    ('PROPELLER CAP', [(-9, 9, 0, 4, -9, 9, A), (-9, 9, 0, 1, 8, 13, A), (-9, -3, 0, 4, -9, 9, B), (3, 9, 0, 4, -9, 9, B), (-1, 1, 4, 7, -1, 1, BLACK), (-9, 9, 7, 8, -1, 1, YELLOW)]),
    ('TIARA', [(-6, 6, 0, 2, 6, 8, SILVER), (-3, 3, 2, 4, 6, 8, SILVER), (-1, 1, 4, 6, 6, 8, SILVER), (-1, 1, 2, 4, 8, 9, A)]),
    ('PIRATE HAT', [(-12, 12, 0, 3, -6, 6, BLACK), (-6, 6, 3, 7, -6, 6, BLACK), (-1, 1, 3, 5, 6, 7, WHITE), (-12, 12, 0, 1, -6, 6, GOLD)]),
    ('SOMBRERO', [(-16, 16, 0, 1, -16, 16, A), (-15, 15, 1, 2, -15, -13, A), (-15, 15, 1, 2, 13, 15, A), (-6, 6, 1, 9, -6, 6, A), (-4, 4, 9, 11, -4, 4, A), (-7, 7, 1, 3, -7, 7, B)]),
    ('GRAD CAP', [(-8, 8, 0, 3, -8, 8, BLACK), (-11, 11, 3, 4, -11, 11, BLACK), (-1, 1, 4, 5, -1, 1, GOLD), (9, 10, -4, 4, 0, 1, GOLD)]),
    ('SAILOR CAP', [(-9, 9, 0, 5, -9, 9, WHITE), (-9, 9, 1, 2, -9, 9, B), (-10, 10, 5, 6, -10, 10, WHITE)]),
    ('HEAD WRAP', [(-9, 9, -5, 1, -9, 9, A), (-2, 2, -7, -3, -11, -9, A), (-3, -1, -10, -6, -11, -9, A), (1, 3, -10, -6, -11, -9, A)]),
    ('ANTLERS', [(-9, 9, 0, 1, -1, 1, BROWN)] + mirror((4, 6, 1, 8, -1, 1, BROWN), (6, 10, 6, 8, -1, 1, BROWN), (8, 10, 8, 12, -1, 1, BROWN), (2, 4, 6, 10, -1, 1, BROWN))),
]
FACE = [
    ('NONE', []),
    ('EYE PATCH', [(1, 6, -2, 2, 0, 1, BLACK), (-8, 8, 2, 3, 0, 1, BLACK)]),
    ('MONOCLE', ring(1, 6, -2, 3, 0, 0, 1, GOLD)[:0] + [(1, 6, 2, 3, 0, 1, GOLD), (1, 6, -3, -2, 0, 1, GOLD), (1, 2, -2, 2, 0, 1, GOLD), (5, 6, -2, 2, 0, 1, GOLD), (5, 6, -9, -3, 0, 1, GOLD)]),
    ('VR VISOR', [(-8, 8, -3, 3, 0, 4, BLACK), (-6, 6, -2, 2, 4, 5, A)] + mirror((8, 9, -1, 1, -9, 2, BLACK))),
    ('SKI GOGGLES', [(-7, 7, -2, 3, 0, 2, A), (-6, 6, -1, 2, 2, 3, B)] + mirror((7, 9, -1, 1, -9, 1, BLACK))),
    ('FACE MASK', [(-6, 6, -9, -3, 0, 1, A)] + mirror((6, 9, -6, -5, -6, 1, WHITE))),
    ('CLOWN NOSE', [(-2, 2, -5, -2, 0, 3, RED)]),
    ('MUSTACHE', [(-4, 4, -6, -5, 0, 1, HAIR)] + mirror((4, 6, -7, -5, 0, 1, HAIR))),
    ('PIPE', [(1, 6, -7, -6, 0, 4, BROWN), (5, 7, -7, -3, 3, 6, BROWN)]),
    ('LOLLIPOP', [(2, 3, -7, -6, 0, 6, WHITE), (0, 5, -9, -4, 6, 7, A)]),
    ('FUNKY SHADES', [(-7, -1, -2, 3, 0, 1, A), (1, 7, -2, 3, 0, 1, A), (-1, 1, 1, 2, 0, 1, B), (-8, -7, 3, 4, 0, 1, B), (7, 8, 3, 4, 0, 1, B)]),
    ('3D GLASSES', [(-7, 7, 2, 3, 0, 1, WHITE), (-6, -1, -1, 2, 0, 1, RED), (1, 6, -1, 2, 0, 1, SILVER), (-7, -6, -1, 2, 0, 1, WHITE), (6, 7, -1, 2, 0, 1, WHITE)]),
    ('NOSE RING', [(-1, 1, -6, -5, 0, 2, GOLD)]),
    ('BUBBLE GUM', [(-3, 3, -10, -4, 0, 5, PINK), (-2, 2, -9, -5, 5, 6, PINK)]),
]
NECK = [
    ('NONE', []),
    ('SCARF', [(-9, 9, -3, 1, -17, 1, A), (3, 7, -13, -3, 1, 2, A), (3, 7, -9, -8, 2, 3, B)]),
    ('BOW TIE', [(-1, 1, -3, -1, 0, 2, A), (-5, -1, -4, 0, 0, 2, A), (1, 5, -4, 0, 0, 2, A)]),
    ('TIE', [(-4, 4, -2, 0, 0, 1, WHITE), (-1, 1, -3, -1, 0, 2, A), (-2, 2, -13, -3, 0, 1, A), (-1, 1, -15, -13, 0, 1, A), (-2, 2, -8, -7, 1, 2, B)]),
    ('GOLD CHAIN', [(-6, 6, -7, -6, 0, 1, GOLD), (-2, 2, -11, -7, 0, 1, GOLD)] + mirror((6, 7, -6, 0, 0, 1, GOLD))),
    ('PEARLS', [(-6, 6, -5, -4, 0, 1, WHITE)] + mirror((6, 7, -4, 0, 0, 1, WHITE))),
    ('MEDAL', [(-1, 1, -8, 0, 0, 1, A), (-2, 2, -12, -8, 0, 1, GOLD)]),
    ('BELL COLLAR', [(-9, 9, -2, 0, -17, 1, A), (-2, 2, -6, -2, 1, 4, GOLD)]),
    ('FLOWER LEI', [(-8, 8, -6, -4, 0, 2, A), (-6, -4, -6, -4, 2, 3, B), (-1, 1, -7, -5, 2, 3, B), (4, 6, -6, -4, 2, 3, B)] + mirror((8, 9, -5, 0, -2, 2, A))),
    ('BANDANA', [(-6, 6, -3, -1, 0, 1, A), (-4, 4, -5, -3, 0, 1, A), (-2, 2, -7, -5, 0, 1, A), (-1, 1, -4, -3, 1, 2, B)]),
    ('HEART LOCKET', [(-5, 5, -6, -5, 0, 1, GOLD), (-3, 3, -9, -6, 0, 1, RED), (-2, 2, -10, -9, 0, 1, RED), (-3, -1, -6, -5, 1, 2, RED), (1, 3, -6, -5, 1, 2, RED)]),
]
BACK = [
    ('NONE', []),
    ('BACKPACK', [(-6, 6, -14, -2, 0, 6, A), (-4, 4, -12, -7, 6, 8, B)] + mirror((4, 6, -1, 1, -16, 0, B))),
    ('CAPE', [(-9, 9, -30, 0, 0, 1, A), (-9, 9, -1, 1, 0, 2, B), (-10, 10, -30, -26, 0, 2, A)]),
    ('GUITAR', [(-5, 5, -26, -16, 0, 3, A), (-4, 4, -16, -10, 0, 3, A), (-2, 2, -20, -18, 3, 4, BLACK), (-1, 1, -10, 6, 1, 2, BROWN), (-2, 2, 6, 9, 1, 2, BLACK), (-6, -4, -12, 2, 0, 1, B)]),
    ('JETPACK', [(-7, -1, -16, 0, 0, 6, SILVER), (1, 7, -16, 0, 0, 6, SILVER), (-6, -2, -19, -16, 1, 5, ORANGE), (2, 6, -19, -16, 1, 5, ORANGE), (-5, -3, -22, -19, 2, 4, YELLOW), (3, 5, -22, -19, 2, 4, YELLOW), (-1, 1, -12, -4, 2, 5, A)]),
    ('TURTLE SHELL', [(-9, 9, -20, 0, 0, 5, A), (-7, 7, -18, -2, 5, 7, A), (-3, 3, -13, -7, 7, 8, B), (-8, -4, -6, -2, 5, 6, B), (4, 8, -6, -2, 5, 6, B), (-8, -4, -18, -14, 5, 6, B), (4, 8, -18, -14, 5, 6, B)]),
    ('FAIRY WINGS', mirror((2, 20, -12, 6, 1, 2, A), (2, 14, -22, -12, 1, 2, B), (14, 18, 2, 6, 2, 3, B))),
    ('ANGEL WINGS', mirror((3, 14, -10, 4, 1, 3, WHITE), (8, 20, -6, 6, 1, 3, WHITE), (14, 24, 0, 8, 1, 3, WHITE), (6, 16, -16, -10, 1, 3, WHITE))),
    ('SKATEBOARD', [(-4, 4, -26, 4, 0, 1, A), (-4, 4, -24, -22, 1, 2, BLACK), (-4, 4, 0, 2, 1, 2, BLACK), (-3, 3, -18, -6, -1, 0, B)]),
    ('SCUBA TANK', [(-4, 4, -22, 0, 0, 7, A), (-1, 1, 0, 3, 2, 5, SILVER), (-4, 4, -10, -8, 7, 8, BLACK)]),
    ('BEDROLL', [(-10, 10, -3, 3, 0, 6, A)] + mirror((5, 7, -4, 4, -1, 7, B))),
]
HAND = [
    ('NOTHING', []),
    ('BALLOON', [(0, 1, 0, 44, 1, 2, WHITE), (-5, 6, 44, 56, -4, 6, A), (-3, 4, 56, 58, -2, 4, A), (-1, 2, 42, 44, 0, 3, A)]),
    ('PHONE', [(-2, 2, -5, 4, 2, 4, BLACK), (-2, 2, -4, 3, 4, 5, A)]),
    ('ICE CREAM', [(-1, 1, -2, 2, 2, 4, BROWN), (-2, 2, 2, 5, 1, 5, A), (-1, 1, 5, 6, 2, 4, B)]),
    ('COFFEE', [(-2, 2, -4, 2, 2, 6, WHITE), (-2, 2, -2, 0, 2, 6, BROWN), (-2, 2, 2, 3, 2, 6, BLACK)]),
    ('BOOK', [(-1, 1, -8, 2, -4, 6, A), (-1, 1, -7, 1, 6, 7, WHITE)]),
    ('UMBRELLA', [(0, 1, -4, 40, 1, 2, BLACK), (-12, 12, 40, 43, -11, 13, A), (-8, 8, 43, 46, -7, 9, A), (-2, 2, 46, 48, -1, 3, B), (-12, -8, 40, 43, -11, 13, B), (8, 12, 40, 43, -11, 13, B)]),
    ('MICROPHONE', [(-1, 1, -6, 4, 1, 3, BLACK), (-2, 2, 4, 8, 0, 4, SILVER)]),
    ('FLOWERS', [(-2, 2, -8, 2, 1, 3, GREEN), (-4, 4, 2, 6, -1, 5, A), (-2, 2, 6, 8, 1, 3, B), (-5, -3, 3, 5, 1, 3, B)]),
    ('TEDDY BEAR', [(-3, 3, -10, -2, 1, 6, BROWN), (-3, 3, -2, 3, 1, 6, BROWN), (-4, -2, 3, 5, 2, 4, BROWN), (2, 4, 3, 5, 2, 4, BROWN), (-1, 1, -1, 1, 6, 7, BLACK), (-3, 3, -5, -4, 6, 7, A)]),
    ('LANTERN', [(-1, 1, 0, 2, 1, 3, BLACK), (-3, 3, -8, 0, -1, 5, A), (-2, 2, -7, -1, 5, 6, YELLOW), (-4, 4, -9, -8, -2, 6, BLACK)]),
    ('MAGIC WAND', [(0, 1, -4, 10, 1, 2, BLACK), (-2, 3, 10, 14, 0, 3, GOLD), (-1, 2, 14, 15, 1, 2, GOLD)]),
    ('GAME PAD', [(-4, 4, -3, 0, 2, 6, A), (2, 3, 0, 1, 3, 4, RED), (-3, -1, 0, 1, 3, 5, BLACK)]),
    ('TROPHY', [(-3, 3, -8, -6, 1, 7, BLACK), (-1, 1, -6, -3, 3, 5, GOLD), (-3, 3, -3, 3, 1, 7, GOLD), (-5, -3, -1, 2, 3, 5, GOLD), (3, 5, -1, 2, 3, 5, GOLD)]),
    ('BAGUETTE', [(-1, 1, -6, 14, 1, 3, BROWN), (-1, 1, 2, 3, 3, 4, YELLOW), (-1, 1, 7, 8, 3, 4, YELLOW)]),
]
WAIST = [
    ('NONE', []),
    ('BELT', [(-9, 9, 0, 2, -17, 1, A), (-2, 2, 0, 2, 1, 2, GOLD)]),
    ('TOOL BELT', [(-9, 9, 0, 2, -17, 1, BROWN), (-8, -4, -5, 1, 1, 4, A), (4, 8, -5, 1, 1, 4, A), (6, 7, -9, -1, 4, 5, SILVER), (5, 8, -1, 1, 4, 5, SILVER)]),
    ('FANNY PACK', [(-9, 9, 1, 2, -17, 1, BLACK), (-5, 5, -3, 3, 1, 5, A), (-4, 4, 1, 2, 5, 6, B)]),
    ('TUTU', [(-11, 11, -2, 1, -19, 3, A), (-10, 10, -4, -2, -18, 2, A), (-9, 9, 1, 2, -17, 1, B)]),
    ('GRASS SKIRT', [(-10, 10, -10, 1, -18, 2, GREEN), (-10, 10, 0, 2, -18, 2, B)]),
    ('APRON', [(-7, 7, -18, 2, 1, 2, A), (-5, 5, 2, 14, 1, 2, A), (-9, 9, 0, 1, -17, 1, B), (-3, 3, -10, -6, 2, 3, B)]),
    ('KARATE BELT', [(-9, 9, 0, 2, -17, 1, A), (-3, -1, -8, 0, 1, 2, A), (1, 3, -8, 0, 1, 2, A), (-2, 2, -1, 2, 1, 3, A)]),
    ('HULA HOOP', ring(-14, 14, -22, 6, 0, 1, 2, A) + [(-14, -12, 0, 1, -10, -6, B), (12, 14, 0, 1, -10, -6, B)]),
]
COLOURS = [('RED', (29, 4, 4)), ('ORANGE', (31, 15, 2)), ('YELLOW', (31, 27, 4)), ('LIME', (16, 29, 4)), ('GREEN', (3, 20, 6)), ('TEAL', (3, 22, 20)),
           ('SKY', (10, 22, 31)), ('BLUE', (4, 9, 28)), ('NAVY', (3, 4, 14)), ('PURPLE', (17, 5, 26)), ('PINK', (31, 13, 22)), ('WHITE', (30, 30, 29)),
           ('SILVER', (21, 22, 24)), ('BLACK', (4, 4, 5)), ('BROWN', (15, 8, 3)), ('GOLD', (30, 23, 6))]
FIXED = [None, None, (30, 23, 6), (4, 4, 5), (30, 30, 29), (21, 22, 24), (28, 4, 4), (4, 21, 6), (31, 28, 6), (15, 8, 3), None, None, (31, 15, 22), (31, 15, 2)]
PATTERNS = ['NONE', 'STRIPES', 'WIDE STRIPES', 'PINSTRIPES', 'CHECKS', 'PLAID', 'DOTS', 'ZIGZAG', 'DIAMONDS', 'STARS', 'HEARTS', 'CAMO', 'TIE-DYE', 'DENIM', 'LEOPARD']

def pattern(name):
    """a 16x16 mask: 1 where the pattern shows (the shade the player picks), 0 the plain cloth"""
    import math
    m = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            v = 0
            if name == 'STRIPES': v = (y % 4) < 2
            elif name == 'WIDE STRIPES': v = (y % 8) < 4
            elif name == 'PINSTRIPES': v = x % 4 == 0
            elif name == 'CHECKS': v = ((x // 4) + (y // 4)) % 2
            elif name == 'PLAID': v = (x % 8 in (2, 3)) or (y % 8 in (5, 6))
            elif name == 'DOTS': v = ((x % 8) - 3.5) ** 2 + ((y % 8) - 3.5) ** 2 < 4.5
            elif name == 'ZIGZAG': v = (y + abs((x % 8) - 4)) % 8 < 2
            elif name == 'DIAMONDS': v = abs((x % 8) - 3.5) + abs((y % 8) - 3.5) < 2.6
            elif name == 'STARS':
                cx, cy = x % 8 - 3.5, y % 8 - 3.5; r = math.hypot(cx, cy); a = math.atan2(cy, cx)
                v = r < 1.6 + 1.6 * max(0, math.cos(5 * a + math.pi / 2)) ** 3
            elif name == 'HEARTS':
                cx, cy = (x % 8 - 3.5) / 3.2, -(y % 8 - 3.0) / 3.2
                v = (cx * cx + cy * cy - 1) ** 3 - cx * cx * cy ** 3 < 0
            elif name == 'CAMO': v = (math.sin(x * 0.9 + 1.3) + math.sin(y * 0.7 + x * 0.3) + math.sin((x + y) * 0.45)) > 0.6
            elif name == 'TIE-DYE': v = int(math.hypot(x - 7.5, y - 7.5) * 0.9 + math.atan2(y - 7.5, x - 7.5) * 1.3) % 3 == 0
            elif name == 'DENIM': v = (x + y) % 3 == 0 or (x * 7 + y * 13) % 11 == 0
            elif name == 'LEOPARD':
                for (sx, sy) in ((3, 3), (11, 5), (6, 11), (13, 13), (1, 9)):
                    d = math.hypot((x - sx + 8) % 16 - 8, (y - sy + 8) % 16 - 8)
                    if 1.2 < d < 2.4: v = 1
            m[y][x] = 1 if v else 0
    return m

def emit():
    here = os.path.dirname(os.path.abspath(__file__)); src = os.path.join(here, '..', 'src')
    cats = [('Head', HEAD), ('Face', FACE), ('Neck', NECK), ('Back', BACK), ('Hand', HAND), ('Waist', WAIST)]
    with open(os.path.join(src, 'wear_names.inc'), 'w') as f:
        f.write('// GENERATED by tools/make_wear.py: the WEAR tab (names, colours) and the clothes patterns\n')
        for nm, cat in cats:
            f.write('#ifndef NX_%s\n#define NX_%s %d\n#endif\n' % (nm.upper(), nm.upper(), len(cat)))
            f.write('static const char *const x%sNm[NX_%s] = {%s};\n' % (nm, nm.upper(), ','.join('"%s"' % n for n, _ in cat)))
        f.write('#define NX_PAT %d\nstatic const char *const xPatNm[NX_PAT] = {%s};\n' % (len(PATTERNS), ','.join('"%s"' % p for p in PATTERNS)))
        f.write('static const u16 xCol[NX_COL] = {%s};\n' % ','.join('RGB(%d,%d,%d)' % c for _, c in COLOURS))
    with open(os.path.join(src, 'wear_models.inc'), 'w') as f:
        f.write('// GENERATED by tools/make_wear.py: every accessory as boxes {x0,x1,y0,y1,z0,z1,colour} (see the tool for the units)\n')
        f.write('typedef struct { const s8 (*b)[7]; u8 n; } WearModel;\n')
        for nm, cat in cats: f.write('#ifndef NX_%s\n#define NX_%s %d\n#endif\n' % (nm.upper(), nm.upper(), len(cat)))
        for nm, cat in cats:
            for i, (n, boxes) in enumerate(cat):
                if boxes:
                    for bx in boxes: assert bx[0] < bx[1] and bx[2] < bx[3] and bx[4] < bx[5], (nm, n, bx)
                    f.write('static const s8 w%s%d[%d][7] = {%s};\n' % (nm, i, len(boxes), ','.join('{%s}' % ','.join(str(v) for v in bx) for bx in boxes)))
            f.write('static const WearModel wear%s[%d] = {%s};\n' % (nm, len(cat), ','.join(('{w%s%d,%d}' % (nm, i, len(b))) if b else '{0,0}' for i, (_, b) in enumerate(cat))))
        f.write('static const u16 wearPal[%d] = {%s};   // the COLOUR 1 / COLOUR 2 choices\n' % (len(COLOURS), ','.join('RGB(%d,%d,%d)' % c for _, c in COLOURS)))
        f.write('static const u16 wearFixed[14] = {%s};   // the fixed colours (A, B, HAIR and SKIN are looked up per person)\n' % ','.join('RGB(%d,%d,%d)' % c if c else '0' for c in FIXED))
        f.write('#define NPAT %d\nstatic const u16 patMask[NPAT][16] = {\n' % len(PATTERNS))
        for p in PATTERNS:
            m = pattern(p) if p != 'NONE' else [[0] * 16 for _ in range(16)]
            f.write('    {%s},   // %s\n' % (','.join('0x%04x' % sum(m[y][x] << x for x in range(16)) for y in range(16)), p))
        f.write('};\n')
    print('wardrobe: ' + ', '.join('%d %s' % (len(c) - 1, n.lower()) for n, c in cats) + ', %d patterns' % (len(PATTERNS) - 1))

emit()
