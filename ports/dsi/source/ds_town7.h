// ds_town7.h -- (D79) WORLD VERSION 7 ON THE DS: homes built from the town plan, turned over for the lots that face north. See docs/DS_TOWN7.md.
// The plan (src/bigmap.c: bm_lot_plan, LotPlan) is in block tiles: a lot is tiles 5..17 of its block both ways. The DS plot is 13 x 13 cells and one cell is one
// tile, so plot cell c is tile c + 5 and a body of w x d tiles is a house of w x d cells (the DS's walls sit on cell edges, so its rooms are roomier than the PS1's).
// A home is built as if it looked south (door on the front wall, the front yard below it), then the whole plot is turned over (flip7) if its plan faces north,
// the way the PS1 builds south and mirrors (src/lots7.inc). The rooms follow docs/HOUSING_DESIGN.md 4b/4c: groups, not rows (a living group round the telly,
// a dining set, a kitchen run with its clear tile, a bed with its nightstands, a bath along a wall), the tiles inside every door kept clear.
static u8 g7o[GN][GN];                                    // cells taken (1) or kept clear (2) while a home is laid out
static u8 g7f[GN][GN];                                    // cells with a flat thing (a rug) on them: other things may stand on it
static u8 g7u[GN][GN];                                    // the same for the loft (the storey above)
static int h7_x0, h7_x1, h7_y0, h7_mir;                   // the home being laid out: its first and last column, its back row, mirrored (the plan's mirror: garage side west)
static int h7_n0, h7_rw, h7_rd, h7_rdc, h7_lv;            // the home being furnished: its first thing, its width and depth, the front door's column, the storey (0 ground, 1 the loft)
static u32 h7_rs;                                         // the home's dice (xorshift, seeded from the lot)
static u8 h7_bd[10]; static int h7_nbd, h7_bi;            // the household's beds in order of need (item ids), and how many have a room already
static int h7x(int c) { return h7_mir ? h7_x1 - c : h7_x0 + c; }
static int h7_rnd(int n) { h7_rs ^= h7_rs << 13; h7_rs ^= h7_rs >> 17; h7_rs ^= h7_rs << 5; return (int)((h7_rs >> 8) % (u32)n); }
static void h7_abs(int x, int y, int id, int rot) {       // a thing on a cell (yards and loose things; the rooms use h7_pl)
    if (id < 0 || x < 0 || y < 0 || x >= GN || y >= GN || nobj >= 400) return;
    if (!(rot & 12)) { if (g7o[y][x]) return; g7o[y][x] = 1; }
    objs[nobj].x = (u8)x; objs[nobj].y = (u8)y; objs[nobj].c = (u8)(id >> 2); objs[nobj].i = (u8)(id & 3); objs[nobj].rot = (u8)rot; nobj++; occ_ok = 0; }
static void h7_res(int c, int r) { int x = h7x(c), y = h7_y0 + r; if (x >= 0 && y >= 0 && x < GN && y < GN && !g7o[y][x]) g7o[y][x] = 2; }
static void h7_vw(int r, int v, int t) { int x = h7_mir ? h7_x1 + 1 - v : h7_x0 + v; if (h7_y0 + r >= 0 && h7_y0 + r < GN && x >= 0 && x <= GN) vwl[h7_y0 + r][x] = (u8)t; }
static void h7_hw(int l, int c, int t) { int x = h7x(c); if (h7_y0 + l >= 0 && h7_y0 + l <= GN && x >= 0 && x < GN) hwl[h7_y0 + l][x] = (u8)t; }
static void h7_fl(int c0, int r0, int c1, int r1, int t) { for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) { int x = h7x(c), y = h7_y0 + r; if (x >= 0 && x < GN && y >= 0 && y < GN) flr[y][x] = (u8)t; } }
static void h7_shell(int w, int d, int ow, int fl) {      // the outer walls (on the cell edges round the w x d cells) and one floor
    h7_fl(0, 0, w - 1, d - 1, fl);
    for (int c = 0; c < w; c++) { h7_hw(0, c, ow); h7_hw(d, c, ow); }
    for (int r = 0; r < d; r++) { h7_vw(r, 0, ow); h7_vw(r, w, ow); } }
static int h7_let(char ch) { return PS1ID[(u8)ch] - 1; }

// ---- (D80) the room engine. Things are put by what they are (a bed, a sofa) and how big the item table says they are (1 x 1, 2 x 1, 2 x 2: the DS item table is the PS1's, sizes included),
// against a wall of a room, turned the way they are used. Every put is checked: the free cells of the home must all still be reachable from the front door and every thing must keep a free side
// to be got at from; a put that breaks that is taken back (so a house is never shut in, whatever sizes the items have).
typedef struct { int c0, r0, c1, r1; } R7;                // a room: local cells, first and last column and row
static int h7_step(int x, int y, int nx, int ny) { int w = nx > x ? vwl[y][nx] : nx < x ? vwl[y][x] : ny > y ? hwl[ny][x] : hwl[y][x]; return w == 0 || w == 5; }
static int h7_cfree(int x, int y) { return x >= 0 && y >= 0 && x < GN && y < GN && flr[y][x] >= 1 && flr[y][x] <= 4 && g7o[y][x] != 1; }
static int h7_ok(void) {                                  // is the home still all reachable, with a free side for every thing?
    static u8 seen[GN][GN]; static u8 qx[GN * GN], qy[GN * GN]; static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    int xa = h7x(0), xb = h7x(h7_rw - 1), ya = h7_y0, yb = h7_y0 + h7_rd - 1; if (xa > xb) { int t = xa; xa = xb; xb = t; }
    if (xa < 0) xa = 0; if (xb > GN - 1) xb = GN - 1; if (yb > GN - 1) yb = GN - 1;
    memset(seen, 0, sizeof seen);
    int sx = h7x(h7_rdc), sy = yb; if (!h7_cfree(sx, sy)) return 0;
    int h = 0, t = 0, tot = 0, got = 0; qx[t] = (u8)sx; qy[t++] = (u8)sy; seen[sy][sx] = 1;
    while (h < t) { int x = qx[h], y = qy[h++]; got++; for (int k = 0; k < 4; k++) { int nx = x + DX[k], ny = y + DY[k]; if (nx < xa || nx > xb || ny < ya || ny > yb || seen[ny][nx] || !h7_cfree(nx, ny) || !h7_step(x, y, nx, ny)) continue; seen[ny][nx] = 1; qx[t] = (u8)nx; qy[t++] = (u8)ny; } }
    for (int y = ya; y <= yb; y++) for (int x = xa; x <= xb; x++) if (h7_cfree(x, y)) tot++;
    if (got != tot) return 0;
    for (int i = h7_n0; i < nobj; i++) { const Obj *o = &objs[i]; if ((o->rot >> 2) & 3) continue; const Item *it = IT_OF(o); if (it->kind == 1) continue; int w, hh, ok = 0; obj_fp(o, &w, &hh);
        for (int y = o->y; y < o->y + hh && !ok; y++) for (int x = o->x; x < o->x + w && !ok; x++) for (int k = 0; k < 4; k++) { int nx = x + DX[k], ny = y + DY[k]; if (nx >= o->x && nx < o->x + w && ny >= o->y && ny < o->y + hh) continue; if (nx < xa || nx > xb || ny < ya || ny > yb || !seen[ny][nx] || !h7_step(x, y, nx, ny)) continue; ok = 1; break; }
        if (!ok) return 0; }
    return 1; }
