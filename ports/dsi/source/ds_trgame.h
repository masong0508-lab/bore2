// ds_trgame.h (D73): the traits' side of the game: who has which traits (assigned from personality, saved), the counters and wishes, the rewards, the
// mood points, the save block. Included by main.c after ds_games.c (it reads the mini-games' numbers). The hooks themselves sit in the game's own code
// (trf_* calls in main.c, ds_games.c, ds_fight_logic.h); the pure parts are in ds_trfx.h. docs/DS_TRAITS.md.
#ifndef DS_TRGAME_H
#define DS_TRGAME_H
static int trg_quiet = 3, trg_dist, trg_cheat_asked;
static char trg_tb[4][56]; static int trg_tbi;
static const char *trg_say(const char *s) {   // a toast in plain words: Sentence case (the data's names are in capitals)
    char *b = trg_tb[trg_tbi++ & 3]; int n = 0; for (; s[n] && n < 54; n++) { char c = s[n]; if (n && c >= 'A' && c <= 'Z') c = (char)(c + 32); b[n] = c; } b[n] = 0; return b; }
void tr_toast(const char *s) { if (trg_quiet) return; say(trg_say(s)); }
void tr_won(TrState *t, int r) {   // (D74) an earned trait came in (1) or found no slot (-1)
    static char b[48]; int i = (int)(t - dtr); if (trg_quiet) return;
    if (r > 0) { if (i >= 0 && i < nsim && i != ctl) sprintf(b, "%.14s won a new trait!", sname[i]); else strcpy(b, "You won a new trait!"); say(b); } else say("Trait won! Free a slot for it");
}
static void trg_title(char *d, const char *s, int mx) {   // "BIG CHEF" -> "Big Chef"
    int n = 0, up = 1; for (; s[n] && n < mx - 1; n++) { char c = s[n]; if (c >= 'A' && c <= 'Z') c = up ? c : (char)(c + 32); else if (c >= 'a' && c <= 'z') c = up ? (char)(c - 32) : c; d[n] = c; up = c == ' ' || c == '/' || c == '-'; } d[n] = 0; }
static const char *trg_tname(int id) { return tr_names + tr_nameoff[id]; }
static void trg_fxtext(char *d, int e, int tier, int mx) {   // one effect in words: "Gets hungry 10% faster"
    const char *s = tier > 0 ? tr_hook_up[e] : tr_hook_dn[e]; int n = tier < 0 ? -tier : tier, v = n * tr_unit[e], k = 0; char t[64]; int j = 0;
    for (; s[j] && k < 60; j++) { if (s[j] == '{') { k += sprintf(t + k, "%d", v); while (s[j] && s[j] != '}') j++; if (!s[j]) break; } else t[k++] = s[j]; } t[k] = 0;
    for (int q = 0; t[q] && q < mx - 1; q++) { char c = t[q]; if (q && c >= 'A' && c <= 'Z') c = (char)(c + 32); d[q] = c; d[q + 1] = 0; } if (!k) d[0] = 0; }
static int trg_nopen(const TrState *t) { int n = 0; for (int k = 0; k < TR_SLOTS; k++) n += t->tid[k] != 0; return n; }
static int trg_wdone_n(void) { int n = 0; for (int i = 0; i < nsim && i < SIM_MAX; i++) n += tr_wd_count(&dtr[i]); return n; }   // (everyone's wishes done count for the household's slots)
static int trg_joblv(void) { return life[ctl].job ? life[ctl].lvl + 1 : 0; }
static int trg_cheat_now(void) { if (!tr_cheat_opt) return 0; for (int i = 0; i < nsim; i++) if (dtr[i].f & TF_CHEAT) return 1; return 0; }   // a cheat trait is on: no medals, no high scores, no awards
static void trg_pend_all(void) { for (int i = 0; i < nsim; i++) if (dtr[i].f & TF_AUTO) dtr[i].f |= TF_PEND; }

