/* Only included by a DS_SKATE_CONTROLS_TEST ROM. Exercise the real controller,
 * object occupancy and rail models, then reload the generated skate plaza. */
volatile int sk_controls_done, sk_controls_result[12], sk_controls_speed[8];

static int sk_controls_fixture(int yaw, int axis) {
    LV = 0; ed_lv = 0; ups_clear();
    memset(flr, 2, sizeof flr);
    memset(hwl, 0, sizeof hwl); memset(vwl, 0, sizeof vwl);
    nobj = 0; occ_ok = 0;
    if (axis) for (int i = 4; i <= 8; ++i) {
        if (z_put(axis == 1 ? i : 6, axis == 1 ? 6 : i, '=', axis == 1 ? 1 : 0) < 0) return 0;
    }
    occ_build(); /* invalidate the physics cache after replacing the fixture */
    Sim *m = &sims[ctl];
    m->x = m->tx = m->wx = 6 * 16; m->y = m->ty = m->wy = 6 * 16;
    m->act = -1; m->act_left = 0; slv[ctl] = 0; fq4[ctl] = yaw / 1024;
    skate_on(); k.v = 24 * FX;
    return 1;
}

static int sk_controls_roll(int yaw, int input, int steps) {
    if (!sk_controls_fixture(yaw, 0)) return -1;
    int wx = input < 0 ? 0 : isin(input), wz = input < 0 ? 0 : -icos(input);
    for (int i = 0; i < steps; ++i) {
        k.held = k.pr = 0; skate_step(wx, wz);
        if (!on_board || k.st != KS_GROUND) return -1;
    }
    return k.v / FX;
}

static int sk_controls_rail(int yaw) {
    int axis = (yaw == 1024 || yaw == 3072) ? 1 : 2;
    if (!sk_controls_fixture(yaw, axis)) return 0;
    k.held = k.pr = SKB_TRI; skate_step(0, 0);
    if (k.st != KS_GRIND || k.dir != yaw) return 0;
    int x = skr.x, z = skr.z;
    for (int i = 0; i < 6; ++i) { k.held = SKB_TRI; k.pr = 0; skate_step(0, 0); }
    if (k.st != KS_GRIND || k.dir != yaw || board_lift != sk_ih('=')) return 0;
    if ((skr.x - x) * isin(yaw) - (skr.z - z) * icos(yaw) <= 0) return 0;
    if ((axis == 1 && skr.z != z) || (axis == 2 && skr.x != x)) return 0;
    k.held = k.pr = SKB_X; skate_step(0, 0);
    k.held = k.pr = 0; skate_step(0, 0);
    if (k.st != KS_AIR) return 0;
    x = skr.x; z = skr.z; skate_step(0, 0);
    return k.dir == yaw && (skr.x - x) * isin(yaw) - (skr.z - z) * icos(yaw) > 0;
}

static void ds_skate_controls_test(void) {
    int bx = lot_bx, by = lot_by;
    long score = skate_score, best = skate_best;
    for (int i = 0; i < 4; ++i) {
        int yaw = i * 1024;
        int coast = sk_controls_roll(yaw, -1, 32);
        int stop = sk_controls_roll(yaw, (yaw + 2048) & 4095, 32);
        sk_controls_speed[i] = coast; sk_controls_speed[4 + i] = stop;
        sk_controls_result[i] = coast >= 20 && stop == 0;
        sk_controls_result[4 + i] = sk_controls_rail(yaw);
        sk_controls_result[8 + i] = sk_controls_roll(yaw, yaw, 32) >= 22 &&
            sk_controls_roll(yaw, (yaw + 1024) & 4095, 12) > 16;
    }
    on_board = 0; lot_enter(bx, by); skate_on();
    skate_score = score; skate_best = best;
    sk_controls_done = 1;
}
