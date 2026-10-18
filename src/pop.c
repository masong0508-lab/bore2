// pop: see pop.h
#include "pop.h"
#include "save.h"
#include "bigmap.h"
#include "look.h"
#include "sim.h"
#include "world.h"
#include "citizens.h"

Fam pop_fam[NFAM];
Per pop_per[PER_MAX];
#define PLACE_MAX 1400
Place pop_place[PLACE_MAX];
int pop_nper, pop_nplace;
static int nfam;
static u16 set_p0[32], set_pn[32];                 // each town's places: first, count (pop_place is in town order)

static u32 prs;
static u32 pr(void) { prs ^= prs << 13; prs ^= prs >> 17; prs ^= prs << 5; return prs; }
static int prn(int n) { return (int)((pr() >> 4) % (u32)n); }

// ---- names ----
static const char *const fNm[128] = {
    "ADA", "ALF", "AMOS", "ANYA", "ARLO", "BEA", "BEN", "BETTY", "BIFF", "BO", "BRAM", "CASS", "CHET", "CLEO", "COLA", "DALE",
    "DARLA", "DEX", "DIPPA", "DOT", "EARL", "EDNA", "EFFIE", "ELLA", "ENZO", "ETTA", "EZRA", "FAY", "FINN", "FLO", "GIL", "GUS",
    "HAL", "HANK", "HAZEL", "IDA", "IGGY", "IKE", "IVY", "JADE", "JAX", "JUNE", "JUNO", "KAI", "KIT", "LARS", "LEO", "LOU",
    "LUNA", "MABEL", "MAE", "MARS", "MAX", "MILO", "MOE", "MOSS", "NED", "NELL", "NOVA", "OLLIE", "OPAL", "OTIS", "OZ", "PAM",
    "PEARL", "PIXEL", "POPPY", "QUINN", "RAY", "REX", "RHODA", "ROSA", "ROY", "RUBY", "RUFUS", "SAL", "SID", "SKY", "STAN", "SUE",
    "SUNNY", "TAD", "TESS", "TOBY", "TREE", "TRIX", "UMA", "VERN", "VI", "WALT", "WANDA", "WES", "WILLA", "WYNN", "YARA", "ZED",
    "ZELDA", "ZIGGY", "BARRY", "CLOUD", "ACE", "BOMB", "MISSY", "JUNIPER", "GOTTCHO", "WHISTLER", "SUNMAN", "BUSTER", "DOLLY", "FERN", "GRETA", "HUGO",
    "IRMA", "JASPER", "KENNY", "LOIS", "MERV", "NORMA", "OSCAR", "PERCY", "RITA", "SYLVIA", "TED", "VELMA", "WILBUR", "YURI", "ZORA", "AGNES" };
static const char *const oldNm[16] = { "AGNES", "MABEL", "EDNA", "EARL", "WALT", "IRMA", "MERV", "NORMA", "PERCY", "WILBUR", "VELMA", "SYLVIA", "HAZEL", "OTIS", "ETTA", "GRETA" };
static const char *const sA[32] = { "BUTTER", "PICKLE", "MOON", "SUN", "FROG", "BROOK", "STONE", "HILL", "GOLD", "SILVER", "WOOD", "ASH", "BEAN", "CORN", "MUD", "SNOW",
                                    "RAIN", "PEB", "HUM", "DOZ", "NAP", "SNOOZ", "YAWN", "SLOW", "GRUM", "MOSS", "DIP", "WIG", "BIS", "POT", "KETTLE", "HONEY" };
static const char *const sB[24] = { "WORTH", "FIELD", "SON", "MAN", "BY", "TON", "WICK", "FORD", "BOTTOM", "SMITH", "LEY", "STEIN", "BERG", "ER", "KINS", "WELL",
                                    "SLEY", "HAM", "LOCK", "BURY", "FOOT", "SHAW", "MORE", "DALE" };