static void h7_pop(void) {                                // take the last thing back
    if (nobj <= 0) return; Obj *o = &objs[--nobj]; int w, h; obj_fp(o, &w, &h); int lv = (o->rot >> 2) & 3, flat = IT_OF(o)->kind == 1;
    for (int y = o->y; y < o->y + h; y++) for (int x = o->x; x < o->x + w; x++) { if (lv) g7u[y][x] = 0; else if (flat) g7f[y][x] = 0; else g7o[y][x] = 0; } occ_ok = 0; }
static void h7_undo(int n) { while (nobj > n) h7_pop(); }
static int h7_pl(int id, int c, int r, int rot) {         // a thing with its top-left cell at local (c, r), turned rot; 1 if it went in
    if (id < 0 || id >= NITEM || nobj >= 400) return 0;
    const Item *it = &ITEMS[id]; int w = (rot & 1) ? it->fl : it->fw, h = (rot & 1) ? it->fw : it->fl, flat = it->kind == 1;
    int x0 = h7_mir ? h7_x1 - (c + w - 1) : h7_x0 + c, y0 = h7_y0 + r, ar = (h7_mir && (rot & 1) ? rot ^ 2 : rot) | (h7_lv ? 4 : 0);
    for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) {
        if (x < 0 || y < 0 || x >= GN || y >= GN) return 0;
        if (h7_lv) { if (!uflr[0][y][x] || g7u[y][x]) return 0; } else { if (flr[y][x] < 1 || flr[y][x] > 4 || g7o[y][x] || (flat && g7f[y][x])) return 0; } }
    for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) { if (h7_lv) g7u[y][x] = 1; else if (flat) g7f[y][x] = 1; else g7o[y][x] = 1; }
    objs[nobj].x = (u8)x0; objs[nobj].y = (u8)y0; objs[nobj].c = (u8)(id >> 2); objs[nobj].i = (u8)(id & 3); objs[nobj].rot = (u8)ar; nobj++; occ_ok = 0;
    if (!h7_lv && !h7_ok()) { h7_pop(); return 0; }
    return 1; }
// a piece against wall s of room R (0 the back wall, 1 east, 2 the front, 3 west): a cells along it (from its west or north end), b out into the room; f its facing: 0 into the room, 1 along (+a), 2 at the wall, 3 along (-a)
static int h7_rot(int s, int f) { static const int IN[4] = { 0, 3, 2, 1 }, AL[4] = { 1, 0, 1, 0 }; return f == 0 ? IN[s] : f == 2 ? IN[s] ^ 2 : f == 1 ? AL[s] : AL[s] ^ 2; }
static int h7_len(const R7 *R, int s) { return (s & 1) ? R->r1 - R->r0 + 1 : R->c1 - R->c0 + 1; }
static int h7_dep(const R7 *R, int s) { return (s & 1) ? R->c1 - R->c0 + 1 : R->r1 - R->r0 + 1; }
static void h7_ab(int id, int s, int f, int *A, int *B) { int w, h; const Item *it = &ITEMS[id]; int rot = h7_rot(s, f); if (rot & 1) { w = it->fl; h = it->fw; } else { w = it->fw; h = it->fl; } *A = (s & 1) ? h : w; *B = (s & 1) ? w : h; }
static int h7_f(int id, const R7 *R, int s, int a, int b, int f) {
    if (id < 0 || a < 0 || b < 0) return 0; int A, B; h7_ab(id, s, f, &A, &B); int rot = h7_rot(s, f), ca, ra, cb, rb;
    if (s == 0) { ca = R->c0 + a; ra = R->r0 + b; cb = ca + A - 1; rb = ra + B - 1; }
    else if (s == 2) { ca = R->c0 + a; ra = R->r1 - b; cb = ca + A - 1; rb = ra - (B - 1); }
    else if (s == 3) { ca = R->c0 + b; ra = R->r0 + a; cb = ca + B - 1; rb = ra + A - 1; }
    else { ca = R->c1 - b; ra = R->r0 + a; cb = ca - (B - 1); rb = ra + A - 1; }
    if (cb < R->c0 || cb > R->c1 || rb < R->r0 || rb > R->r1 || ca < R->c0 || ca > R->c1 || ra < R->r0 || ra > R->r1) return 0;
    return h7_pl(id, ca < cb ? ca : cb, ra < rb ? ra : rb, rot); }
static int h7_wallany(int id, const R7 *R, int sides, int f, int corners) {   // somewhere along the walls: the sides in the dice's order, from the end the dice pick; corners 1: only where two walls meet
    int s0 = h7_rnd(4);
    for (int k = 0; k < 4; k++) { int s = (s0 + k) & 3; if (!((sides >> s) & 1)) continue; int len = h7_len(R, s), A, B; h7_ab(id, s, f, &A, &B); int dir = h7_rnd(2);
        for (int i = 0; i + A <= len; i++) { int a = dir ? len - A - i : i; if (corners && a != 0 && a != len - A) continue; if (h7_f(id, R, s, a, 0, f)) return 1; } }
    return 0; }
