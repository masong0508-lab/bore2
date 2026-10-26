// places: see places.h
#include "places.h"
#include "bigmap.h"
#include "world.h"
#include "sim.h"
#include "pop.h"

#define L(x) (1u << (x))
//                   name           where                                         from to  days  pay0  +lvl  stations  titles
const Career careers[CAR_N] = {
    { "FACTORY",     0,                                         9, 17, 0x1F, 1400, 800, "x",     { "LINE WORKER", "MACHINIST", "FOREMAN", "SUPERVISOR", "PLANT MANAGER", "FACTORY BOSS" } },
    { "CULINARY",    L(LOT_DINER) | L(LOT_CAFE) | L(LOT_RESTAURANT), 7, 15, 0x1F,  900, 700, "y/Fc-", { "DISHWASHER", "LINE COOK", "SOUS CHEF", "CHEF", "HEAD CHEF", "RESTAURATEUR" } },
    { "RETAIL",      L(LOT_SHOP) | L(LOT_CONV) | L(LOT_GAS),    10, 18, 0x7C,  800, 650, "y7I",   { "STOCK CLERK", "CASHIER", "SALES LEAD", "SUPERVISOR", "STORE MANAGER", "REGIONAL BOSS" } },
    { "FITNESS",     L(LOT_GYM),                                6, 14, 0x1F, 1000, 800, "]m_",   { "TOWEL ATTENDANT", "TRAINER", "SENIOR TRAINER", "HEAD COACH", "GYM MANAGER", "FITNESS GURU" } },
    { "LIFEGUARD",   L(LOT_POOL),                              10, 18, 0x67,  900, 600, "~>",    { "POOL CLEANER", "LIFEGUARD", "SWIM COACH", "HEAD LIFEGUARD", "POOL MANAGER", "AQUA LEGEND" } },
    { "HOSPITALITY", L(LOT_INN) | L(LOT_HOTEL),                 8, 16, 0x1F, 1000, 750, "yIS",   { "PORTER", "HOUSEKEEPER", "RECEPTIONIST", "CONCIERGE", "HOTEL MANAGER", "HOTELIER" } },
    { "GAMING",      L(LOT_ARCADE),                            14, 22, 0x7C,  700, 700, "!y",    { "TOKEN CLERK", "ATTENDANT", "GAME TESTER", "TECHNICIAN", "ARCADE MANAGER", "ARCADE TYCOON" } },
    { "BUSINESS",    L(LOT_OFFICE) | L(LOT_TOWER),              9, 17, 0x1F, 1500, 1000, "dx[",  { "INTERN", "OFFICE WORKER", "TEAM LEAD", "MANAGER", "DIRECTOR", "CEO" } },
    { "EDUCATION",   L(LOT_SCHOOL) | L(LOT_LIBRARY),            8, 15, 0x1F, 1300, 700, "db[y",  { "TEACHING AIDE", "TEACHER", "SENIOR TEACHER", "HEAD OF YEAR", "VICE PRINCIPAL", "PRINCIPAL" } },
    { "NIGHTLIFE",   L(LOT_BAR),                               16, 24, 0x7C,  900, 750, "yjAF",  { "BARBACK", "BARTENDER", "MIXOLOGIST", "DJ", "BAR MANAGER", "CLUB OWNER" } },
    { "MEDICAL",     L(LOT_CLINIC),                             8, 16, 0x1F, 1600, 1100, "yS[d-", { "ORDERLY", "NURSE", "DOCTOR", "SURGEON", "CHIEF OF STAFF", "MEDICAL LEGEND" } },
    { "FILM",        L(LOT_CINEMA),                            14, 22, 0x7C,  800, 800, "yv:A",  { "USHER", "PROJECTIONIST", "FILM CRITIC", "SCREENWRITER", "DIRECTOR", "MOVIE MOGUL" } },
    { "BEAUTY",      L(LOT_SALON) | L(LOT_SPA),                10, 18, 0x3E,  900, 700, "`y-S>H", { "SHAMPOOER", "STYLIST", "SENIOR STYLIST", "COLOURIST", "SALON OWNER", "STYLE ICON" } },
    { "ARTS",        L(LOT_GALLERY),                           10, 18, 0x3E,  800, 900, ")y}",   { "GALLERY GUIDE", "ART TEACHER", "PAINTER", "CURATOR", "GALLERY OWNER", "GRAND MASTER" } },
};
#undef L
int  job_car = CAR_FACTORY, job_bx = -1, job_by = -1;
char job_place[24] = "THE FACTORY";
int  groceries, room_bx = -1, room_by = -1, room_until;