static int name_of(u32 seed) { return (int)(((seed * 2654435761u) >> 9) & 127); }
const char *pop_first(int p, char *out) { strcpy(out, fNm[name_of(pop_per[p].seed)]); return out; }   // (from the seed alone: a name lasts a whole life)
static u32 old_mask[4];   // which of the names are the old-fashioned ones (an elder is mostly given one)
static int old_name(int i) {
    if (!old_mask[0] && !old_mask[1] && !old_mask[2] && !old_mask[3])
        for (int k = 0; k < 128; k++) for (int j = 0; j < 16; j++) if (!strcmp(fNm[k], oldNm[j])) old_mask[k >> 5] |= 1u << (k & 31);
    return (old_mask[i >> 5] >> (i & 31)) & 1;
}
const char *pop_surname(int f, char *out) {
    u32 s = pop_fam[f].sur;
    if ((s & 15) == 0) { strcpy(out, look_last_name((int)(s >> 4))); return out; }   // now and then one of the old BORE names
    const char *a = sA[(s >> 4) & 31], *b = sB[(s >> 9) % 24u];
    if (strlen(a) + strlen(b) > NAME_LEN - 1) b = "S";
    strcpy(out, a); strcat(out, b);
    return out;
}
static const char *const kindNm[HK_N] = { "SINGLE", "OLD TIMER", "COUPLE", "RETIRED COUPLE", "ROOMMATES", "YOUNG FAMILY", "BIG FAMILY", "SINGLE PARENT", "THREE GENERATIONS" };
const char *pop_kind_name(int k) { return kindNm[k % HK_N]; }
#define NTRAIT 16
static const char *const traitNm[NTRAIT] = { "CHEERFUL", "GRUMPY", "LAZY", "ACTIVE", "CREATIVE", "GEEK", "ROMANTIC", "FOODIE", "LONER", "OUTGOING",
                                             "NEAT", "SLOB", "GOOFBALL", "BOOKWORM", "MUSIC LOVER", "FAMILY-ORIENTED" };
const char *pop_trait_name(int t) { return traitNm[t % NTRAIT]; }
int pop_stage_of(int p) { return pop_per[p].stage; }