static int h7_fillw(const R7 *R, int sides, const u8 *ids, int n, int corners) { int k = 0; for (int i = 0; i < n; i++) k += h7_wallany(ids[i], R, sides, 0, corners); return k; }

enum { I_CHAIR = 0, I_ARM = 1, I_SOFA = 2, I_DTAB = 4, I_DESK = 5, I_CTAB = 6, I_CNT = 7, I_TV = 8, I_COMP = 9, I_STEREO = 10, I_FRIDGE = 11, I_LAMP = 12, I_RUG = 13, I_PLANT = 14, I_STATUE = 15, I_BED = 16, I_TUB = 17, I_WC = 18, I_SINK = 19, I_CRIB = 20, I_TOY = 21, I_BOOKS = 22,
       I_STOVE = 28, I_MICRO = 29, I_KSINK = 30, I_MIRROR = 33, I_TOWELS = 35, I_PIANO = 36, I_SHELF = 40, I_AQUA = 51, I_DRESSER = 72, I_FIRE = 73, I_LOVE = 95, I_REC = 96, I_POSTER = 177, I_FAMBED = 178, I_NIGHT = 100, I_WARD = 101,
       I_ISLAND = 104, I_DISHW = 106, I_WASHER = 107, I_CLOCK = 117, I_VASE = 118, I_ART = 120, I_GLOBE = 121, I_CANDLE = 122, I_ARMOR = 123, I_SINGLE = 188, I_STAIRS = 189 };
static void h7_beds(u32 h, int real) {                              // the beds this household needs: the pairs share a family bed, the rest each one of their own, the babies a crib
    int ad = 0, tn = 0, kd = 0, bb = 0;
    if (real && lot_bx == home_bx && lot_by == home_by && nsim > 0) { for (int i = 0; i < nsim && i < SIM_MAX; i++) { int a = sims[i].age; if (a >= 18) ad++; else if (a >= 13) tn++; else if (a >= 3) kd++; else bb++; } }
    else { ad = 1 + (int)(h & 1); kd = (int)((h >> 1) & 1); tn = (int)((h >> 2) & 1) & (int)((h >> 3) & 1); }
    h7_nbd = 0; h7_bi = 0;
    for (int i = 0; i < ad / 2 && h7_nbd < 9; i++) h7_bd[h7_nbd++] = I_FAMBED;
    for (int i = 0; i < (ad & 1) + tn + kd && h7_nbd < 9; i++) h7_bd[h7_nbd++] = I_SINGLE;
    for (int i = 0; i < bb && h7_nbd < 9; i++) h7_bd[h7_nbd++] = I_CRIB;
    if (!h7_nbd) h7_bd[h7_nbd++] = I_SINGLE; }
static void h7_bedroom(const R7 *R, int maxb, int toys, int sm, int guest) {  // a bedroom: the beds' heads on the back wall each between its nightstands, a rug at the foot, a wardrobe, a desk and its computer, a lamp, a plant (sm: the walls other than the back one that are walls)
    int len = h7_len(R, 0), cur = 0, placed = 0, rugc = -1, rugb = 0;
    if (guest && h7_bi >= h7_nbd && h7_nbd < 10) h7_bd[h7_nbd++] = I_SINGLE;   // a guest room
    while (placed < maxb && h7_bi < h7_nbd) {
        int id = h7_bd[h7_bi], ok = 0, A, B;
        for (int tryid = 0; tryid < 2 && !ok; tryid++) { if (tryid) { if (id != I_FAMBED) break; id = I_SINGLE; }
            h7_ab(id, 0, 0, &A, &B); int night = id != I_CRIB;
            for (int a = cur + (night && !placed ? 1 : 0); a + A <= len; a++) { if (!h7_f(id, R, 0, a, 0, 0)) continue;
                if (night && a - 1 >= 0) h7_f(I_NIGHT, R, 0, a - 1, 0, 0);
                if (night && a + A < len) h7_f(I_NIGHT, R, 0, a + A, 0, 0);
                if (rugc < 0 && night) { rugc = a + (A - 1) / 2; rugb = B; }
                cur = a + A + (night ? 1 : 0); ok = 1; break; } }
        if (!ok) break;
        h7_bi++; placed++; }
    if (rugc >= 0) h7_f(I_RUG, R, 0, rugc, rugb, 0);
    while (placed < maxb && h7_bi < h7_nbd) {   // a bed that did not fit on the back wall: on a side wall
        int id = h7_bd[h7_bi]; if (id == I_FAMBED) id = I_SINGLE; if (!h7_wallany(id, R, sm, 0, 0)) break; h7_bi++; placed++; }
    if (!placed) return;
    if (toys) h7_wallany(I_TOY, R, sm, 0, 0);
    h7_wallany(h7_rnd(2) ? I_WARD : I_DRESSER, R, sm, 0, 0);
    { int ok = 0, s0 = h7_rnd(4);   // the desk set on a side wall: the desk, a chair at it, the computer beside
        for (int k = 0; k < 4 && !ok; k++) { int sd = (s0 + k) & 3, len2 = h7_len(R, sd), A, B; if (!((sm >> sd) & 1)) continue; h7_ab(I_DESK, sd, 0, &A, &B);
            for (int a = 0; a + A <= len2 && !ok; a++) { int n0 = nobj; if (!h7_f(I_DESK, R, sd, a, 0, 0)) continue; if (!h7_f(I_CHAIR, R, sd, a, B, 2)) { h7_undo(n0); continue; } if (!h7_f(I_COMP, R, sd, a + A, 0, 0)) h7_f(I_COMP, R, sd, a - 1, 0, 0); ok = 1; } } }
    static const u8 FILL[] = { I_LAMP, I_PLANT, I_SHELF, I_LAMP }; int ar = (R->c1 - R->c0 + 1) * (R->r1 - R->r0 + 1); h7_fillw(R, 15, FILL, 2 + (ar >= 20) + (ar >= 30), 1); }
