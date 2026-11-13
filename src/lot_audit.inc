// lot_audit.inc - THE LOT AUDIT. Rules every generated lot must keep, checked on the finished map of a lot (the ground and every storey):
//   overlap       a long thing (a bed, a double bed) covers a tile with a wall or another thing on it
//   door          a door with a blocked tile in front of it or behind it (a thing in the doorway, a wall across it)
//   lane          the second tile in from a door is blocked too (nowhere to step round)
//   unreach_floor a floor tile (indoors or in a yard) you cannot get to from the street (walled in by furniture, walls, a fence with no gate)
//   unreach_item  a thing with no walkable tile beside it that you can get to (nobody can use it)
//   floating      furniture indoors that touches no wall and is not part of a cluster that does (or a dining / counter / pool-table set),
//                 (a seat with its rug beside it is a living group: it may stand out in the room, facing the telly), or a rug with nothing within two tiles
//   loud          a home with a factory floor (concrete, steel, grate, hazard stripes, oil) under it
//   nobed / nowc / nokit   a home without a bed, a toilet, or a fridge
//   path          a home's front door the street cannot reach
//   offplot       a tile outside the 13 x 13 lot that a generator changed (things, walls, floors)
// Used by the host simulator (ports/host/lotsim) and by the game's own test builds (-DLOT_AUDIT=1 in world.c). It needs lifeMap, floorMap, lv_get() and the homes list.
typedef struct { long lots, items, homes, overlap, door, lane, unreach_floor, unreach_item, floating, loud, nobed, nowc, nokit, path, offplot, floors_sum; } LotAud;
static LotAud lot_aud, lot_aud_k[40];
static unsigned char au_in[MH][MW], au_reach[MH][MW], au_gr[MH][MW];   // indoors (walled off from the lot's edge), reached from the street

