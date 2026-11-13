// ds_town7.h -- (D79) WORLD VERSION 7 ON THE DS: homes built from the town plan, turned over for the lots that face north. See docs/DS_TOWN7.md.
// The plan (src/bigmap.c: bm_lot_plan, LotPlan) is in block tiles: a lot is tiles 5..17 of its block both ways. The DS plot is 13 x 13 cells and one cell is one
// tile, so plot cell c is tile c + 5 and a body of w x d tiles is a house of w x d cells (the DS's walls sit on cell edges, so its rooms are roomier than the PS1's).
// A home is built as if it looked south (door on the front wall, the front yard below it), then the whole plot is turned over (flip7) if its plan faces north,
// the way the PS1 builds south and mirrors (src/lots7.inc). The rooms follow docs/HOUSING_DESIGN.md 4b/4c: groups, not rows (a living group round the telly,
// a dining set, a kitchen run with its clear tile, a bed with its nightstands, a bath along a wall), the tiles inside every door kept clear.
static u8 g7o[GN][GN];                                    // cells taken (1) or kept clear (2) while a home is laid out
static int h7_x0, h7_x1, h7_y0, h7_mir;                   // the home being laid out: its first and last column, its back row, mirrored (the plan's mirror: garage side west)
static int h7x(int c) { return h7_mir ? h7_x1 - c : h7_x0 + c; }
static void h7_abs(int x, int y, int id, int rot) {       // a thing on a cell (the upper storeys, rot bits 2-3, are not checked against the ground floor)
    if (id < 0 || x < 0 || y < 0 || x >= GN || y >= GN || nobj >= 400) return;
    if (!(rot & 12)) { if (g7o[y][x]) return; g7o[y][x] = 1; }
    objs[nobj].x = (u8)x; objs[nobj].y = (u8)y; objs[nobj].c = (u8)(id >> 2); objs[nobj].i = (u8)(id & 3); objs[nobj].rot = (u8)rot; nobj++; occ_ok = 0; }
static void h7_put(int c, int r, int id, int rot) { h7_abs(h7x(c), h7_y0 + r, id, h7_mir && (rot & 1) ? rot ^ 2 : rot); }
static void h7_res(int c, int r) { int x = h7x(c), y = h7_y0 + r; if (x >= 0 && y >= 0 && x < GN && y < GN && !g7o[y][x]) g7o[y][x] = 2; }
static void h7_vw(int r, int v, int t) { int x = h7_mir ? h7_x1 + 1 - v : h7_x0 + v; if (h7_y0 + r >= 0 && h7_y0 + r < GN && x >= 0 && x <= GN) vwl[h7_y0 + r][x] = (u8)t; }
static void h7_hw(int l, int c, int t) { int x = h7x(c); if (h7_y0 + l >= 0 && h7_y0 + l <= GN && x >= 0 && x < GN) hwl[h7_y0 + l][x] = (u8)t; }
static void h7_fl(int c0, int r0, int c1, int r1, int t) { for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) { int x = h7x(c), y = h7_y0 + r; if (x >= 0 && x < GN && y >= 0 && y < GN) flr[y][x] = (u8)t; } }
static void h7_shell(int w, int d, int ow, int fl) {      // the outer walls (on the cell edges round the w x d cells) and one floor
    h7_fl(0, 0, w - 1, d - 1, fl);
    for (int c = 0; c < w; c++) { h7_hw(0, c, ow); h7_hw(d, c, ow); }
    for (int r = 0; r < d; r++) { h7_vw(r, 0, ow); h7_vw(r, w, ow); } }
static int h7_let(char ch) { return PS1ID[(u8)ch] - 1; }