static void h7_bath(const R7 *R) {                        // a bathroom: the toilet, the bath, a sink and its mirror, a plant where there is room
    h7_wallany(I_WC, R, 15, 0, 0); h7_wallany(I_TUB, R, 15, 0, 0);
    { int n0 = nobj, s0 = h7_rnd(4), ok = 0; for (int k = 0; k < 4 && !ok; k++) { int s = (s0 + k) & 3, len = h7_len(R, s), A, B; h7_ab(I_SINK, s, 0, &A, &B);
            for (int a = 0; a + A <= len && !ok; a++) { n0 = nobj; if (!h7_f(I_SINK, R, s, a, 0, 0)) continue; if (!h7_f(I_MIRROR, R, s, a + A, 0, 0) && !h7_f(I_MIRROR, R, s, a - 1, 0, 0)) h7_f(I_TOWELS, R, s, a + A, 0, 0); ok = 1; } } }
    if ((R->c1 - R->c0 + 1) * (R->r1 - R->r0 + 1) >= 9) h7_wallany(I_TOWELS, R, 15, 0, 1);
    h7_wallany(I_PLANT, R, 15, 0, 1); }
static int h7_living(const R7 *R, int sides, int grand) { // the telly on a wall, a rug, the coffee table, the sofa facing the telly across it, an armchair at a right angle, a lamp; a big room also gets a stereo, a bookshelf, a plant
    int s0 = h7_rnd(4);
    for (int k = 0; k < 4; k++) { int s = (s0 + k) & 3; if (!((sides >> s) & 1)) continue; int dep = h7_dep(R, s), len = h7_len(R, s), tA, tB, sA, sB; if (dep < 3) continue;
        h7_ab(I_TV, s, 0, &tA, &tB); h7_ab(I_SOFA, s, 2, &sA, &sB); int n0 = nobj, at = (len - tA) / 2 + (len - tA > 2 ? h7_rnd(3) - 1 : 0); if (at < 0) at = 0;
        if (!h7_f(I_TV, R, s, at, 0, 0)) { h7_undo(n0); continue; }
        int mid = at + tA / 2, as = mid - sA / 2; if (as < 0) as = 0; if (as + sA > len) as = len - sA; int bs = dep - sB < 3 ? dep - sB : 3; if (bs < 2) bs = 2; if (bs + sB > dep) { h7_undo(n0); continue; }
        if (!h7_f(I_RUG, R, s, as + sA / 2, bs - 1, 0)) h7_f(I_RUG, R, s, as, bs - 1, 0);
        if (!h7_f(I_SOFA, R, s, as, bs, 2)) { h7_undo(n0); continue; }
        h7_f(I_CTAB, R, s, as + sA / 2, bs - 1, 0);
        int arm = 0; if (as - 1 >= 0) arm = h7_f(I_ARM, R, s, as - 1, bs - 1, 1); if (!arm && as + sA < len) arm = h7_f(I_ARM, R, s, as + sA, bs - 1, 3); if (!arm || grand) { if (as + sA < len) h7_f(grand ? I_LOVE : I_ARM, R, s, as + sA, bs - 1, 3); }
        h7_f(I_STEREO, R, s, at + tA, 0, 0) || h7_f(I_STEREO, R, s, at - 1, 0, 0);
        h7_f(I_LAMP, R, s, as + sA, bs, 0) || h7_f(I_LAMP, R, s, as - 1, bs, 0);
        static const u8 X[] = { I_PLANT, I_SHELF, I_LAMP, I_PLANT }; h7_fillw(R, 15 & ~(1 << s), X, grand ? 4 : 2, 1);
        return 1; }
    return 0; }
static int h7_dining(const R7 *R, int chairs) {           // a table with its chairs round it, in the middle of the room if it can; 1 when it got at least three chairs (two as a last resort)
    int cc = (R->c0 + R->c1) / 2, rc = (R->r0 + R->r1) / 2;
    for (int need = chairs; need >= 2; need--) for (int dist = 0; dist < 16; dist++) for (int r = R->r0; r <= R->r1; r++) for (int c = R->c0; c <= R->c1; c++) {
        if (abs(c - cc) + abs(r - rc) != dist) continue; int n0 = nobj; if (!h7_pl(I_DTAB, c, r, 0)) continue; int A = ITEMS[I_DTAB].fw, B = ITEMS[I_DTAB].fl, k = 0;
        k += h7_pl(I_CHAIR, c, r - 1, 0); k += h7_pl(I_CHAIR, c, r + B, 2); k += h7_pl(I_CHAIR, c - 1, r, 1); k += h7_pl(I_CHAIR, c + A, r, 3);
        if (A > 1) { k += h7_pl(I_CHAIR, c + 1, r - 1, 0); } if (B > 1) { k += h7_pl(I_CHAIR, c - 1, r + 1, 1); }
        if (k >= need) return 1; h7_undo(n0); }
    return 0; }
static int h7_kitchen(const R7 *R, int sides, int island) {   // the kitchen run along a wall: the fridge, the stove and the sink in some order, counters in the slack between them (a dishwasher, a microwave in a long one); tried every way round until all three stand in it
    static const u8 ORD[6][3] = { { I_FRIDGE, I_STOVE, I_KSINK }, { I_FRIDGE, I_KSINK, I_STOVE }, { I_STOVE, I_KSINK, I_FRIDGE }, { I_KSINK, I_STOVE, I_FRIDGE }, { I_STOVE, I_FRIDGE, I_KSINK }, { I_KSINK, I_FRIDGE, I_STOVE } };
    int s0 = h7_rnd(4), v0 = h7_rnd(6), d0 = h7_rnd(2);
    for (int k = 0; k < 4; k++) { int s = (s0 + k) & 3; if (!((sides >> s) & 1)) continue; int len = h7_len(R, s);
        for (int vi = 0; vi < 6; vi++) for (int di = 0; di < 2; di++) { int v = (v0 + vi) % 6, dir = d0 ^ di, n0 = nobj, need = 0, A, B;
            for (int i = 0; i < 3; i++) { h7_ab(ORD[v][i], s, 0, &A, &B); need += A; } if (need > len) continue;
            int slack = len - need, id[7], n = 0; id[n++] = ORD[v][0]; if (slack >= 1) { id[n++] = I_CNT; slack--; } id[n++] = ORD[v][1]; id[n++] = ORD[v][2];
            if (slack >= 1) { id[n++] = h7_rnd(2) ? I_DISHW : I_CNT; slack--; } if (slack >= 1) { id[n++] = I_MICRO; slack--; }
            int p = 0, ok = 1; for (int i = 0; i < n && ok; i++) { h7_ab(id[i], s, 0, &A, &B); int a = dir ? len - p - A : p; if (!h7_f(id[i], R, s, a, 0, 0)) { if (id[i] == I_CNT || id[i] == I_DISHW || id[i] == I_MICRO) continue; ok = 0; break; } p += A; }
            if (ok) { if (island && h7_dep(R, s) >= 5) h7_wallany(I_ISLAND, R, 15 & ~(1 << s), 0, 0); return 1; } h7_undo(n0); } }
    return 0; }