// ---- the counters (the PS1's EV_*) for the family: they are counted into the one state that is not automatic (the player's) ----
static void trg_evi(int i, int ev, int n) { if (i >= 0 && i < SIM_MAX && ev > 0 && ev < EV_N && trf_evok[ev] && tr_ev_mem(ev)) tr_ev(&dtr[i], ev, n); }   // (D74: every Sim counts what they do themselves)
static void trg_ev(int ev, int n) { if (trg_pl >= 0 && ev > 0 && ev < EV_N && trf_evok[ev]) tr_ev(&dtr[trg_pl], ev, n); }
static void trg_evs(int ev, int v) { if (trg_pl >= 0 && ev > 0 && ev < EV_N && trf_evok[ev]) tr_ev_set(&dtr[trg_pl], ev, v); }
void tr_reward(TrState *t, int kind, int arg, int wish) {   // a lifetime wish is done: the prize is real (the DS has no groceries or bus pass: money for them)
    int i = (int)(t - dtr); if (i < 0 || i >= nsim) i = ctl; static char b[48]; const char *w = tr_wish_name[wish]; char nm[24]; trg_title(nm, w, 24);
    switch (kind) {
        case RK_CASH: funds += arg; sprintf(b, "Wish done: %s! +$%d", nm, arg); break;
        case RK_TRAIT: { int r = tr_give(t, arg); sprintf(b, r < 0 ? "Wish done: %s! Free a slot" : "Wish done: %s! New trait", nm); break; }
        case RK_OUTFIT: sims[i].shirt = (u8)(arg % 6); sims[i].cc[2] = 0; if (!csx[i].hat) csx[i].hat = (u8)(1 + arg % 5); sprintf(b, "Wish done: %s! New look", nm); break;
        case RK_FOOD: funds += arg * 40; sprintf(b, "Wish done: %s! +$%d food", nm, arg * 40); break;
        case RK_BUS: funds += arg * 30; sprintf(b, "Wish done: %s! +$%d fares", nm, arg * 30); break;
        case RK_TOKENS: arcade_tix += arg; sprintf(b, "Wish done: %s! +%d tickets", nm, arg); break;
        default: { int p = life[i].pts + arg; life[i].pts = (u16)(p > 60000 ? 60000 : p); life_ver++; sprintf(b, "Wish done: %s! +%d wants", nm, arg); } }
    trg_fsync(); if (!trg_quiet) { say(b); sfx(3); }
}

