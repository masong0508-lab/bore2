// the calm palette: homes and shops draw floors and walls from short lists (no factory stripes, hazard plate or grating indoors)
static int cfl(unsigned h) { static const u8 t[9] = { 4, 1, 0, 2, 3, 5, 6, 11, 13 }; return t[h % 9u]; }
static int cwp(unsigned h) { static const u8 t[12] = { 10, 18, 20, 21, 22, 41, 42, 43, 48, 58, 87, 41 }; return NWP + t[h % 12u]; }
static int pick_wp(unsigned h) { unsigned d = city_h((blk_bx >> 2) * 5 + (blk_by >> 2), 77); return NWP + NWX + (int)(((h >> 16) % 4u) ? (d % NWPNEW) : ((d >> 8) + (h >> 20)) % NWPNEW); }   // (the outer wall follows the district: brick, siding, stucco, shingle, beadboard or stone, 1 in 4 a neighbour's mix)

// ---- furnishing a room for its people ----
// the storey being built on: the ground (the map) or one of the storeys above (the generators write through these)
static char (*sm)[MW + 1] = lifeMap; static u8 (*sfl)[MW] = floorMap, (*swp)[MW] = wallMap;
static int storey(int l) {
    if (l <= 0) { sm = lifeMap; sfl = floorMap; swp = wallMap; return 1; }
    Level *L = lv_need(l - 1); if (!L) { storey(0); return 0; }
    sm = L->map; sfl = L->fl; swp = L->wp; return 1;
}
static void sRoom(int x0, int y0, int x1, int y1, int fl, int wp) {   // a room on the storey: floor, walls round it, open floor inside
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { sfl[y][x] = (u8)fl;
        if (x == x0 || x == x1 || y == y0 || y == y1) { sm[y][x] = 'W'; swp[y][x] = (u8)wp; } else sm[y][x] = '.'; }
}
static void sLine(int x0, int y0, int x1, int y1, char c, int wp) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { sm[y][x] = c; swp[y][x] = (u8)wp; } }
__attribute__((noinline)) static void sFree(int x, int y, char c) { if (x >= 0 && y >= 0 && x < MW && y < MH && sm[y][x] == '.') sm[y][x] = c; }
static unsigned fz_h;
static int fz_rnd(int n) { fz_h = fz_h * 1103515245u + 12345u; return (int)((fz_h >> 16) % (unsigned)n); }
static int fz_x0, fz_y0, fz_x1, fz_y1, fz_dx, fz_dy, fz_ix, fz_iy, fz_ox, fz_oy;
static u8 fz_res[24][2]; static int fz_nres;           // tiles to keep clear (stairs)
static int fz_keep(int x, int y) {                    // the lane in from the door (the tile inside it, the next one in, the ones beside it), and anything reserved
    if (x == fz_ix && y == fz_iy) return 1;
    if (x == fz_ix + fz_ox && y == fz_iy + fz_oy) return 1;
    if (fz_ox == 0 && y == fz_iy && ABS(x - fz_ix) == 1) return 1;
    if (fz_oy == 0 && x == fz_ix && ABS(y - fz_iy) == 1) return 1;
    for (int i = 0; i < fz_nres; i++) if (fz_res[i][0] == x && fz_res[i][1] == y) return 1;
    return 0;
}
static int fz_free(int x, int y) { return x >= fz_x0 && x <= fz_x1 && y >= fz_y0 && y <= fz_y1 && sm[y][x] == '.' && !fz_keep(x, y); }
static u8 rgx[64], rgy[64]; static int rgn;
static void fz_ring(void) {                           // the tiles along the walls, clockwise: a run of them is always side by side
    rgn = 0;
    for (int x = fz_x0; x <= fz_x1; x++) { rgx[rgn] = (u8)x; rgy[rgn++] = (u8)fz_y0; }
    for (int y = fz_y0 + 1; y <= fz_y1; y++) { rgx[rgn] = (u8)fz_x1; rgy[rgn++] = (u8)y; }
    if (fz_y1 > fz_y0) for (int x = fz_x1 - 1; x >= fz_x0; x--) { rgx[rgn] = (u8)x; rgy[rgn++] = (u8)fz_y1; }
    if (fz_x1 > fz_x0) for (int y = fz_y1 - 1; y > fz_y0; y--) { rgx[rgn] = (u8)fz_x0; rgy[rgn++] = (u8)y; }
}
static int fz_run(const char *items, int start) {     // the items side by side along a wall, from about there on
    int k = (int)strlen(items); if (!rgn || k > rgn) return -1;
    for (int s = 0; s < rgn; s++) {
        int i0 = (start + s) % rgn, ok = 1;
        for (int j = 0; j < k && ok; j++) { int i = (i0 + j) % rgn; if (!fz_free(rgx[i], rgy[i])) ok = 0; }
        if (!ok) continue;
        for (int j = 0; j < k; j++) { int i = (i0 + j) % rgn; sm[rgy[i]][rgx[i]] = items[j]; }
        return i0;
    }
    return -1;
}
static int fz_walk(char c) { return c == '.' || c == 'r' || c == 'D'; }
static void fz_reach(void) {                          // everything must be reachable: what is walled in by other things goes again
    static u8 seen[16][16]; static u8 qx[256], qy[256];
    for (int pass = 0; pass < 3; pass++) {
        memset(seen, 0, sizeof seen);
        int h = 0, t = 0, W_ = fz_x1 - fz_x0 + 1, H_ = fz_y1 - fz_y0 + 1;
        if (W_ > 16 || H_ > 16) return;
        int sx = fz_ix, sy = fz_iy; if (sx < fz_x0 || sx > fz_x1 || sy < fz_y0 || sy > fz_y1) return;
        seen[sy - fz_y0][sx - fz_x0] = 1; qx[t] = (u8)sx; qy[t++] = (u8)sy;
        while (h < t) { int x = qx[h], y = qy[h++];
            static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k];
                if (a < fz_x0 || a > fz_x1 || b < fz_y0 || b > fz_y1 || seen[b - fz_y0][a - fz_x0] || !fz_walk(sm[b][a])) continue;
                seen[b - fz_y0][a - fz_x0] = 1; qx[t] = (u8)a; qy[t++] = (u8)b; } }
        int gone = 0;
        for (int y = fz_y0; y <= fz_y1; y++) for (int x = fz_x0; x <= fz_x1; x++) {
            char c = sm[y][x]; if (fz_walk(c) || c == '|' || c == 'W' || c == 'E' || c == ' ') continue;
            int ok = 0;
            static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k];
                if (a >= fz_x0 && a <= fz_x1 && b >= fz_y0 && b <= fz_y1 && seen[b - fz_y0][a - fz_x0]) ok = 1; }
            if (!ok) { sm[y][x] = '.'; gone++; }
        }
        if (!gone) return;
    }
}
static int fz_Wl, fz_Dd, fz_ld, fz_px, fz_py;                    // the room as seen from its door: l across (fz_ld at the door), d deep
static int fz_lx(int l, int d) { return fz_ix + fz_ox * d + fz_px * (l - fz_ld); }
static int fz_ly(int l, int d) { return fz_iy + fz_oy * d + fz_py * (l - fz_ld); }
static int fz_pfree(int l, int d) { return l >= 0 && l < fz_Wl && d >= 0 && d < fz_Dd && fz_free(fz_lx(l, d), fz_ly(l, d)); }
static void fz_pput(int l, int d, char c) { if (fz_pfree(l, d)) sm[fz_ly(l, d)][fz_lx(l, d)] = c; }
static u8 fz_pl[4][24], fz_pd[4][24]; static int fz_np[4], fz_pc[4];   // four paths of tiles along the walls (beds, kitchen, living, telly) and where each has got to
static void fz_padd(int p, int l, int d) { if (fz_np[p] < 24 && l >= 0 && l < fz_Wl && d >= 0 && d < fz_Dd) { fz_pl[p][fz_np[p]] = (u8)l; fz_pd[p][fz_np[p]++] = (u8)d; } }
static int fz_pp(int p, char c) {                    // the next free tile on the path takes the item (-1: the wall is full)
    if (!fz_pc[p] && !fz_np[p]) return -1;
    for (; fz_pc[p] < fz_np[p]; fz_pc[p]++) { int l = fz_pl[p][fz_pc[p]], d = fz_pd[p][fz_pc[p]]; if (fz_pfree(l, d)) { fz_pput(l, d, c); fz_pc[p]++; return 0; } }
    return -1;
}
enum { R_BATH = 1, R_BED = 2, R_KIT = 4, R_LIV = 8, R_STUDY = 16, R_GRAND = 32, R_PLAY = 64, R_ALL = R_BATH | R_BED | R_KIT | R_LIV | R_STUDY };
typedef struct { u8 abeds, bunks, cribs, tbeds, toys, extra; } Beds;   // what the sleepers in a room need: beds, bunks, cribs, teens' beds, the kids' toys, a guest bed
static Beds beds_of(const Occ *o) {
    Beds b; memset(&b, 0, sizeof b);
    int grown = o->adults + o->elders;
    b.abeds = (u8)(o->couple ? 1 + MAX(0, grown - 2) : grown); if (!b.abeds && !o->kids && !o->teens && !o->babies) b.abeds = 1;
    b.bunks = (u8)((o->kids + 1) / 2); b.cribs = o->babies; b.tbeds = o->teens; b.toys = (u8)(o->kids ? 1 : 0);
    return b;
}
static void fz_frame(int x0, int y0, int x1, int y1, int ix, int iy, int ox, int oy, unsigned h) {   // a room to furnish, seen from its door
    fz_x0 = x0; fz_y0 = y0; fz_x1 = x1; fz_y1 = y1; fz_ix = ix; fz_iy = iy; fz_ox = ox; fz_oy = oy; fz_dx = ix; fz_dy = iy; fz_h = h | 1;
    fz_ring(); fz_pc[0] = fz_pc[1] = fz_pc[2] = fz_pc[3] = 0; fz_np[0] = fz_np[1] = fz_np[2] = fz_np[3] = 0;
    fz_px = -oy; fz_py = ox;
    int c0 = fz_px * x0 + fz_py * y0, c1 = fz_px * x1 + fz_py * y1, cmin = MIN(c0, c1), cmax = MAX(c0, c1);
    int d0 = ox * x0 + oy * y0, d1 = ox * x1 + oy * y1, di = ox * ix + oy * iy;
    fz_Wl = cmax - cmin + 1; fz_ld = fz_px * ix + fz_py * iy - cmin; fz_Dd = MAX(d0, d1) - di + 1;
}
// ---- A LAYOUT, not a scatter: a room read from its door in (l across, d deep) and given the zones its roles ask for ----
//   the back wall: the bedroom at one end, the bathroom (toilet, shower, sink) in the other corner; the kitchen runs down the bathroom's
//   side wall (the plumbing is all on one side), or along the back wall in an open front room; the bedroom's wall carries on with the
//   sofa; the telly stands across from it, a rug between; the dining table in the middle by the kitchen; a study corner and the odd hobby.
static void fz_layout(int roles, const Beds *bd, const Occ *o) {
    const int RW = fz_Wl, RD = fz_Dd, back = RD - 1, mid = RD / 2, area = RW * RD;
    int bathEnd = fz_rnd(2) ? RW - 1 : 0, bedEnd = RW - 1 - bathEnd, sg = bedEnd == 0 ? 1 : -1, bathw = 0;
    if (roles & R_BATH) {
        char bath[4] = { '-', 'T', fz_rnd(4) ? 'H' : ',', 0 }; if (area < 24 && RW < 5) bath[2] = 0; if (RW < 2) bath[1] = 0;
        int k = (int)strlen(bath), ok = 0;
        for (int tries = 0; tries < 2 && !ok; tries++) {   // (the other corner if this one is in the way)
            int be = tries ? bedEnd : bathEnd, dir = be == 0 ? 1 : -1; ok = 1;
            for (int j = 0; j < k && ok; j++) if (!fz_pfree(be + dir * j, back)) ok = 0;
            if (ok) { for (int j = 0; j < k; j++) fz_pput(be + dir * j, back, bath[j]); bathw = k; if (tries) { bathEnd = be; bedEnd = RW - 1 - be; sg = -dir; } }
        }
        if (roles & R_BED) { int sh = fz_rnd(2) ? 1 : 0; (void)sh; }
    }
    const int fronty = !(roles & (R_BED | R_BATH));
    int var = fz_rnd(3); if (fronty) var = (RD >= 5 && fz_rnd(3)) ? 0 : 1;               // an open front room: the kitchen takes the back wall (or a side wall when deep)
    for (int j = 0; j < RW - bathw; j++) fz_padd(0, bedEnd + sg * j, back);              // the beds: along the back wall from their end ...
    for (int d = back - 1; d >= (RD + 1) / 2 && d >= 2; d--) fz_padd(0, bedEnd, d);     // ... then down the side wall to the middle
    if (var == 1) { for (int j = 0; j < RW - 1; j++) fz_padd(1, bathEnd - sg * j, back); }   // the kitchen along the back wall from the bathroom's end
    else for (int d = fronty ? back : back - 1; d >= (fronty ? mid : (RD + 1) / 2) && d >= 2; d--) fz_padd(1, bathEnd, d);    // or down the bathroom's side wall
    if (!fronty) {
        for (int d = mid - 1; d >= 1; d--) fz_padd(2, bedEnd, d);                        // the living end of the bedroom's wall
        for (int d = mid - 1; d >= 1; d--) fz_padd(3, bathEnd, d);                       // and the opposite wall: the telly
    } else {                                                                             // an open front room: the whole of the bedroom side's wall, the telly below the kitchen (or on the whole opposite wall)
        for (int d = MIN(mid, back); d <= back; d++) fz_padd(2, bedEnd, d);
        for (int d = MIN(mid, back) - 1; d >= 1; d--) fz_padd(2, bedEnd, d);
        if (var == 1) { for (int d = 1; d <= back - 1; d++) fz_padd(3, bathEnd, d); } else for (int d = mid - 1; d >= 1; d--) fz_padd(3, bathEnd, d);
    }
    if (roles & R_BED) {
        for (int b = 0; b < bd->abeds; b++) { fz_pp(0, 'S'); if (!b) fz_pp(0, 'i'); }
        for (int k = 0; k < bd->bunks; k++) fz_pp(0, '<');
        for (int k = 0; k < bd->cribs; k++) fz_pp(0, '\x81');
        for (int k = 0; k < bd->tbeds; k++) fz_pp(0, fz_rnd(2) ? 'S' : '<');
        if (bd->extra) { fz_pp(0, 'S'); fz_pp(0, 'i'); }
        if (bd->toys) { fz_pp(0, 'n'); if (!fz_rnd(2)) fz_pp(0, '{'); }
        if (area >= 12 && bd->abeds && fz_rnd(3) == 0) fz_pp(0, 'b');
    }
    if (roles & R_KIT) {
        char kit[6] = { 'F', 'y', '/', fz_rnd(2) ? ':' : 'c', 'y', 0 }; if (area < 30) kit[3] = 0; if (area < 18) kit[2] = 0; if (area < 12) kit[1] = 0;
        if (var == 1 && RW >= 7) { kit[4] = 'c'; kit[5] = 0; }                           // (a long run along the back wall)
        for (int j = 0; kit[j]; j++) fz_pp(1, kit[j]);
        if (var == 1) fz_pp(1, '-');
    }
    if (roles & R_LIV) {
        int lvr = o->elders && !o->adults ? ';' : 'C';
        if (roles & R_GRAND) { if (var == 0 && fz_pfree(RW / 2, back)) fz_pput(RW / 2, back, 'o'); else fz_pp(2, 'o'); fz_pp(3, '$'); if (RD >= 5) fz_pp(2, ';'); }
        int sofa = area >= 12 ? fz_pp(2, (char)lvr) : -1;
        if (sofa >= 0) { fz_pp(3, 'v'); fz_pp(2, 'l'); if (fz_rnd(2)) fz_pp(3, 'A'); }
        if (o->elders) { fz_pp(3, 'R'); if (fz_rnd(2)) fz_pp(2, ';'); }
        if (o->teens) { fz_pp(3, fz_rnd(2) ? 'U' : 'A'); fz_pp(2, 'U'); }
        if (RW >= 6 && RD >= 4) {                                                        // the dining table in the middle, chairs on both sides; a rug before the sofa
            int tl = bathEnd == 0 ? 2 : RW - 3, td = RD >= 5 ? RD - 3 : 2;
            if (fz_pfree(tl, td) && fz_pfree(tl - 1, td) && fz_pfree(tl + 1, td)) { fz_pput(tl, td, 't'); fz_pput(tl - 1, td, 'h'); fz_pput(tl + 1, td, 'h'); if ((roles & R_GRAND) && fz_pfree(tl, td + 1) && td + 1 < RD - 1) fz_pput(tl, td + 1, 'h'); }
            int rl = bedEnd == 0 ? 2 : RW - 3, rd = MAX(1, mid - 1);
            if (fz_pfree(rl, rd)) fz_pput(rl, rd, 'r');
        }
    }
    if (roles & R_PLAY) {                                                                // a games room: the jukebox, the tellies, the beanbags; or a home gym
        if (fz_rnd(2)) { fz_pp(0, 'j'); fz_pp(0, 'q'); fz_pp(1, 'v'); fz_pp(1, 'A'); fz_pp(2, 'U'); fz_pp(2, 'U'); fz_pp(3, '!'); }
        else { fz_pp(0, 'm'); fz_pp(0, 'm'); fz_pp(1, ']'); fz_pp(1, ']'); fz_pp(2, '`'); fz_pp(3, 'Q'); }
    }
    if (roles & R_STUDY) {                                                               // the desk, a chair at it, the computer and the shelves
        for (int j = 0; j < RW; j++) if (fz_pfree(j, back) && fz_pfree(j + sg, back)) { fz_pput(j, back, 'd'); fz_pput(j + sg, back, '['); if (RD >= 4 && fz_pfree(j, back - 1)) fz_pput(j, back - 1, 'h'); break; }
        for (int j = RW - 1; j >= 0; j--) if (fz_pfree(j, back)) { fz_pput(j, back, 'b'); break; }
        fz_pp(2, 'b'); fz_pp(3, 'b');
    }
    if ((roles & R_LIV) && (o->adults || o->teens)) { for (int j = 0; j < RW; j++) if (fz_pfree(j, back) && (fz_pfree(j + 1, back) || fz_pfree(j - 1, back)) && !(roles & R_KIT)) { fz_pput(j, back, 'd'); fz_pput(j + 1, back, '['); break; } }
    if ((roles & R_LIV) && o->adults) {                                                  // the hobbies, along the front wall by the door
        static const char hobby[] = "b(`$)qoV?";
        int n = 1 + (area >= 35) + (area >= 48) + ((roles & R_GRAND) ? 1 : 0);
        for (int i = 0; i < n; i++) { char c = hobby[fz_rnd(9)];
            for (int j = 0; j < RW; j++) { int l = (j * 3 + i * 2 + 1) % RW; if (fz_pfree(l, 0) && fz_pfree(l, 1) && l != fz_ld) { fz_pput(l, 0, c); break; } } }
    }
    fz_pput(0, 0, 'p'); fz_pput(RW - 1, 0, 'p'); if (area >= 30) fz_pput(bedEnd, 0, 'p');   // the touches: plants in the front corners
    if (roles & (R_BED | R_STUDY)) { fz_pput(bedEnd, 0, 'p'); if (area >= 9) fz_pput(RW / 2, back - 1 > 1 ? back - 1 : back, 'r'); }
}
static void fz_need(const char *has, char c) {   // a home must have its toilet, fridge and bed: if the layout left one out, it goes on the first free wall tile that is not in the doorway
    for (int y = fz_y0; y <= fz_y1; y++) for (int x = fz_x0; x <= fz_x1; x++) if (sm[y][x] && strchr(has, sm[y][x])) return;
    for (int y = fz_y0; y <= fz_y1; y++) for (int x = fz_x0; x <= fz_x1; x++) {
        if (sm[y][x] != '.' || (ABS(x - fz_ix) + ABS(y - fz_iy)) < 2 || !fz_free(x, y)) continue;
        if (sm[y - 1][x] == 'W' || sm[y + 1][x] == 'W' || sm[y][x - 1] == 'W' || sm[y][x + 1] == 'W') { sm[y][x] = c; return; } }
}
static int fz_no_bath;                            // 1: the bathroom is a room of its own, not this layout's corner
static void furnish_at(int x0, int y0, int x1, int y1, int ix, int iy, int ox, int oy, const Occ *o, unsigned h) {   // (ix,iy): the tile just inside the door; (ox,oy): the way in
    fz_frame(x0, y0, x1, y1, ix, iy, ox, oy, h);
    int st = fz_rnd(MAX(1, rgn));
    if (o->vacant) {                                  // nobody lives here: a couple of boxes the last people left behind
        if (fz_rnd(2)) fz_run("#", st);
        if (!fz_rnd(3)) fz_run("p", st + rgn / 2);
        fz_nres = 0; return;
    }
    Beds bd = beds_of(o);
    fz_layout((fz_no_bath ? R_ALL & ~R_BATH : R_ALL) | ((x1 - x0 + 1) * (y1 - y0 + 1) >= 60 ? R_GRAND : 0), &bd, o);
    fz_reach();
    if (!fz_no_bath) fz_need("T", 'T');
    fz_need("F", 'F'); fz_need("S<", 'S');
    fz_nres = 0;
}
// A BIG HOME is built as rooms: a front room (living, kitchen, dining; split in two in a mansion), and behind a wall with a door to each
// of the back rooms (the bedrooms, the bathroom, a study, a games room). The plan comes from the dice: how wide each room is, which door
// goes where, what each room is for, and what is in it.
static int fz_upx = -1, fz_upy, fz_holex = -1, fz_holey, fz_upper;   // a staircase up from this storey (a plant by it), a stairwell hole in its floor from the one below, and whether it is an upper floor
static unsigned fz_geo;                                                               // the dice of a house's room plan: the same on every storey, so the doors and the stairs keep their places
static int fp_geo(int X0, int Y0, int X1, int Y1, int *kk, int *wyy, int *cuts, int *rx0, int *rx1, int *ddx) {   // the back rooms' spans, their doors and the wall in front of them (0: too small to divide)
    int Wt = X1 - X0 + 1, Dt = Y1 - Y0 + 1; if (Wt < 7 || Dt < 6) return 0;
    unsigned g = fz_geo | 1; int br = Dt >= 11 ? 5 : Dt >= 9 ? 4 : Dt >= 7 ? 3 : 2, k = Wt >= 11 ? 3 : 2;
    int rw = (Wt - (k - 1)) / k, extra = (Wt - (k - 1)) - rw * k, x = X0;
    for (int i = 0; i < k; i++) { int w = rw + (i < extra ? 1 : 0); g = g * 1103515245u + 12345u; if (((g >> 16) & 1) && i < k - 1 && w > 3) w--; rx0[i] = x; rx1[i] = MIN(x + w - 1, X1); x = rx1[i] + 2; if (i < k - 1) cuts[i] = rx1[i] + 1; }
    if (rx1[k - 1] < X1) rx1[k - 1] = X1;
    for (int i = 0; i < k; i++) ddx[i] = (rx0[i] + rx1[i]) / 2;
    if (Dt < 7) { ddx[0] = rx1[0]; ddx[k - 1] = rx0[k - 1]; }    // (rooms two deep: the doors flank the dividing wall, so the back wall of each room is free for its things)
    *kk = k; *wyy = Y0 + br; return 1;
}
static void furnish_plan(int X0, int Y0, int X1, int Y1, int dx, const Occ *o, unsigned h, int grand) {
    int k, wy, cuts[3], rx0[3], rx1[3], ddx[3];
    if (!fp_geo(X0, Y0, X1, Y1, &k, &wy, cuts, rx0, rx1, ddx)) { furnish_at(X0, Y0, X1, Y1, dx, Y1, 0, -1, o, h); return; }
    fz_h = h | 1;
    int wpi = cwp(h >> 5);
    for (int x = X0; x <= X1; x++) { sm[wy][x] = 'W'; swp[wy][x] = (u8)wpi; }
    for (int i = 0; i < k - 1; i++) for (int y = Y0; y < wy; y++) { sm[y][cuts[i]] = 'W'; swp[y][cuts[i]] = (u8)wpi; }
    for (int i = 0; i < k; i++) sm[wy][ddx[i]] = 'D';   // a door to each room
    for (int i = 0; i < k; i++) { int fl = cfl(fz_rnd(9)); for (int y = Y0; y < wy; y++) for (int x = rx0[i]; x <= rx1[i]; x++) sfl[y][x] = (u8)fl; }
    if (o->vacant) { furnish_at(X0, wy + 1, X1, Y1, dx, Y1, 0, -1, o, h); return; }       // (an empty home keeps its rooms and a box or two)
    // who sleeps where
    Beds all = beds_of(o); int roles[3] = { 0, 0, 0 }; Beds rb[3]; memset(rb, 0, sizeof rb);
    int bathAt = fz_rnd(k); if (wy - Y0 < 3 && k == 3 && bathAt == 1) bathAt = fz_rnd(2) * 2;   // which room is the bathroom (not the middle one of three when the rooms are two deep)
    int order[3], n = 0; for (int i = 0; i < k; i++) if (i != bathAt) order[n++] = i;
    roles[bathAt] = R_BATH | (grand && k == 3 && fz_rnd(2) ? 0 : 0);
    rb[order[0]].abeds = all.abeds ? (all.abeds > 2 ? 2 : all.abeds) : 1; rb[order[0]].cribs = all.cribs; roles[order[0]] = R_BED;
    int left = all.abeds > 2 ? all.abeds - 2 : 0;
    if (n > 1) {
        int kidsStuff = all.bunks + all.tbeds + all.toys + left;
        if (kidsStuff) { rb[order[1]].bunks = all.bunks; rb[order[1]].tbeds = all.tbeds; rb[order[1]].toys = all.toys; rb[order[1]].abeds = (u8)left; roles[order[1]] = R_BED; }
        else roles[order[1]] = fz_rnd(2) ? (R_STUDY | R_BED) : (grand ? R_PLAY : R_STUDY);   // a study with a guest bed, a games room, or just a study
        if (!rb[order[1]].abeds && (roles[order[1]] & R_BED) && !kidsStuff) rb[order[1]].extra = 1;
    } else { rb[order[0]].bunks = all.bunks; rb[order[0]].tbeds = all.tbeds; rb[order[0]].toys = all.toys; rb[order[0]].abeds = (u8)(rb[order[0]].abeds + left); }
    // the front: one room, or in a mansion a living room and a kitchen with dining beside it
    int fx0 = X0, fx1 = X1, fy0 = wy + 1;
    // the walls are up: now the things in the rooms
    fz_nres = 0;
    for (int i = 0; i < k; i++) { fz_nres = 0; fz_frame(rx0[i], Y0, rx1[i], wy - 1, ddx[i], wy - 1, 0, -1, fz_h + (unsigned)i * 7919u); fz_layout(roles[i], &rb[i], o); }
    fz_nres = 0;
    for (int i = 0; i < k; i++) { static const signed char rs[4][2] = { { 0, 1 }, { -1, 1 }, { 1, 1 }, { 0, 2 } };   // (the way from each door stays open, with room to step round)
        for (int j = 0; j < 4; j++) { fz_res[fz_nres][0] = (u8)(ddx[i] + rs[j][0]); fz_res[fz_nres++][1] = (u8)(wy + rs[j][1]); } }
    Beds none; memset(&none, 0, sizeof none);
    { static const signed char rs2[5][2] = { { 0, 0 }, { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };   // the stairs keep a clear square round them
      for (int j = 0; j < 5; j++) { if (fz_upx >= 0) { fz_res[fz_nres][0] = (u8)(fz_upx + rs2[j][0]); fz_res[fz_nres++][1] = (u8)(fz_upy + rs2[j][1]); }
                                    if (fz_holex >= 0) { fz_res[fz_nres][0] = (u8)(fz_holex + rs2[j][0]); fz_res[fz_nres++][1] = (u8)(fz_holey + rs2[j][1]); } } }
    fz_frame(fx0, fy0, fx1, Y1, dx, Y1, 0, -1, fz_h * 3u); fz_layout(fz_upper ? (R_LIV | R_PLAY | R_GRAND) : (R_KIT | R_LIV | (grand ? R_GRAND : 0)), &none, o);
    if (fz_upx >= 0) { sm[fz_upy][fz_upx] = '|'; sm[fz_upy - 1][fz_upx] = 'p'; }
    if (fz_holex >= 0) sm[fz_holey][fz_holex] = ' ';
    fz_x0 = X0; fz_y0 = Y0; fz_x1 = X1; fz_y1 = Y1; fz_ix = dx; fz_iy = Y1; fz_ox = 0; fz_oy = -1;
    fz_reach();
    if (!fz_upper) {                                                                  // a home has its fridge in the front room, and a toilet and a bed in the back
        fz_y0 = wy + 1; fz_need("F", 'F'); fz_y0 = Y0; fz_y1 = wy - 1; fz_need("T", 'T'); fz_need("S<", 'S'); fz_y1 = Y1; }
    fz_nres = 0;
}
static void furnish(int x0, int y0, int x1, int y1, int dx, int dy, const Occ *o, unsigned h) { furnish_at(x0, y0, x1, y1, dx, y1, 0, -1, o, h); (void)dy; }   // the door in the south wall
static int PLx, PLy;                                                         // the lot being dressed: nothing outside its 13 x 13 (and nothing on the kerb row or column) is touched
static void pf(int x, int y, char c) { if (x > PLx && x <= PLx + 12 && y > PLy && y <= PLy + 12) gFree(x, y, c); }
static void yard(int lx, int ly, int dx, int dy, int vacant, unsigned h) {   // the front yard: a fenced garden with a gate on the path, flanking planters and beds, a lamp and a mailbox; a sale sign if for sale
    PLx = lx; PLy = ly; int n = ly + 12 - dy;                                // rows between the door and the road
    pf(dx - 2, dy + 1, 'Z'); pf(dx + 2, dy + 1, 'Z');
    pf(lx + 1 + (int)((h >> 3) % 2u), ly + 1, TREE); pf(lx + 11 - (int)((h >> 7) % 2u), ly + 1, TREE);
    if (vacant) { pf(dx + 2, dy + 2, '8'); return; }
    unsigned g = h >> 13;
    pf(dx + 1, dy + 1, 'l');                                                  // a lamp by the path
    for (int k = 4; k <= 6; k += 2) { char c = (g & 1) ? (char)0xE2 : 'p'; pf(dx - k, dy + 1, c); pf(dx + k, dy + 1, c); }   // beds either side, matched
    if (n >= 3) {                                                             // a fence across the front with a gate at the path: a hedge or pickets for the whole lot
        int fy = ly + 11; char fc = ((g >> 1) & 1) ? (char)0xE1 : (char)0xDF;
        for (int x = lx + 1; x <= lx + 11; x++) if (ABS(x - dx) > 1) pf(x, fy, fc);
        pf(dx + 2, ly + 12, (char)0xE0);                                      // the mailbox by the gate
        if ((g >> 2) & 1) pf(dx - 2, ly + 12, 'N');                           // and a bin
    }
    if (n >= 2 && ((g >> 3) & 3u) == 0) pf(dx - 3, dy + 2, (char)0xE3);       // now and then a gnome
    for (int y = dy + 1; y <= MIN(ly + 12, MH - 2); y++) floorMap[y][dx] = (u8)(((g >> 5) & 3u) == 0 ? FL_GRAVEL : ((g >> 5) & 3u) == 1 ? FL_PATIO : FL_PAVING);   // the path
}

static void backyard(int lx, int ly, int hx, int hy, int w, int dy, unsigned h) {   // behind the home (the lot's north): a pool, a garden, a patio with a barbecue or a lawn, fenced on the back and the sides that leave a way round
    PLx = lx; PLy = ly; int rows = hy - ly - 1; unsigned g = h >> 11; int kind = (int)(g % 5u);
    int by = ly + 1, x0 = lx + 2 + (int)((g >> 4) % 2u);
    if (rows >= 3) {
        if (kind == 0 && rows >= 5) {                       // a swimming pool: water in a ring of pool tile, loungers and plants beside it
            int pw = 5, ph = rows >= 6 ? 4 : 3; x0 = lx + 3;
            for (int y = by; y <= by + ph + 1 && y < hy - 1; y++) for (int x = x0 - 1; x <= x0 + pw; x++) { if (x < lx + 1 || x > lx + 11) continue; floorMap[y][x] = (u8)((y == by || y == by + ph + 1 || x == x0 - 1 || x == x0 + pw) ? FL_PATIO : FL_WATER); }
            pf(x0 + pw + 1, by + 1, 'Z'); pf(lx + 1, by, TREE); pf(lx + 11, by, TREE);
        } else if (kind == 1) {                             // a flower and vegetable garden in rows
            for (int y = by + 1; y < hy - 1; y += 2) for (int x = lx + 2; x <= lx + 10; x += 2) pf(x, y, (y & 2) ? 'p' : (char)0xE2);
            pf(lx + 1, by, TREE); pf(lx + 11, by, TREE);
        } else if (kind == 2) {                             // a patio with a barbecue and a table
            for (int y = by + 1; y <= by + 2 && y < hy - 1; y++) for (int x = x0; x <= x0 + 4; x++) floorMap[y][x] = FL_PATIO;
            pf(x0 + 1, by + 1, (char)0xEA); pf(x0 + 3, by + 2, 't'); pf(lx + 11, by, TREE); pf(lx + 1, by, TREE);
        } else {                                            // a lawn, trees in a pair and a bird bath
            pf(lx + 2, by, TREE); pf(lx + 10, by, TREE); pf(lx + 6, by + 1, (char)0xE4);
        }
    }
    int sl = hx - lx >= 3, sr = lx + 12 - (hx + w - 1) >= 3;   // a fence on the back, and down a side only where a way round the house stays open beside it
    if (rows >= 2 && ((g >> 12) % 5u < 3) && (sl || sr)) {
        char fc = ((g >> 15) & 1) ? (char)0xE1 : (char)0xDF;
        for (int x = lx + 1; x <= lx + 11; x++) pf(x, ly + 1, fc);
        if (sl) for (int y = ly + 2; y < hy; y++) pf(lx + 1, y, fc);
        if (sr) for (int y = ly + 2; y < hy; y++) pf(lx + 11, y, fc);
    }
    (void)dy;
}
// ---- the homes ----
static int reg_home(int x0, int y0, int x1, int y1, int dx, int dy);
static void city_house(int lx, int ly, unsigned h) {
    int grand = ((h >> 24) & 3) == 0;                                       // one in four is a MANSION: the whole lot, rooms and an arch, a fountain out front
    int sz = (int)((h >> 26) & 15);                                          // how big: mansions run from 11x10 up to a rare 13x12 ESTATE (1 in 16); big houses 9..12 wide, 7..10 deep
    int w = grand ? (sz == 0 ? 13 : sz < 5 ? 11 : sz < 9 ? 12 : 13) : 9 + (int)((h >> 3) % 4);
    int hh = grand ? (sz == 0 ? 12 : sz < 5 ? 10 : 11) : 8 + (int)((h >> 5) % 3);          // (every house is deep enough for a back row of rooms and a front room)
    int hx = lx + (13 - w) / 2, hy = grand ? ly : ly + 1 + (int)((h >> 7) % 2);
    if (!grand && hy + hh - 1 > ly + 10) hy = ly + 11 - hh;
    int fl = cfl(h >> 9), wp = pick_wp(h);
    gRoom(hx, hy, hx + w - 1, hy + hh - 1, fl, wp);
    int dx = grand ? hx + w / 2 + (int)((h >> 20) % 3u) - 1 : hx + 2 + (int)((h >> 20) % (unsigned)(w - 4)), dy = hy + hh - 1;
    gPut(dx, dy, 'D');
    city_path(dx, dy, ly);
    if (grand) { for (int x = hx + 2; x < hx + w - 2; x += 3) { gPut(x, hy, 'E'); if (x != dx && ABS(x - dx) > 1) gPut(x, dy, 'E'); } for (int y = hy + 2; y < dy - 1; y += 3) { gPut(hx, y, 'E'); gPut(hx + w - 1, y, 'E'); } }
    else { gPut(hx + 2, hy, 'E'); gPut(hx + w - 3, hy, 'E'); gPut(hx, hy + hh / 2, 'E'); gPut(hx + w - 1, hy + hh / 2, 'E');   // windows
      if (dx - 2 > hx) gPut(dx - 2, dy, 'E'); if (dx + 2 < hx + w - 1) gPut(dx + 2, dy, 'E'); }
    int ix0 = hx + 1, iy0 = hy + 1, ix1 = hx + w - 2, iy1 = hy + hh - 2;
    int u = reg_home(ix0, iy0, ix1, iy1, dx, dy);
    int ns = (grand && bm_gen >= 4) ? (sz == 0 ? 3 : sz < 12 ? 2 : 1) : 1;                  // a mansion has an upstairs (the estate two): stairs up from the front room
    fz_geo = h * 2246822519u + 7u;
    int wy_ = iy0 + (iy1 - iy0 + 1 >= 11 ? 5 : (iy1 - iy0 + 1 >= 9 ? 4 : 3)), sty = wy_ + 2, stA = ix0 + 1, stB = ix1 - 1;
    if (ns > 1) {                                                                           // the stairs go in the front room, two tiles clear of every back room's door and of the front door
        int k_, wy2, cu[3], r0[3], r1[3], dd[3]; stA = stB = -1;
        if (fp_geo(ix0, iy0, ix1, iy1, &k_, &wy2, cu, r0, r1, dd)) for (int x = ix0 + 1; x <= ix1 - 1; x++) { int ok = ABS(x - dx) >= 2; for (int i = 0; i < k_; i++) if (ABS(x - dd[i]) < 2) ok = 0;
            if (ok) { if (stA < 0) stA = x; stB = x; } }
        if (stA < 0 || stA == stB) ns = 1;
    }
    if (ns > 1) { fz_upx = stA; fz_upy = sty; fz_holex = -1; }
    furnish_plan(ix0, iy0, ix1, iy1, dx, occ_of(u), h * 2654435761u, grand);
    fz_upx = fz_holex = -1;
    for (int l = 1; l < ns; l++) {                                                          // the upper floors: the same walls and windows, a suite, a study or a games room, a lounge
        if (!storey(l)) break;
        sRoom(hx, hy, hx + w - 1, hy + hh - 1, cfl(city_h((int)h + l, 5)), wp);
        for (int x = hx + 2; x < hx + w - 2; x += 3) { sm[hy][x] = 'E'; sm[hy + hh - 1][x] = 'E'; }
        for (int y = hy + 2; y < hy + hh - 2; y += 3) { sm[y][hx] = 'E'; sm[y][hx + w - 1] = 'E'; }
        fz_holex = (l & 1) ? stA : stB; fz_holey = sty;
        if (l + 1 < ns) { fz_upx = (l & 1) ? stB : stA; fz_upy = sty; } else fz_upx = -1;
        fz_upper = 1;
        furnish_plan(ix0, iy0, ix1, iy1, hx + w / 2, &occ_any, (h + (unsigned)l * 977u) * 2654435761u, 1);
        fz_upper = 0; fz_upx = fz_holex = -1;
    }
    storey(0); roof_dirty = 1;
    yard(lx, ly, dx, dy, occ_of(u)->vacant, h >> 3);
    if (!grand) backyard(lx, ly, hx, hy, w, dy, h * 2246822519u);
    if (grand) { gFree(dx - 3, dy + 2, '}'); gFree(dx + 3, dy + 2, '}'); gFree(hx + 1, dy + 1, 'Z'); gFree(hx + w - 2, dy + 1, 'Z'); gFree(dx - 4, dy + 1, TREE); gFree(dx + 4, dy + 1, TREE); }
}
static void city_cottage(int lx, int ly, unsigned h) {   // a small home: a bathroom in the back corner behind a door of its own, and one room for everything else
    int w = 8 + (int)((h >> 4) & 1), hh = 7 + (int)((h >> 5) & 1);
    int hx = lx + 2 + (int)((h >> 6) & 1), hy = ly + 2 + (int)((h >> 7) & 1);
    if ((h >> 8) & 1) hy = ly + 11 - hh;                                       // (half of them stand at the front of the lot: a big backyard behind)
    int fl = cfl(h >> 9), wp = pick_wp(h);
    gRoom(hx, hy, hx + w - 1, hy + hh - 1, fl, wp);
    int dx = hx + w / 2, dy = hy + hh - 1;
    gPut(dx, dy, 'D'); city_path(dx, dy, ly);
    gPut(hx + 2, hy, 'E'); gPut(hx + w - 3, hy, 'E'); gPut(hx, hy + 3, 'E'); gPut(hx + w - 1, hy + 3, 'E');
    int x0 = hx + 1, y0 = hy + 1, x1 = hx + w - 2, y1 = hy + hh - 2;
    int u = reg_home(x0, y0, x1, y1, dx, dy);
    int right = (int)((h >> 10) & 1), cx0 = right ? x1 - 2 : x0, cx1 = cx0 + 2, wx = right ? cx0 - 1 : cx1 + 1, bdx = cx0 + 1, wpi = cwp(h >> 12);   // the bath cell: 3 wide, 2 deep, its wall to the room and a door in the middle of it
    for (int y = y0; y <= y0 + 2; y++) { sm[y][wx] = 'W'; swp[y][wx] = (u8)wpi; }
    for (int x = MIN(cx0, wx); x <= MAX(cx1, wx); x++) { sm[y0 + 2][x] = 'W'; swp[y0 + 2][x] = (u8)wpi; }
    sm[y0 + 2][bdx] = 'D'; for (int y = y0; y <= y0 + 1; y++) for (int x = cx0; x <= cx1; x++) { sm[y][x] = 'W'; sfl[y][x] = (u8)cfl(h >> 14); }
    fz_nres = 2; fz_res[0][0] = (u8)bdx; fz_res[0][1] = (u8)(y0 + 3); fz_res[1][0] = (u8)bdx; fz_res[1][1] = (u8)(y0 + 4);
    fz_no_bath = 1; furnish(x0, y0, x1, y1, dx, dy, occ_of(u), h * 2246822519u); fz_no_bath = 0; fz_nres = 0;
    for (int y = y0; y <= y0 + 1; y++) for (int x = cx0; x <= cx1; x++) sm[y][x] = '.';
    if (!occ_of(u)->vacant) { sm[y0][cx0] = 'T'; sm[y0][cx1] = '-'; if (hh > 7 && ((h >> 15) & 1)) sm[y0 + 1][cx1] = ','; }   // the toilet and the sink in the back corners, the lane from the door between them clear
    yard(lx, ly, dx, dy, occ_of(u)->vacant, h >> 5);
    backyard(lx, ly, hx, hy, w, dy, h * 2654435761u);
}
static void city_town(int lx, int ly, unsigned h) {   // two homes sharing the wall between them, each with its own door, floors and wallpaper
    int hy = ly + 1;
    for (int u = 0; u < 2; u++) {
        unsigned hu = city_h((int)(h & 0xFFFF) + u * 31, u + 9);
        int ax = lx + 6 * u, fl = cfl(hu >> 9), wp = cwp(hu >> 13);
        gRoom(ax, hy, ax + 6, hy + 8, fl, wp);
        int dx = ax + 3, dy = hy + 8;
        gPut(dx, dy, 'D'); city_path(dx, dy, ly);
        gPut(ax + 2, hy, 'E'); gPut(ax + 4, hy, 'E'); gPut(u ? ax + 6 : ax, hy + 4, 'E');                       // the party wall has no window
        int k = reg_home(ax + 1, hy + 1, ax + 5, hy + 7, dx, dy);
        furnish(ax + 1, hy + 1, ax + 5, hy + 7, dx, dy, occ_of(k), hu * 2654435761u);
        gFree(dx - 2, dy + 1, 'Z');                                  // (a townhouse is not sold on its own: no sign)
    }
    gFree(lx + 3, ly, TREE); gFree(lx + 9, ly, TREE); gFree(lx + 6, ly + 12, 'Y');
}
static void city_two(int lx, int ly, unsigned h) {   // a two-storey house: the household downstairs; a loft (spare beds, a desk) up the stairs
    int hx = lx + 2, hy = ly + 1;
    int fl = cfl(h >> 9), wp = cwp(h >> 13), fl2 = cfl(h >> 17), wp2 = cwp(h >> 3);
    gRoom(hx, hy, hx + 8, hy + 7, fl, wp);
    int dx = hx + 4, dy = hy + 7;
    gPut(dx, dy, 'D'); city_path(dx, dy, ly);
    gPut(hx + 2, hy, 'E'); gPut(hx + 6, hy, 'E'); gPut(hx, hy + 3, 'E'); gPut(hx + 8, hy + 3, 'E'); gPut(hx + 2, dy, 'E'); gPut(hx + 6, dy, 'E');
    int x0 = hx + 1, y0 = hy + 1;                                                    // the inside: 7 wide, 6 deep
    int sx = x0 + ((h >> 21) & 1 ? 5 : 1), sy = y0 + 2;                              // the stairs rise to the north: open floor above and below them
    lifeMap[sy][sx] = '|';
    int u = reg_home(x0, y0, x0 + 6, y0 + 5, dx, dy);
    const Occ *o = occ_of(u);
    fz_nres = 2; fz_res[0][0] = (u8)sx; fz_res[0][1] = (u8)(sy - 1); fz_res[1][0] = (u8)sx; fz_res[1][1] = (u8)(sy + 1);
    Occ down = *o; if (!o->vacant && (o->kids || o->teens)) { down.kids = 0; down.teens = 0; }   // the kids sleep upstairs
    furnish(x0, y0, x0 + 6, y0 + 5, dx, dy, &down, h * 2654435761u);
    Level *U = lv_need(0);                                                           // the loft covers the whole footprint
    if (U) {
        for (int j = hy; j <= hy + 7; j++) for (int i = hx; i <= hx + 8; i++) {
            int edge = (i == hx || i == hx + 8 || j == hy || j == hy + 7);
            U->map[j][i] = edge ? 'W' : '.'; U->fl[j][i] = (u8)fl2; U->wp[j][i] = (u8)wp2;
        }
        U->map[hy][hx + 2] = 'E'; U->map[hy][hx + 6] = 'E'; U->map[hy + 3][hx] = 'E'; U->map[hy + 3][hx + 8] = 'E'; U->map[hy + 7][hx + 2] = 'E'; U->map[hy + 7][hx + 6] = 'E';
        U->map[sy][sx] = ' ';                                                        // the stairwell
        if (!o->vacant) {                                                            // the loft: the kids' room, or a spare bed and a desk
            int lx0 = sx > x0 + 3 ? x0 : x0 + 6, step = sx > x0 + 3 ? 1 : -1;
            if (o->kids || o->teens) { U->map[y0][lx0] = '<'; U->map[y0][lx0 + step * 2] = o->teens ? 'S' : '<'; U->map[y0 + 5][lx0] = 'n'; U->map[y0 + 3][lx0 + step * 3] = 'r'; U->map[y0 + 5][lx0 + step * 2] = o->teens ? '[' : '{'; }
            else { U->map[y0][lx0] = 'S'; U->map[y0][lx0 + step * 2] = 'b'; U->map[y0 + 5][lx0] = 'd'; U->map[y0 + 5][lx0 + step] = '['; U->map[y0 + 3][lx0 + step * 3] = 'r'; }
            U->map[y0 + 5][x0 + 3] = 'p';
        }
    }
    roof_dirty = 1;
    yard(lx, ly, dx, dy, o->vacant, h >> 7);
}

// ---- the places ----
// A PARK is one of five designs, mirrored half the time, the same for a block every time (the dice are the block's): the paths run from the gate at the front to the middle,
// benches stand on the path side, the trees in matched pairs, the beds in rows, and a bin and the flag at the gate.
static int pk_m;                                                                    // 1: mirrored left to right
static int pk_x(int x) { return PLx + (pk_m ? 12 - x : x); }
static void pk_put(int x, int y, char c) { gFree(pk_x(x), PLy + y, c); }
static void pk_box(int x0, int y0, int x1, int y1, int fl) { int a = pk_x(x0), b = pk_x(x1); gBox(MIN(a, b), PLy + y0, MAX(a, b), PLy + y1, fl); }
static void pk_tree(int x, int y) { pk_put(x, y, TREE); }
static void lot_park(int lx, int ly, unsigned h) {
    PLx = lx; PLy = ly; pk_m = (int)(h & 1);
    int v = (int)((h >> 2) % 5u), tp = (int)((h >> 6) & 1);
    pk_box(6, 0, 6, 12, FL_PAVING);                                                    // the walk up from the gate
    if (v == 0) {                                                                       // FORMAL: a cross of paths, a fountain on a plaza with a bench at each corner, a bed in each quarter
        pk_box(0, 6, 12, 6, FL_PAVING); pk_box(4, 4, 8, 8, FL_PAVING); gPut(pk_x(6), ly + 6, '}');
        pk_put(4, 4, 'N'); pk_put(8, 4, 'N'); pk_put(4, 8, 'N'); pk_put(8, 8, 'N');
        for (int q = 0; q < 4; q++) { int sx = (q & 1) ? 10 : 2, sy = (q & 2) ? 10 : 2; pk_box(sx - 1, sy - 1, sx + 1, sy + 1, FL_SOIL); pk_put(sx, sy, 'p'); }
        pk_tree(1, 5); pk_tree(11, 5); pk_tree(1, 7); pk_tree(11, 7); pk_put(5, 11, 'Z'); pk_put(7, 11, 'Z');
    } else if (v == 1) {                                                                // THE POND: a path round the water, benches by it, a grove behind, a picnic table
        pk_box(1, 1, 5, 5, FL_PATIO); pk_box(2, 2, 4, 4, FL_WATER); pk_box(0, 6, 12, 6, FL_PAVING);
        pk_put(1, 7, 'N'); pk_put(4, 7, 'N'); pk_put(6, 4, 'N');
        pk_tree(8, 1); pk_tree(10, 1); pk_tree(9, 2); pk_tree(11, 3); pk_tree(8, 4); pk_tree(0, 1); pk_tree(0, 3);
        pk_box(8, 8, 10, 10, FL_PATIO); pk_put(9, 9, 'K'); pk_put(11, 9, 'Y'); pk_tree(11, 11); pk_tree(1, 11); pk_tree(2, 9); pk_tree(1, 8);
    } else if (v == 2) {                                                                // THE COURT: a paved court with a hoop at each end, benches along its side, trees round the edge
        pk_box(1, 2, 5, 9, FL_PAVING); pk_put(3, 2, '_'); pk_put(3, 9, '_'); pk_put(7, 4, 'N'); pk_put(7, 8, 'N'); pk_box(7, 6, 11, 6, FL_PAVING);
        for (int y = 1; y <= 11; y += 2) pk_tree(11, y); pk_tree(8, 1); pk_tree(9, 1); pk_put(9, 9, 'K'); pk_put(10, 10, 'Y');
    } else if (v == 3) {                                                                // THE GARDEN: beds of flowers in rows either side of the walk, a bird bath, benches at the end, trees in the corners
        for (int r = 0; r < 3; r++) { int y = 2 + r * 3; pk_box(1, y, 4, y, FL_SOIL); pk_box(8, y, 11, y, FL_SOIL); for (int x = 1; x <= 4; x++) { pk_put(x, y, 'p'); pk_put(12 - x, y, 'p'); } }
        pk_put(6, 1, (char)0xE4); pk_put(5, 11, 'N'); pk_put(7, 11, 'N'); pk_tree(0, 0); pk_tree(12, 0); pk_tree(0, 12); pk_tree(12, 12); pk_put(0, 6, 'Z'); pk_put(12, 6, 'Z');
    } else {                                                                            // THE LAWN: an open lawn, a ring of trees, picnic tables, the benches along the walk
        pk_box(0, 6, 12, 6, FL_PAVING); pk_put(5, 5, 'N'); pk_put(7, 5, 'N'); pk_put(5, 7, 'N'); pk_put(7, 7, 'N');
        pk_tree(1, 1); pk_tree(11, 1); pk_tree(3, 2); pk_tree(9, 2); pk_tree(1, 11); pk_tree(11, 11); pk_tree(2, 9); pk_tree(10, 9);
        pk_put(9, 4, 'K'); pk_put(3, 4, 'K'); if (tp) pk_put(6, 2, '}');
    }
    gFree(lx + 5, ly + 12, 'Y'); gFree(lx + 7, ly + 12, 'a');                           // the bin and the flag at the gate
}
static void city_square(int lx, int ly) {
    gBox(lx, ly, lx + 12, ly + 12, FL_PAVING);
    gPut(lx + 6, ly + 6, '}');
    gFree(lx + 4, ly + 6, 'N'); gFree(lx + 8, ly + 6, 'N'); gFree(lx + 6, ly + 4, 'N'); gFree(lx + 6, ly + 8, 'N');
    gFree(lx + 1, ly + 1, 'Z'); gFree(lx + 11, ly + 1, 'Z'); gFree(lx + 1, ly + 11, 'Z'); gFree(lx + 11, ly + 11, 'Z');
    gFree(lx + 3, ly + 3, TREE); gFree(lx + 9, ly + 3, TREE); gFree(lx + 3, ly + 9, TREE); gFree(lx + 9, ly + 9, TREE);
    gFree(lx + 12, ly + 1, '+'); gFree(lx + 1, ly + 6, 'a'); gFree(lx + 11, ly + 6, 'Y');   // the bus stop, the community flag
}
static void gFace(int x, int y, char c, int dir) { if (x >= 0 && y >= 0 && x < MW && y < MH && lifeMap[y][x] == '.') { lifeMap[y][x] = c; wallMap[y][x] = (u8)(0x80 | dir); } }   // an item turned (0 north 1 west 2 south 3 east: a ramp's lip)
static void city_skate(int lx, int ly) {   // the SKATE PLAZA, life size: quarter pipes round three sides (two tiles deep: 1.65 m), a funbox with a kicker either side,
    gBox(lx, ly, lx + 12, ly + 12, 7);     // a rail, a ledge, a manual pad and a launch ramp; the west side is open to the street
    for (int x = 1; x <= 10; x++) { gFace(lx + x, ly, '5', 0); gFace(lx + x, ly + 1, '5', 0); gFace(lx + x, ly + 12, '5', 2); gFace(lx + x, ly + 11, '5', 2); }
    for (int y = 2; y <= 10; y++) { gFace(lx + 12, ly + y, '5', 3); gFace(lx + 11, ly + y, '5', 3); }
    for (int y = 5; y <= 6; y++) { gFree(lx + 4, ly + y, '4'); for (int x = 5; x <= 7; x++) gFree(lx + x, ly + y, 'X'); gFree(lx + 8, ly + y, '2'); }   // the funbox: up on the west, down on the east
    for (int x = 3; x <= 8; x++) gFree(lx + x, ly + 3, '=');
    for (int x = 3; x <= 8; x++) gFree(lx + x, ly + 9, 'L');
    for (int y = 5; y <= 6; y++) { gFree(lx + 1, ly + y, 'M'); gFree(lx + 2, ly + y, 'M'); }
    gFace(lx + 1, ly + 8, '9', 3); gFace(lx + 2, ly + 8, '9', 3);
    gFree(lx + 12, ly, 'Y'); gFree(lx + 12, ly + 12, 'Y'); gFree(lx, ly, 'k'); gFree(lx + 11, ly, 'Y');
}
static int shell(int lx, int ly, int x0, int y0, int x1, int y1, int fl, int wp, int glass) {   // a place's building: walls, windows, a door to the street
    gRoom(x0, y0, x1, y1, fl, wp);
    int dx = (x0 + x1) / 2; gPut(dx, y1, 'D');
    if (glass) { for (int x = x0 + 1; x < x1; x++) if (x != dx && ((x - dx) & 1)) gPut(x, y1, 'E'); }
    else { gPut(dx - 2, y1, 'E'); gPut(dx + 2, y1, 'E'); }
    gPut(x0 + 2, y0, 'E'); gPut(x1 - 2, y0, 'E');
    gBox(dx, y1 + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING);
    return dx;
}
static void sh_run(int x0, int x1, int y, const char *s) { int n = (int)strlen(s); for (int x = x0, i = 0; x <= x1; x++, i++) gFree(x, y, s[i % n]); }   // a run of things along a row (the wall's), the pattern repeating
static void lot_shop(int lx, int ly, unsigned h) {   // a shop: shelves along the back wall and down the side walls, rows of stands either side of the aisle from the door (all joined to a wall), the till by the door, plants at the window
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 9, kind = (int)(h % 5u), lay = (int)((h >> 5) & 1);
    static const u8 fls[5] = { 3, 1, 6, 5, 4 };
    int dx = shell(lx, ly, x0, y0, x1, y1, fls[kind], pick_wp(h), 1);
    int ix0 = x0 + 1, iy0 = y0 + 1, ix1 = x1 - 1, iy1 = y1 - 1;
    static const char *back[5] = { "F7", "i`", "vv[R", "SiCb", "$(" };                       // the back wall: fridges and shelves, dressers and mirrors, tellies and computers, beds and cases, pianos and guitars
    static const char *rows[5] = { "7", "i", "y", "i", "A" };                                // the stands in the rows
    sh_run(ix0, ix1, iy0, back[kind]);
    int ra = iy0 + 2, rb = lay ? iy0 + 2 : iy0 + 4;                                           // two rows of stands (or one), split by the aisle from the door
    for (int side = 0; side < 2; side++) { int a = side ? dx + 2 : ix0, b = side ? ix1 : dx - 2; if (b - a < 2) continue;
        for (int x = a; x <= b; x++) { gFree(x, ra, rows[kind][0]); if (rb != ra) gFree(x, rb, rows[kind][0]); } }
    gFree(ix1, iy1, 'y'); gFree(ix1 - 1, iy1, 'y'); gFree(ix1 - 2, iy1, 'I');                  // the till, in the corner by the door wall
    gFree(ix0, iy1, 'p'); gFree(ix0 + 1, iy1, 'l'); gFree(dx - 2, iy1 - 1, 'r'); gFree(dx + 2, iy1 - 1, 'r');
    gFree(lx + 2, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z'); gFree(lx + 3, ly + 11, 'N'); gFree(lx + 9, ly + 11, 'Y'); gFree(lx + 1, ly + 12, 'Z');   // the planters and bench by the shop front
}
static void city_diner(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, (h & 1) ? 3 : 2, pick_wp(h)); gPut(lx + 6, y1, 'D'); gPut(lx + 7, y1, 'D');
    gBox(lx + 6, y1 + 1, lx + 7, MIN(ly + 12, MH - 2), FL_PAVING);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, '/'); gFree(x0 + 3, y0 + 1, 'y'); gFree(x0 + 4, y0 + 1, 'y'); gFree(x0 + 5, y0 + 1, 'c'); gFree(x1 - 1, y0 + 1, 'T');
    for (int k = 0; k < 2; k++) { int tx = x0 + 2 + k * 4; gFree(tx, y0 + 4, 't'); gFree(tx - 1, y0 + 4, 'h'); gFree(tx + 1, y0 + 4, 'h'); }
    if (h & 4) { gFree(x1 - 2, y0 + 3, 'C'); gFree(x1 - 1, y0 + 4, 't'); }
    gFree(x1 - 1, y1 - 1, (h & 8) ? 'q' : 'j'); gFree(x0 + 1, y1 - 1, (h & 8) ? 'j' : 'p'); gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E');
    gFree(lx + 3, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void city_gym(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, 7, NWP + 61); gPut(lx + 6, y1, 'D'); gBox(lx + 6, y1 + 1, lx + 6, MIN(ly + 12, MH - 2), FL_PAVING);
    for (int x = x0 + 1; x <= x0 + 5; x++) gFree(x, y0 + 1, 'm');                      // the treadmills in a row along the back wall, the benches in a row off the side wall
    for (int x = x0 + 1; x <= x0 + 3 + (int)(h & 1); x++) gFree(x, y0 + 4, ']'); gFree(x0 + 1, y0 + 6, 'Q'); gFree(x0 + 1, y0 + 7, 'Q');
    gFree(x1 - 1, y0 + 1, 'H'); gFree(x1 - 2, y0 + 1, 'H'); gFree(x1 - 3, y0 + 1, 'T'); gFree(x1 - 1, y1 - 1, 'F'); gFree(x1 - 3, y1 - 1, 'R');
    if (h & 2) gFree(x1 - 1, y0 + 4, '_');
    gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E'); gFree(lx + 3, ly + 11, 'Y'); gFree(lx + 9, ly + 11, 'Z');
}
static void city_office(int lx, int ly, unsigned h) {   // where work is: the work marker
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    gRoom(x0, y0, x1, y1, 3, pick_wp(h)); gPut(lx + 6, y1, 'D'); gBox(lx + 6, y1 + 1, lx + 6, MIN(ly + 12, MH - 2), FL_PAVING);
    for (int k = 0; k < 3; k++) { int dxk = x0 + 2 + k * 3; gFree(dxk, y0 + 2, 'd'); gFree(dxk, y0 + 3, 'h'); gFree(dxk, y0 + 1, '['); }
    gFree(x1 - 1, y1 - 1, 'x'); gFree(x0 + 1, y1 - 1, 'F'); gFree(x0 + 2, y1 - 1, 'c'); gFree(x0 + 1, y0 + 1, 'b'); gFree(x1 - 1, y0 + 1, 'T');
    gFree(x0 + 5, y1 - 2, 'x'); gFree(x1 - 1, y0 + 3, (h & 2) ? 'q' : 'p'); gPut(x0 + 3, y0, 'E'); gPut(x1 - 3, y0, 'E'); gFree(lx + 3, ly + 11, 'Z');
}
static void lot_inn(int lx, int ly, unsigned h) {   // three rooms off a hall, the lobby by the door
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 11, fl = cfl(h >> 5), fl2 = cfl(h >> 9), wp = cwp(h >> 13);
    gRoom(x0, y0, x1, y1, fl2, wp);
    gBox(x0 + 1, y0 + 1, x1 - 1, y0 + 4, fl);
    gLine(x0, y0 + 5, x1, y0 + 5, 'W', wp); gLine(x0 + 4, y0, x0 + 4, y0 + 5, 'W', wp); gLine(x0 + 8, y0, x0 + 8, y0 + 5, 'W', wp);
    for (int r = 0; r < 3; r++) {
        int rx = x0 + 1 + r * 4; gPut(rx + 1, y0 + 5, 'D'); gPut(rx + 1, y0, 'E');
        gFree(rx, y0 + 1, 'S'); gFree(rx + 2, y0 + 1, (h >> r) & 1 ? 'i' : 'l'); gFree(rx + 2, y0 + 4, (h >> (r + 3)) & 1 ? 'p' : 'v');
    }
    int dx = lx + 6; gPut(dx, y1, 'D'); gPut(dx - 3, y1, 'E'); gPut(dx + 3, y1, 'E');
    gBox(dx, y1 + 1, dx, MIN(ly + 12, MH - 2), FL_PAVING);
    gFree(x1 - 3, y0 + 8, 'y'); gFree(x1 - 2, y0 + 8, 'y'); gFree(x1 - 1, y0 + 8, 'I');           // the front desk
    gFree(x0 + 1, y0 + 9, 'C'); gFree(x0 + 2, y0 + 9, 'C'); gFree(x0 + 1, y0 + 7, ';'); gFree(x0 + 3, y0 + 8, 'r'); gFree(x1 - 1, y0 + 6, 'p'); gFree(x0 + 1, y0 + 6, 'l');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_pool(int lx, int ly, unsigned h) {   // the community pool: a tiled deck, the water, a diving board, ladders, a lifeguard chair, loungers and umbrellas, showers, a hot tub
    gBox(lx, ly, lx + 12, ly + 12, FL_POOLTILE);
    gLine(lx, ly, lx + 12, ly, 'w', 13); gLine(lx, ly, lx, ly + 12, 'w', 13); gLine(lx + 12, ly, lx + 12, ly + 12, 'w', 13); gLine(lx, ly + 12, lx + 12, ly + 12, 'w', 13);
    lifeMap[ly + 12][lx + 6] = '.'; lifeMap[ly + 12][lx + 5] = '.'; floorMap[ly + 12][lx + 6] = FL_PAVING; floorMap[ly + 12][lx + 5] = FL_PAVING;   // the gate
    int pw = 6 + (int)(h & 1), ph = 4 + (int)((h >> 1) & 1);
    gBox(lx + 3, ly + 3, lx + 2 + pw, ly + 2 + ph, FL_WATER);
    for (int x = lx + 3; x <= lx + 2 + pw; x += 2) gFree(x, ly + 4 + ph, '~');
    gFree(lx + 1, ly + 1, 'H'); gFree(lx + 2, ly + 1, 'H'); gFree(lx + 11, ly + 1, '>'); gFree(lx + 11, ly + 3, '~'); gFree(lx + 11, ly + 5, '~');
    gFree(lx + 1, ly + 4, 'Z'); gFree(lx + 1, ly + 8, 'Z'); gFree(lx + 11, ly + 10, 'Y'); gFree(lx + 9, ly + 1, '#'); gFree(lx + 1, ly + 11, 'N');
    gFree(lx + 3 + pw / 2, ly + 2, (char)0xE5);                                  // the diving board, over the deep end
    gFree(lx + 2, ly + 3, (char)0xE6); gFree(lx + 3 + pw, ly + 4, (char)0xE6);  // ladders
    gFree(lx + 10, ly + 8, (char)0xE7);                                          // the lifeguard's chair
    gFree(lx + 4, ly + 5 + ph, (char)0xE8); gFree(lx + 5, ly + 5 + ph, (char)0xE9); gFree(lx + 8, ly + 5 + ph, (char)0xE8);   // umbrellas and towels
    gFree(lx + 11, ly + 8 - (int)(h & 1), (char)0xEA);
}
static void lot_school(int lx, int ly, unsigned h) {   // a classroom, and a playground out front
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 8;
    int dx = shell(lx, ly, x0, y0, x1, y1, cfl(h >> 5), pick_wp(h), 0);
    for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) { int x = x0 + 2 + c * 3, y = y0 + 3 + r * 2; if (x == dx) continue; gFree(x, y, 'd'); gFree(x, y + 1, 'h'); }
    gFree(x0 + 5, y0 + 1, 'd'); gFree(x0 + 6, y0 + 1, '['); gFree(x0 + 1, y0 + 1, 'b'); gFree(x0 + 2, y0 + 1, 'b'); gFree(x1 - 1, y0 + 1, 'n'); gFree(x1 - 2, y0 + 1, 'p');
    gFree(lx + 1, ly + 10, '_'); gFree(lx + 4, ly + 11, 'N'); gFree(lx + 9, ly + 10, 'K'); gFree(lx + 12, ly + 10, 'a'); gFree(lx + 11, ly + 12, 'Y'); gFree(lx + 2, ly + 12, TREE);
}
static void lot_arcade(int lx, int ly, unsigned h) {
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, 7, pick_wp(h), 0);
    for (int x = x0 + 1; x <= x1 - 1; x++) gFree(x, y0 + 1, '!');
    gFree(x0 + 1, y0 + 3, '!'); gFree(x0 + 1, y0 + 4, '!'); gFree(x1 - 1, y0 + 3, 'j'); gFree(x1 - 1, y0 + 5, 'V');
    gFree(x0 + 4, y0 + 4, '?'); gFree(x0 + 1, y1 - 1, 'C'); gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 1, 'c'); gFree(x0 + 6, y0 + 4, 'U');
    gFree(lx + 2, ly + 11, 'Y');
}
// ---- more places to go: a cafe, a bar, a cinema, a salon, a restaurant, a clinic, a library, a spa, an art gallery ----
static void lot_cafe(int lx, int ly, unsigned h) {   // the counter and its machines along the back, little tables, armchairs by the window
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 0, pick_wp(h), 1);
    gFree(x0 + 1, y0 + 1, 'c'); gFree(x0 + 2, y0 + 1, 'c'); gFree(x0 + 3, y0 + 1, 'y'); gFree(x0 + 4, y0 + 1, 'y'); gFree(x0 + 5, y0 + 1, 'F'); gFree(x0 + 6, y0 + 1, '-');
    for (int k = 0; k < 2; k++) { int tx = k ? x1 - 3 : x0 + 2; gFree(tx, y0 + 4, 't'); gFree(tx - 1, y0 + 4, 'h'); gFree(tx + 1, y0 + 4, 'h'); }
    gFree(x1 - 1, y0 + 1, 'b'); gFree(x1 - 2, y0 + 1, '['); gFree(x0 + 1, y1 - 1, ';'); gFree(x0 + 2, y1 - 1, 'l'); gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, 'p');
    if (h & 2) gFree(x1 - 1, y0 + 3, ')');                                     // (some show a painting)
    gFree(lx + 2, ly + 11, 'K'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_gas(int lx, int ly, unsigned h) {   // a gas station: the shop at the back, the forecourt in front with its pump islands and the air and water, a sign by the road
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 5;
    gBox(lx, ly + 6, lx + 12, ly + 12, FL_ASPHALT);                                   // the forecourt
    int dx = shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 3, pick_wp(h), 1);
    gBox(dx, y1 + 1, dx, ly + 7, FL_PAVING);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, 'F'); gFree(x0 + 3, y0 + 1, 'c'); for (int x = x0 + 4; x <= x0 + 6; x++) gFree(x, y0 + 1, '7'); gFree(x1 - 1, y0 + 1, ':');
    gFree(x1 - 1, y1 - 1, 'y'); gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 2, 'I'); gFree(x0 + 1, y1 - 1, '7'); gFree(x0 + 2, y1 - 1, '7');   // the till, a rack by the door
    for (int k = 0; k < 2; k++) for (int j = 0; j < 2; j++) { gFree(lx + 2 + k * 8, ly + 8 + j * 2, '\x82'); gFree(lx + 3 + k * 8, ly + 8 + j * 2, '\x82'); }   // four islands of two pumps
    gFree(lx + 1, ly + 12, 'O'); gFree(lx + 11, ly + 7, 'Y'); gFree(lx + 12, ly + 12, 'Z'); gFree(lx + 5, ly + 10, 'J'); gFree(lx + 7, ly + 10, 'J');
    gFree(lx + 12, ly + 6, '"'); gFree(lx, ly + 6, '"');
}
static void lot_conv(int lx, int ly, unsigned h) {   // a corner store: shelves in aisles, the fridges along the wall, the till by the door, a bench and a bin outside
    int x0 = lx + 2, y0 = ly + 3, x1 = lx + 10, y1 = ly + 9;
    int dx = shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 4, pick_wp(h), 1);
    int ix0 = x0 + 1, iy0 = y0 + 1, ix1 = x1 - 1, iy1 = y1 - 1;
    for (int x = ix0; x <= ix1; x++) gFree(x, iy0, (x & 1) ? 'F' : '7');
    for (int x = ix0 + 1; x <= ix1 - 2; x++) if (x != dx && x != dx - 1) { gFree(x, iy0 + 2, '7'); gFree(x, iy0 + 3, '7'); }
    gFree(ix1, iy1, 'y'); gFree(ix1 - 1, iy1, 'y'); gFree(ix1 - 2, iy1, 'I'); gFree(ix0, iy1, 'c'); gFree(ix0 + 1, iy1, ':'); gFree(ix0, iy1 - 1, 'p');
    gFree(lx + 1, ly + 10, 'N'); gFree(lx + 11, ly + 10, 'Y'); gFree(lx + 1, ly + 12, 'Z'); gFree(lx + 11, ly + 12, 'Z');
    gBox(lx + 1, ly + 3, lx + 1, ly + 9, FL_PAVING);
}
static void lot_bar(int lx, int ly, unsigned h) {   // the counter and its stools, the back bar behind, a little stage, a booth, the jukebox
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 13, pick_wp(h), 0);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 5, y0 + 1, 'c');                                        // the back bar (room to stand between)
    for (int x = x0 + 1; x <= x0 + 5; x++) { gFree(x, y0 + 2, 'y'); gFree(x, y0 + 3, 'h'); }       // the counter, the stools
    gFree(x1 - 1, y0 + 1, '$'); gFree(x1 - 2, y0 + 1, '('); gFree(x1 - 3, y0 + 1, 'A');             // the stage
    gBox(x1 - 3, y0 + 2, x1 - 1, y0 + 3, 3);                                                       // (and a dance floor before it)
    gFree(x0 + 1, y1 - 1, 'C'); gFree(x0 + 2, y1 - 1, 't'); gFree(x0 + 3, y1 - 1, 'C');
    gFree(x1 - 1, y1 - 1, 'j'); gFree(x1 - 1, y1 - 2, 'V'); gFree(x1 - 3, y1 - 1, '?');
    gFree(lx + 2, ly + 11, 'Y'); gFree(lx + 10, ly + 11, 'O');
}
static void lot_cinema(int lx, int ly, unsigned h) {   // the screen across the back, two rows of seats facing it (a low wall behind each), the lobby by the door
    int x0 = lx, y0 = ly, x1 = lx + 12, y1 = ly + 11;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 2 : 13, pick_wp(h), 0);
    for (int x = x0 + 3; x <= x1 - 3; x++) gFree(x, y0 + 1, 'v');                                  // the screen: a wall of screens
    gFree(x0 + 2, y0 + 1, 'A'); gFree(x1 - 2, y0 + 1, 'A');
    for (int r = 0; r < 2; r++) { int y = y0 + 3 + r * 3; for (int x = x0 + 2; x <= x1 - 2; x++) gFree(x, y, ';'); gLine(x0 + 2, y + 1, x1 - 2, y + 1, 'w', 13); }
    gBox(x0 + 1, y0 + 9, x1 - 1, y1 - 1, 3);                                                       // the lobby
    gFree(x0 + 1, y0 + 9, 'y'); gFree(x0 + 2, y0 + 9, 'y'); gFree(x0 + 3, y0 + 9, ':');            // tickets, popcorn
    gFree(x1 - 1, y0 + 9, '!'); gFree(x1 - 2, y0 + 9, '!'); gFree(x1 - 1, y1 - 1, 'p');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_salon(int lx, int ly, unsigned h) {   // mirrors along the back with a chair at each, the basins, the front desk, the waiting chairs
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 5 : 3, pick_wp(h), 1);
    for (int k = 0; k < 3; k++) { int x = x0 + 1 + k * 2; gFree(x, y0 + 1, '`'); gFree(x, y0 + 2, 'h'); }
    gFree(x0 + 7, y0 + 1, '-'); gFree(x0 + 8, y0 + 1, '-'); gFree(x1 - 1, y0 + 1, 'i');
    gFree(x1 - 2, y1 - 2, 'y'); gFree(x1 - 1, y1 - 2, 'y');
    gFree(x0 + 1, y1 - 1, ';'); gFree(x0 + 2, y1 - 1, ';'); gFree(x0 + 3, y1 - 1, 'p'); gFree(x0 + 1, y1 - 3, 'b'); gFree(x0 + 4, y0 + 4, 'r');
    gFree(lx + 2, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_restaurant(int lx, int ly, unsigned h) {   // the kitchen behind a low wall at the back, tables laid for two, the host's stand by the door
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 10;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 1, pick_wp(h), 1);
    gBox(x0 + 1, y0 + 1, x1 - 1, y0 + 2, 3);
    gFree(x0 + 1, y0 + 1, 'F'); gFree(x0 + 2, y0 + 1, '/'); gFree(x0 + 3, y0 + 1, '/'); gFree(x0 + 4, y0 + 1, '-'); gFree(x0 + 5, y0 + 1, 'y'); gFree(x1 - 1, y0 + 1, 'F');
    gLine(x0 + 1, y0 + 3, x1 - 4, y0 + 3, 'w', 13);                                               // the pass (open at the right end)
    for (int r = 0; r < 2; r++) for (int c = 0; c < 2; c++) { int tx = c ? x0 + 9 : x0 + 2, ty = y0 + 5 + r * 2;
        gFree(tx, ty, 't'); gFree(tx - 1, ty, 'h'); gFree(tx + 1, ty, 'h'); }
    gFree(x0 + 1, y1 - 1, 'y'); gFree(x0 + 2, y1 - 1, 'p'); gFree(x1 - 1, y1 - 1, 'p'); gFree(x1 - 2, y1 - 1, 'l'); gFree(x0 + 6, y0 + 6, '}');
    gFree(lx + 3, ly + 12, 'Z'); gFree(lx + 9, ly + 12, 'Z');
}
static void lot_clinic(int lx, int ly, unsigned h) {   // two exam rooms at the back, the waiting room and the reception by the door
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 10, wp = cwp(h >> 9);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 11, wp, 0);
    gLine(x0, y0 + 4, x1, y0 + 4, 'W', wp); gLine(x0 + 5, y0, x0 + 5, y0 + 4, 'W', wp);
    gPut(x0 + 2, y0 + 4, 'D'); gPut(x0 + 8, y0 + 4, 'D');
    for (int r = 0; r < 2; r++) { int rx = r ? x0 + 6 : x0 + 1; gFree(rx, y0 + 1, 'S'); gFree(rx + 2, y0 + 1, '-'); gFree(rx + 3, y0 + 1, 'd'); gFree(rx + 3, y0 + 2, 'h'); gFree(rx, y0 + 3, 'p'); }
    gFree(x1 - 1, y0 + 6, 'y'); gFree(x1 - 2, y0 + 6, 'y'); gFree(x1 - 1, y0 + 5, '[');
    for (int y = y0 + 6; y <= y1 - 1; y++) gFree(x0 + 1, y, ';');
    gFree(x0 + 3, y1 - 1, 'p'); gFree(x0 + 4, y0 + 7, 'r'); gFree(x0 + 1, y0 + 5, 'b');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_library(int lx, int ly, unsigned h) {   // the stacks, a reading table, computers, armchairs and lamps, the front desk
    int x0 = lx, y0 = ly + 1, x1 = lx + 12, y1 = ly + 10;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 1 : 4, pick_wp(h), 0);
    for (int r = 0; r < 2; r++) for (int x = x0 + 1; x <= x0 + 5; x++) gFree(x, y0 + 1 + r * 2, 'b');
    for (int x = x1 - 5; x <= x1 - 1; x++) gFree(x, y0 + 1, 'b');
    gFree(x1 - 4, y0 + 4, 't'); gFree(x1 - 5, y0 + 4, 'h'); gFree(x1 - 3, y0 + 4, 'h');
    gFree(x0 + 1, y0 + 6, '['); gFree(x0 + 2, y0 + 6, 'h'); gFree(x0 + 1, y0 + 8, '['); gFree(x0 + 2, y0 + 8, 'h');
    gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, 'l'); gFree(x1 - 1, y1 - 3, ';'); gFree(x0 + 4, y1 - 1, 'y'); gFree(x0 + 3, y1 - 1, 'y'); gFree(x0 + 8, y0 + 6, 'r');
    gFree(lx + 2, ly + 12, 'N'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_space(int lx, int ly, unsigned h) {   // the SPACE CENTRE: mission control (consoles, a briefing desk) and the launch pad with its rocket
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 6, wp = cwp(h >> 9);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 6, wp, 1);
    for (int x = x0 + 1; x <= x1 - 1; x += 2) { gFree(x, y0 + 1, (char)0xD9); gFree(x, y0 + 2, 'h'); }
    gFree(x0 + 1, y1 - 1, '['); gFree(x1 - 1, y1 - 1, '['); gFree(x0 + 5, y1 - 1, 'd');
    gBox(lx, ly + 8, lx + 12, ly + 12, FL_PAVING); gBox(lx + 4, ly + 9, lx + 8, ly + 12, FL_ROCK);
    gFree(lx + 6, ly + 10, (char)0xD8); gFree(lx + 2, ly + 9, 'O'); gFree(lx + 10, ly + 9, 'O');
}
static void lot_obs(int lx, int ly, unsigned h) {   // the OBSERVATORY: the public scopes (a hobby scope, then a research telescope and a dome for the certified), a radio dish out the back
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9, wp = cwp(h >> 9);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 4 : 5, wp, 0);
    gFree(x0 + 2, y0 + 2, (char)0xD4); gFree(x0 + 5, y0 + 2, (char)0xD5); gFree(x1 - 2, y0 + 2, (char)0xD6);
    gFree(x0 + 1, y0 + 5, 'b'); gFree(x0 + 1, y0 + 6, 'b'); gFree(x1 - 1, y0 + 5, '['); gFree(x1 - 1, y0 + 6, 'h'); gFree(x0 + 4, y1 - 1, 'y');
    gFree(lx + 2, ly + 11, (char)0xD7); gFree(lx + 10, ly + 11, (char)0xD3); gFree(lx + 6, ly + 11, 'N');
}
static void lot_spa(int lx, int ly, unsigned h) {   // the massage room, a sauna's showers, two hot tubs, a fountain, the front desk
    int x0 = lx + 1, y0 = ly + 1, x1 = lx + 11, y1 = ly + 10, wp = cwp(h >> 9);
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 6 : 5, wp, 0);
    gLine(x0 + 5, y0, x0 + 5, y0 + 4, 'W', wp); gLine(x0, y0 + 4, x0 + 5, y0 + 4, 'W', wp); gPut(x0 + 3, y0 + 4, 'D');
    gFree(x0 + 1, y0 + 1, 'S'); gFree(x0 + 3, y0 + 1, 'S'); gFree(x0 + 1, y0 + 3, 'p'); gFree(x0 + 4, y0 + 3, 'l');
    gFree(x0 + 6, y0 + 1, 'H'); gFree(x0 + 7, y0 + 1, 'H'); gFree(x0 + 7, y0 + 3, '>'); gFree(x1 - 1, y0 + 3, '>');
    gFree(x0 + 1, y1 - 1, 'y'); gFree(x0 + 2, y1 - 1, 'y'); gFree(x0 + 7, y0 + 6, '}');
    gFree(x1 - 1, y1 - 1, ';'); gFree(x1 - 2, y1 - 1, ';'); gFree(x0 + 1, y0 + 6, 'p'); gFree(x1 - 1, y0 + 5, 'p');
    gFree(lx + 2, ly + 12, 'Z'); gFree(lx + 10, ly + 12, 'Z');
}
static void lot_gallery(int lx, int ly, unsigned h) {   // paintings round the walls, sculptures, a fountain, benches, the desk
    int x0 = lx + 1, y0 = ly + 2, x1 = lx + 11, y1 = ly + 9;
    shell(lx, ly, x0, y0, x1, y1, (h & 1) ? 7 : 4, pick_wp(h), 1);
    for (int x = x0 + 1; x <= x1 - 1; x += 2) gFree(x, y0 + 1, ')');
    gFree(x0 + 1, y0 + 3, ')'); gFree(x0 + 1, y0 + 5, ')'); gFree(x1 - 1, y0 + 3, ')'); gFree(x1 - 1, y0 + 5, ')');
    gFree(x0 + 3, y0 + 4, '*'); gFree(x1 - 3, y0 + 4, '*'); gFree(x0 + 5, y0 + 4, '}');
    gFree(x0 + 3, y0 + 3, 'r'); gFree(x1 - 3, y0 + 3, 'r');
    gFree(x1 - 2, y1 - 1, 'y'); gFree(x1 - 1, y1 - 1, 'b'); gFree(x0 + 1, y1 - 1, 'p'); gFree(x0 + 2, y1 - 1, 'N');
    gFree(lx + 2, ly + 11, 'Z'); gFree(lx + 10, ly + 11, 'Z');
}
static void lot_empty(int lx, int ly, unsigned h) {   // nothing built yet: rough ground, junk, the sign
    for (int i = 0; i < 6; i++) { unsigned r = city_h((int)h + i * 3, i); int x = lx + (int)(r % 13u), y = ly + (int)((r >> 8) % 13u), w = 1 + (int)((r >> 16) % 3u);
        gBox(x, y, MIN(lx + 12, x + w), MIN(ly + 12, y + w), (r >> 20) & 1 ? FL_SOIL : FL_ROCK); }
    gFree(lx + 6, ly + 11, '8');
    unsigned r = h >> 3;
    if (r & 1) { gFree(lx + 2, ly + 3, '#'); gFree(lx + 3, ly + 3, '#'); }
    if (r & 2) { gFree(lx + 9, ly + 5, 'O'); gFree(lx + 10, ly + 6, 'O'); }
    if (r & 4) gFree(lx + 4, ly + 8, '*');
    if (r & 8) gFree(lx + 11, ly + 1, TREE);
}
static void lot_farm(int lx, int ly, unsigned h) {   // rows of crops, a barn, a fence
    for (int y = ly + 1; y <= ly + 11; y += 2) gBox(lx + 1, y, lx + 6, y, FL_SOIL);
    for (int y = ly + 2; y <= ly + 10; y += 2) for (int x = lx + 1; x <= lx + 6; x += 2) gFree(x, y, (char)0xDB);   // (FARM LIFE: crop beds)
    gRoom(lx + 8, ly + 1, lx + 12, ly + 6, 4, cwp(h >> 9));
    gPut(lx + 10, ly + 6, 'D'); gFree(lx + 9, ly + 2, (char)0xDC); gFree(lx + 9, ly + 4, (char)0xDC); gFree(lx + 11, ly + 2, (char)0xDD); gFree(lx + 11, ly + 4, (char)0xDD); gFree(lx + 11, ly + 3, 'O');
    gLine(lx, ly + 12, lx + 12, ly + 12, 'w', 9); lifeMap[ly + 12][lx + 7] = '.';
    gFree(lx + 9, ly + 9, 'K'); gFree(lx + 11, ly + 10, TREE); gFree(lx + 8, ly + 11, (char)0xDE);
}
// ---- the tall ones: blocks of flats, office towers, hotels (storeys of them, stairs up through each) ----
static void stairs_up(int l, int st, int x, const int *ys, int nys) {   // flights alternate between the spots in ys: storey l's goes up, the one from below arrives
    if (l + 1 < st) sm[ys[l % nys]][x] = '|';
    if (l > 0) sm[ys[(l - 1) % nys]][x] = ' ';
}
static void lot_apart(int lx, int ly, unsigned h, int st) {   // two flats a storey either side of a stair hall (two wide: the stairs take one side); the ground floor's open onto the street
    int x0 = lx, x1 = lx + 12, y0 = ly + 1, y1 = ly + 10, hx = lx + 5;   // the hall: hx (the way through) and hx + 1 (the stairs)
    int flh = cfl(h >> 3), wpo = cwp(h >> 7);
    int ys[2] = { y0 + 3, y0 + 6 };
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, flh, wpo);
        sLine(hx - 1, y0, hx - 1, y1, 'W', wpo); sLine(hx + 2, y0, hx + 2, y1, 'W', wpo);
        for (int y = y0 + 1; y < y1; y++) { sm[y][hx] = '.'; sm[y][hx + 1] = '.'; }
        sm[y0][lx + 2] = 'E'; sm[y0][lx + 10] = 'E'; sm[y0 + 4][x0] = 'E'; sm[y0 + 4][x1] = 'E'; sm[y0][hx] = 'E';
        if (l) { sm[y1][lx + 2] = 'E'; sm[y1][lx + 10] = 'E'; sm[y1][hx] = 'E'; sm[y1][hx + 1] = 'E'; }
        if (l + 1 < st) { sm[ys[l & 1]][hx + 1] = '|'; sm[ys[l & 1] - 1][hx + 1] = 'p'; }   // up: the stairs rise north (a plant blocks the top end down here)
        if (l > 0) sm[ys[(l - 1) & 1]][hx + 1] = ' ';                                       // the stairwell from below
        for (int side = 0; side < 2; side++) {
            int fx0 = side ? hx + 3 : x0 + 1, fx1 = side ? x1 - 1 : hx - 2, fy0 = y0 + 1, fy1 = y1 - 1, u = l * 2 + side;
            unsigned hu = city_h((int)(h & 0xFFFF) + l * 7 + side, l * 3 + side + 11);
            int ffl = cfl(hu >> 9);
            for (int y = fy0; y <= fy1; y++) for (int x = fx0; x <= fx1; x++) sfl[y][x] = (u8)ffl;
            blk_unit = u;
            if (l == 0) {                                  // the street door
                int dx = side ? lx + 10 : lx + 2;
                sm[y1][dx] = 'D'; city_path(dx, y1, ly);
                reg_home(fx0, fy0, fx1, fy1, dx, y1);
                furnish(fx0, fy0, fx1, fy1, dx, y1, occ_of(u), hu * 2654435761u);
            } else {                                       // the door off the hall
                int dy = fy1 - 1;
                sm[dy][side ? hx + 2 : hx - 1] = 'D';
                furnish_at(fx0, fy0, fx1, fy1, side ? fx0 : fx1, dy, side ? 1 : -1, 0, occ_of(u), hu * 2654435761u);
            }
        }
        if (l == 0) { sm[y1][hx] = 'D'; city_path(hx, y1, ly); }
    }
    storey(0); blk_unit = 2 * st; roof_dirty = 1;
    gFree(lx, ly + 12, TREE); gFree(lx + 12, ly + 12, TREE); gFree(lx + 4, ly + 12, 'Y'); gFree(lx + 8, ly + 11, 'Z');
}
static void lot_tower(int lx, int ly, unsigned h, int st) {   // an office tower: a lobby, then floor after floor of desks
    int x0 = lx + 1, x1 = lx + 11, y0 = ly + 1, y1 = ly + 10, sx = x1 - 2;
    int fl = cfl(h >> 3), wp = cwp(h >> 7), fl2 = cfl(h >> 11);
    int ys[2] = { y0 + 2, y0 + 5 };
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, l ? fl2 : fl, wp);
        for (int x = x0 + 1; x < x1; x++) if ((x - x0) & 1) { sm[y0][x] = 'E'; if (l) sm[y1][x] = 'E'; }   // glass all round
        for (int y = y0 + 1; y < y1; y++) if ((y - y0) & 1) { sm[y][x0] = 'E'; sm[y][x1] = 'E'; }
        stairs_up(l, st, sx, ys, 2);
        fz_nres = 0;
        if (l == 0) {                                      // the lobby: the front desk, sofas, the work markers
            int dx = (x0 + x1) / 2; sm[y1][dx] = 'D'; sm[y1][dx - 1] = 'E'; sm[y1][dx + 1] = 'E'; city_path(dx, y1, ly);
            sFree(x0 + 2, y0 + 3, 'y'); sFree(x0 + 3, y0 + 3, 'y'); sFree(x0 + 4, y0 + 3, 'I'); sFree(x0 + 3, y0 + 1, 'b');
            sFree(x0 + 1, y1 - 2, 'C'); sFree(x0 + 1, y1 - 3, 'C'); sFree(x0 + 2, y1 - 1, 'p'); sFree(x1 - 1, y1 - 1, 'p');
            sFree(x0 + 6, y0 + 2, 'x'); sFree(x0 + 6, y0 + 5, 'x'); sFree(x0 + 1, y0 + 1, 'c'); sFree(x0 + 5, y1 - 2, 'r');
        } else {                                           // a floor of desks
            for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
                int x = x0 + 1 + c * 2 + (c > 0), y = y0 + 2 + r * 2;
                if (x >= sx - 1 || y >= y1 - 1) continue;
                sFree(x, y, 'd'); sFree(x, y + 1, 'h'); if ((h >> (r * 3 + c)) & 1) sFree(x + 1, y, '[');
            }
            sFree(x1 - 1, y1 - 1, (l & 1) ? '-' : 'F'); sFree(x0 + 1, y1 - 1, 'p'); sFree(x1 - 1, y0 + 1, (l & 1) ? 'b' : '#'); sFree(x0 + 1, y0 + 1, 'p');
        }
    }
    storey(0); roof_dirty = 1;
    gFree(lx + 1, ly + 12, 'Z'); gFree(lx + 11, ly + 12, 'Z'); gFree(lx + 3, ly + 12, 'N'); gFree(lx + 9, ly + 12, 'Y');
}
static void lot_hotel(int lx, int ly, unsigned h, int st) {   // a hotel: a corridor (two wide) through each floor, rooms either side, the lobby at the front
    int x0 = lx, x1 = lx + 12, y0 = ly, y1 = ly + 12;
    int fl = cfl(h >> 3), fl2 = cfl(h >> 9), wp = cwp(h >> 13);
    int xs[2] = { x0 + 4, x0 + 8 };      // (stairs and their plant stand between the doors' lanes, not in one)
    for (int l = 0; l < st; l++) {
        if (!storey(l)) break;
        sRoom(x0, y0, x1, y1, fl2, wp);
        sLine(x0, y0 + 4, x1, y0 + 4, 'W', wp); sLine(x0, y0 + 7, x1, y0 + 7, 'W', wp);
        for (int x = x0 + 1; x < x1; x++) { sm[y0 + 5][x] = '.'; sm[y0 + 6][x] = '.'; sfl[y0 + 5][x] = (u8)fl; sfl[y0 + 6][x] = (u8)fl; }
        for (int r = 0; r < 3; r++) { int rx = x0 + 1 + r * 4;
            if (r) { sLine(rx - 1, y0, rx - 1, y0 + 4, 'W', wp); sLine(rx - 1, y0 + 7, rx - 1, y1, 'W', wp); }
            sm[y0][rx + 1] = 'E'; if (l) sm[y1][rx + 1] = 'E';
            for (int side = 0; side < 2; side++) {
                int ry0 = side ? y0 + 8 : y0 + 1, ry1 = side ? y1 - 1 : y0 + 3;
                if (l == 0 && side && r == 1) continue;     // the lobby
                sm[side ? y0 + 7 : y0 + 4][rx + 1] = 'D';
                unsigned hr = city_h((int)(h & 0xFFF) + r * 5 + side + l * 11, r);
                int by = side ? ry1 : ry0, fy = side ? ry0 : ry1;
                sFree(rx, by, 'S'); sFree(rx + 2, by, (hr & 1) ? 'i' : 'l'); sFree(rx + 2, fy, (hr & 2) ? 'v' : 'p');
                if (side && (hr & 4)) { sFree(rx, fy, 'T'); }
            }
        }
        if (l + 1 < st) { int x = xs[l & 1]; sm[y0 + 6][x] = '|'; sm[y0 + 6][x - 1] = 'p'; }   // up: the stairs rise west (a plant blocks the top end down here)
        if (l > 0) sm[y0 + 6][xs[(l - 1) & 1]] = ' ';
        if (l == 0) {                                      // the lobby, through the front door
            int dx = x0 + 6;
            sm[y0 + 7][dx] = 'D'; sm[y1][dx] = 'D'; sm[y1][dx - 1] = 'E'; sm[y1][dx + 1] = 'E';
            for (int y = y0 + 8; y < y1; y++) for (int x = x0 + 5; x <= x0 + 7; x++) sfl[y][x] = (u8)fl;
            sFree(x0 + 5, y0 + 8, 'y'); sFree(x0 + 7, y0 + 8, 'I'); sFree(x0 + 5, y1 - 1, 'p'); sFree(x0 + 7, y1 - 1, ';');
        }
    }
    storey(0); roof_dirty = 1;
}