// ---- the rooms. Local frame: column c from the home's west edge (mirrored if the plan is), row r from its back wall; dc the door's column in the front wall (row d). ----
// rot: 0 faces south, 1 east, 2 north, 3 west (a thing is turned the way it is used).
static void kitD(int w, int d, int dc, int ow, int two) {    // a divided house (box, L, mansion, two-storey): bed and bath rooms along the back, living | kitchen and dining along the front
    int bd = d >= 8 ? 4 : 3, fd = d - bd, wb = w >= 11 ? 7 : w >= 9 ? 6 : 5, kc0 = w >= 11 ? 7 : w - 4, last = d - 1, iw = 4;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, wb - 1, bd - 1, 4); h7_fl(wb, 0, w - 1, bd - 1, 3); h7_fl(kc0, bd, w - 1, last, 2);
    for (int c = 0; c < w; c++) h7_hw(bd, c, iw);
    for (int r = 0; r < bd; r++) h7_vw(r, wb, iw);
    h7_hw(bd, 2, 5); h7_hw(bd, wb + 1, 5); h7_hw(d, dc, 5);   // the bedroom's door, the bathroom's door, the front door
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(2, bd); h7_res(2, bd - 1); h7_res(wb + 1, bd); h7_res(wb + 1, bd - 1);
    int cs = kc0 - 1;
    if (two) { h7_res(cs, bd + 1);   // the flight up: from the living room's back row, rising to the back; the loft is a floor over the back rooms with walls round it
        for (int r = 0; r < bd; r++) for (int c = 0; c < w; c++) { int x = h7x(c), y = h7_y0 + r; uflr[0][y][x] = 2; }
        for (int c = 0; c < w; c++) { int x = h7x(c); uhwl[0][h7_y0][x] = (u8)ow; uhwl[0][h7_y0 + bd][x] = (u8)ow; }
        for (int r = 0; r < bd; r++) { uvwl[0][h7_y0 + r][h7_x0] = (u8)ow; uvwl[0][h7_y0 + r][h7_x1 + 1] = (u8)ow; }
        uhwl[0][h7_y0 + bd][h7x(cs)] = 0; h7_put(cs, bd, ID_STAIRS, 0);
        h7_put(1, 0, 16, 4); h7_put(0, 0, 100, 4); h7_put(1, 1, 13, 4); h7_put(w - 3, 0, 5, 4); h7_put(w - 2, 0, 9, 4); h7_put(w - 3, 1, 0, 2 | 4);   // up there: a bed and its nightstand, a rug at its foot, a desk, its computer and chair
        roof_st = 2; }
    int bc = (wb + 1) / 2;   // the bedroom: the bed's head on the back wall, a nightstand either side, a rug at its foot; a wardrobe and a lamp on the side wall; the desk, its chair and computer on the other
    h7_put(bc - 1, 0, 100, 0); h7_put(bc, 0, 16, 0); h7_put(bc + 1, 0, 100, 0); if (wb - 2 != bc) h7_put(bc, 1, 13, 0);
    h7_put(0, 1, 101, 1); h7_put(0, 0, 12, 0); h7_put(wb - 1, bd - 2, 5, 3); h7_put(wb - 2, bd - 2, 0, 1); h7_put(wb - 1, bd - 1, 9, 3); h7_put(0, bd - 1, 14, 0);
    h7_put(w - 1, 0, 17, 3); h7_put(w - 1, 2, 18, 3); h7_put(wb, bd - 2, 19, 1); h7_put(wb, bd - 3, 33, 1); if (bd >= 4) h7_put(w - 1, bd - 1, 14, 0);   // the bathroom: the bath and the toilet down the far wall, the sink by the door with its mirror
    int tvr = bd + (fd >= 5 ? 2 : 1);   // the living group: the telly on the side wall (the stereo and a lamp by it), a rug, the coffee table, the sofa facing the telly three tiles away, an armchair at a right angle
    h7_put(0, tvr, 8, 1); h7_put(0, tvr + 1, 10, 1); h7_put(0, last, 12, 0); h7_put(1, tvr, 13, 0); h7_put(2, tvr, 6, 0); h7_put(3, tvr, 2, 3); if (fd >= 5) h7_put(3, tvr - 1, 2, 3); h7_put(2, tvr + 1, 1, 2);
    if (!two && kc0 - 2 >= 4) h7_put(kc0 - 2, bd, 22, 0); if (!two) h7_put(kc0 - 1, bd, 14, 0);
    h7_put(w - 1, bd, 11, 3); h7_put(w - 1, bd + 1, 7, 3); h7_put(w - 1, bd + 2, 28, 3); h7_put(w - 1, bd + 3, 30, 3);   // the kitchen: one run down the end wall, the column before it clear
    int tc = w - 3, tr = bd + (fd >= 5 ? 3 : 2); if (dc == tc) tc = w - 4;   // the dining set beside it: the table with a chair on each side (not on the front door's tile)
    h7_put(tc, tr, 4, 0); h7_put(tc, tr - 1, 0, 0); h7_put(tc, tr + 1, 0, 2); if (tc - 1 >= 5) h7_put(tc - 1, tr, 0, 1);
}
static void kitR(int w, int d, int dc, int ow) {              // a long ranch (5 or 6 deep): the bedroom at one end, the living group in the middle, the kitchen and dining at the other end, the bathroom in its back corner
    int last = d - 1, iw = 4, lw = w - 8;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, 3, last, 4); h7_fl(w - 2, 0, w - 1, 1, 3); h7_fl(w - 4, 2, w - 1, last, 2);
    for (int r = 0; r < d; r++) h7_vw(r, 4, iw);
    for (int c = w - 2; c < w; c++) h7_hw(2, c, iw); for (int r = 0; r < 2; r++) h7_vw(r, w - 2, iw);
    h7_vw(d - 2, 4, 5); h7_hw(2, w - 2, 5); h7_hw(d, dc, 5);   // the bedroom's door, the bathroom's, the front door
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(3, d - 2); h7_res(4, d - 2); h7_res(w - 2, 2); h7_res(w - 2, 1);
    h7_put(0, 0, 101, 0); h7_put(1, 0, 100, 0); h7_put(2, 0, 16, 0); h7_put(3, 0, 100, 0); h7_put(2, 1, 13, 0);   // the bedroom: a wardrobe, the bed between its nightstands, a rug; the desk, its chair and computer on the side wall
    h7_put(0, d - 2, 5, 1); h7_put(1, d - 2, 0, 3); h7_put(0, d - 1, 9, 1);
    h7_put(w - 1, 0, 18, 0); h7_put(w - 2, 0, 19, 0); h7_put(w - 1, 1, 17, 3);                                       // the bathroom
    int tv = 5;
    h7_put(4, 0, 12, 0); h7_put(tv, 0, 8, 0); h7_put(tv + 1, 0, 10, 0); h7_put(tv, 1, 13, 0); h7_put(tv, 2, 6, 0); h7_put(tv, 3, 2, 2); if (lw >= 3) h7_put(tv + 1, 2, 1, 3);   // the living group: the telly on the back wall, a rug, the coffee table, the sofa facing it, an armchair
    h7_put(w - 1, 2, 11, 3); h7_put(w - 1, 3, 28, 3); h7_put(w - 1, 4, 30, 3); if (d >= 6) h7_put(w - 1, 5, 7, 3);   // the kitchen run down the end wall
    int tc = w - 3; h7_put(tc, 3, 4, 0); h7_put(tc, 2, 0, 0); h7_put(tc, 4, 0, 2); h7_put(tc - 1, 3, 0, 1);       // the dining set
    if (d >= 6) h7_put(5, d - 1, 14, 0);
}
static void kitC(int w, int d, int dc, int ow) {              // a cottage or one home of a terrace: one main room, the bathroom walled off in the back corner, the bed in the alcove beside it
    int last = d - 1, iw = 4, tvr = d - 3;
    h7_shell(w, d, ow, 1); h7_fl(0, 0, w - 3, 1, 4); h7_fl(w - 2, 0, w - 1, 1, 3); h7_fl(w - 4, 2, w - 1, last, 2);
    for (int c = w - 2; c < w; c++) h7_hw(2, c, iw); for (int r = 0; r < 2; r++) h7_vw(r, w - 2, iw);
    h7_hw(2, w - 2, 5); h7_hw(d, dc, 5);
    h7_res(dc, last); h7_res(dc, last - 1); h7_res(w - 2, 2); h7_res(w - 2, 1);
    h7_put(w - 1, 0, 18, 0); h7_put(w - 2, 0, 19, 0); h7_put(w - 1, 1, 17, 3);                                       // the bathroom
    h7_put(0, 0, 100, 0); h7_put(1, 0, 16, 0); h7_put(2, 0, 100, 0); h7_put(1, 1, 13, 0); if (w >= 8) h7_put(3, 0, 101, 0); h7_put(0, 1, 12, 0);   // the bed in its alcove between nightstands, a rug, a lamp, a wardrobe
    h7_put(w - 1, 2, 11, 3); h7_put(w - 1, 3, 7, 3); h7_put(w - 1, 4, 28, 3); h7_put(w - 1, 5, 30, 3);   // the kitchen run under the bathroom
    h7_put(0, tvr, 8, 1); h7_put(1, tvr, 13, 0); if (w >= 7) h7_put(2, tvr, 6, 0); h7_put(w >= 7 ? 3 : 2, tvr, 2, 3); if (w >= 7) h7_put(2, tvr + 1, 1, 2); h7_put(0, last, 12, 0);   // the living group
    int tr = 4; if (w >= 7) { int tc = w - 3; if (tc != 3) { h7_put(tc, tr, 4, 0); h7_put(tc, tr - 1, 0, 0); h7_put(tc, tr + 1, 0, 2); if (tc - 1 > 4) h7_put(tc - 1, tr, 0, 1); } }
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
    int ow = 1 + ((q.pal * 2 + q.wall) & 3); (void)h;
    memset(g7o, 0, sizeof g7o); h7_y0 = by0; h7_mir = mir; h7_x0 = bx0; h7_x1 = bx1;
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
            kitC(ww, d, dcl, ow); }
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
