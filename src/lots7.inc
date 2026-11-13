// World version 7: the homes built from the town plan (bm_lot_plan), and the mirror that turns a finished lot to face north.
// A lot is always BUILT as if it looked south (its door on the south wall, its path running down to the lot's last row, 17); a lot that faces north
// is then turned over (mirror7: every row of the lot, ground and storeys, the homes it registered) so its door looks at the block's own street.
// Included after lots.inc (the furnishing and the old builders are used as they are).
static int wp7(const LotPlan *q) { return NWP + NWX + (int)((q->pal * 2u + q->wall) % NWPNEW); }   // the neighbourhood's two wall materials
static void flip7(LotPlan *q) {   // a north-facing plan seen from the south (rows 5..17 turned over)
    u8 a = (u8)(22 - q->y1), b = (u8)(22 - q->y0); q->y0 = a; q->y1 = b; q->dy = (u8)(22 - q->dy);
    if (q->wx1) { a = (u8)(22 - q->wy1); b = (u8)(22 - q->wy0); q->wy0 = a; q->wy1 = b; }
    q->face = LF_SOUTH;
}
static void win7(int hx, int hy, int hx1, int hy1, int dx, int sideL, int sideR) {   // the windows: along the back, down the sides, either side of the door
    gPut(hx + 2, hy, 'E'); if (hx1 - 2 > hx + 2) gPut(hx1 - 2, hy, 'E');
    if (sideL) gPut(hx, hy + (hy1 - hy) / 2, 'E'); if (sideR) gPut(hx1, hy + (hy1 - hy) / 2, 'E');
    if (dx - 2 > hx + 1) gPut(dx - 2, hy1, 'E'); if (dx + 2 < hx1 - 1) gPut(dx + 2, hy1, 'E');
}
__attribute__((noinline)) static void yard7(int ox, int oy, const LotPlan *q, int hy, int hy1, int dx, int vacant, unsigned h, int kind) {   // the front: porch, path, planters, lamp, mailbox, hedges; the drive
    PLx = ox + BM_LOT0; PLy = oy + BM_LOT0;
    int n = oy + 16 - hy1, g = (int)(h >> 13), ry = oy + BM_BLK - 1;                   // rows between the front wall and the last row (n >= 0)
    int hedge = kind == NK_SUBURB || kind == NK_ESTATE || kind == NK_GREEN, dv = q->drv ? ox + q->drv : 0;
    if (dv) {                                                                          // the drive: two tiles of concrete, to the garage (the wing's front, the front wall) or back beside the house
        int top = q->wx1 ? oy + q->wy1 + 1 : q->type == HT_RANCH ? hy1 + 1 : hy;
        for (int y = top; y <= ry; y++) { floorMap[y][dv] = FL_PAVING; floorMap[y][dv + 1] = FL_PAVING; }
        if (q->type == HT_RANCH) { wallMap[hy1][dv] = (u8)cwp(h >> 3); wallMap[hy1][dv + 1] = (u8)cwp(h >> 3); }   // (the garage door in the front wall)
    }
    if (dx - 1 >= PLx && n >= 1) { for (int x = dx - 1; x <= dx + 1; x++) floorMap[hy1 + 1][x] = FL_PATIO; }   // the porch
    for (int y = hy1 + 1; y <= ry; y++) floorMap[y][dx] = (u8)(((g >> 5) & 3) == 0 ? FL_GRAVEL : ((g >> 5) & 3) == 1 ? FL_PATIO : FL_PAVING);   // the path
    if (vacant) { if (n >= 2) pf(dx + 2, hy1 + 2, '8'); return; }
    if (n >= 1) { pf(dx + 1, hy1 + 1, 'l'); pf(dx - 2, hy1 + 1, 'Z'); pf(dx + 2, hy1 + 1, 'Z'); }
    if (n >= 3) { char c = (g & 1) ? (char)0xE2 : 'p'; if (dx - 4 > PLx + 2) pf(dx - 4, hy1 + 1, c); if (dx + 4 < PLx + 10) pf(dx + 4, hy1 + 1, c); }
    if (n >= 3) {
        if (!hedge || (g & 2)) pf(dx - 2, oy + 16, (char)0xE0);                        // the mailbox at the gap
        else pf(dx + 2, oy + 16, (char)0xE0);
        if (hedge) for (int x = PLx + 1; x <= PLx + 12; x++) if (ABS(x - dx) > 1 && (!dv || x < dv || x > dv + 1)) pf(x, oy + 16, (char)0xE1);   // a low hedge across the front, a gap at the walk and the drive
    }
    if (hedge) for (int s = 0; s < 2; s++) {                                           // hedges down the sides, where there is room round the house
        int c = s ? PLx + 11 : PLx + 1;
        if (dv && (c == dv || c == dv + 1)) continue;
        for (int y = PLy + 1; y <= oy + 15; y++) pf(c, y, (char)0xE1);
    }
    if (n >= 2 && kind != NK_CORE && kind != NK_MAIN) { int e = dv ? (dv - ox < 9) : (dx - ox < 11); pf(e ? ox + 16 : ox + 7, oy + 15, TREE); }   // a yard tree on the side away from the drive
}
static void garage7(int ox, int oy, const LotPlan *q, int fl, int wp) {   // the garage wing (L houses): a solid block with its door to the drive
    (void)fl; int x0 = ox + q->wx0, x1 = ox + q->wx1, y0 = oy + q->wy0, y1 = oy + q->wy1;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { lifeMap[y][x] = 'W'; wallMap[y][x] = (u8)wp; floorMap[y][x] = FL_PAVING; }
}
__attribute__((noinline)) static void house7(int lx, int ly, unsigned h, const BInfo *b, const LotPlan *p0) {
    LotPlan q = *p0; if (q.face == LF_NORTH) flip7(&q);
    int ox = lx - BM_LOT0, oy = ly - BM_LOT0, ty = q.type, kind = bm_nb(blk_bx, blk_by);
    int hx = ox + q.x0, hy = oy + q.y0, hx1 = ox + q.x1, hy1 = oy + q.y1, w = hx1 - hx + 1, hh = hy1 - hy + 1, dx = ox + q.dx;
    int fl = cfl(h >> 9), wp = wp7(&q); (void)b;
    PLx = lx; PLy = ly;
    if (ty == HT_TERRACE) {                                                           // two homes sharing a wall, each with its own door
        for (int u = 0; u < 2; u++) {
            unsigned hu = city_h((int)(h & 0xFFFF) + u * 31, u + 9);
            int ax = u ? ox + q.wx0 : hx, ax1 = u ? ox + q.wx1 : hx1, ddx = u ? ox + q.d2x : dx;
            gRoom(ax, hy, ax1, hy1, cfl(hu >> 9), wp);
            gPut(ddx, hy1, 'D'); win7(ax, hy, ax1, hy1, ddx, u == 0, u == 1);
            int k = reg_home(ax + 1, hy + 1, ax1 - 1, hy1 - 1, ddx, hy1);
            furnish(ax + 1, hy + 1, ax1 - 1, hy1 - 1, ddx, hy1, occ_of(k), hu * 2654435761u);
            for (int y = hy1 + 1; y <= oy + BM_BLK - 1; y++) floorMap[y][ddx] = FL_PAVING;
            for (int x = ddx - 1; x <= ddx + 1; x++) floorMap[hy1 + 1][x] = FL_PATIO;
            gFree(ddx - 2, hy1 + 1, 'Z');
        }
        pf(hx + 3, ly + 1, TREE); pf(ox + q.wx1 - 3, ly + 1, TREE);
        return;
    }
    gRoom(hx, hy, hx1, hy1, fl, wp);
    gPut(dx, hy1, 'D'); win7(hx, hy, hx1, hy1, dx, 1, 1);
    int ix0 = hx + 1, iy0 = hy + 1, ix1 = hx1 - 1, iy1 = hy1 - 1, u = reg_home(ix0, iy0, ix1, iy1, dx, hy1);
    const Occ *o = occ_of(u);
    if (ty == HT_COMPACT) {                                                            // the cottage: one room, the bathroom walled off in a back corner
        furnish(ix0, iy0, ix1, iy1, dx, hy1, o, h * 2246822519u);
    } else if (ty == HT_TWO) {                                                         // two storeys: the household downstairs, a loft up the stairs
        int sx = ix0 + ((h >> 21) & 1 ? 5 : 1), sy = iy0 + 2, wp2 = cwp(h >> 3), fl2 = cfl(h >> 17);
        lifeMap[sy][sx] = '|';
        fz_nres = 2; fz_res[0][0] = (u8)sx; fz_res[0][1] = (u8)(sy - 1); fz_res[1][0] = (u8)sx; fz_res[1][1] = (u8)(sy + 1);
        Occ down = *o; if (!o->vacant && (o->kids || o->teens)) { down.kids = 0; down.teens = 0; }
        furnish(ix0, iy0, ix1, iy1, dx, hy1, &down, h * 2654435761u);
        Level *U = lv_need(0);
        if (U) {
            for (int j = hy; j <= hy1; j++) for (int i = hx; i <= hx1; i++) { int edge = (i == hx || i == hx1 || j == hy || j == hy1); U->map[j][i] = edge ? 'W' : '.'; U->fl[j][i] = (u8)fl2; U->wp[j][i] = (u8)wp2; }
            U->map[hy][hx + 2] = 'E'; U->map[hy][hx1 - 2] = 'E'; U->map[hy + 3][hx] = 'E'; U->map[hy + 3][hx1] = 'E'; U->map[hy1][hx + 2] = 'E'; U->map[hy1][hx1 - 2] = 'E';
            U->map[sy][sx] = ' ';
            if (!o->vacant && storey(1)) {
                Occ up = *o; up.adults = up.elders = up.babies = up.couple = 0; Beds ub = beds_of(&up);
                fz_nres = 1; fz_res[0][0] = (u8)sx; fz_res[0][1] = (u8)(sy + 1);
                fz_frame(ix0, iy0, ix1, iy1, sx, sy - 1, sx > ix0 + 3 ? -1 : 1, 0, h * 40503u);
                fz_room(R_BED | R_STUDY, &ub, o); fz_reach(); fz_nres = 0; storey(0);
            }
        }
        fz_nres = 0; roof_dirty = 1;
    } else {                                                                           // a ranch, an L, a box, a mansion: the front room and the back rooms
        int grand = ty == HT_MANSION, ns = grand ? 2 : 1;
        fz_geo = h * 2246822519u + 7u;
        int wy_ = iy0 + (iy1 - iy0 + 1 >= 11 ? 5 : (iy1 - iy0 + 1 >= 9 ? 4 : 3)), sty = wy_ + 2, stA = ix0 + 1, stB = ix1 - 1;
        if (ns > 1) {
            int k_, wy2, cu[3], r0[3], r1[3], dd[3]; stA = stB = -1;
            if (fp_geo(ix0, iy0, ix1, iy1, &k_, &wy2, cu, r0, r1, dd)) for (int x = ix0 + 1; x <= ix1 - 1; x++) { int ok = ABS(x - dx) >= 2; for (int i = 0; i < k_; i++) if (ABS(x - dd[i]) < 2) ok = 0;
                if (ok) { if (stA < 0) stA = x; stB = x; } }
            if (stA < 0 || stA == stB) ns = 1;
        }
        if (ns > 1) { fz_upx = stA; fz_upy = sty; fz_holex = -1; }
        furnish_plan(ix0, iy0, ix1, iy1, dx, o, h * 2654435761u, grand);
        fz_upx = fz_holex = -1;
        for (int l = 1; l < ns; l++) {
            if (!storey(l)) break;
            sRoom(hx, hy, hx1, hy1, cfl(city_h((int)h + l, 5)), wp);
            for (int x = hx + 2; x < hx1 - 1; x += 3) { sm[hy][x] = 'E'; sm[hy1][x] = 'E'; }
            for (int y = hy + 2; y < hy1 - 1; y += 3) { sm[y][hx] = 'E'; sm[y][hx1] = 'E'; }
            fz_holex = (l & 1) ? stA : stB; fz_holey = sty;
            if (l + 1 < ns) { fz_upx = (l & 1) ? stB : stA; fz_upy = sty; } else fz_upx = -1;
            fz_upper = 1;
            furnish_plan(ix0, iy0, ix1, iy1, hx + w / 2, &occ_any, (h + (unsigned)l * 977u) * 2654435761u, 1);
            fz_upper = 0; fz_upx = fz_holex = -1;
        }
        storey(0); roof_dirty = 1;
        if (grand) { for (int x = hx + 2; x < hx1 - 1; x += 3) { gPut(x, hy, 'E'); if (x != dx && ABS(x - dx) > 1) gPut(x, hy1, 'E'); } for (int y = hy + 2; y < hy1 - 1; y += 3) { gPut(hx, y, 'E'); gPut(hx1, y, 'E'); } }
        if (q.wx1) garage7(ox, oy, &q, fl, wp);
    }
    yard7(ox, oy, &q, hy, hy1, dx, o->vacant, h >> 3, kind);
    if (ty == HT_MANSION) { pf(hx + 1, ly + 1, TREE); pf(hx1 - 1, ly + 1, TREE); pf(ox + 7, oy + 8, TREE); pf(ox + 16, oy + 8, TREE); }
    else {
        if (hy - ly - 1 >= 4 && !o->vacant) {                                              // a shed in the back corner away from the drive (a solid little block)
            int sx = q.drv && q.drv < 9 ? ox + 15 : ox + 7;
            int ok = 1; for (int y = ly + 1; y <= ly + 2; y++) for (int x = sx; x <= sx + 1; x++) if (lifeMap[y][x] != '.') ok = 0;
            if (ok) for (int y = ly + 1; y <= ly + 2; y++) for (int x = sx; x <= sx + 1; x++) { lifeMap[y][x] = 'W'; wallMap[y][x] = (u8)cwp(h >> 5); }
        }
        backyard(lx, ly, hx, hy, w, hy1, h * 2246822519u);
    }
}
__attribute__((noinline)) static void mirror7(int ox, int oy, int n0, int ground) {   // a finished lot (rows 5..17, columns 5..17 of the block) turned over to face north
    int x0 = ox + BM_LOT0, x1 = ox + BM_BLK - 1, r5 = oy + BM_LOT0, r17 = oy + BM_BLK - 1, nw = x1 - x0 + 1;
    static char sl[13]; static u8 sf[13], sw[13];                                          // (the street row's tiles kept as they were)
    for (int i = 0; i < nw; i++) { sl[i] = lifeMap[r5][x0 + i]; sf[i] = floorMap[r5][x0 + i]; sw[i] = wallMap[r5][x0 + i]; }
    for (int a = r5, c = r17; a < c; a++, c--) {
        for (int x = x0; x <= x1; x++) {
            char t = lifeMap[a][x]; lifeMap[a][x] = lifeMap[c][x]; lifeMap[c][x] = t;
            u8 u = floorMap[a][x]; floorMap[a][x] = floorMap[c][x]; floorMap[c][x] = u;
            u = wallMap[a][x]; wallMap[a][x] = wallMap[c][x]; wallMap[c][x] = u;
        }
        for (int k = 0; k < NUP; k++) { Level *L = lv_get(k); if (!L) continue;
            for (int x = x0; x <= x1; x++) { char t = L->map[a][x]; L->map[a][x] = L->map[c][x]; L->map[c][x] = t; u8 u = L->fl[a][x]; L->fl[a][x] = L->fl[c][x]; L->fl[c][x] = u; u = L->wp[a][x]; L->wp[a][x] = L->wp[c][x]; L->wp[c][x] = u; } }
    }
    for (int y = r5; y <= r17; y++) for (int x = x0; x <= x1; x++) { u8 w = wallMap[y][x]; if ((w & 0x80) && !(w & 1)) wallMap[y][x] = (u8)(w ^ 2); }   // (an item turned to the north looks south now, and back)
    for (int i = 0; i < nw; i++) {                                                          // the street row as it was; the far row is the lot's back: ground (what the lot put there stays)
        lifeMap[r5][x0 + i] = sl[i]; floorMap[r5][x0 + i] = sf[i]; wallMap[r5][x0 + i] = sw[i];
        if (floorMap[r17][x0 + i] == FL_PAVING && i) floorMap[r17][x0 + i] = (u8)ground;
    }
    int z = world_zone % ZONE_N, Y = 2 * oy + BM_LOT0 + BM_BLK - 1;
    for (int i = n0; i < nhomes[z]; i++) { Home *hm = &homes[z][i]; u8 a = (u8)(Y - hm->y1), c = (u8)(Y - hm->y0); hm->y0 = a; hm->y1 = c; hm->dy = (u8)(Y - hm->dy); hm->fn = 1; }
}
static void trees7(int ox, int oy, int rd) {   // the street's trees at an even spacing, one every four tiles along the pavement, where there is open grass to plant in
    for (int i = 0; i < 3; i++) {
        if (rd & 1) { int x = ox + 8 + 4 * i, y = oy + BM_LOT0 + 1; if (lifeMap[y][x] == '.' && floorMap[y][x] == FL_GRASS && lifeMap[y + 1][x] == '.') lifeMap[y][x] = TREE; }
        if (rd & 2) { int x = ox + BM_LOT0 + 1, y = oy + 8 + 4 * i; if (lifeMap[y][x] == '.' && floorMap[y][x] == FL_GRASS && lifeMap[y][x + 1] == '.') lifeMap[y][x] = TREE; }
    }
}