// ---- making them ----
static int is_work(int lot) { return lot == LOT_OFFICE || lot == LOT_TOWER || lot == LOT_HOTEL || lot == LOT_SHOP || lot == LOT_DINER || lot == LOT_GYM || lot == LOT_INN || lot == LOT_POOL || lot == LOT_SCHOOL || lot == LOT_ARCADE || lot >= LOT_CAFE; }
static int find_place(int set, int want_lot, int any_work) {   // a place of that kind in the town (-1: none)
    if (set >= 32 || !set_pn[set]) return -1;
    int n = set_pn[set], k0 = prn(n);
    for (int k = 0; k < n; k++) { int i = set_p0[set] + (k0 + k) % n; int l = pop_place[i].lot;
        if (any_work ? is_work(l) : l == want_lot) return i; }
    return -1;
}
static u8 regen_p[(PER_MAX + 7) / 8], chg_p[(PER_MAX + 7) / 8], regen_f[(NFAM + 7) / 8]; static int gen_done;   // (for the save: who was made anew since the world began, whose days changed, which households were replaced)
static void mark(u8 *a, int i) { a[i >> 3] |= (u8)(1 << (i & 7)); }
static void days_of(Per *q) {   // what they do with their days at this stage of life: school, a job, retired; in a lot, or out
    const Fam *fm = &pop_fam[q->fam]; int stage = q->stage;
    q->job = 0xFFFF; q->flags &= (u8)~PF_HOMEBODY; if (gen_done) mark(chg_p, (int)(q - pop_per));
    if (stage == AG_ADULT && prn(100) < 86) { int pl = find_place(fm->set, 0, 1); if (pl >= 0) q->job = (u16)pl; }
    else if (stage == AG_ELDER && prn(100) < 15) { int pl = find_place(fm->set, 0, 1); if (pl >= 0) q->job = (u16)pl; }
    else if (stage == AG_CHILD || stage == AG_TEEN) { int pl = find_place(fm->set, LOT_SCHOOL, 0); if (pl >= 0) q->job = (u16)pl; }
    if (stage == AG_BABY || (stage == AG_ELDER && prn(100) < 50) || (stage == AG_ADULT && q->job == 0xFFFF && prn(2))) q->flags |= PF_HOMEBODY;
    if (q->trait == 8 && prn(2)) q->flags |= PF_HOMEBODY;               // a loner, mostly in
}
static void set_person(Per *q, int f, int stage, int newborn) {   // somebody new: who they are from the dice, how far into this stage of life
    if (gen_done) mark(regen_p, (int)(q - pop_per));
    q->seed = pr(); q->fam = (u16)f; q->stage = (u8)stage; q->trait = (u8)prn(NTRAIT); q->flags = 0; q->slot = 255;
    q->rel = 0; q->rom = 0; q->hp = 100; q->shift = (u8)pr();
    if (stage == AG_ELDER && prn(16) < 10) for (int t = 0; t < 64 && !old_name(name_of(q->seed)); t++) q->seed = pr();   // (mostly an old-fashioned name)
    days_of(q);
    if (!newborn) q->flags |= (u8)(MIN(63, prn(MAX(1, sim_stage_len(stage)))) << 2);   // (nobody in town is all the same age)
}
static void add_person(int f, int stage) {
    if (pop_nper >= PER_MAX) return;
    set_person(&pop_per[pop_nper++], f, stage, 0);
    pop_fam[f].n++;
}
static int kid_stage(void) { return prn(100) < 40 ? AG_TEEN : AG_CHILD; }
static void make_members(int f) {
    Fam *fm = &pop_fam[f]; fm->first = (u16)pop_nper; fm->n = 0;
    switch (fm->kind) {
        case HK_SINGLE: add_person(f, AG_ADULT); break;
        case HK_OLDTIMER: add_person(f, AG_ELDER); break;
        case HK_COUPLE: add_person(f, AG_ADULT); add_person(f, AG_ADULT); break;
        case HK_RETIRED: add_person(f, AG_ELDER); add_person(f, AG_ELDER); break;
        case HK_ROOMMATES: { int n = 2 + (prn(3) == 0); for (int i = 0; i < n; i++) add_person(f, AG_ADULT); break; }
        case HK_YOUNGFAM: add_person(f, AG_ADULT); add_person(f, AG_ADULT); if (prn(3) == 0) add_person(f, AG_BABY); { int n = 1 + prn(2); for (int i = 0; i < n; i++) add_person(f, AG_CHILD); } break;
        case HK_BIGFAM: { add_person(f, AG_ADULT); add_person(f, AG_ADULT); int n = 3 + prn(2); for (int i = 0; i < n; i++) add_person(f, kid_stage()); break; }
        case HK_SINGLEPARENT: { add_person(f, AG_ADULT); int n = 1 + prn(2); for (int i = 0; i < n; i++) add_person(f, kid_stage()); break; }
        default: add_person(f, AG_ELDER); add_person(f, AG_ADULT); add_person(f, AG_ADULT); { int n = 1 + prn(2); for (int i = 0; i < n; i++) add_person(f, AG_CHILD); } break;
    }
}
static int kind_for(int lot) {                     // who lives in a home like that
    static const u8 small[] = { HK_SINGLE, 30, HK_OLDTIMER, 22, HK_COUPLE, 26, HK_RETIRED, 14, HK_ROOMMATES, 8, 0, 0 };
    static const u8 town[] = { HK_SINGLE, 22, HK_OLDTIMER, 8, HK_COUPLE, 22, HK_ROOMMATES, 18, HK_SINGLEPARENT, 16, HK_YOUNGFAM, 14, 0, 0 };
    static const u8 big[] = { HK_SINGLE, 8, HK_OLDTIMER, 6, HK_COUPLE, 14, HK_RETIRED, 8, HK_ROOMMATES, 12, HK_YOUNGFAM, 20, HK_BIGFAM, 12, HK_SINGLEPARENT, 10, HK_THREEGEN, 10, 0, 0 };
    const u8 *t = lot == LOT_COTTAGE ? small : (lot == LOT_TOWN || lot == LOT_APART) ? town : big;
    int v = prn(100);
    for (; t[1]; t += 2) { if (v < t[1]) return t[0]; v -= t[1]; }
    return HK_COUPLE;
}
typedef struct { u16 bx, by; u8 unit, set, lot, pad; } Unit;
static int unit_key(const Unit *u) { return u->by * 2048 * 4 + u->bx * 4 + u->unit; }
void pop_new(u32 seed, void *scratch, u32 scratch_bytes) {
    prs = seed ^ 0xC0FFEE11u; if (!prs) prs = 1;
    pop_nper = 0; pop_nplace = 0; nfam = 0; gen_done = 0; memset(regen_p, 0, sizeof regen_p); memset(chg_p, 0, sizeof chg_p); memset(regen_f, 0, sizeof regen_f);
    memset(set_pn, 0, sizeof set_pn);
    Unit *U = (Unit *)scratch; int maxu = (int)(scratch_bytes / sizeof(Unit)), nu = 0;
    // every home and every place in every town
    for (int si = 0; si < bm_nset() && si < 32; si++) {
        const Settle *s = bm_set(si);
        set_p0[si] = (u16)pop_nplace;
        for (int by = s->y - s->r; by <= s->y + s->r; by++) for (int bx = s->x - s->r; bx <= s->x + s->r; bx++) {
            BInfo b; bm_block(bx, by, &b);
            if (b.set != si) continue;
            if (bm_is_place(b.lot) && pop_nplace < PLACE_MAX) { Place *p = &pop_place[pop_nplace++]; p->bx = (u16)bx; p->by = (u16)by; p->lot = b.lot; p->set = (u8)si; }
            for (int u = 0; u < bm_units(&b) && nu < maxu; u++) { Unit *x = &U[nu++]; x->bx = (u16)bx; x->by = (u16)by; x->unit = (u8)u; x->set = (u8)si; x->lot = b.lot; }
        }
        set_pn[si] = (u16)(pop_nplace - set_p0[si]);
    }
    // which homes are lived in: a shuffle, the first 874 (the rest stand empty)
    for (int i = nu - 1; i > 0; i--) { int j = prn(i + 1); Unit t = U[i]; U[i] = U[j]; U[j] = t; }
    int nf = MIN(nu, NFAM);
    for (int gap = nf / 2; gap > 0; gap /= 2) for (int i = gap; i < nf; i++) {   // in block order, so a home's household is found by a search
        Unit t = U[i]; int j = i; while (j >= gap && unit_key(&U[j - gap]) > unit_key(&t)) { U[j] = U[j - gap]; j -= gap; } U[j] = t; }
    for (int f = 0; f < nf; f++) {
        Fam *fm = &pop_fam[f]; fm->bx = U[f].bx; fm->by = U[f].by; fm->unit = U[f].unit; fm->set = U[f].set;
        fm->kind = (u8)kind_for(U[f].lot); fm->sur = (u16)pr();
        make_members(f);
    }
    nfam = nf; gen_done = 1;
}
int pop_family_at(int bx, int by, int unit) {
    int key = by * 2048 * 4 + bx * 4 + unit, lo = 0, hi = nfam - 1;
    while (lo <= hi) { int m = (lo + hi) / 2; const Fam *f = &pop_fam[m]; int k = f->by * 2048 * 4 + f->bx * 4 + f->unit;
        if (k == key) return m; if (k < key) lo = m + 1; else hi = m - 1; }
    return -1;
}
void pop_family_kinds(int f, u8 *adults, u8 *kids, u8 *teens, u8 *elders, u8 *couple) {
    const Fam *fm = &pop_fam[f]; *adults = *kids = *teens = *elders = 0;
    for (int i = 0; i < fm->n; i++) { if (pop_per[fm->first + i].flags & PF_GONE) continue; int st = pop_per[fm->first + i].stage;
        if (st == AG_ADULT) (*adults)++; else if (st == AG_ELDER) (*elders)++; else if (st == AG_TEEN) (*teens)++; else (*kids)++; }
    *couple = fm->kind == HK_COUPLE || fm->kind == HK_RETIRED || fm->kind == HK_YOUNGFAM || fm->kind == HK_BIGFAM || fm->kind == HK_THREEGEN;
}