// ---- the homes. Local frame: column c from the home's west edge (mirrored if the plan is), row r from its back wall; dc the door's column in the front wall (row d). ----
static void h7_hall(const R7 *R, int n) { static const u8 HL[] = { I_PLANT, I_LAMP, I_SHELF, I_VASE }; h7_fillw(R, 15, HL, n, 1); }
static void kitC(int w, int d, int dc, int ow) {              // a cottage or one home of a terrace: one main room, the bathroom walled off in the back corner, the beds in the alcove beside it
    int last = d - 1, iw = 4; h7_rw = w; h7_rd = d; h7_rdc = dc; h7_n0 = nobj; h7_lv = 0;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, w - 3, 1, 4); h7_fl(w - 2, 0, w - 1, 1, 3); h7_fl(w - 4, 2, w - 1, last, 2);
    for (int c = w - 2; c < w; c++) h7_hw(2, c, iw); for (int r = 0; r < 2; r++) h7_vw(r, w - 2, iw);
    h7_hw(2, w - 2, 5); h7_hw(d, dc, 5);
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(w - 2, 2); h7_res(w - 2, 1);
    R7 bed = { 0, 0, w - 3, 1 }, bath = { w - 2, 0, w - 1, 1 }, kit = { w - 4, 2, w - 1, last }, liv = { 0, 2, w - 5, last };
    h7_bedroom(&bed, 3, 0, 10, 0);
    h7_bath(&bath);
    h7_kitchen(&kit, 2 | 4, 0);
    h7_living(&liv, 8 | 4, 0);
    h7_dining(&kit, 4);
    h7_hall(&liv, 2); }
static void kitT(int w, int d, int dc, int ow) {              // one home of a terrace (6 or 7 wide, 9 deep): the bedroom and the bathroom across the back, the kitchen and dining behind them, the living room at the front
    int bd = 3, wb = w - 3, last = d - 1, iw = 4, ky1 = bd + 2; h7_rw = w; h7_rd = d; h7_rdc = dc; h7_n0 = nobj; h7_lv = 0;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, wb - 1, bd - 1, 4); h7_fl(wb, 0, w - 1, bd - 1, 3); h7_fl(0, bd, w - 1, ky1, 2);
    for (int c = 0; c < w; c++) h7_hw(bd, c, iw);
    for (int r = 0; r < bd; r++) h7_vw(r, wb, iw);
    h7_hw(bd, 1, 5); h7_hw(bd, wb + 1, 5); h7_hw(d, dc, 5);
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(1, bd); h7_res(1, bd - 1); h7_res(wb + 1, bd); h7_res(wb + 1, bd - 1);
    R7 bed = { 0, 0, wb - 1, bd - 1 }, bath = { wb, 0, w - 1, bd - 1 }, kit = { 0, bd, w - 1, ky1 }, liv = { 0, ky1 + 1, w - 1, last };
    h7_bedroom(&bed, 2, 0, 14, 0);
    h7_bath(&bath);
    h7_kitchen(&kit, 2 | 8, 0);
    h7_living(&liv, 2 | 8, 0);
    h7_dining(&kit, 4);
    h7_hall(&liv, 2); }
static void kitR(int w, int d, int dc, int ow) {              // a long ranch (5 or 6 deep): the bedroom at one end, the living group in the middle, the kitchen and dining at the other end, the bathroom in its back corner
    int last = d - 1, iw = 4; h7_rw = w; h7_rd = d; h7_rdc = dc; h7_n0 = nobj; h7_lv = 0;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, 3, last, 4); h7_fl(w - 2, 0, w - 1, 1, 3); h7_fl(w - 4, 2, w - 1, last, 2);
    for (int r = 0; r < d; r++) h7_vw(r, 4, iw);
    for (int c = w - 2; c < w; c++) h7_hw(2, c, iw); for (int r = 0; r < 2; r++) h7_vw(r, w - 2, iw);
    h7_vw(d - 2, 4, 5); h7_hw(2, w - 2, 5); h7_hw(d, dc, 5);   // the bedroom's door, the bathroom's, the front door
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(3, d - 2); h7_res(4, d - 2); h7_res(w - 2, 2); h7_res(w - 2, 1);
    R7 bed = { 0, 0, 3, last }, bath = { w - 2, 0, w - 1, 1 }, kit = { w - 4, 2, w - 1, last }, liv = { 4, 0, w - 5, last };
    h7_bedroom(&bed, 3, 0, 14, 0);
    h7_bath(&bath);
    h7_kitchen(&kit, 2 | 4, 0);
    h7_living(&liv, 1, 0);
    h7_dining(&kit, 4);
    h7_hall(&liv, 2); }
