#include "band.h"
#include "citizens.h"
#include "save.h"
#include "wants.h"
#include <string.h>

Band band;
static const struct { const char *nm, *blurb; s8 sk; } RL[RL_N] = {
    { "NO ROLE", "", -1 },
    { "FRONT BORE", "THE FACE OF THE BAND. SINGS LEAD AND OWNS THE STAGE", BS_VOICE },
    { "LEAD VOCAL", "SINGS THE MAIN LINE", BS_VOICE },
    { "BACKING VOCAL", "HARMONIES AND CHOIRS", BS_VOICE },
    { "RAPPER", "THE FAST WORDS", BS_VOICE },
    { "LEAD GUITAR", "SOLOS AND RIFFS", BS_GTR },
    { "RHYTHM GUITAR", "THE CHUGS AND CHORDS", BS_GTR },
    { "BASSIST", "THE LOW END THAT MOVES THE FLOOR", BS_GTR },
    { "KEYS", "PIANO, ORGAN AND ELECTRIC KEYS", BS_KEYS },
    { "SYNTH WIZARD", "LEADS, PADS AND KNOB TWISTING", BS_KEYS },
    { "HORNS AND STRINGS", "THE BIG MELODIES", BS_KEYS },
    { "DRUMMER", "THE HEARTBEAT", BS_DRUM },
    { "PERCUSSIONIST", "CONGAS, SHAKERS AND BELLS", BS_DRUM },
    { "DJ AND TURNTABLES", "BEATS, BREAKS AND SCRATCHES", BS_DRUM },
    { "LEAD DANCER", "THE SHOW AT THE FRONT OF THE STAGE", BS_DANCE },
    { "DANCE CREW", "MOVES AS ONE", BS_DANCE },
    { "HYPE BORE", "WORKS THE CROWD", BS_DANCE },
    { "BANDLEADER", "STEADIES THE WHOLE BAND. ONLY ONE", -1 },
    { "ROADIE", "GEAR, CABLES AND LOWER COSTS", -1 },
    { "MANAGER", "MORE PAY AND MORE FAME", -1 },
};
const char *band_role_name(int r) { return RL[r < 0 || r >= RL_N ? 0 : r].nm; }
const char *band_role_blurb(int r) { return RL[r < 0 || r >= RL_N ? 0 : r].blurb; }
int band_role_skill(int r) { return RL[r < 0 || r >= RL_N ? 0 : r].sk; }