// ---- the day: where everyone is ----
s8 pop_offset(int p) { return (s8)((int)((pop_per[p].seed >> 20) % 61u) - 30); }
static int work_hours(int p, int *a, int *b) {     // today's shift at their job (0: a day off)
    const Per *q = &pop_per[p]; if (q->job == 0xFFFF) return 0;
    int lot = pop_place[q->job].lot, late = q->shift & 1, d = clock_day;
    int off1 = (q->shift >> 1) % 7u, off2 = (off1 + 1 + ((q->shift >> 4) & 1)) % 7;
    switch (lot) {
        case LOT_OFFICE: case LOT_TOWER: case LOT_SCHOOL: if (d >= 5) return 0; *a = lot == LOT_SCHOOL ? 480 : 540; *b = lot == LOT_SCHOOL ? 960 : 1020; return 1;
        case LOT_ARCADE: if (d == off1) return 0; *a = 840; *b = 1380; return 1;
        case LOT_POOL: if (d == off1) return 0; *a = 600; *b = 1080; return 1;
        case LOT_INN: case LOT_HOTEL: case LOT_CLINIC: if (d == off1 || d == off2) return 0; *a = late ? 900 : 420; *b = late ? 1380 : 900; return 1;
        case LOT_BAR: if (d == off1 || d == off2) return 0; *a = 960; *b = 1440; return 1;
        case LOT_CINEMA: if (d == off1) return 0; *a = 780; *b = 1380; return 1;
        case LOT_LIBRARY: case LOT_GALLERY: if (d == off1 || d == off2) return 0; *a = 540; *b = 1050; return 1;
        default: if (d == off1 || d == off2) return 0; *a = late ? 840 : 360; *b = late ? 1320 : 840; if (lot == LOT_SHOP) { *a += 120; *b = MIN(*b + 60, 1320); } return 1;
    }
}
static int leisure(int p, int slot, int *place) {   // out for a couple of hours: somewhere fun in their town (-1: they stay in)
    const Per *q = &pop_per[p]; const Fam *fm = &pop_fam[q->fam];
    u32 h = bm_hash((int)q->seed & 0x7FFF, clock_day * 16 + slot, q->seed);
    int chance = q->stage == AG_ELDER ? 30 : q->stage == AG_ADULT ? 40 : 60;
    if (q->trait == 9) chance += 25; if (q->trait == 2 || q->trait == 8) chance -= 15;   // outgoing / lazy, loner
    if ((int)(h % 100u) >= chance) return 0;
    int set = fm->set; if (set >= 32 || !set_pn[set]) return 0;
    for (int t = 0; t < 10; t++) {
        int i = set_p0[set] + (int)((h >> (8 + t)) % set_pn[set]); int l = pop_place[i].lot;
        if (t < 7 && ABS(pop_place[i].bx - fm->bx) + ABS(pop_place[i].by - fm->by) > 5) continue;   // somewhere in the neighbourhood, mostly
        int ok = q->stage <= AG_TEEN ? (l == LOT_PARK || l == LOT_SKATE || l == LOT_POOL || l == LOT_ARCADE || l == LOT_SQUARE || l == LOT_DINER || l == LOT_CAFE || l == LOT_CINEMA || l == LOT_LIBRARY)
               : q->stage == AG_ELDER ? (l == LOT_PARK || l == LOT_SQUARE || l == LOT_DINER || l == LOT_SHOP || l == LOT_INN || l == LOT_CAFE || l == LOT_LIBRARY || l == LOT_GALLERY || l == LOT_RESTAURANT || l == LOT_CINEMA)
               : (l != LOT_SCHOOL && l != LOT_OFFICE && l != LOT_TOWER && l != LOT_CLINIC);
        if (ok) { *place = i; return 1; }
    }
    return 0;
}
int pop_where(int p, int *place) {
    const Per *q = &pop_per[p]; *place = -1;
    int m = clock_min + pop_offset(p); if (m < 0) m += 1440; if (m >= 1440) m -= 1440;
    int st = q->stage, sleep = st <= AG_CHILD ? (m >= 1230 || m < 420) : st == AG_ELDER ? (m >= 1290 || m < 390) : st == AG_TEEN ? (m >= 1380 || m < 420) : (m >= 1350 || m < 400);
    if (sleep) return AT_SLEEP;
    if (st == AG_BABY) return AT_HOME;
    int a, b;
    if ((st == AG_CHILD || st == AG_TEEN) && q->job != 0xFFFF && clock_day < 5 && m >= 480 && m < 900) { *place = q->job; return AT_SCHOOL; }
    if (st >= AG_ADULT && work_hours(p, &a, &b) && m >= a && m < b) { *place = q->job; return AT_WORK; }
    if (q->flags & PF_HOMEBODY) return AT_HOME;
    int from = st <= AG_TEEN ? 600 : st == AG_ELDER ? 540 : 1020, to = st <= AG_TEEN ? 1170 : st == AG_ELDER ? 1020 : 1320;
    if (clock_day >= 5 && st == AG_ADULT) from = 600;
    if (m >= from && m < to && leisure(p, m / 120, place)) return AT_OUT;
    return AT_HOME;
}
int pop_want(int p) { int pl, w = pop_where(p, &pl); return w == AT_SLEEP ? 2 : w == AT_HOME ? 1 : 0; }
int pop_count_where(int at) { int n = 0, pl; for (int p = 0; p < pop_nper; p++) if (!(pop_per[p].flags & PF_GONE) && pop_where(p, &pl) == at) n++; return n; }