static int au_isw(char c) { return c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f'; }
static int au_free(char c, int f) { return f != FL_WATER && (c == '.' || c == 'D' || c == '|' || (items_is_item(c) && !items_blocking(c))); }
static int au_furn(char c) { return c != '.' && c != ' ' && c != 0 && !au_isw(c) && c != 'D' && c != 'P' && items_is_item(c) && items_blocking(c); }
static int au_flat(char c) { return c != '.' && c != ' ' && items_is_item(c) && !items_blocking(c) && c != 'D' && c != 'P' && c != 'x' && c != 'B'; }
static int au_table(char c) { return c == 't' || c == (char)0x95 || c == (char)0x9A || c == (char)0xA1 || c == (char)0xA4 || c == 'y' || c == 'd'; }   // (a desk with its chair is a table set: a workstation can stand in the room)
static void au_foot_tile(char c, int x, int y, int r, int da, int db, int *tx, int *ty) { (void)c;
    *tx = x + (r == 0 ? da : r == 1 ? db : r == 2 ? -da : -db); *ty = y + (r == 0 ? db : r == 1 ? -da : r == 2 ? -db : da); }
static int au_facing(char (*m)[MW + 1], u8 (*wp)[MW], int x, int y) {   // as world_item_facing: a turn the player chose, else away from the wall (towards the open side)
    static const signed char dx[4] = { 0, 1, 0, -1 }, dy[4] = { 1, 0, -1, 0 };
    if (items_rotates(m[y][x]) && (wp[y][x] & 0x80)) return wp[y][x] & 3;
    for (int d = 0; d < 4; d++) { int a = x + dx[d], b = y + dy[d], a2 = x - dx[d], b2 = y - dy[d];
        int o1 = a >= 0 && b >= 0 && a < MW && b < MH && (m[b][a] == '.' || m[b][a] == 'D'), o2 = a2 >= 0 && b2 >= 0 && a2 < MW && b2 < MH && (m[b2][a2] == '.' || m[b2][a2] == 'D');
        if (o1 && !o2) return d; }
    for (int d = 0; d < 4; d++) { int a = x + dx[d], b = y + dy[d]; if (a >= 0 && b >= 0 && a < MW && b < MH && (m[b][a] == '.' || m[b][a] == 'D')) return d; }
    return 0;
}
// one storey of the lot (l 0: the ground), the rectangle x0..x1 x y0..y1; below: the storey under it (for where its stairs come up)
static void au_level(int l, char (*m)[MW + 1], u8 (*fl)[MW], u8 (*wp)[MW], char (*below)[MW + 1], int x0, int y0, int x1, int y1, int kind) {
    LotAud *K = &lot_aud_k[kind];
    // 1. indoors: the tiles a flood from the lot's edge cannot reach without crossing a wall, window or door
    static short q[MW * MH]; int h = 0, t = 0;
    for (int y = y0 - 1; y <= y1 + 1; y++) for (int x = x0 - 1; x <= x1 + 1; x++) au_in[y][x] = 1;
    for (int y = y0 - 1; y <= y1 + 1; y++) for (int x = x0 - 1; x <= x1 + 1; x++) if ((y == y0 - 1 || y == y1 + 1 || x == x0 - 1 || x == x1 + 1)) { au_in[y][x] = 0; q[t++] = (short)(y * MW + x); }
    while (h < t) { int p = q[h++], x = p % MW, y = p / MW;
        static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k]; if (a < x0 - 1 || a > x1 + 1 || b < y0 - 1 || b > y1 + 1 || !au_in[b][a]) continue;
            char c = m[b][a]; if (au_isw(c) || c == 'D') continue; au_in[b][a] = 0; q[t++] = (short)(b * MW + a); } }
    if (l > 0) for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (m[y][x] == ' ') au_in[y][x] = 0;   // (open air on an upper storey)
    // 2. reach: a flood from the lot's edge (the street), or from beside a stairwell on an upper storey, through everything walkable
    memset(au_reach, 0, sizeof au_reach); h = t = 0;
    if (l == 0) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if ((y == y0 || y == y1 || x == x0 || x == x1) && au_free(m[y][x], fl[y][x]) && !au_in[y][x]) { au_reach[y][x] = 1; q[t++] = (short)(y * MW + x); } }
    else for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (below && below[y][x] == '|') for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
        int a = x + i, b = y + j; if (a >= x0 && a <= x1 && b >= y0 && b <= y1 && au_free(m[b][a], fl[b][a]) && !au_reach[b][a]) { au_reach[b][a] = 1; q[t++] = (short)(b * MW + a); } }
    while (h < t) { int p = q[h++], x = p % MW, y = p / MW;
        static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k]; if (a < x0 || a > x1 || b < y0 || b > y1 || au_reach[b][a] || !au_free(m[b][a], fl[b][a])) continue;
            au_reach[b][a] = 1; q[t++] = (short)(b * MW + a); } }
    static unsigned char cov[MH][MW]; memset(cov, 0, sizeof cov);
    if (l == 0) memcpy(au_gr, au_reach, sizeof au_gr);
    // 3. long things: the tiles they cover must be bare floor, and are not floor you can stand on
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { char c = m[y][x]; int fw, fL; if (!au_furn(c) || !items_foot(c, &fw, &fL)) continue;
        int r = au_facing(m, wp, x, y) & 3;
        for (int da = 0; da < fw; da++) for (int db = 0; db < fL; db++) { if (!(da | db)) continue; int tx, ty; au_foot_tile(c, x, y, r, da, db, &tx, &ty);
            if (tx < 0 || ty < 0 || tx >= MW || ty >= MH || (m[ty][tx] != '.' && !au_flat(m[ty][tx])) || cov[ty][tx]) {   // (a rug may lie under a bed's foot)
                lot_aud.overlap++; K->overlap++; } else cov[ty][tx] = 1; } }
    // 4. things: reached, against a wall or in a cluster that is
    static unsigned char sup[MH][MW], seen[MH][MW]; memset(sup, 0, sizeof sup); memset(seen, 0, sizeof seen);
    h = t = 0;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { char c = m[y][x];
        if (au_furn(c)) { lot_aud.items++; K->items++;
            int ok = 0; static const signed char ox[4] = { 1, -1, 0, 0 }, oy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k]; if (a >= 0 && b >= 0 && a < MW && b < MH && au_reach[b][a] && !cov[b][a]) ok = 1; }
            if (!ok) { int fw, fL; if (items_foot(c, &fw, &fL)) {   // a long thing: it is reached from beside any tile it covers
                int r = au_facing(m, wp, x, y) & 3;
                for (int da = 0; da < fw; da++) for (int db = 0; db < fL; db++) { if (!(da | db)) continue; int tx, ty; au_foot_tile(c, x, y, r, da, db, &tx, &ty);
                    for (int k = 0; k < 4; k++) { int a = tx + ox[k], b = ty + oy[k]; if (a >= 0 && b >= 0 && a < MW && b < MH && au_reach[b][a] && !cov[b][a]) ok = 1; } } } }
            if (!ok) { lot_aud.unreach_item++; K->unreach_item++; }
            if (!au_in[y][x]) continue;                                  // (out of doors: the yard's things are checked by where they are, not by a wall)
            for (int k = 0; k < 4; k++) { int a = x + ox[k], b = y + oy[k]; if (a >= 0 && b >= 0 && a < MW && b < MH && au_isw(m[b][a])) { sup[y][x] = 1; } }
            if (sup[y][x]) q[t++] = (short)(y * MW + x); }
    }
    while (h < t) { int p = q[h++], x = p % MW, y = p / MW;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { int a = x + i, b = y + j; if (a < x0 || a > x1 || b < y0 || b > y1 || sup[b][a] || !au_furn(m[b][a]) || !au_in[b][a]) continue; sup[b][a] = 1; q[t++] = (short)(b * MW + a); } }
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { char c = m[y][x];
        if (au_furn(c) && au_in[y][x] && !sup[y][x] && !seen[y][x]) {      // a cluster nobody supports: a dining set, a pool table or a counter's island is allowed
            int n = 0, tab = 0, chair = 0, same = 0, rug = 0; char c0 = c; h = t = 0; q[t++] = (short)(y * MW + x); seen[y][x] = 1;
            while (h < t) { int p = q[h++], cx = p % MW, cy = p / MW; n++; char cc = m[cy][cx]; if (cc == c0) same++; if (au_table(cc)) tab++; if (cc == 'h' || cc == (char)0x92 || cc == ';' || cc == 'C' || cc == (char)0x90) chair++;
                if (m[cy][cx - 1] == 'r' || m[cy][cx + 1] == 'r' || m[cy - 1][cx] == 'r' || m[cy + 1][cx] == 'r') rug = 1;
                for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) { int a = cx + i, b = cy + j; if (a < x0 || a > x1 || b < y0 || b > y1 || seen[b][a] || sup[b][a] || !au_furn(m[b][a]) || !au_in[b][a]) continue; seen[b][a] = 1; q[t++] = (short)(b * MW + a); } }
            if (!(tab && n >= 2 && chair >= (n >= 3 ? 2 : 1)) && !(tab && n >= 4) && !(n >= 3 && same * 3 >= n * 2) && !(chair && rug)) { lot_aud.floating += n; K->floating += n; }
        }
        if (c == 'r' || c == (char)0xF9) { int near = 0; for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) { int a = x + i, b = y + j; if (a >= 0 && b >= 0 && a < MW && b < MH && au_furn(m[b][a]) && au_in[b][a]) near = 1; }
            if (!near && au_in[y][x]) { lot_aud.floating++; K->floating++; } }
    }
    // 5. doors: the tile in front, the one behind, and the next one in each way
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (m[y][x] == 'D') {
        int hz = au_isw(m[y][x - 1]) || m[y][x - 1] == 'D', vt = au_isw(m[y - 1][x]) || m[y - 1][x] == 'D';
        if (hz == vt) continue;                                          // (a door in a corner or a gap: not a doorway)
        int ax = hz ? 0 : 1, ay = hz ? 1 : 0, bad = 0, bad2 = 0;
        for (int s = -1; s <= 1; s += 2) { int a = x + ax * s, b = y + ay * s; if (a < 0 || b < 0 || a >= MW || b >= MH) continue;
            char c = m[b][a]; if (au_isw(c) || (items_is_item(c) && items_blocking(c) && c != 'D')) bad = 1;
            a += ax * s; b += ay * s; if (a >= 0 && b >= 0 && a < MW && b < MH) { c = m[b][a]; if (au_isw(c) || (items_is_item(c) && items_blocking(c) && c != 'D')) bad2 = 1; } }
        if (bad) { lot_aud.door++; K->door++; } else if (bad2) { lot_aud.lane++; K->lane++; }
    }
    // 6. floor you cannot get to
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (m[y][x] == '.' && !cov[y][x] && fl[y][x] != FL_WATER && !au_reach[y][x] && !(l && !au_in[y][x])) { lot_aud.unreach_floor++; K->unreach_floor++; }
}
// the whole lot at (lx, ly): the ground and every storey, then the homes in it
static void lot_audit(int lx, int ly, int kind) {
    lot_aud.lots++; lot_aud_k[kind].lots++;
    int x0 = lx, y0 = ly, x1 = lx + 12, y1 = ly + 12;
    au_level(0, lifeMap, floorMap, wallMap, 0, x0, y0, x1, y1, kind);
    for (int l = 1; l < NUP; l++) { Level *L = lv_get(l - 1); if (!L) continue; int any = 0;
        for (int y = y0; y <= y1 && !any; y++) for (int x = x0; x <= x1; x++) if (L->map[y][x] != ' ') { any = 1; break; }
        if (!any) continue;
        au_level(l, L->map, L->fl, L->wp, l == 1 ? lifeMap : lv_get(l - 2)->map, x0, y0, x1, y1, kind); }
    // what is outside the plot: a thing, a wall or a floor changed in the ring round it
    for (int y = y0 - 3; y <= y1 + 3; y++) for (int x = x0 - 3; x <= x1 + 3; x++) { if (x >= x0 && x <= x1 && y >= y0 && y <= y1) continue;
        if (lifeMap[y][x] != '.' || floorMap[y][x] != FL_GRASS) { lot_aud.offplot++; lot_aud_k[kind].offplot++; } }
    // the homes of this block: a bed, a toilet, a fridge, no factory floor, a way in from the street
    for (int i = 0; i < nhomes[ZONE_CITY]; i++) { const Home *hm = &homes[ZONE_CITY][i]; if (hm->bx != blk_bx || hm->by != blk_by) continue;
        int bed = 0, wc = 0, kit = 0, loud = 0; unsigned fmask = 0;
        for (int l = 0; l < NUP; l++) { char (*m)[MW + 1] = l ? (lv_get(l - 1) ? lv_get(l - 1)->map : 0) : lifeMap; u8 (*fl)[MW] = l ? (lv_get(l - 1) ? lv_get(l - 1)->fl : 0) : floorMap; if (!m) continue;
            for (int y = hm->y0; y <= hm->y1; y++) for (int x = hm->x0; x <= hm->x1; x++) { char c = m[y][x];
                if (c == 'S' || c == '<' || c == (char)0xEB || c == (char)0xEC || c == (char)0xF6 || c == (char)0xED || c == (char)0x81 || c == (char)0x93) bed++;
                if (c == 'T') wc++; if (c == 'F') kit++;
                if (l && m[y][x] == ' ') continue; if (c == 'W' || c == ' ' || (l && c == 'E')) continue;
                int f = fl[y][x]; fmask |= 1u << (f & 31); if (f == 7 || f == 8 || f == 9 || f == 10 || f == 12) loud = 1; } }
        lot_aud.homes++; lot_aud_k[kind].homes++;
        int vac = occ_blk[hm->unit].vacant;   // (an empty home keeps its boxes and nothing else)
        if (!vac && !bed) { lot_aud.nobed++; lot_aud_k[kind].nobed++; } if (!vac && !wc) { lot_aud.nowc++; lot_aud_k[kind].nowc++; } if (!vac && !kit) { lot_aud.nokit++; lot_aud_k[kind].nokit++; }
        if (loud) { lot_aud.loud++; lot_aud_k[kind].loud++; }
        { int n = 0; for (unsigned v = fmask; v; v >>= 1) n += v & 1; lot_aud.floors_sum += n; }
        if (hm->dy + home_out(hm) >= 0 && hm->dy + home_out(hm) < MH && !au_gr[hm->dy + home_out(hm)][hm->dx]) { lot_aud.path++; lot_aud_k[kind].path++; }
    }
}
#ifndef LOT_AUDIT_NOPRINT
static void audit_line(FILE *f, const char *nm, const LotAud *a) {
    fprintf(f, "%-12s lots %5ld items %6ld | overlap %ld door %ld lane %ld unreach floor %ld item %ld floating %ld | homes %ld loud %ld nobed %ld nowc %ld nokit %ld noway %ld floors/home %.2f | offplot %ld\n", nm, a->lots, a->items, a->overlap, a->door, a->lane, a->unreach_floor, a->unreach_item, a->floating, a->homes, a->loud, a->nobed, a->nowc, a->nokit, a->path, a->homes ? (double)a->floors_sum / a->homes : 0.0, a->offplot);
}
static void audit_report(FILE *f, int one) {
    if (one) { audit_line(f, "this lot", &lot_aud); memset(&lot_aud, 0, sizeof lot_aud); memset(lot_aud_k, 0, sizeof lot_aud_k); return; }
    for (int k = 0; k < 40; k++) if (lot_aud_k[k].lots) audit_line(f, bm_lot_name(k), &lot_aud_k[k]);
    audit_line(f, "ALL", &lot_aud);
}
#endif