// ---- who has which traits ----
static void trg_pers1(int i, int st, TrPers *p) {   // one Sim's own personality: from their seed (D76: a townie's card, a baby), else from the name (old saves, Create-a-Sim)
    if (psd[i]) { trf_pers_from_seed(psd[i], st, p); return; }
    char f[20], l[20]; const char *s = sname[i], *sp = strrchr(s, ' ');
    if (sp) { int n = (int)(sp - s); if (n > 19) n = 19; memcpy(f, s, (size_t)n); f[n] = 0; strncpy(l, sp + 1, 19); l[19] = 0; } else { strncpy(f, s, 19); f[19] = 0; l[0] = 0; }
    trf_pers_from_name(f, l, st, p);
}
static u32 trg_seed_of(int i) { if (psd[i]) return psd[i]; TrPers p; trg_pers1(i, 4, &p); return p.seed; }
static void trg_pers(int i, TrPers *p) {   // a Sim's personality (as the PS1's from its Bore); a child's from its parents'
    trg_pers1(i, stage_of(sims[i].age), p);
    int pa = -1, pb = -1; for (int j = 0; j < nsim; j++) if (j != i) { if (fpar[i][0] && fid[j] == fpar[i][0]) pa = j; if (fpar[i][1] && fid[j] == fpar[i][1]) pb = j; }
    if (pa >= 0 && pb >= 0) { TrPers q, r; trg_pers1(pa, 4, &q); trg_pers1(pb, 4, &r);
        for (int b = 0; b < 5; b++) { int v = (q.bar[b] + r.bar[b]) / 2 + ((int)p->bar[b] - 5) / 2; p->bar[b] = (u8)(v < 0 ? 0 : v > 10 ? 10 : v); }
        p->asp = (p->seed & 1) ? q.asp : r.asp; }
}
static void trg_assign(int i, int announce) {   // fill the open slots of a Sim that has automatic traits (a move-in, a baby, a new life stage, a new slot)
    TrState *t = &dtr[i]; TrPers p; u16 inh[6]; int ninh = 0, st = trf_stage(stage_of(sims[i].age)), had = trg_nopen(t);
    trg_pers(i, &p);
    for (int k = 0; k < 2; k++) { int par = -1; if (fpar[i][k]) for (int j = 0; j < nsim; j++) if (j != i && fid[j] == fpar[i][k]) par = j; if (par < 0) continue; int c = 0; for (int s = 0; s < TR_SLOTS && c < 3; s++) { int id = dtr[par].tid[s]; if (id && id < TR_FIRST_EARN && ninh < 6) { inh[ninh++] = (u16)id; c++; } } }
    trf_auto_pick(&p, inh, ninh, t->tid, tr_nslots(st)); trf_rebuild(t); tr_stage_set(t, st); t->f = (u8)(t->f & ~TF_PEND);
    if ((t->f & TF_AUTO) && !t->wish && st >= 1) tr_wish_pick(t, trf_auto_wish(&p, t));   // (D74) their own wish, from their personality
    if (announce && !trg_quiet && trg_nopen(t) > had) { static char b[44]; sprintf(b, "%.14s has %d traits", sname[i], trg_nopen(t)); say(b); }
}
static void trg_newsim(int i) { memset(&dtr[i], 0, sizeof dtr[i]); dtr[i].f = TF_AUTO | TF_PEND; }   // (a baby, a move-in: new_sim) its traits come at the next look, when its parents are known
static void trg_died(int k) { (void)k; int any = 0; for (int i = 0; i < nsim; i++) if (!(dtr[i].f & TF_AUTO)) any = 1; if (!any && nsim > 0) dtr[0].f = (u8)(dtr[0].f & ~(TF_AUTO | TF_PEND)); }   // the family's counters and wish pass to the one who plays on
static void trg_cheat_set(int on) { tr_cheat_opt = (u8)on; for (int i = 0; i < SIM_MAX; i++) tr_cheat_refresh(&dtr[i], on); }
static void trg_reset_all(void) {   // a new household
    memset(dtr, 0, sizeof dtr); memset(tr_wait, 0, sizeof tr_wait); tr_unl = 0; tr_earned = 0; memset(tr_recent, 0, sizeof tr_recent); trg_dist = 0; trg_pl = -1; trg_quiet = 3; trg_loaded = 0;
    memset(trg_mp, 0, sizeof trg_mp); memset(trg_mf, 0, sizeof trg_mf); trg_fsync();
#ifdef DS_CHEATDEF
    tr_unl = 2;   // (shot builds: two extra slots open, to show the cheat slots)
#endif
}
static void trg_begin(void) {   // the household is made (Create-a-Sim is done, or a game starts): sim 0 keeps the traits picked (if any), the others get theirs from their personality
    trf_init(); memset(dtr, 0, sizeof dtr); for (int i = 1; i < nsim; i++) dtr[i].f = TF_AUTO | TF_PEND;
    dtr[0].f = 0; trg_assign(0, 0); dtr[0].f = (u8)(dtr[0].f & ~(TF_AUTO | TF_PEND));   // the player's Sim: the traits its personality gives (Create-a-Sim lets you pick them instead)
    for (int w = 1; w <= WISH_N; w++) if (trf_won(w)) { tr_wish_pick(&dtr[0], w); break; }
    trg_quiet = 3; trg_fsync();
}
static void trg_after_load(void) {   // a save is loaded: a save of before D73 has no 'TR' block (its old traits and wish are mapped), the toasts wait
    if (!trg_loaded) trg_migrate(); trg_quiet = 3; trg_fsync(); trg_pl = -1; memset(trg_mp, 0, sizeof trg_mp); memset(trg_mf, 0, sizeof trg_mf);
    for (int i = 0; i < nsim && i < SIM_MAX; i++) if ((dtr[i].f & TF_AUTO) && !dtr[i].tid[0] && !dtr[i].tid[1]) dtr[i].f |= TF_PEND;
}