static const char *job_title(int lot, int late) {
    switch (lot) {
        case LOT_OFFICE: case LOT_TOWER: return late ? "MANAGER" : "OFFICE WORKER"; case LOT_HOTEL: return late ? "CONCIERGE" : "HOUSEKEEPER"; case LOT_SHOP: return "SHOP CLERK"; case LOT_DINER: return late ? "WAITER" : "COOK";
        case LOT_GYM: return "TRAINER"; case LOT_INN: return "INNKEEPER"; case LOT_POOL: return "LIFEGUARD"; case LOT_SCHOOL: return "TEACHER";
        case LOT_ARCADE: return "ARCADE ATTENDANT"; case LOT_CAFE: return "BARISTA"; case LOT_BAR: return "BARTENDER"; case LOT_CINEMA: return "USHER";
        case LOT_SALON: return "STYLIST"; case LOT_RESTAURANT: return late ? "WAITER" : "CHEF"; case LOT_CLINIC: return late ? "DOCTOR" : "NURSE";
        case LOT_LIBRARY: return "LIBRARIAN"; case LOT_SPA: return "THERAPIST"; case LOT_GALLERY: return "CURATOR"; default: return "WORKER";
    }
}
const char *pop_job_name(int p, char *out) {
    const Per *q = &pop_per[p]; char nm[24];
    if (q->stage == AG_BABY) { strcpy(out, "BABY"); return out; }
    if (q->stage <= AG_TEEN) { if (q->job != 0xFFFF) { const Place *pl = &pop_place[q->job]; sprintf(out, "STUDENT, %s", bm_place_name(pl->lot, bm_hash(pl->bx, pl->by, 3), nm)); } else strcpy(out, "STUDENT"); return out; }
    if (q->job == 0xFFFF) { strcpy(out, q->stage == AG_ELDER ? "RETIRED" : (q->seed & 1) ? "BETWEEN JOBS" : "STAYS AT HOME"); return out; }
    const Place *pl = &pop_place[q->job];
    sprintf(out, "%s, %s", job_title(pl->lot, q->shift & 1), bm_place_name(pl->lot, bm_hash(pl->bx, pl->by, 3), nm));
    return out;
}
const char *pop_doing(int p, char *out) {
    if (pop_per[p].flags & PF_GONE) { strcpy(out, pop_per[p].hp ? "MOVED AWAY" : "PASSED AWAY"); return out; }
    int pl, w = pop_where(p, &pl); char nm[24];
    switch (w) {
        case AT_SLEEP: strcpy(out, "ASLEEP AT HOME"); break;
        case AT_HOME: strcpy(out, "AT HOME"); break;
        case AT_WORK: sprintf(out, "AT WORK: %s", bm_place_name(pop_place[pl].lot, bm_hash(pop_place[pl].bx, pop_place[pl].by, 3), nm)); break;
        case AT_SCHOOL: sprintf(out, "AT SCHOOL: %s", bm_place_name(pop_place[pl].lot, bm_hash(pop_place[pl].bx, pop_place[pl].by, 3), nm)); break;
        default: sprintf(out, "OUT AT %s", bm_place_name(pop_place[pl].lot, bm_hash(pop_place[pl].bx, pop_place[pl].by, 3), nm)); break;
    }
    return out;
}

