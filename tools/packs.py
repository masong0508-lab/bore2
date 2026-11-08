#!/usr/bin/env python3
# packs.py: BORE 2's EXPANSION PACKS as patches.
#
# Every pack is one or more files on the disc (an overlay, its art). Each file has a SLOT: a fixed number of sectors. The BASE
# disc holds a stub (it says "pack not installed") in every slot; the FULL disc holds the real files. Both discs lay out the
# same, so the only bytes that differ are inside the slots, and a pack's patch (PPF3) is exactly the difference inside its own
# slots. Packs never share a slot, so any set of patches goes onto the base disc in any order, with no patch needing another.
#
#   packs.py slots <builddir>                  make <builddir>/slot_full/* and slot_base/* (every file padded to its slot)
#   packs.py ppf <base.bin> <full.bin> <outdir> one PPF3 per pack (the raw 2352-byte sectors, so the error codes match too)
#   packs.py apply <in.bin> <out.bin> <a.ppf> ...   put patches on a disc image
#   packs.py test <base.bin> <full.bin> <outdir>    every pack on its own, then all of them in forward, backward and shuffled orders
import os, random, struct, sys

SECTOR = 2352
STUB_MAGIC = b"BSTB"            # (src/ovl.h: OVL_STUB) the first bytes of a stub; a real overlay starts with "BOVL"

# pack id: (title, [(file name, slot in sectors)])  -- the slots leave room to grow; the game's overlay arena is 80 KB
PACKS = {
    "KINGDOM": ("THE LAND BOREFORE TIME", [("KINGDOM.OVL", 40)]),
    "ARCADE":  ("BORTUA FIGHTER AND THE ARCADE", [("ARCADE.OVL", 32), ("ARCADE2.OVL", 40)]),
    "GOTCHI":  ("BOREAGOTCHI!", [("GOTCHI.OVL", 76), ("GOTCHI.ART", 32)]),   # (the overlay slot grew from 48 to 72 sectors for the new needs, goals, skills and games)
    "PARTY":   ("PARTY PACK", [("PARTY.OVL", 8)]),
    "DJ":      ("DJ BOOTH", [("DJ.OVL", 16)]),
    "SUPER":   ("SUPERSTAR", [("SUPER.OVL", 28)]),
    "DOG":     ("CREATE A DOG", [("DOG.OVL", 8)]),
    "PRISON":  ("CRIME AND LAW", [("PRISON.OVL", 24)]),
    "SPACE":   ("SPACE AGE", [("SPACE.OVL", 16)]),
    "PARADISE": ("ISLAND PARADISE", [("PARADISE.OVL", 16)]),
    "FARM":     ("FARM LIFE", [("FARM.OVL", 16)]),
    "SKYSEA":   ("SKY AND SEA", [("SKYSEA.OVL", 28)]),
}
# SLOTS KEPT FOR PACKS NOT MADE YET (docs/EXPANSION_PACKS.md). Both the base and the full disc hold a stub in each, so a pack made later is a patch
# on a disc made today, and no other pack moves. No patch is made for them until they have a real file.  Each has save room too: sim.h, PKX_RES.
RESERVED = {
    "REALWORLD": ("REAL WORLD",           [("REALWRLD.OVL", 12)]),
    "SPEEDCITY": ("SPEED CITY",           [("SPEEDCTY.OVL", 24)]),
    "BIGSCREEN": ("BIG SCREEN",           [("BIGSCREN.OVL", 20)]),
    "MAGIC":     ("MAGIC AND MYTH",       [("MAGICMYT.OVL", 28)]),
    "WILDWEST":  ("WILD WEST",            [("WILDWEST.OVL", 28)]),
    "BOREOUT":   ("BORE-OUT MAN!",        [("BOREOUT.OVL", 12)]),
    "BOREZIES":  ("BOREZIES!",            [("BOREZIES.OVL", 20)]),
    "WWBORE":    ("WORLD WAR BORE",       [("WWBORE.OVL", 20)]),
    "BOREPILOG": ("BOREPILOGUE",          [("BOREPILG.OVL", 12)]),
}
MAX_SLOT = 40                    # sectors: the overlay arena's room is a little over 80 KB (GOTCHI loads at the start of the arena: 76 sectors)