static void kitD(int w, int d, int dc, int ow, int two) {    // a divided house (box, L, mansion, two-storey): bed and bath rooms along the back, living | kitchen and dining along the front, a loft over the back rooms
    int bd = d >= 8 ? 4 : 3, fd = d - bd, mans = w >= 11, wb = mans ? 8 : w >= 9 ? 6 : 5, kc0 = mans ? 7 : w - 4, last = d - 1, iw = 4; (void)fd;
    h7_rw = w; h7_rd = d; h7_rdc = dc; h7_n0 = nobj; h7_lv = 0;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, wb - 1, bd - 1, 4); h7_fl(wb, 0, w - 1, bd - 1, 3); h7_fl(kc0, bd, w - 1, last, 2);
    for (int c = 0; c < w; c++) h7_hw(bd, c, iw);
    for (int r = 0; r < bd; r++) h7_vw(r, wb, iw);
    h7_hw(bd, 2, 5); h7_hw(bd, wb + 1, 5); h7_hw(d, dc, 5);   // the bedroom's door, the bathroom's door, the front door
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(2, bd); h7_res(2, bd - 1); h7_res(wb + 1, bd); h7_res(wb + 1, bd - 1);
    int wb2 = mans ? 5 : -1;
    if (mans) { for (int r = 0; r < bd; r++) h7_vw(r, wb2, iw); h7_hw(bd, 6, 5); h7_res(6, bd); h7_res(6, bd - 1); }
    int cs = kc0 - 1;
    R7 bedA = { 0, 0, (mans ? wb2 : wb) - 1, bd - 1 }, bedB = { wb2, 0, wb - 1, bd - 1 }, bath = { wb, 0, w - 1, bd - 1 }, kit = { kc0, bd, w - 1, last }, liv = { 0, bd, kc0 - 1, last }, loft = { 0, 0, w - 1, bd - 1 };
    if (two) {   // the flight up: from the living room's back row, rising to the back; the loft is a floor over the back rooms with walls round it
        h7_res(cs, bd + 1);
        for (int r = 0; r < bd; r++) for (int c = 0; c < w; c++) { int x = h7x(c), y = h7_y0 + r; uflr[0][y][x] = 2; }
        for (int c = 0; c < w; c++) { int x = h7x(c); uhwl[0][h7_y0][x] = (u8)ow; uhwl[0][h7_y0 + bd][x] = (u8)ow; }
        for (int r = 0; r < bd; r++) { uvwl[0][h7_y0 + r][h7_x0] = (u8)ow; uvwl[0][h7_y0 + r][h7_x1 + 1] = (u8)ow; }
        uhwl[0][h7_y0 + bd][h7x(cs)] = 0; roof_st = 2; }
    if (two) { h7_pl(I_STAIRS, cs, bd, 0); }
    h7_bedroom(&bedA, mans ? 2 : 3, 0, 14, 0);
    if (mans) h7_bedroom(&bedB, 2, 0, 14, 1);
    h7_bath(&bath);
    h7_kitchen(&kit, 1 | 2, mans);
    h7_living(&liv, 1 | 8, mans);
    h7_dining(&kit, mans ? 6 : 4);
    if (mans) { R7 hall = { 0, bd, w - 1, last }; static const u8 GL[] = { I_PIANO, I_STATUE, I_CLOCK, I_PLANT, I_SHELF, I_ART, I_AQUA }; h7_fillw(&hall, 15, GL, 5, 0); }
    h7_hall(&liv, 3);
    if (two) {   // up there: what the household has left (beds, a desk and its computer, a rug), the landing in front of the stairs kept clear
        h7_lv = 1; int gx = h7x(cs), gy = h7_y0 + bd - 1; g7u[gy][gx] = 2; if (gy - 1 >= h7_y0) g7u[gy - 1][gx] = 2;
        h7_bedroom(&loft, 4, 1, 14, 1); h7_lv = 0; }
}

static void flip7(void) {                                  // turn the plot over: north becomes south (a lot that fronts a street to its north)
    for (int y = 0; y < GN / 2; y++) for (int x = 0; x < GN; x++) { u8 t = flr[y][x]; flr[y][x] = flr[GN - 1 - y][x]; flr[GN - 1 - y][x] = t; }
    for (int y = 0; y <= GN / 2; y++) for (int x = 0; x < GN; x++) { u8 t = hwl[y][x]; hwl[y][x] = hwl[GN - y][x]; hwl[GN - y][x] = t; }
    for (int y = 0; y < GN / 2; y++) for (int x = 0; x <= GN; x++) { u8 t = vwl[y][x]; vwl[y][x] = vwl[GN - 1 - y][x]; vwl[GN - 1 - y][x] = t; }
    for (int L = 0; L < UPN; L++) {
        for (int y = 0; y < GN / 2; y++) for (int x = 0; x < GN; x++) { u8 t = uflr[L][y][x]; uflr[L][y][x] = uflr[L][GN - 1 - y][x]; uflr[L][GN - 1 - y][x] = t; }
        for (int y = 0; y <= GN / 2; y++) for (int x = 0; x < GN; x++) { u8 t = uhwl[L][y][x]; uhwl[L][y][x] = uhwl[L][GN - y][x]; uhwl[L][GN - y][x] = t; }
        for (int y = 0; y < GN / 2; y++) for (int x = 0; x <= GN; x++) { u8 t = uvwl[L][y][x]; uvwl[L][y][x] = uvwl[L][GN - 1 - y][x]; uvwl[L][GN - 1 - y][x] = t; } }
    for (int i = 0; i < nobj; i++) { int w, h; obj_fp(&objs[i], &w, &h); objs[i].y = (u8)(GN - 1 - (objs[i].y + h - 1)); if (!(objs[i].rot & 1)) objs[i].rot ^= 2; }
    door_cy = GN - door_cy; pcy = GN - 1 - pcy; for (int k = 0; k < 3; k++) spawn_y[k] = GN - 1 - spawn_y[k]; occ_ok = 0; }
static void h7_spawn(int dcx, int dcy, int r) {            // the household stands in the hall: the free cells nearest the front door's inside
    int n = 0;
    for (int dist = 0; dist < 14 && n < 3; dist++) for (int y = dcy; y >= dcy - 5 && n < 3; y--) for (int x = dcx - 5; x <= dcx + 5 && n < 3; x++) {
        if (x < 0 || y < 0 || x >= GN || y >= GN || !flr[y][x] || g7o[y][x] == 1 || abs(x - dcx) + (dcy - y) != dist) continue;
        int taken = 0; for (int k = 0; k < n; k++) if (spawn_x[k] == x && spawn_y[k] == y) taken = 1; if (taken) continue; spawn_x[n] = x; spawn_y[n] = y; n++; }
    for (; n < 3; n++) { spawn_x[n] = dcx; spawn_y[n] = dcy; } (void)r; }