// ---- bringing them to life around you ----
int dbg_take_ok, dbg_take_fail, dbg_in;
void pop_tick(void) {
    if (world_zone != ZONE_CITY || world_wbx < 0 || !pop_nper) return;
    static int cur;
    for (int k = 0; k < 40; k++) {
        int p = cur++; if (cur >= pop_nper) cur = 0;
        Per *q = &pop_per[p];
        if (q->flags & PF_GONE) continue;
        int place, at = pop_where(p, &place), bx, by;
        const Fam *fm = &pop_fam[q->fam];
        if (at <= AT_HOME) { bx = fm->bx; by = fm->by; } else { bx = pop_place[place].bx; by = pop_place[place].by; }
        int in = bx >= world_wbx && bx < world_wbx + 5 && by >= world_wby && by < world_wby + 5;
        if (in && at <= AT_HOME && world_home_find(ZONE_CITY, bx, by, fm->unit) < 0) in = 0;
        if (q->slot != 255) {
            int mp = citizens_place_of(q->slot);
            if (!in || (at <= AT_HOME) != (mp < 0) || (at > AT_HOME && mp != place)) citizens_release(q->slot);
        } else if (in) { if (citizens_take(p, at, at <= AT_HOME ? -1 : place) >= 0) dbg_take_ok++; else dbg_take_fail++; }
        if (in) dbg_in++;
    }
}

