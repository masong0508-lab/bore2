// home: see home.h
#include "home.h"
#include "bigmap.h"
#include "pop.h"
#include "sim.h"
#include "world.h"
#include "audio.h"
#include "wants.h"

int home_city, home_bx = -1, home_by = -1, home_paid, home_move_asked;

void home_reset(void) { home_city = 0; home_bx = home_by = -1; home_paid = 0; home_move_asked = 0; }
static int buyable(int lot) { return lot == LOT_HOUSE || lot == LOT_COTTAGE || lot == LOT_TWO || lot == LOT_EMPTY; }
int home_for_sale(int bx, int by) {
    if (bx < 0 || by < 0 || bx >= BM_N || by >= BM_N) return 0;
    if (home_city && bx == home_bx && by == home_by) return 0;   // (yours)
    BInfo b; bm_block(bx, by, &b);
    if (!buyable(b.lot) || b.terrain == BT_SEA) return 0;
    return b.lot == LOT_EMPTY || pop_family_at(bx, by, 0) < 0;   // (an empty lot, or a home nobody lives in)
}
int home_price(int bx, int by) {   // by what it is, and where: dearer in a city, cheaper in a village
    BInfo b; bm_block(bx, by, &b);
    int p = b.lot == LOT_EMPTY ? 3000 : b.lot == LOT_COTTAGE ? 7000 : b.lot == LOT_TWO ? 16000 : 11000;
    const Settle *s = b.set != 255 ? bm_set(b.set) : 0;
    int k = !s ? 4 : s->kind == ST_CITY ? 6 : s->kind == ST_TOWN ? 4 : 3;
    p = p * k / 4;
    p += p * ((int)(b.h % 21u) - 10) / 100;                  // (no two the same)
    return p / 100 * 100;
}
const char *home_address(int bx, int by, char *out) { BInfo b; bm_block(bx, by, &b); return bm_street(b.set, bx, by, 0, out); }
const char *home_kind(int lot) { return lot == LOT_EMPTY ? "EMPTY LOT" : lot == LOT_COTTAGE ? "COTTAGE" : lot == LOT_TWO ? "TWO-STOREY" : "HOUSE"; }
const char *home_blurb(int lot) {
    switch (lot) {
        case LOT_EMPTY: return "BARE LAND: BUILD YOUR OWN HOME";
        case LOT_COTTAGE: return "ONE ROOM, FURNISHED FOR YOU";
        case LOT_TWO: return "TWO FLOORS, FURNISHED FOR YOU";
        default: return "A HOUSE AND YARD, FURNISHED";
    }
}
int home_list(int cbx, int cby, HomeAd *out, int n) {   // (in and round the town you are in)
    int k = 0;
    for (int r = 0; r <= 14 && k < n; r++)                          // in rings round the block: the nearest first
        for (int dy = -r; dy <= r && k < n; dy++) for (int dx = -r; dx <= r && k < n; dx++) {
            if (ABS(dx) != r && ABS(dy) != r) continue;
            int bx = cbx + dx, by = cby + dy;
            if (!home_for_sale(bx, by)) continue;
            BInfo b; bm_block(bx, by, &b);
            HomeAd *a = &out[k++]; a->bx = (u16)bx; a->by = (u16)by; a->lot = b.lot; a->set = b.set; a->price = home_price(bx, by);
        }
    return k;
}
int home_is_here(void) {
    if (!home_city) return world_zone == ZONE_HOME;
    int wx, wy; if (!world_own_block(&wx, &wy)) return 0;
    const Sim *p = &sims[player];
    return sim_tile_x(p) / BM_BLK == wx && sim_tile_y(p) / BM_BLK == wy;
}
static void move_in(int bx, int by, int price) {   // the lot is theirs: furnished for whoever is in the household
    home_city = 1; home_bx = bx; home_by = by; home_paid = price;
    u8 occ[6] = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < nsims; i++) { int st = sims[i].bore.stage;
        if (st == AG_BABY) occ[5]++; else if (st == AG_CHILD) occ[1]++; else if (st == AG_TEEN) occ[2]++; else if (st == AG_ELDER) occ[3]++; else occ[0]++;
        if (sims[i].partner) occ[4] = 1; }
    if (!occ[0] && !occ[3]) occ[0] = 1;
    world_own_set(bx, by, occ);
}
int home_start(void) {   // a new life begins in SAN BOREAS, in a lived-in sort of house nearest the capital's square: a gift, so it sells back for nothing
    int cap = bm_capital(), nset = bm_nset();
    for (int k = 0; k <= nset; k++) {                                  // the capital first, then every town in turn if it has nothing to give
        int si = k == 0 ? cap : k - 1; if (si < 0 || si >= nset) continue;
        const Settle *s = bm_set(si); if (!s) continue;
        HomeAd ad[40]; int n = home_list(s->x, s->y, ad, 40), pick = -1;
        for (int i = 0; i < n && pick < 0; i++) if (ad[i].lot == LOT_HOUSE || ad[i].lot == LOT_TWO) pick = i;       // (a house or a two-storey: nearest first)
        for (int i = 0; i < n && pick < 0; i++) if (ad[i].lot == LOT_COTTAGE) pick = i;
        if (pick >= 0) { move_in(ad[pick].bx, ad[pick].by, 0); home_move_asked = 1; return 1; }
    }
    return 0;                                                          // (nothing free anywhere: THE MAN BASE, as before)
}
int home_buy(int bx, int by) {
    char t[40], a[24];
    if (!home_for_sale(bx, by)) { sim_note("SORRY, THAT HOME IS NOT FOR SALE"); return 0; }
    int price = home_price(bx, by), refund = home_city ? home_paid : 0;
    if (cash + refund < price) { sprintf(t, "NOT ENOUGH MONEY: IT IS $%d", price); sim_note(t); audio_voice(VOICE_NAH); return 0; }
    if (home_city) { cash += refund; sprintf(t, "OLD HOME SOLD FOR $%d", refund); sim_note(t); world_own_clear(); }
    cash -= price;
    move_in(bx, by, price);                                          // who is moving in: it is furnished for them
    snprintf(t, sizeof t, "BOUGHT %s!", home_address(bx, by, a)); sim_note(t);
    audio_voice(VOICE_WIN);
    home_move_asked = 1;
    for (int i = 0; i < nsims; i++) wants_event(&sims[i], W_HOME, 0);
    return 1;
}