def stub(pack, fname, sectors):
    head = STUB_MAGIC + struct.pack("<I", sectors) + (pack + ":" + fname).encode().ljust(24, b"\0")
    return head.ljust(sectors * 2048, b"\0")

def cmd_slots(bd):
    os.makedirs(bd + "/slot_full", exist_ok=True); os.makedirs(bd + "/slot_base", exist_ok=True)
    for pid, (title, files) in RESERVED.items():       # (a stub on both discs: nothing to patch yet)
        for fname, sectors in files:
            assert sectors <= MAX_SLOT
            for side in ("slot_full", "slot_base"): open(os.path.join(bd, side, fname), "wb").write(stub(pid, fname, sectors))
    for pid, (title, files) in PACKS.items():
        for fname, sectors in files:
            assert sectors <= (76 if pid == "GOTCHI" else MAX_SLOT)
            real = open(os.path.join(bd, fname), "rb").read()
            if len(real) > sectors * 2048: sys.exit("%s is %d bytes: its slot (%d sectors) is too small" % (fname, len(real), sectors))
            open(os.path.join(bd, "slot_full", fname), "wb").write(real.ljust(sectors * 2048, b"\0"))
            open(os.path.join(bd, "slot_base", fname), "wb").write(stub(pid, fname, sectors))