int place_at(int tx, int ty, PlaceInfo *pi) {
    if (world_zone != ZONE_CITY || world_wbx < 0 || tx < 0 || ty < 0 || tx >= MW || ty >= MH) return 0;
    int bx = world_wbx + tx / BM_BLK, by = world_wby + ty / BM_BLK, lx = tx % BM_BLK, ly = ty % BM_BLK;
    if (lx < BM_LOT0 || ly < BM_LOT0) return 0;                       // the street and the pavement
    BInfo b; bm_block(bx, by, &b);
    if (!bm_is_place(b.lot)) return 0;
    if (pi) { pi->lot = b.lot; pi->h = b.h; pi->bx = bx; pi->by = by; bm_place_name(b.lot, bm_hash(bx, by, 3), pi->name); }
    return 1;
}
// opening hours: [from, to) in minutes (to past 1440: open past midnight), and the days (bit 0 MONDAY)
static void hours_of(int lot, int *a, int *b, int *days) {
    *days = 0x7F;
    switch (lot) {
        case LOT_CAFE:   *a = 360; *b = 1200; break;
        case LOT_BAR:    *a = 960; *b = 1560; break;                         // 4 PM till 2 AM
        case LOT_CINEMA: *a = 720; *b = 1440; break;
        case LOT_SALON:  *a = 540; *b = 1140; *days = 0x3E; break;
        case LOT_RESTAURANT: *a = 660; *b = 1380; break;
        case LOT_CLINIC: *a = 480; *b = 1200; break;
        case LOT_LIBRARY: *a = 540; *b = 1200; *days = 0x3F; break;
        case LOT_SPA:    *a = 540; *b = 1260; break;
        case LOT_GALLERY: *a = 600; *b = 1080; *days = 0x7E; break;
        case LOT_SHOP:   *a = 480; *b = 1320; break;
        case LOT_DINER:  *a = 360; *b = 1380; break;
        case LOT_GYM:    *a = 360; *b = 1320; break;
        case LOT_OFFICE: case LOT_TOWER: *a = 480; *b = 1080; *days = 0x1F; break;
        case LOT_POOL:   *a = 540; *b = 1200; break;
        case LOT_SPACE:  *a = 360; *b = 1320; break;
        case LOT_SCHOOL: *a = 450; *b = 960; *days = 0x1F; break;
        case LOT_ARCADE: *a = 660; *b = 1440; break;
        default:         *a = 0; *b = 1440; break;                     // inns, hotels, parks, squares: always
    }
}
int place_open(int lot, int min, int day) {
    int a, b, d; hours_of(lot, &a, &b, &d);
    if (((d >> day) & 1) && min >= a && min < b) return 1;
    return b > 1440 && min < b - 1440 && ((d >> ((day + 6) % 7)) & 1);   // (still open from last night)
}
int place_opens_at(int lot) { int a, b, d; hours_of(lot, &a, &b, &d); return a / 60; }
int place_closes_at(int lot) { int a, b, d; hours_of(lot, &a, &b, &d); return (b / 60) % 24 ? (b / 60) % 24 : 24; }
const char *hour_name(int h, char *out) { h %= 24; sprintf(out, "%d %s", h % 12 ? h % 12 : 12, h < 12 ? "AM" : "PM"); return out; }
const char *days_name(int mask, char *out) {
    static const char *const dn[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
    if ((mask & 0x7F) == 0x7F) { strcpy(out, "EVERY DAY"); return out; }
    int s = 0; for (int d = 0; d < 7; d++) if (((mask >> d) & 1) && !((mask >> ((d + 6) % 7)) & 1)) { s = d; break; }   // where the run of days starts
    int e = s; while (((mask >> ((e + 1) % 7)) & 1) && (e + 1) % 7 != s) e = (e + 1) % 7;
    sprintf(out, "%s - %s", dn[s], dn[e % 7]); return out;
}
const char *place_hours(int lot, char *out) {
    int a, b, d; hours_of(lot, &a, &b, &d);
    if (a == 0 && b == 1440) { strcpy(out, "OPEN ALL DAY"); return out; }
    char h1[8], h2[8], dd[12]; hour_name(a / 60, h1); hour_name(b / 60, h2);
    if (d != 0x7F) sprintf(out, "%s %s - %s", days_name(d, dd), h1, h2); else sprintf(out, "OPEN %s - %s", h1, h2);
    return out;
}
int place_shop_kind(u32 h) { return (int)((h >> 8) % 5u); }   // h: bm_hash(bx, by, 3), the name's dice (0 food, 1 clothes, 2 games, 3 electronics, 4 records)

// ---- the job ----
int job_shift_len(void) { const Career *c = &careers[job_car]; return (c->to - c->from) * 60; }
int job_on_shift(void) { const Career *c = &careers[job_car]; extern int job_none; if (job_none) return 0; return ((c->days >> clock_day) & 1) && clock_min >= c->from * 60 && clock_min < c->to * 60; }
int job_pay(void) { const Career *c = &careers[job_car]; return c->pay0 + c->pay_lvl * job_level; }
const char *job_title(void) { return careers[job_car].title[CLAMP(job_level, 0, 5)]; }
int job_here(int tx, int ty, char c) {
    const Career *cr = &careers[job_car];
    if (!c || !strchr(cr->station, c)) return 0;
    if (job_car == CAR_FACTORY) return world_zone == ZONE_HOME;           // the factory's work marker, back home
    PlaceInfo pi;
    return place_at(tx, ty, &pi) && pi.bx == job_bx && pi.by == job_by;
}
void job_take(int car, int bx, int by, const char *place) {
    if (car != job_car) job_level = 0;                                   // a new career starts at the bottom
    job_car = car; job_bx = bx; job_by = by;
    strncpy(job_place, place, 23); job_place[23] = 0;
    job_worked = 0; job_days_good = 0; job_strikes = 0;
}
int job_openings(int refbx, int refby, int *car, int *place, int n) {
    int k = 0;
    for (int c = 1; c < CAR_N && k < n; c++) {                            // the nearest place of each kind
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < pop_nplace; i++) {
            const Place *p = &pop_place[i];
            if (!((careers[c].lots >> p->lot) & 1)) continue;
            if (c == job_car && p->bx == job_bx && p->by == job_by) continue;   // (that is the job you have)
            int d = ABS(p->bx - refbx) + ABS(p->by - refby);
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0) { car[k] = c; place[k] = best; k++; }
    }
    if (job_car != CAR_FACTORY && k < n) { car[k] = CAR_FACTORY; place[k] = -1; k++; }   // the old factory, back home
    return k;
}