// ---- growing up, all over SAN BOREAS: once a day at midnight, the same life stages as your own household ----
int pop_aging = 1;
int pop_alive(int f) { const Fam *fm = &pop_fam[f]; int n = 0; for (int i = 0; i < fm->n; i++) if (!(pop_per[fm->first + i].flags & PF_GONE)) n++; return n; }
void pop_midnight(void) {
    if (!pop_aging || !aging || !pop_nper) return;
    for (int p = 0; p < pop_nper; p++) {   // a day older: a birthday at the end of each stage, and at the end of a long life, goodbye
        Per *q = &pop_per[p]; if (q->flags & PF_GONE) continue;
        int d = (q->flags >> 2) + 1, L = sim_stage_len(q->stage);
        if (!L) continue;
        if (d < L || (q->stage == AG_ELDER && (!old_age || d == L))) { q->flags = (u8)((q->flags & 3) | (MIN(d, 63) << 2)); continue; }
        if (q->slot != 255) citizens_release(q->slot);                   // (out in the world right now: back in, as they are now)
        if (q->stage == AG_ELDER) { q->flags |= PF_GONE; q->hp = 0; continue; }   // (passed away, at home)
        q->stage++; q->flags &= 3;
        if (q->stage == AG_ADULT && prn(2)) {                             // grown up: half of them leave home (if there are grown-ups enough there)
            const Fam *fm = &pop_fam[q->fam]; int ad = 0;
            for (int i = 0; i < fm->n; i++) { const Per *o = &pop_per[fm->first + i]; if (o != q && !(o->flags & PF_GONE) && o->stage == AG_ADULT) ad++; }
            if (ad >= 2) { q->flags |= PF_GONE; continue; }
        }
        if (q->stage != AG_TEEN) days_of(q);                              // school, a first job, retirement (a teen stays at the same school)
    }
    for (int f = 0; f < nfam; f++) {
        Fam *fm = &pop_fam[f]; int alive = 0, adults = 0, gone = -1;
        for (int i = 0; i < fm->n; i++) { const Per *q = &pop_per[fm->first + i];
            if (q->flags & PF_GONE) { if (gone < 0 && q->slot == 255) gone = fm->first + i; continue; }
            alive++; if (q->stage == AG_ADULT) adults++; }
        if (!alive && gone >= 0 && !prn(4)) {   // an empty home: before long, a new household moves in (in the old one's places)
            static const u8 kk[4] = { HK_SINGLE, HK_COUPLE, HK_YOUNGFAM, HK_BIGFAM };
            fm->kind = kk[MIN(fm->n, 4) - 1]; fm->sur = (u16)pr(); mark(regen_f, f);
            for (int i = 0; i < fm->n; i++) { Per *q = &pop_per[fm->first + i]; if (q->slot != 255) continue;
                set_person(q, f, i < 2 ? AG_ADULT : prn(3) ? AG_CHILD : AG_BABY, 0); }
            continue;
        }
        if (adults >= 2 && gone >= 0 && fm->kind != HK_ROOMMATES && !prn(12)) set_person(&pop_per[gone], f, AG_BABY, 1);   // a baby!
    }
}