def iso_files(img):              # {name: (first sector, byte size)} from the ISO9660 root of a MODE2/2352 image
    def user(lba):               # the 2048 bytes of a Mode 2 Form 1 sector (sync 12 + header 4 + subheader 8)
        o = lba * SECTOR + 24; return img[o:o + 2048]
    pvd = user(16); assert pvd[1:6] == b"CD001", "not an ISO9660 disc"
    root = struct.unpack_from("<I", pvd, 156 + 2)[0]; rsize = struct.unpack_from("<I", pvd, 156 + 10)[0]
    out = {}
    for s in range((rsize + 2047) // 2048):
        d = user(root + s); i = 0
        while i < 2048 and d[i]:
            n = d[i]; ln = d[i + 32]; name = d[i + 33:i + 33 + ln].decode("ascii", "replace").split(";")[0]
            out[name] = (struct.unpack_from("<I", d, i + 2)[0], struct.unpack_from("<I", d, i + 10)[0]); i += n
    return out

def write_ppf(path, desc, changes):   # changes: [(offset, bytes)]  (PPF 3.0, binary image, no validation, no undo data)
    with open(path, "wb") as f:
        f.write(b"PPF30" + bytes([2]) + desc.encode("ascii", "replace")[:50].ljust(50, b" ") + bytes([0, 0, 0, 0]))
        for off, data in changes:
            for p in range(0, len(data), 255):
                chunk = data[p:p + 255]; f.write(struct.pack("<QB", off + p, len(chunk)) + chunk)

def diff_range(a, b, lo, hi):         # the runs inside [lo, hi) where the two images differ (runs closer than 16 bytes are joined)
    runs = []; i = lo
    while i < hi:
        if a[i] == b[i]: i += 1; continue
        j = i
        while j < hi:
            if a[j] != b[j]: j += 1; continue
            k = j
            while k < hi and k < j + 16 and a[k] == b[k]: k += 1
            if k < hi and k < j + 16 and a[k] != b[k]: j = k
            else: break
        runs.append((i, b[i:j])); i = j
    return runs

def cmd_ppf(basep, fullp, outd):
    a = open(basep, "rb").read(); b = open(fullp, "rb").read()
    if len(a) != len(b): sys.exit("the base and full discs differ in size: the slots are not fixed")
    fa, fb = iso_files(a), iso_files(b)
    if fa != fb: sys.exit("the base and full discs lay their files out differently")
    os.makedirs(outd, exist_ok=True); covered = set()
    for pid, (title, files) in PACKS.items():
        changes = []
        for fname, sectors in files:
            lba, size = fb[fname]
            if size != sectors * 2048: sys.exit("%s on the disc is %d bytes, not %d" % (fname, size, sectors * 2048))
            lo, hi = lba * SECTOR, (lba + sectors) * SECTOR; covered.add((lo, hi))
            changes += diff_range(a, b, lo, hi)
        write_ppf(os.path.join(outd, "BORE2-%s.ppf" % pid), "BORE 2 pack: " + title, changes)
        print("%-8s %-30s %7d bytes in %d runs" % (pid, title, sum(len(d) for _, d in changes), len(changes)))
    for pid, (title, files) in RESERVED.items():          # reserved slots: the same stub on both discs, nothing to patch
        for fname, sectors in files:
            lba, size = fb[fname]
            if size != sectors * 2048: sys.exit("%s (reserved) on the disc is %d bytes, not %d" % (fname, size, sectors * 2048))
            if a[lba * SECTOR:(lba + sectors) * SECTOR] != b[lba * SECTOR:(lba + sectors) * SECTOR]: sys.exit("%s (reserved) differs between the discs" % fname)
            covered.add((lba * SECTOR, (lba + sectors) * SECTOR))
        print("%-9s %-30s reserved (%d sectors), no patch yet" % (pid, title, sum(n for _, n in files)))
    # outside the slots the two builds may differ only in the file dates the disc maker stamps (a few directory sectors)
    out = [i // SECTOR for i in range(0, len(a), SECTOR) if a[i:i + SECTOR] != b[i:i + SECTOR] and not any(lo <= i < hi for lo, hi in covered)]
    if len(out) > 6: sys.exit("the discs differ outside the pack slots in %d sectors (%s ...)" % (len(out), out[:8]))
    print("(outside the slots only the disc maker's dates differ: sectors %s)" % out)

def apply_ppf(img, path):
    p = open(path, "rb").read(); assert p[:5] == b"PPF30", "not a PPF 3.0 patch"
    i = 60                                      # "PPF30", method, 50 bytes of description, image type, block check, undo data, dummy
    if p[57]: i += 1024                         # (a block-check block, if the patch has one)
    undo = p[58]
    while i < len(p):
        off, n = struct.unpack_from("<QB", p, i); i += 9
        img[off:off + n] = p[i:i + n]; i += n + (n if undo else 0)
    return img

def cmd_apply(inp, outp, *ppfs):
    img = bytearray(open(inp, "rb").read())
    for f in ppfs: apply_ppf(img, f)
    open(outp, "wb").write(img)

def cmd_test(basep, fullp, outd):
    cmd_ppf(basep, fullp, outd)
    base = open(basep, "rb").read(); full = open(fullp, "rb").read()
    ppfs = {pid: os.path.join(outd, "BORE2-%s.ppf" % pid) for pid in PACKS}
    bad = 0
    def run(order, label):
        nonlocal bad
        img = bytearray(base)
        for pid in order: apply_ppf(img, ppfs[pid])
        return bytes(img)
    ids = list(PACKS)
    got = run(ids, "forward")
    cov = []
    for pid, (title, files) in PACKS.items():
        for fname, sectors in files: lba = iso_files(full)[fname][0]; cov.append((lba * SECTOR, (lba + sectors) * SECTOR))
    if any(got[lo:hi] != full[lo:hi] for lo, hi in cov): print("FAIL: all packs forward do not fill every slot as the full disc has it"); bad += 1
    if run(ids[::-1], "backward") != got: print("FAIL: backward"); bad += 1
    rnd = random.Random(2)
    for t in range(12):
        o = ids[:]; rnd.shuffle(o)
        if run(o, "shuffle") != got: print("FAIL: order", o); bad += 1
    for pid in ids:                          # each pack on its own: the other slots stay stubs
        img = run([pid], pid); fa = iso_files(img)
        if fa != iso_files(base): print("FAIL: %s changes the file layout" % pid); bad += 1
    open(os.path.join(outd, "BORE2-full.bin"), "wb").write(got)   # (the base with every pack on: the full game's disc)
    print("order test:", "ALL GOOD (every order gives the same full disc)" if not bad else "%d FAILED" % bad)
    sys.exit(1 if bad else 0)

if __name__ == "__main__":
    c = sys.argv[1] if len(sys.argv) > 1 else ""
    f = {"slots": cmd_slots, "ppf": cmd_ppf, "apply": cmd_apply, "test": cmd_test}.get(c)
    if not f: sys.exit(__doc__)
    f(*sys.argv[2:])