void band_reset(void) { memset(&band, 0, sizeof band); memset(band.set, 255, sizeof band.set); strcpy(band.name, "THE BORES"); band.tone = 3; }
int band_count(void) { int n = 0; for (int i = 0; i < BAND_MAX; i++) n += band.m[i].kind != 0; return n; }
static Sim *sim_by_uid(int uid) { for (int i = 0; i < nsims; i++) if (sims[i].uid == uid) return &sims[i]; return 0; }
int band_add(int kind, int ref) {
    int free_ = -1;
    for (int i = 0; i < BAND_MAX; i++) { if (band.m[i].kind == kind && band.m[i].ref == ref) return -1; if (!band.m[i].kind && free_ < 0) free_ = i; }
    if (free_ < 0) return -1;
    BandMem *m = &band.m[free_]; memset(m, 0, sizeof *m); m->kind = (u8)kind; m->ref = (u16)ref;
    if (kind == 2) { u32 h = (u32)ref * 2654435761u; for (int k = 0; k < 5; k++) { m->sk[k] = (u8)(((h >> (k * 5)) & 31) % 60); } }   // someone who plays a little
    return free_;
}
void band_remove(int i) { if (i >= 0 && i < BAND_MAX) memset(&band.m[i], 0, sizeof band.m[i]); }
void band_member_name(int i, char *out) {
    const BandMem *m = &band.m[i]; out[0] = 0;
    if (m->kind == 1) { Sim *s = sim_by_uid(m->ref); if (s) strcpy(out, s->bore.first); else strcpy(out, "AWAY"); }
    else if (m->kind == 2) citizens_names(m->ref, out);
}
int band_level(int i, int skill) {
    const BandMem *m = &band.m[i];
    if (m->kind == 1) { Sim *s = sim_by_uid(m->ref); return s ? wants_skill_level(s->skill_band[skill]) : 0; }
    int l = m->sk[skill] / 25; return l > 10 ? 10 : l;
}
void band_train(int i, int skill, int amt) {
    BandMem *m = &band.m[i];
    if (m->kind == 1) { Sim *s = sim_by_uid(m->ref); if (s) s->skill_band[skill] = MIN(4400, s->skill_band[skill] + amt); }
    else if (m->kind == 2) { int v = m->sk[skill] + MAX(1, amt / 16); m->sk[skill] = (u8)MIN(250, v); }
}
int band_leader(void) { for (int i = 0; i < BAND_MAX; i++) if (band.m[i].kind && (band.m[i].role[0] == RL_LEADER || band.m[i].role[1] == RL_LEADER)) return i; return -1; }
static int plays(int i, int skill, int *share) {   // does member i play this skill? (two roles at once: 85 % each)
    const BandMem *m = &band.m[i]; int a = band_role_skill(m->role[0]) == skill, b = band_role_skill(m->role[1]) == skill;
    if (!(a || b)) return 0;
    *share = (m->role[0] && m->role[1] && band_role_skill(m->role[0]) != band_role_skill(m->role[1])) ? 85 : 100; return 1;
}
int band_quality(int skill) {
    int best = -1, second = -1;
    for (int i = 0; i < BAND_MAX; i++) { int sh; if (!band.m[i].kind || !plays(i, skill, &sh)) continue; int e = band_level(i, skill) * sh / 100; if (e > best) { second = best; best = e; } else if (e > second) second = e; }
    int q = best < 0 ? 90 : 40 + ((second < 0 ? best * 3 : best * 2 + second) * 7);   // best*21 for a lone player, a little less with a weaker second
    if (band_leader() >= 0) q += 12;
    return q > 256 ? 256 : q;
}
int band_dance(void) {
    int n = 0, sum = 0;
    for (int i = 0; i < BAND_MAX; i++) { int sh; if (band.m[i].kind && plays(i, BS_DANCE, &sh)) { n++; sum += band_level(i, BS_DANCE) * sh / 100; } }
    if (!n) return 20;
    int q = 30 + (sum / n) * 18 + MIN(n, 6) * 6; return q > 256 ? 256 : q;
}
void band_save(void) {
    sv_u8(band.made); sv_str(band.name, 16); sv_u8(band.genre); sv_u8(band.logo); sv_u8(band.tone); sv_u16(band.fame); sv_u16(band.fans); sv_u8(band.gigs);
    for (int i = 0; i < BAND_MAX; i++) { const BandMem *m = &band.m[i]; sv_u8(m->kind); sv_u16(m->ref); sv_u8(m->role[0]); sv_u8(m->role[1]); for (int k = 0; k < 5; k++) sv_u8(m->sk[k]); }
    sv_n(band.set, BAND_SET); sv_u8(band.pad);   // (version 26: the tour bus)
}
void band_load(int ver) {
    band_reset(); if (ver < 25) return;
    band.made = (u8)ld_u8(); ld_str(band.name, 16); band.genre = (u8)ld_u8(); band.logo = (u8)ld_u8(); band.tone = (u8)ld_u8(); band.fame = ld_u16(); band.fans = ld_u16(); band.gigs = (u8)ld_u8();
    for (int i = 0; i < BAND_MAX; i++) { BandMem *m = &band.m[i]; m->kind = (u8)ld_u8() & 3; m->ref = ld_u16(); m->role[0] = (u8)ld_u8(); m->role[1] = (u8)ld_u8(); for (int k = 0; k < 5; k++) m->sk[k] = (u8)ld_u8();
        if (m->role[0] >= RL_N) m->role[0] = 0; if (m->role[1] >= RL_N) m->role[1] = 0; }
    ld_n(band.set, BAND_SET); band.pad = ver >= 26 ? (u8)(ld_u8() & 1) : 0;
}

#if defined(AUTO_SUPER) || defined(AUTO_PKSCR)
void band_test_setup(int lv) {   // (test builds) the family as a band, every skill at level lv (0..10), a setlist of the first songs
    band.made = 1; band.fame = 70;
    for (int i = 0; i < nsims && i < BAND_MAX; i++) { int k = band_add(1, sims[i].uid); if (k < 0) continue; static const u8 rs[8] = { RL_FRONT, RL_LEADG, RL_KEYS, RL_DRUMS, RL_BASS, RL_DANCER, RL_LEADER, RL_SYNTH }; band.m[k].role[0] = rs[i & 7]; if (i == 0) band.m[k].role[1] = RL_LEADV;
        for (int s = 0; s < 5; s++) sims[i].skill_band[s] = lv * 400; }
    for (int i = 0; i < 5; i++) band.set[i] = (u8)(i * 5);
}
#endif