// ---- the memory card (save.c): the world is made again from its seed, so only what changed since is kept ----
extern u16 bfs_q[];   // (sim.c: scratch, free here)
static int bit(const u8 *a, int i) { return (a[i >> 3] >> (i & 7)) & 1; }
static void plane(int off, int n) { u8 *t = (u8 *)bfs_q; for (int i = 0; i < n; i++) t[i] = ((const u8 *)&pop_per[i])[off]; sv_rle(t, n); }
static void unplane(int off, int n) { u8 *t = (u8 *)bfs_q; ld_rle(t, n); for (int i = 0; i < n; i++) ((u8 *)&pop_per[i])[off] = t[i]; }
__attribute__((optimize("Os"))) void pop_save(void) {
    int n = pop_nper;
    sv_u32(prs); sv_u16(n);
    plane((int)__builtin_offsetof(Per, stage), n); plane((int)__builtin_offsetof(Per, flags), n);
    plane((int)__builtin_offsetof(Per, rel), n); plane((int)__builtin_offsetof(Per, rom), n); plane((int)__builtin_offsetof(Per, hp), n);
    sv_rle(regen_p, (n + 7) / 8); sv_rle(chg_p, (n + 7) / 8); sv_rle(regen_f, (nfam + 7) / 8);
    for (int i = 0; i < n; i++) if (bit(regen_p, i)) { sv_u32(pop_per[i].seed); sv_u8(pop_per[i].trait); sv_u8(pop_per[i].shift); }
    for (int i = 0; i < n; i++) if (bit(regen_p, i) || bit(chg_p, i)) sv_u16(pop_per[i].job);
    for (int f = 0; f < nfam; f++) if (bit(regen_f, f)) { sv_u8(pop_fam[f].kind); sv_u16(pop_fam[f].sur); }
}
__attribute__((optimize("Os"))) void pop_load(void) {   // (after pop_new from the same seed)
    prs = ld_u32(); int n = ld_u16(); if (n != pop_nper) n = MIN(n, pop_nper);
    unplane((int)__builtin_offsetof(Per, stage), n); unplane((int)__builtin_offsetof(Per, flags), n);
    unplane((int)__builtin_offsetof(Per, rel), n); unplane((int)__builtin_offsetof(Per, rom), n); unplane((int)__builtin_offsetof(Per, hp), n);
    ld_rle(regen_p, (n + 7) / 8); ld_rle(chg_p, (n + 7) / 8); ld_rle(regen_f, (nfam + 7) / 8);
    for (int i = 0; i < n; i++) if (bit(regen_p, i)) { pop_per[i].seed = ld_u32(); pop_per[i].trait = (u8)ld_u8(); pop_per[i].shift = (u8)ld_u8(); }
    for (int i = 0; i < n; i++) if (bit(regen_p, i) || bit(chg_p, i)) pop_per[i].job = (u16)ld_u16();
    for (int f = 0; f < nfam; f++) if (bit(regen_f, f)) { pop_fam[f].kind = (u8)ld_u8(); pop_fam[f].sur = (u16)ld_u16(); }
}