static void gen_home7(const LotPlan *p0, u32 h, int terrain, int kind) {   // a home from its plan (the same types as the PS1: compact, ranch, L, box, two-storey, terrace pair, mansion)
    LotPlan q = *p0; int north = q.face == LF_NORTH, ty = q.type, mir = q.mir;
    if (north) { u8 a = (u8)(22 - q.y1), b = (u8)(22 - q.y0); q.y0 = a; q.y1 = b; q.dy = (u8)(22 - q.dy); if (q.wx1) { a = (u8)(22 - q.wy1); b = (u8)(22 - q.wy0); q.wy0 = a; q.wy1 = b; } }
    int bx0 = q.x0 - BM_LOT0, bx1 = q.x1 - BM_LOT0, by0 = q.y0 - BM_LOT0, by1 = q.y1 - BM_LOT0, dxc = q.dx - BM_LOT0, w = bx1 - bx0 + 1, d = by1 - by0 + 1;
    int ow = 1 + ((q.pal * 2 + q.wall) & 3);
    memset(g7o, 0, sizeof g7o); memset(g7f, 0, sizeof g7f); memset(g7u, 0, sizeof g7u); h7_rs = h * 2654435761u + 0x9E3779B9u; if (!h7_rs) h7_rs = 1; h7_beds(h, 1); h7_lv = 0; h7_y0 = by0; h7_mir = mir; h7_x0 = bx0; h7_x1 = bx1;
    int dv = q.drv ? q.drv - BM_LOT0 : -1;
    // the garage wing (L houses): a little walled room of its own, floor of concrete, its door to the drive
    if (ty == HT_L && q.wx1) { int wa = q.wx0 - BM_LOT0, wb2 = q.wx1 - BM_LOT0, wy0 = q.wy0 - BM_LOT0, wy1 = q.wy1 - BM_LOT0; if (!mir) { if (wa <= bx1) wa = bx1 + 1; } else { if (wb2 >= bx0) wb2 = bx0 - 1; }
        if (wa < 0) wa = 0; if (wb2 > GN - 1) wb2 = GN - 1;
        if (wb2 >= wa) {
            for (int y = wy0; y <= wy1; y++) for (int x = wa; x <= wb2; x++) { flr[y][x] = 5; g7o[y][x] = 1; }
            for (int x = wa; x <= wb2; x++) { hwl[wy0][x] = (u8)ow; hwl[wy1 + 1][x] = (u8)ow; } for (int y = wy0; y <= wy1; y++) { vwl[y][wa] = (u8)ow; vwl[y][wb2 + 1] = (u8)ow; }
            int gdoor = dv >= 0 && dv + 1 >= wa && dv + 1 <= wb2 ? dv + 1 : (wa + wb2) / 2; hwl[wy1 + 1][gdoor] = 5; g7o[wy1][gdoor] = 2;
            if (wy1 - wy0 >= 1) { h7_abs(wa, wy0, 103, 0); h7_abs(wb2, wy0, 105, 0); if (wb2 - wa >= 2) h7_abs(wa + (wb2 - wa) / 2, wy0, 107, 0); } } }
    if (ty == HT_TERRACE) {   // two homes sharing a wall, each with its own door
        int xa0 = q.wx0 - BM_LOT0, xa1 = q.wx1 - BM_LOT0, bxb0, bxb1;
        if (!mir) { bxb0 = bx1 + 1; bxb1 = xa1; } else { bxb0 = xa0; bxb1 = bx0 - 1; }
        for (int u = 0; u < 2; u++) {
            int ax0 = u ? bxb0 : bx0, ax1 = u ? bxb1 : bx1, ddx = u ? q.d2x - BM_LOT0 : dxc; h7_x0 = ax0; h7_x1 = ax1; h7_mir = mir;
            int ww = ax1 - ax0 + 1, dcl = mir ? ax1 - ddx : ddx - ax0; if (dcl < 1) dcl = 1; if (dcl > ww - 2) dcl = ww - 2;
            if (u) h7_beds(h + 77u, 0); kitT(ww, d, dcl, ow); }
        h7_x0 = bx0; h7_x1 = bx1; h7_mir = mir;
    } else if (ty == HT_RANCH) { int dcl = mir ? bx1 - dxc : dxc - bx0; kitR(w, d, dcl < 1 ? 1 : dcl, ow); }
    else if (ty == HT_COMPACT) { int dcl = mir ? bx1 - dxc : dxc - bx0; kitC(w, d, dcl < 1 ? 1 : dcl, ow); }
    else { int dcl = mir ? bx1 - dxc : dxc - bx0; kitD(w, d, dcl < 1 ? 1 : dcl, ow, ty == HT_TWO || ty == HT_MANSION); }
    // ---- the yard (docs/HOUSING_DESIGN.md section 3): the walk from the door, a porch, planters and a lamp, a low hedge across the front with a gap at the walk and the drive, a mailbox, trees; the drive; the back yard
    int fr = by1 + 1, n = 11 - by1;   // the first yard row; rows between the front wall and the hedge row (cell row 11)
    char TR = terrain == BT_FOREST || terrain == BT_ROCKY ? '6' : terrain == BT_DESERT ? 'u' : terrain == BT_BEACH ? 'g' : '3'; int TREE_ = h7_let(TR);
    for (int y = fr; y < GN; y++) if (!flr[y][dxc]) flr[y][dxc] = 5;
    if (n >= 2) for (int x = dxc - 1; x <= dxc + 1; x++) if (x >= 0 && x < GN && !flr[fr][x]) flr[fr][x] = 2;
    if (ty == HT_TERRACE) { int d2 = q.d2x - BM_LOT0; for (int y = fr; y < GN; y++) if (!flr[y][d2]) flr[y][d2] = 5; if (n >= 2) for (int x = d2 - 1; x <= d2 + 1; x++) if (x >= 0 && x < GN && !flr[fr][x]) flr[fr][x] = 2; }
    if (dv >= 0) { int top = q.wx1 && ty == HT_L ? q.wy1 - BM_LOT0 + 1 : ty == HT_RANCH ? fr : by0; for (int y = top; y < GN; y++) for (int x = dv; x <= dv + 1; x++) if (x >= 0 && x < GN && !flr[y][x]) { flr[y][x] = 5; g7o[y][x] = 2; } }
    if (ty == HT_TERRACE) { int d2 = q.d2x - BM_LOT0; h7_abs(dxc - 2, fr, h7_let('Z'), 0); h7_abs(dxc + 2, fr, h7_let('Z'), 0); h7_abs(d2 - 2, fr, h7_let('Z'), 0); h7_abs(d2 + 2, fr, h7_let('Z'), 0); }
    else {
        if (n >= 1) { h7_abs(dxc + 1, fr, h7_let('l'), 0); h7_abs(dxc - 2, fr, h7_let('Z'), 0); h7_abs(dxc + 2, fr, h7_let('Z'), 0); }
        if (n >= 3) { int fc = (h & 2) ? 0xE2 : 'p'; h7_abs(dxc - 4, fr, PS1ID[fc & 255] - 1, 0); h7_abs(dxc + 4, fr, PS1ID[fc & 255] - 1, 0); }
    }
    int hedge = kind == NK_SUBURB || kind == NK_ESTATE || kind == NK_GREEN;
    if (n >= 3) {
        h7_abs((h & 4) || !hedge ? dxc - 2 : dxc + 2, 11, h7_let((char)0xE0), 0);
        if (hedge) for (int x = 1; x <= 12; x++) if (abs(x - dxc) > 1 && (dv < 0 || x < dv || x > dv + 1) && !(ty == HT_TERRACE && abs(x - (q.d2x - BM_LOT0)) <= 1)) h7_abs(x, 11, h7_let((char)0xE1), 0);
    }
    if (n >= 2 && kind != NK_CORE && kind != NK_MAIN) { int e = dv >= 0 ? dv < 4 : dxc < 6; h7_abs(e ? 11 : 2, 10, TREE_, 0); }   // a yard tree on the side away from the drive
    if (ty == HT_MANSION) { h7_abs(bx0 + 1, fr + 1, TREE_, 0); h7_abs(bx1 - 1, fr + 1, TREE_, 0); }
    for (int i = 0; i < 3; i++) { int x = 3 + 4 * i; if (kind != NK_CORE && abs(x - dxc) > 1 && (dv < 0 || x < dv - 1 || x > dv + 2) && !flr[12][x]) h7_abs(x, 12, TREE_, 0); }   // the street's trees, one every four cells, on the verge
    if (by0 >= 2 && ty != HT_TERRACE) {   // the back yard: a patio at the back wall, then a barbecue, flower beds or swings
        int pc = (bx0 + bx1) / 2, bk = (int)((h >> 12) & 3), e = dv >= 0 ? dv < 4 : dxc < 6;
        for (int y = by0 - 1; y >= (by0 >= 3 ? by0 - 2 : by0 - 1); y--) for (int x = pc - 1; x <= pc + 1; x++) if (!flr[y][x]) flr[y][x] = 2;
        if (bk == 0) { h7_abs(pc + 2, by0 - 1, 45, 0); } else if (bk == 1) { for (int x = bx0 + 1; x <= bx1 - 1 && x < bx0 + 5; x += 2) h7_abs(x, 0, 27, 0); } else if (bk == 2) { h7_abs(e ? bx1 : bx0, by0 - 1, 46, 0); } else { h7_abs(pc - 2, by0 - 1, 14, 0); h7_abs(pc + 2, by0 - 1, 14, 0); }
        h7_abs(e ? 11 : 2, 0, TREE_, 0); }
    pcx = dxc; pcy = fr; door_cx = dxc; door_cy = by1 + 1;
    h7_spawn(dxc, by1, 0);
    if (north) flip7();
}
// ---- the neighbourhood: plans and streets of the lots round (nb_load calls this) ----
static void nb7_load(void) {
    for (int dy = -RING_MAX; dy <= RING_MAX + 1; dy++) for (int dx = -RING_MAX; dx <= RING_MAX + 1; dx++) {
        int x = lot_bx + dx, y = lot_by + dy; BInfo t; const BInfo *b;
        if (dx <= RING_MAX && dy <= RING_MAX) b = &nbf[dy + RING_MAX][dx + RING_MAX];
        else { if (x < 0 || y < 0 || x >= BM_N || y >= BM_N) { nrd[dy + RING_MAX][dx + RING_MAX] = 0; continue; } bm_block(x, y, &t); b = &t; }
        int rd = b->road & 7; if (b->lot != LOT_RIVER && (b->road & BR_MEGA)) { int mp = BR_PART(b->road); if (mp & 1) rd &= ~2; if (mp & 2) rd &= ~1; }
        nrd[dy + RING_MAX][dx + RING_MAX] = (u8)(b->terrain == BT_SEA ? 0 : rd);
        if (dx <= RING_MAX && dy <= RING_MAX) { LotPlan *p = &nlp[dy + RING_MAX][dx + RING_MAX]; if (b->terrain == BT_SEA || x < 0 || y < 0 || x >= BM_N || y >= BM_N) { memset(p, 0, sizeof *p); p->face = LF_SOUTH; } else bm_lot_plan(x, y, b, p); } }
}
static int st_s(int sx, int sy) { return nrd[sy + RING_MAX + 1][sx + RING_MAX] & 1; }          // a street along the slot's south side
static int st_e(int sx, int sy) { return (nrd[sy + RING_MAX][sx + RING_MAX + 1] >> 1) & 1; }   // ... and its east side
static int st_c(int sx, int sy) { return nrd[sy + RING_MAX + 1][sx + RING_MAX + 1]; }          // what is paved at its south-east corner (1 east-west, 2 north-south, 4 only the corner)
static int far_foot7(const BInfo *b, const LotPlan *p, int *x0, int *z0, int *x1, int *z1) {   // the building's cells on its plot (x1, z1 exclusive)
    if (p->type) { *x0 = p->x0 - BM_LOT0; *x1 = p->x1 - BM_LOT0 + 1; *z0 = p->y0 - BM_LOT0; *z1 = p->y1 - BM_LOT0 + 1;
        if (p->wx1) { int a = p->wx0 - BM_LOT0, c = p->wx1 - BM_LOT0 + 1, e = p->wy0 - BM_LOT0, f = p->wy1 - BM_LOT0 + 1; if (a < *x0) *x0 = a; if (c > *x1) *x1 = c; if (e < *z0) *z0 = e; if (f > *z1) *z1 = f; } return 1; }
    far_foot(b, x0, z0, x1, z1); if (p->face == LF_NORTH) { int a = 13 - *z1, c = 13 - *z0; *z0 = a; *z1 = c; } return 0; }