// ---- the watch, every 8 frames: mood points; every 64: assigning, counters, unlocks ----
static void trg_ctx(int i, TrCtx *c) {
    const Sim *m = &sims[i]; int cx = fdiv(m->x + 8, 16), cy = fdiv(m->y + 8, 16), L = slv[i], near = 0; memset(c, 0, sizeof *c);
    c->night = (u8)night_now(); c->up = L > 0; c->partner = fpt[i] != 0; c->pet = dog_have != 0; c->season = (u8)fm_season();
    c->out = (u8)(L == 0 && (cx < 0 || cy < 0 || cx >= GN || cy >= GN || !flr[cy][cx]));
    c->swim = (u8)(m->act_left > 0 && trg_ak[i] == 12);
    for (int j = 0; j < nsim; j++) if (j != i && slv[j] == L && abs(sims[j].x - m->x) < 80 && abs(sims[j].y - m->y) < 80) near++;
    if (i == ctl) for (int k = 0; k < NCIT && near < 2; k++) if (cits[k].on && abs(cits[k].x - m->x) < 80 && abs(cits[k].y - m->y) < 80) near++;
    c->near = (u8)near;
}
static int trg_friends(void) { int n = 0; for (int j = 0; j < nsim; j++) if (j != ctl && sims[j].rel >= 5) n++; for (int k = 0; k < 64; k++) if (crt[k].id && crt[k].rel >= 5) n++; return n; }
static void trg_tick(int frames) {
    TrCtx c;
    for (int i = 0; i < nsim; i++) { const TrState *t = &dtr[i]; int any = 0; for (int h = 1; h < 8 && !any; h++) any = 0; (void)any;
        if (!t->tid[0] && !t->tid[1] && !t->tid[2] && !t->tid[3] && !t->tid[4] && !t->tid[5]) { trg_mp[i] = 0; trg_mf[i] = 0; continue; }
        trg_ctx(i, &c); trg_mp[i] = (s8)trf_mood_pts(t, &c); trg_mf[i] = (s8)(t->fx[E_MOOD_MIN] > 0 ? trf_mood_floor(t) : 0); }
    if ((frames & 63) != 0) return;
    if (trg_quiet) trg_quiet--;
    trg_pl = -1; for (int i = 0; i < nsim; i++) if (!(dtr[i].f & TF_AUTO)) { trg_pl = i; break; }
    for (int i = 0; i < nsim; i++) { TrState *t = &dtr[i]; if ((t->f & TF_AUTO) && ((t->f & TF_PEND) || tr_stage_get(t) != trf_stage(stage_of(sims[i].age)))) trg_assign(i, 1);
        else if (!(t->f & TF_AUTO) && tr_stage_get(t) != trf_stage(stage_of(sims[i].age))) tr_stage_set(t, trf_stage(stage_of(sims[i].age))); }
    if (trg_pl < 0) return;
    TrState *t = &dtr[trg_pl];
    { static int lk = -1; if (lot_bx >= 0 && lot_bx < 2000 && (lot_bx * 64 + lot_by) != lk) { lk = lot_bx * 64 + lot_by; BInfo bi; bm_block(lot_bx, lot_by, &bi); trg_dist |= 1 << (bi.lot & 15); } }
    int dist = __builtin_popcount(trg_dist & 0xFFFF), hat = 0, el = 0; { const Toy *g = gtoy_of(trg_pl); if (g) { hat = g->hatched; for (int k = 0; k < GB_N; k++) if (g->b[k].on && g->b[k].stage == GS_ELDER) el = 1; } }
    int v[EV_PET - EV_DAYS + 1]; memset(v, 0, sizeof v);
    #define SETV(e, x) v[(e) - EV_DAYS] = (x)
    SETV(EV_DAYS, st_days / 5); SETV(EV_EARNED, (int)(tr_earned / 500 > 255 ? 255 : tr_earned / 500)); SETV(EV_CASH, funds / 1000 < 0 ? 0 : funds / 1000 > 255 ? 255 : funds / 1000); SETV(EV_JOBLV, trg_joblv()); SETV(EV_FRIENDS, trg_friends());
    SETV(EV_FAMILY, nsim); SETV(EV_DISTRICTS, dist); SETV(EV_FIGHTS, fight_wins); SETV(EV_CHAMP, fight_champ & 1); SETV(EV_ARCPLAYS, ar_plays); SETV(EV_TIX, arcade_tix / 10); SETV(EV_HOME, nown);
    SETV(EV_VEHICLES, (has_board ? 1 : 0) + __builtin_popcount((u32)hs_stolen >> 5)); SETV(EV_PARTNER, fpt[trg_pl] ? 1 : 0); SETV(EV_HATCHED, hat); SETV(EV_GELDER, el); SETV(EV_PET, dog_have ? 1 : 0);
    #undef SETV
    for (int k = EV_DAYS; k <= EV_PET; k++) { if (!trf_evok[k]) continue; int x = v[k - EV_DAYS]; x = x > 255 ? 255 : x < 0 ? 0 : x; if (t->f & TF_BASE) tr_ev_set(t, k, x); else tr_ev_quiet(t, k, x); }
    { int x = fm_skl(trg_pl); if (t->f & TF_BASE) tr_ev_set(t, EV_SK_FARM, x); else tr_ev_quiet(t, EV_SK_FARM, x); }
    t->f |= TF_BASE;
    for (int i = 0; i < nsim && i < SIM_MAX; i++) { TrState *m = &dtr[i]; if (i == trg_pl || !(m->f & TF_AUTO)) continue; int x = fm_skl(i); if (m->f & TF_BASE) tr_ev_set(m, EV_SK_FARM, x); else tr_ev_quiet(m, EV_SK_FARM, x); m->f |= TF_BASE; }   // (D74: a housemate counts their own farming skill, the first look quietly)
    { int n = tr_unl_check(trg_wdone_n(), trg_joblv()); if (n) { static char b[40]; sprintf(b, "Trait slot %d unlocked!", TR_BASE + tr_unl); if (!trg_quiet) { say(b); sfx(3); } trg_pend_all();
        for (int i = 0; i < nsim && i < SIM_MAX; i++) if (!(dtr[i].f & TF_AUTO)) { u16 sv[TR_BASE]; for (int k = 0; k < TR_BASE; k++) sv[k] = dtr[i].tid[k]; trg_assign(i, 0); for (int k = 0; k < TR_BASE; k++) dtr[i].tid[k] = sv[k]; trf_rebuild(&dtr[i]); } }   // (the played Sim's new slot fills itself from its personality; its picked slots stay)
      if (n || !(frames & 511)) tr_regive(t); }
}
static void trg_act_end(int i, int an) {   // an action finished: what it counts for the wishes (by the item's own words), the fun spark
    int k = trg_ak[i]; (void)an; trg_ak[i] = trg_ah[i] = 0; if (!k) return;
    switch (k) { case 1: trg_evi(i, EV_COOKED, 1); break; case 2: trg_evi(i, EV_ATEOUT, 1); break; case 3: trg_evi(i, EV_SNACKS, 1); break; case 4: trg_evi(i, EV_SLEPT, 1); break; case 5: trg_evi(i, EV_SHOWERS, 1); break;
        case 6: trg_evi(i, EV_FILMS, 1); trg_evi(i, EV_FUNS, 1); break; case 7: trg_evi(i, EV_BOOKS, 1); trg_evi(i, EV_FUNS, 1); break; case 8: trg_evi(i, EV_COFFEES, 1); break; case 9: trg_evi(i, EV_RUNS, 1); trg_evi(i, EV_FUNS, 1); break;
        case 10: trg_evi(i, EV_CALLS, 1); break; case 11: trg_evi(i, EV_FUNS, 1); break; case 12: trg_evi(i, EV_SWIMS, 1); trg_evi(i, EV_FUNS, 1); break; case 13: trg_evi(i, EV_FUNS, 1); break; }
    if ((k == 11 || k == 13) && dtr[i].fx[E_SPARK]) { static u32 sr; sr = sr * 1664525u + 1013904223u + (u32)minutes; if ((int)((sr >> 12) % 100u) < trf_spark_pct(&dtr[i], 15)) { Sim *m = &sims[i]; m->need[1] = cl100(m->need[1] + 12); say("A spark of inspiration!"); } }
}
static int trg_kind_of(int i, int obj) {   // the item's kind of action by its words: 1 cook 2 eat out 3 snack 4 sleep 5 wash 6 TV 7 read 8 coffee 9 run 10 call 11 play 12 swim 13 creative; 0 none; and trf_act_hook for the hook
    if (obj < 0 || obj >= nobj) return 0; const Item *a = IT_OF(&objs[obj]); const char *l = a->lab; int n = a->need;
    if (n == 0) { if (!strcmp(l, "Cook") || !strcmp(l, "Cook and Eat") || !strcmp(l, "Grill") || !strcmp(l, "Microwave")) return 1; if (!strcmp(l, "Snack") || !strcmp(l, "Grab a Bite") || !strcmp(l, "Drink")) return 3; return (lot_bx != home_bx || lot_by != home_by) ? 2 : 0; }
    if (n == 2) { if (!strcmp(l, "Sleep") || !strcmp(l, "Nap")) return 4; if (!strcmp(l, "A Cup")) return 8; return 0; }
    if (n == 3) return 10; if (n == 5) return (!strcmp(l, "Shower") || !strcmp(l, "Take a Bath") || !strcmp(l, "Soak")) ? 5 : 0;
    if (n == 1) { if (!strcmp(l, "Watch TV")) return 6; if (!strcmp(l, "Read")) return 7; if (!strcmp(l, "A Run") || !strcmp(l, "Lift Weights") || !strcmp(l, "Punch the Bag")) return 9; if (!strcmp(l, "Swim")) return 12;
        if (!strcmp(l, "Paint") || !strncmp(l, "Play Drums", 10) || !strcmp(l, "Play Guitar") || !strcmp(l, "Play Piano") || !strcmp(l, "Play Synth") || !strcmp(l, "Sing Karaoke") || !strcmp(l, "Play Chess")) return 13; return 11; }
    (void)i; return 0;
}
static void trg_arrive(int i) {   // an action starts: its kind (for the hooks and the counters)
    Sim *m = &sims[i]; trg_ak[i] = 0; trg_ah[i] = 0;
    if (m->act >= 100 || (m->act == 3 && m->soc >= 0)) { trg_ah[i] = E_ACT_TALK; return; }
    if (m->act < 0 || m->act > 5 || m->act_obj < 0 || m->act_obj >= nobj) return;
    const Item *a = IT_OF(&objs[m->act_obj]); trg_ak[i] = (u8)trg_kind_of(i, m->act_obj); trg_ah[i] = (u8)trf_act_hook(a->lab, a->need);
}
// ---- the save block 'TR' ----
static int trg_pack(u8 *o) {
    int n = 0; o[n++] = 1; for (int i = 0; i < SIM_MAX; i++) { const TrState *t = &dtr[i]; for (int k = 0; k < TR_SLOTS; k++) { o[n++] = (u8)t->tid[k]; o[n++] = (u8)(t->tid[k] >> 8); } o[n++] = t->f; o[n++] = t->wish; o[n++] = t->wbase; o[n++] = t->title; }
    { const TrState *pt = &dtr[trg_pl >= 0 ? trg_pl : 0]; memcpy(o + n, pt->ev, EV_N); n += EV_N; memcpy(o + n, pt->wd, 8); n += 8; } memcpy(o + n, tr_wait, 10); n += 10; o[n++] = tr_unl; o[n++] = tr_cheat_ok; memcpy(o + n, &tr_earned, 4); n += 4;
    for (int k = 0; k < 8; k++) { o[n++] = (u8)tr_recent[k]; o[n++] = (u8)(tr_recent[k] >> 8); } o[n++] = (u8)trg_dist; o[n++] = (u8)(trg_dist >> 8);
    o[n++] = 2; for (int i = 0; i < SIM_MAX; i++) { memcpy(o + n, dtr[i].ev, EV_N); n += EV_N; memcpy(o + n, dtr[i].wd, 8); n += 8; }   // (D74: every Sim's own counters and wishes done; a d73 save ends before this)
    return n;
}
static int trg_unpack(const u8 *p, int len) {
    int n = 0; if (len < 1 || p[0] != 1) return 0; n = 1; if (len < 1 + SIM_MAX * (TR_SLOTS * 2 + 4) + EV_N + 8 + 10 + 2 + 4 + 16 + 2) return 0;
    trf_init(); for (int i = 0; i < SIM_MAX; i++) { TrState *t = &dtr[i]; memset(t, 0, sizeof *t); for (int k = 0; k < TR_SLOTS; k++) { int id = p[n] | p[n + 1] << 8; n += 2; t->tid[k] = (u16)(id <= TR_MAXID && trf_on(id) ? id : 0); } t->f = (u8)(p[n++] & (TF_AUTO | TF_PEND | TF_BASE | TF_STAGE)); t->wish = p[n++]; t->wbase = p[n++]; t->title = p[n++]; if (t->wish > WISH_N || !trf_won(t->wish)) t->wish = 0; }
    u8 old_ev[EV_N], old_wd[8]; memcpy(old_ev, p + n, EV_N); n += EV_N; memcpy(old_wd, p + n, 8); n += 8; memcpy(tr_wait, p + n, 10); n += 10; tr_unl = (u8)(p[n] > TR_NUNL ? TR_NUNL : p[n]); n++; tr_cheat_ok = p[n++] & 1; memcpy(&tr_earned, p + n, 4); n += 4;
    for (int k = 0; k < 8; k++) { tr_recent[k] = (u16)(p[n] | p[n + 1] << 8); n += 2; } trg_dist = p[n] | p[n + 1] << 8; n += 2;
    { int pl = 0; for (int i = 0; i < SIM_MAX; i++) if (!(dtr[i].f & TF_AUTO)) { pl = i; break; } memcpy(dtr[pl].ev, old_ev, EV_N); memcpy(dtr[pl].wd, old_wd, 8); }   // (a d73 save: the family's counters were the played Sim's)
    if (len >= n + 1 + SIM_MAX * (EV_N + 8) && p[n] == 2) { n++; for (int i = 0; i < SIM_MAX; i++) { memcpy(dtr[i].ev, p + n, EV_N); n += EV_N; memcpy(dtr[i].wd, p + n, 8); n += 8; } }
    for (int i = 0; i < SIM_MAX; i++) { dtr[i].f = (u8)(dtr[i].f & ~(TF_CHEAT | TF_CHON)); trf_rebuild(&dtr[i]); }
    trg_loaded = 1; return 1;
}
static void trg_migrate(void) {   // a save from before D73 (no 'TR' block): the old three traits and the lifetime wish of Create-a-Sim become the new ones
    trf_init(); static const u8 OLD[12][2] = { { E_DEC_CLEAN, -1 }, { E_DEC_CLEAN, 1 }, { E_DEC_FOOD, 1 }, { E_DEC_FOOD, -1 }, { E_DEC_REST, -1 }, { E_DEC_REST, 1 }, { E_DEC_SOCIAL, 1 }, { E_DEC_SOCIAL, -1 }, { E_DEC_FUN, 1 }, { E_DEC_FUN, -1 }, { E_DEC_WC, -1 }, { E_DEC_WC, 1 } };
    memset(dtr, 0, sizeof dtr); tr_unl = 0;
    for (int i = 0; i < nsim; i++) { dtr[i].f = TF_AUTO | TF_PEND; }
    int k0 = 0; for (int k = 0; k < 3; k++) { int t = csx[0].tr[k]; if (t < 1 || t > 12) continue; int hook = OLD[t - 1][0], dir = (s8)OLD[t - 1][1];
        for (int id = 1; id <= TR_NREG; id++) { if (!trf_on(id)) continue; const u8 *e = tr_eff[id]; if (e[0] == hook && ((s8)e[3] > 0) == (dir > 0) && !e[1]) { int ok = 1; for (int q = 0; q < k0; q++) if (dtr[0].tid[q] == id || tr_clash(id, dtr[0].tid[q])) ok = 0; if (ok) { dtr[0].tid[k0++] = (u16)id; break; } } } }
    if (nsim > 0) { dtr[0].f = 0; tr_stage_set(&dtr[0], trf_stage(stage_of(sims[0].age))); if (csx[0].wish >= 1 && csx[0].wish <= 5) { if (csx[0].wdone) dtr[0].wd[0] |= (u8)(1 << csx[0].wish); else tr_wish_pick(&dtr[0], csx[0].wish); } }
    for (int i = 0; i < SIM_MAX; i++) trf_rebuild(&dtr[i]); for (int i = 0; i < nsim; i++) csx[i].tr[0] = csx[i].tr[1] = csx[i].tr[2] = 0;
    trg_loaded = 1;
}
#endif
