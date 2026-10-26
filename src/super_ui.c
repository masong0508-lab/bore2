// super_ui.c - the SUPERSTAR pack: form a band of up to 15, give each one a role (or two), pick songs and rehearse. It is the overlay SUPER.OVL
// (ovl.h): a file on the disc of its own. The band lives in the game (band.c, saved); this only changes it. The 25 songs are the jukebox's own;
// tools/band_roles.py says which skill plays each instrument, so a rehearsal sounds as good as the people playing are.
#include "band.h"
#include "citizens.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "ovl.h"
#include <psxgpu.h>
#include <psxpad.h>
#include <stdio.h>
#include <string.h>
#include "band_roles.inc"

enum { L_HOME, L_MEM, L_PICK, L_ROLE, L_SET, L_REH, L_VEN, L_GIG, L_RES };
enum { H_NAME, H_GENRE, H_LOGO, H_TONE, H_MEM, H_SET, H_REH, H_BUS, H_GIG, H_N };
static int lv, sel, top, slot, rsel, anim, msgT, song, reht, rehacc, pool_n, rehOn;
static const char *msg;
static struct { u8 kind; u16 ref; } pool[96];
static const char *const GN[12] = { "TECHNO", "ROCK", "HIP HOP", "HOUSE", "HI-TECH", "POP", "BALLAD", "DISCO", "FUNK", "JAZZ", "CHILL", "WORLD" };
static const char *const NAMES[24] = { "THE BORES", "BORE-DEM", "SMASHING BORES", "ROCKET COMPANY", "FAME LIGHTS", "PUFF PUFF PASS", "STAGE FRIGHT", "THE DIPPERS",
    "NO ENCORE", "LOUD AND BORED", "SAN BOREAS SOUND", "THE FLEXICODES", "BORED OF DIRECTORS", "TURN IT UP", "GOLDEN HOUR", "MIDNIGHT TRACKER",
    "PIXEL RIOT", "THE UNDERPAID", "VOLUME ELEVEN", "CITY LIGHTS", "STATIC BORE", "LAST CALL", "ROCKET FAME", "THE HEADLINERS" };
static const s8 LOGO[8] = { IC_STAR, IC_BOLT, IC_HEART, IC_NOTE, IC_SUN, IC_SMILE, IC_EYE, IC_PARTY };
static const u32 TONE[8] = { UI_RGB(38, 146, 222), UI_RGB(228, 84, 70), UI_RGB(84, 196, 72), UI_RGB(236, 176, 40), UI_RGB(170, 90, 210), UI_RGB(240, 120, 170), UI_RGB(30, 30, 40), UI_RGB(0, 190, 190) };
static const char *const SKN[5] = { "VOICE", "GUITAR", "KEYS", "DRUMS", "DANCE" };
static const u8 ROLE_SK[4] = { BS_DRUM, BS_GTR, BS_KEYS, BS_VOICE };   // the audio roles 1..4 -> the skill that plays them

static void say(const char *s) { msg = s; msgT = 150; }
static int name_ix(void) { for (int i = 0; i < 24; i++) if (!strcmp(NAMES[i], band.name)) return i; return 0; }
static int setn(void) { int n = 0; for (int i = 0; i < BAND_SET; i++) n += band.set[i] != 255; return n; }
static int in_set(int s) { for (int i = 0; i < BAND_SET; i++) if (band.set[i] == s) return i; return -1; }
static void build_pool(void) {
    pool_n = 0;
    for (int i = 0; i < nsims && pool_n < 96; i++) { pool[pool_n].kind = 1; pool[pool_n].ref = sims[i].uid; pool_n++; }
    citizens_known();
    for (int g = 0; g < pg_ng; g++) for (int j = 0; j < pg_grp[g].n && pool_n < 96; j++) { const PGuest *m = &pg_mem[pg_grp[g].first + j]; if (!m->known) continue; pool[pool_n].kind = 2; pool[pool_n].ref = m->ref; pool_n++; }
}
static void pool_name(int i, char *out) {
    out[0] = 0;
    if (pool[i].kind == 1) { for (int k = 0; k < nsims; k++) if (sims[k].uid == pool[i].ref) { strcpy(out, sims[k].bore.first); return; } }
    else citizens_names(pool[i].ref, out);
}
static int pool_used(int i) { for (int k = 0; k < BAND_MAX; k++) if (band.m[k].kind == pool[i].kind && band.m[k].ref == pool[i].ref) return 1; return 0; }

// ---- the rehearsal ----
static void reh_start(int s);
static void play_song(int s) {
    song = s; reht = 0; rehacc = 0;
    int j = audio_song_find(bandsong[s].song);
    if (j < 0) { say("THE SONG CANNOT BE FOUND"); rehOn = 0; return; }
    audio_jukebox_play(j);
    int q[6] = { 256, 0, 0, 0, 0, 256 };
    for (int r = 1; r <= 4; r++) q[r] = band_quality(ROLE_SK[r - 1]);
    audio_band_set(bandsong[s].role, q); rehOn = 1;
}
static void reh_start(int s) { play_song(s); }
static void reh_train(void) {   // every five seconds of playing: each member learns the parts they play (two parts at once learn more slowly)
    for (int i = 0; i < BAND_MAX; i++) {
        const BandMem *m = &band.m[i]; if (!m->kind) continue;
        int two = m->role[0] && m->role[1] && band_role_skill(m->role[0]) != band_role_skill(m->role[1]);
        for (int k = 0; k < 2; k++) { int sk = band_role_skill(m->role[k]); if (sk < 0) continue; band_train(i, sk, two ? 5 : 8); }
    }
}


// ---- the gigs ----
// Seven stages, each asking for fame. A gig is up to three songs from the setlist, 45 seconds of each. The crowd's energy follows how well the band plays
// (the notes played, from the sound player's own count), the stage show, and the band's work: X works the crowd, SQUARE changes the lights.
static const struct { const char *nm; u16 fame, cap, pay; } VEN[7] = {
    { "THE GARAGE", 0, 20, 0 }, { "CAFE STAGE", 5, 50, 40 }, { "CORNER BAR", 20, 120, 120 }, { "THE CLUB", 60, 400, 400 },
    { "CITY THEATRE", 150, 1200, 1200 }, { "ROCKET ARENA", 400, 5000, 5000 }, { "BORE STADIUM", 1000, 20000, 20000 } };
#ifdef AUTO_SUPER
#define SONGLEN 420   // (test builds: short songs)
#else
#define SONGLEN 2700
#endif
static Bore stg[3]; static const Bore *gb[6]; static int gbm[6], gbn, stgn;   // the stage's models: family Bores as they are, friends made again from their seed
static int tour, tpay, tfans, ven, gsel, gn, gix, gt, gen, gcd, gsum, gcnt, glight, gstars, gacc, gset[3], gpay, gfame, gfans, gwalk, gq;
static const u32 LIGHT[4][3] = { { UI_RGB(255, 80, 80), UI_RGB(255, 220, 80), UI_RGB(255, 120, 200) }, { UI_RGB(80, 160, 255), UI_RGB(80, 255, 220), UI_RGB(180, 120, 255) },
                                 { UI_RGB(120, 255, 120), UI_RGB(255, 255, 120), UI_RGB(255, 160, 60) }, { UI_RGB(255, 255, 255), UI_RGB(200, 200, 255), UI_RGB(255, 200, 200) } };
static int front_level(void) { int b = 0; for (int i = 0; i < BAND_MAX; i++) if (band.m[i].kind && (band.m[i].role[0] == RL_FRONT || band.m[i].role[1] == RL_FRONT)) { int l = band_level(i, BS_VOICE); if (l > b) b = l; } return b; }
static int play_pct(void) { int n = 0, m = 0; for (int r = 1; r <= 4; r++) { n += band_n[r]; m += band_miss[r]; } return n > 8 ? (n - m) * 100 / n : 80; }
static int gig_q(void) {
    int q = play_pct() * 65 / 100 + band_dance() * 100 / 256 * 20 / 100 + (band_leader() >= 0 ? 8 : 0);
    if (gix < gn && bandsong[gset[gix]].genre == band.genre) q += 7;
    return q > 100 ? 100 : q;
}
static void stage_cast(void) {   // up to six members as real Bores (the rest stay as figures)
    for (int k = 0; k < stgn; k++) look_release(&stg[k]);
    stgn = 0; gbn = 0;
    for (int i = 0; i < BAND_MAX && gbn < 6; i++) { const BandMem *m = &band.m[i]; if (!m->kind) continue;
        const Bore *b = 0;
        if (m->kind == 1) { for (int k = 0; k < nsims; k++) if (sims[k].uid == m->ref) b = &sims[k].bore; }
        else if (stgn < 3) { Bore *nb = &stg[stgn++]; memset(nb, 0, sizeof *nb); nb->stage = AG_ADULT; u32 old = look_seed((u32)(m->ref + 7 + i) * 2654435761u + 12345u); look_random(nb); look_seed(old); b = nb; }
        if (b) { gb[gbn] = b; gbm[gbn++] = i; } }
}
static void gig_start(int v) {
    ven = v; gn = 0; int ns = setn(), off = ns ? band.gigs % ns : 0;
    for (int k = 0; k < ns && gn < 3; k++) { int c = 0, i = 0; for (; i < BAND_SET; i++) if (band.set[i] != 255) { if (c == (off + k) % ns) break; c++; } gset[gn++] = band.set[i]; }
    stage_cast();
    gix = 0; gt = 0; gen = 300; gcd = 0; gsum = 0; gcnt = 0; glight = 0; gacc = 0; gwalk = 0; play_song(gset[0]); lv = L_GIG;
}
static void gig_end(void) {
    audio_jukebox_stop(); rehOn = 0;
    for (int k = 0; k < stgn; k++) look_release(&stg[k]);
    stgn = 0; gbn = 0;
    int avg = gcnt ? gsum / gcnt : 0;
    gstars = gwalk ? 0 : avg >= 850 ? 5 : avg >= 700 ? 4 : avg >= 550 ? 3 : avg >= 400 ? 2 : avg >= 220 ? 1 : 0;
    int aud = MIN((int)VEN[ven].cap, 5 + band.fans / 4 + band.fame * 3);
    gpay = VEN[ven].pay * gstars / 3; if (band.fame >= 400 && gpay) gpay += gpay / 4;   // (a rocket company pays a little extra once you are known)
    { int mg = 0; for (int i = 0; i < BAND_MAX; i++) if (band.m[i].kind && (band.m[i].role[0] == RL_MANAGER || band.m[i].role[1] == RL_MANAGER)) mg = 1; if (mg) gpay += gpay / 5; }
    gpay += gpay * SKL(sims[player].skill_char) / 40;       // (a charismatic front person: the crowd pays up)
    if (band.pad && gpay) gpay += gpay / 10;                  // (the bus carries the gear: less hassle)
    if (tour) { gpay += gpay / 2; }                            // (on tour: the road pays well)
    gfame = gstars ? gstars * (ven + 1) : -(ven + 1); gfans = gstars * aud / 20; if (tour) gfans *= 2; gfans += gfans * SKL(sims[player].skill_creat) / 15;   // (songwriting: the fans come back)
    cash += gpay; { int f = (int)band.fame + gfame; band.fame = (u16)(f < 0 ? 0 : f > 9999 ? 9999 : f); int fn = band.fans + gfans; band.fans = (u16)(fn > 60000 ? 60000 : fn); }
    if (band.gigs < 250) band.gigs++;
    if (gstars >= 4) audio_voice(VOICE_YAHOO);
    lv = L_RES;
}
static void gig_update(int pr, int steps) {
    gt += steps; if (gcd > 0) gcd -= steps;
    if ((pr & PAD_CROSS) && gcd <= 0) { gen = MIN(1000, gen + 50 + front_level() * 8); gcd = 150; audio_voice(VOICE_YAHOO); }
    if (pr & PAD_SQUARE) glight = (glight + 1) & 3;
    if (pr & PAD_CIRCLE) { gwalk = 1; gig_end(); return; }
    gacc += steps;
    while (gacc >= 30) { gacc -= 30; gq = gig_q(); int tgt = 500 + (gq - (20 + ven * 10)) * 14; tgt = tgt < 0 ? 0 : tgt > 1000 ? 1000 : tgt; gen += (tgt - gen) / 12; gsum += gen; gcnt++; }
    rehacc += steps; if (rehacc >= 300) { rehacc -= 300; reh_train(); reh_train(); }
    if (gt >= SONGLEN) { if (++gix >= gn) { gig_end(); return; } gt = 0; play_song(gset[gix]); }
}
static void gig_draw(void) {
    char b[48]; int e = gen;
    { static const u32 SKY[4][2] = { { 0x180820u, 0x302060u }, { 0x201008u, 0x603820u }, { 0x082008u, 0x206030u }, { 0x181818u, 0x484040u } };   // (0xBBGGRR)
      gfx_set_clear(8, 8, 22); gfx_set_ambient(110, 110); gfx_sky(SKY[glight][0], SKY[glight][1]);
      int cx = 0, cz = -(6 * TILE), cyaw = 2048, pitch = 110, dist = 1000;
      cam.at.vx = cx; cam.at.vy = -150; cam.at.vz = cz;
      cam.eye.vx = cx - ((isin(cyaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vz = cz - ((icos(cyaw) * ((dist * icos(pitch)) >> 12)) >> 12); cam.eye.vy = -150 - ((dist * isin(pitch)) >> 12);
      gfx_camera_update(); gfx_set_fog(0, 0);
      for (int k = 0; k < gbn; k++) { const BandMem *m = &band.m[gbm[k]];
        int sk = band_role_skill(m->role[0]); if (sk < 0) sk = band_role_skill(m->role[1]);
        VECTOR p; setVector(&p, cx + (k * 2 - (gbn - 1)) * 150, 0, cz);
        Gait g; memset(&g, 0, sizeof g); g.clock = anim + k * 23; g.seed = k * 41; g.mood = 40 + e / 14; g.phase = (anim * 60) & 4095;
        g.amp = sk == BS_DANCE ? 1800 : 0; g.sit = sk == BS_DRUM || sk == BS_KEYS; g.squash = (((anim / 4) + k * 3) & 7) < 4 ? 0 : (e > 300 ? 5 : 0);
        if (e > 600 && !(((anim / 20) + k) & 3)) g.wave = 10;
        voxel_draw(gb[k], &p, 2048, &g); gfx_shadow(&p, 70); } }
    for (int k = 0; k < 6; k++) { int x = 30 + k * 52; ui_disc(x, 18, 7, LIGHT[glight][k % 3]); }
    ui_round(0, 168, 320, 56, 0, UI_RGB(30, 24, 40), UI_RGB(10, 8, 16), 0);
    ui_textc(160, 12, 1, VEN[ven].nm, UI_WHITE);
    ui_textc(160, 24, 1, gix < gn ? bandsong[gset[gix]].song : "", UI_AMBER);
    ui_bar(40, 38, 240, 6, e / 10, e > 700 ? UI_GREEN : e > 400 ? UI_AMBER : UI_RED);
    { int n = band_count() - gbn, x0 = 160 - n * 9, k = 0;   // the rest of the band: little figures at the front of the stage
      for (int i = 0; i < BAND_MAX; i++) { const BandMem *m = &band.m[i]; if (!m->kind) continue; int shown = 0; for (int j = 0; j < gbn; j++) if (gbm[j] == i) shown = 1; if (shown) continue;
        int sk = band_role_skill(m->role[0]); if (sk < 0) sk = band_role_skill(m->role[1]);
        u32 c = sk == BS_VOICE ? UI_RED : sk == BS_GTR ? UI_BLUE : sk == BS_KEYS ? UI_GREEN : sk == BS_DRUM ? UI_AMBER : sk == BS_DANCE ? UI_RGB(240, 120, 170) : UI_LINE;
        int bob = ((((anim / 4) + i * 3) & 7) < 4 ? 0 : 2) * (e > 300 ? 1 : 0), x = x0 + k * 18, y = 148 - bob;
        ui_disc(x + 5, y, 9, UI_RGB(250, 220, 190)); ui_round(x, y + 6, 11, 16, 3, c, c, 0); k++; } }
    { int aud = MIN((int)VEN[ven].cap, 5 + band.fans / 4 + band.fame * 3), cn = 8 + aud * 32 / VEN[ven].cap;
      for (int i = 0; i < cn; i++) { int row = i / 14, col = i % 14, jump = (((anim / 3) + i * 5) & 7) < 4 ? 0 : (e / 250);
        ui_disc(26 + col * 20 + (row & 1) * 9, 184 + row * 12 - jump, 11, UI_RGB(60 + (i * 37) % 90, 60 + (i * 53) % 90, 80 + (i * 29) % 90)); } }
    sprintf(b, "SONG %d OF %d", gix + 1, gn); ui_text(10, 12, 1, b, UI_SOFT); sprintf(b, "%d%%", gq); ui_text(280, 12, 1, b, UI_SOFT);
    ui_hints(160, 214, gcd > 0 ? "SQ LIGHTS|O WALK OFF" : "X WORK THE CROWD|SQ LIGHTS|O WALK OFF", 1);
}
static void res_draw(void) {
    ui_backdrop(); int t = ui_window(40, 20, 240, 190, gwalk ? "WALKED OFF" : gstars >= 4 ? "WHAT A NIGHT!" : gstars >= 2 ? "A GOOD SHOW" : gstars == 1 ? "A ROUGH ONE" : "BOOED OFF", IC_STAR); char b[48];
    ui_textc(160, t + 2, 1, VEN[ven].nm, UI_SOFT);
    for (int i = 0; i < 5; i++) ui_icon_s(88 + i * 28, t + 16, IC_STAR, 24, i < gstars ? UI_AMBER : UI_LINE);
    sprintf(b, "PAY  $%d", gpay); ui_textc(160, t + 52, 1, b, UI_GOODT);
    sprintf(b, "FAME %s%d  (%d)", gfame >= 0 ? "+" : "", gfame, band.fame); ui_textc(160, t + 68, 1, b, gfame >= 0 ? UI_INK : UI_BADT);
    sprintf(b, "NEW FANS +%d  (%d)", gfans, band.fans); ui_textc(160, t + 84, 1, b, UI_INK);
    ui_wrapc(54, t + 104, 212, 3, gstars >= 4 ? "THE CROWD WANTS MORE. A BIGGER STAGE IS OPENING UP." : gstars >= 2 ? "NOT BAD. PRACTISE THE WEAK PARTS AND PLAY AGAIN." : "THE BAND NEEDS MORE REHEARSAL BEFORE THE NEXT ONE.", UI_SOFT);
    { int nx = -1; for (int v = 0; v < 7; v++) if (band.fame < VEN[v].fame) { nx = v; break; } if (nx >= 0) { sprintf(b, "NEXT STAGE: %s AT %d FAME", VEN[nx].nm, VEN[nx].fame); ui_textc(160, t + 134, 1, b, UI_BLUE2); } }
    ui_hints(160, 214, tour > 0 ? "X NEXT SHOW ON THE TOUR|O END TOUR" : "X OK", 1);
}

void super_open(int arg) {
    memset(stg, 0, sizeof stg); stgn = 0; gbn = 0; tour = 0;
    lv = L_HOME; sel = 0; top = 0; slot = 0; rsel = 0; anim = 0; msgT = 0; msg = 0; song = 0; rehOn = 0; reht = 0;
    if (!band.made) { band.made = 1; }
    build_pool();
}
int super_update(int hd, int pr, int steps) {
    anim += steps; if (msgT > 0) { msgT -= steps; if (msgT <= 0) { msgT = 0; msg = 0; } }
    if (lv == L_HOME) {
        if (pr & PAD_UP) sel = (sel + H_N - 1) % H_N; if (pr & PAD_DOWN) sel = (sel + 1) % H_N;
        int d = ((pr & PAD_RIGHT) ? 1 : 0) - ((pr & PAD_LEFT) ? 1 : 0);
        if (d) {
            if (sel == H_NAME) strcpy(band.name, NAMES[(name_ix() + 24 + d) % 24]);
            else if (sel == H_GENRE) band.genre = (u8)((band.genre + 12 + d) % 12);
            else if (sel == H_LOGO) band.logo = (u8)((band.logo + 8 + d) & 7);
            else if (sel == H_TONE) band.tone = (u8)((band.tone + 8 + d) & 7);
        }
        if (pr & PAD_CROSS) {
            if (sel == H_MEM) { lv = L_MEM; slot = 0; }
            else if (sel == H_SET) { lv = L_SET; sel = 0; top = 0; }
            else if (sel == H_REH) { if (!band_count()) say("FORM THE BAND FIRST: ADD MEMBERS"); else if (!setn()) say("PICK SONGS FOR THE SETLIST FIRST"); else { int f = 0; while (band.set[f] == 255) f++; lv = L_REH; reh_start(band.set[f]); } }
            else if (sel == H_BUS) { if (band.pad) say("TOUR: PRESS SQUARE ON THE STAGE LIST"); else if (cash < 6000) { say("A TOUR BUS COSTS $6000"); audio_voice(VOICE_NAH); } else { cash -= 6000; band.pad = 1; say("YOUR OWN TOUR BUS!"); audio_voice(VOICE_YAHOO); } }
            else if (sel == H_GIG) { if (!band_count()) say("FORM THE BAND FIRST: ADD MEMBERS"); else if (!setn()) say("PICK SONGS FOR THE SETLIST FIRST"); else { lv = L_VEN; gsel = 0; for (int v = 0; v < 7; v++) if (band.fame >= VEN[v].fame) gsel = v; } }
            else { strcpy(band.name, NAMES[(name_ix() + 1) % 24]); }
        }
        if (pr & PAD_CIRCLE) return 0;
    } else if (lv == L_MEM) {
        if (pr & PAD_UP) slot = (slot + BAND_MAX - 1) % BAND_MAX; if (pr & PAD_DOWN) slot = (slot + 1) % BAND_MAX;
        if (pr & PAD_CIRCLE) { lv = L_HOME; sel = H_MEM; }
        if (pr & PAD_CROSS) { if (!band.m[slot].kind) { lv = L_PICK; sel = 0; top = 0; } else { lv = L_ROLE; rsel = 0; } }
        if ((pr & PAD_TRIANGLE) && band.m[slot].kind) { band_remove(slot); say("LEFT THE BAND"); }
    } else if (lv == L_PICK) {
        if (pool_n) { if (pr & PAD_UP) sel = (sel + pool_n - 1) % pool_n; if (pr & PAD_DOWN) sel = (sel + 1) % pool_n; }
        if (sel < top) top = sel; if (sel > top + 5) top = sel - 5;
        if (pr & PAD_CIRCLE) lv = L_MEM;
        if ((pr & PAD_CROSS) && pool_n) { if (pool_used(sel)) say("ALREADY IN THE BAND"); else { int k = band_add(pool[sel].kind, pool[sel].ref); if (k >= 0) { slot = k; lv = L_ROLE; rsel = 0; band.m[k].role[0] = RL_KEYS; } } }
    } else if (lv == L_ROLE) {
        BandMem *m = &band.m[slot];
        if (pr & (PAD_UP | PAD_DOWN)) rsel ^= 1;
        int d = ((pr & PAD_RIGHT) ? 1 : 0) - ((pr & PAD_LEFT) ? 1 : 0);
        if (d) {
            int r = m->role[rsel] + d; r = (r + RL_N) % RL_N;
            if (r == RL_LEADER) { int l = band_leader(); if (l >= 0 && l != slot) r = (r + d + RL_N) % RL_N; }   // only one bandleader
            m->role[rsel] = (u8)r;
        }
        if (pr & (PAD_CIRCLE | PAD_CROSS)) lv = L_MEM;
    } else if (lv == L_SET) {
        if (pr & PAD_UP) sel = (sel + NBANDSONG - 1) % NBANDSONG; if (pr & PAD_DOWN) sel = (sel + 1) % NBANDSONG;
        if (sel < top) top = sel; if (sel > top + 5) top = sel - 5;
        if (pr & PAD_CIRCLE) { lv = L_HOME; sel = H_SET; }
        if (pr & PAD_CROSS) {
            int at = in_set(sel);
            if (at >= 0) { band.set[at] = 255; for (int i = at; i < BAND_SET - 1; i++) band.set[i] = band.set[i + 1]; band.set[BAND_SET - 1] = 255; }
            else if (setn() >= BAND_SET) say("THE SETLIST HOLDS 12 SONGS");
            else { int f = 0; while (band.set[f] != 255) f++; band.set[f] = (u8)sel; }
        }
    } else if (lv == L_VEN) {
        if (pr & PAD_UP) gsel = (gsel + 6) % 7; if (pr & PAD_DOWN) gsel = (gsel + 1) % 7;
        if (pr & PAD_CIRCLE) { lv = L_HOME; sel = H_GIG; }
        if (pr & PAD_CROSS) { if (band.fame < VEN[gsel].fame) { say("NOT FAMOUS ENOUGH FOR THAT STAGE YET"); audio_voice(VOICE_NAH); } else { tour = 0; gig_start(gsel); } }
        if (pr & PAD_SQUARE) { if (!band.pad) { say("A TOUR NEEDS A TOUR BUS"); audio_voice(VOICE_NAH); } else if (band.fame < VEN[gsel].fame) { say("NOT FAMOUS ENOUGH FOR THAT STAGE YET"); audio_voice(VOICE_NAH); } else if (cash < 60) { say("FUEL FOR A TOUR IS $60"); audio_voice(VOICE_NAH); } else { cash -= 60; tour = 2; tpay = tfans = 0; gig_start(gsel); } }
    } else if (lv == L_GIG) gig_update(pr, steps);
    else if (lv == L_RES) { if (tour > 0 && (pr & PAD_CROSS)) { tour--; gig_start(ven); } else if (pr & (PAD_CROSS | PAD_CIRCLE)) { tour = 0; lv = L_HOME; sel = H_GIG; } }
    else {   // the rehearsal
        reht += steps; rehacc += steps; if (rehacc >= 300) { rehacc -= 300; reh_train(); }
        if (pr & (PAD_LEFT | PAD_RIGHT)) { int d = (pr & PAD_RIGHT) ? 1 : -1, i = song, n = 0; do { i = (i + NBANDSONG + d) % NBANDSONG; n++; } while (in_set(i) < 0 && n < NBANDSONG); reh_start(i); }
        if (pr & PAD_CROSS) reh_start(song);
        if (pr & PAD_CIRCLE) { audio_jukebox_stop(); rehOn = 0; lv = L_HOME; sel = H_REH; }
    }
    return 1;
}

static void member_row(int y, int i, int on) {
    const BandMem *m = &band.m[i]; char nm[24], b[40];
    ui_round(14, y, 290, 19, 5, on ? UI_HILITE : UI_WHITE, on ? UI_HILITE : UI_PANEL, 0);
    if (!m->kind) { ui_text(22, y + 6, 1, "EMPTY", UI_LINE); return; }
    band_member_name(i, nm); ui_textfit(22, y + 3, 120, nm, UI_INK);
    sprintf(b, "%s%s%s", band_role_name(m->role[0]), m->role[1] ? " + " : "", m->role[1] ? band_role_name(m->role[1]) : ""); ui_textfit(22, y + 11, 180, b, UI_SOFT);
}
static void skill_badge(int x, int y, int i) {   // the member's main skill (their first role's) and its level
    const BandMem *m = &band.m[i]; int sk = band_role_skill(m->role[0]); if (sk < 0) sk = band_role_skill(m->role[1]); char b[24];
    if (sk < 0) { ui_badge(x, y, "CREW", UI_LINE, UI_SOFT); return; }
    sprintf(b, "%s %d", SKN[sk], band_level(i, sk)); ui_badge(x, y, b, UI_BLUE, UI_WHITE);
}
void super_draw(void) {
    if (lv == L_GIG) { gig_draw(); return; }
    if (lv == L_RES) { res_draw(); return; }
    ui_backdrop();
    char b[64];
    int t = ui_window(10, 6, 300, 228, band.name, LOGO[band.logo & 7]);
    if (lv == L_HOME) {
        static const char *const nm[H_N] = { "BAND NAME", "GENRE", "LOGO", "COLOUR", "MEMBERS", "SETLIST", "REHEARSE", "TOUR BUS", "PLAY A GIG" };
        static const s8 ic[H_N] = { IC_BOOK, IC_NOTE, IC_STAR, IC_BRUSH, IC_PEOPLE, IC_NOTE, IC_PLAY, IC_ARROW, IC_PARTY };
        for (int i = 0; i < H_N; i++) {
            int y = t + 2 + i * 19, on = i == sel;
            ui_row(18, y, 170, 18, nm[i], ic[i], on);
            if (i == H_NAME) ui_textfit(104, y + 5, 80, band.name, UI_BLUE2);
            else if (i == H_GENRE) ui_textfit(104, y + 5, 80, GN[band.genre % 12], UI_BLUE2);
            else if (i == H_LOGO) ui_icon(104, y + 3, LOGO[band.logo & 7], TONE[band.tone & 7]);
            else if (i == H_TONE) { ui_round(104, y + 4, 40, 10, 4, TONE[band.tone & 7], TONE[band.tone & 7], 0); }
            else if (i == H_MEM) { sprintf(b, "%d / %d", band_count(), BAND_MAX); ui_badge(104, y + 3, b, UI_BLUE, UI_WHITE); }
            else if (i == H_BUS) ui_badge(104, y + 3, band.pad ? "OWNED" : "$6000", band.pad ? UI_GREEN : UI_BLUE, UI_WHITE);
            else if (i == H_SET) { sprintf(b, "%d / %d", setn(), BAND_SET); ui_badge(104, y + 3, b, UI_BLUE, UI_WHITE); }
        }
        ui_round(196, t + 2, 104, 148, 7, UI_WHITE, UI_PANEL, 0);
        ui_icon(236, t + 8, LOGO[band.logo & 7], TONE[band.tone & 7]); ui_textc(248, t + 30, 1, band.name, UI_INK); ui_textc(248, t + 42, 1, GN[band.genre % 12], UI_SOFT);
        sprintf(b, "FAME %d", band.fame); ui_textc(248, t + 58, 1, b, UI_INK); sprintf(b, "FANS %d", band.fans); ui_textc(248, t + 70, 1, b, UI_INK);
        for (int k = 0; k < 4; k++) { sprintf(b, "%s", SKN[ROLE_SK[k]]); ui_text(204, t + 88 + k * 14, 1, b, UI_SOFT); ui_bar(250, t + 90 + k * 14, 44, 5, band_quality(ROLE_SK[k]) * 100 / 256, UI_GREEN); }
        ui_hints(160, 214, "X OPEN|LEFT RIGHT CHANGE|O BACK", 1);
    } else if (lv == L_MEM || lv == L_ROLE) {
        int c0 = CLAMP(slot - 3, 0, BAND_MAX - 7);
        for (int k = 0; k < 7; k++) { int i = c0 + k, y = t + 2 + k * 21; member_row(y, i, i == slot && lv == L_MEM);
            if (band.m[i].kind) skill_badge(212, y + 4, i); }
        if (lv == L_ROLE) {
            const BandMem *m = &band.m[slot]; char nm[24]; band_member_name(slot, nm);
            ui_round(30, t + 40, 260, 100, 8, UI_WHITE, UI_CARD2, 0);
            ui_textc(160, t + 46, 1, nm, UI_BLUE2);
            for (int r = 0; r < 2; r++) { int y = t + 62 + r * 22; ui_round(40, y, 240, 18, 5, rsel == r ? UI_HILITE : UI_PANEL, rsel == r ? UI_HILITE : UI_PANEL, 0);
                ui_text(46, y + 5, 1, r ? "ALSO PLAYS" : "ROLE", UI_SOFT); ui_textc(190, y + 5, 1, band_role_name(m->role[r]), UI_INK);
                ui_text(274 - 10, y + 5, 1, ">", UI_BLUE); ui_text(116, y + 5, 1, "<", UI_BLUE); }
            ui_wrapc(40, t + 108, 240, 2, band_role_blurb(m->role[rsel]), UI_SOFT);
        }
        if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
        ui_hints(160, 214, lv == L_ROLE ? "UP DOWN WHICH ROLE|LEFT RIGHT CHANGE|X DONE" : "X PICK|TRI LEAVE|O BACK", 1);
    } else if (lv == L_PICK) {
        for (int k = 0; k < 6 && top + k < pool_n; k++) {
            int i = top + k, y = t + 2 + k * 22; char nm[24]; pool_name(i, nm);
            ui_round(18, y, 270, 20, 5, i == sel ? UI_HILITE : UI_WHITE, i == sel ? UI_HILITE : UI_PANEL, 0);
            ui_icon(24, y + 4, IC_SMILE, pool_used(i) ? UI_GREEN : UI_LINE); ui_textfit(44, y + 6, 120, nm, UI_INK);
            ui_badge(190, y + 4, pool[i].kind == 1 ? "FAMILY" : "FRIEND", pool[i].kind == 1 ? UI_BLUE : UI_LINE, pool[i].kind == 1 ? UI_WHITE : UI_SOFT);
            if (pool_used(i)) ui_badge(246, y + 4, "IN", UI_GREEN, UI_WHITE);
        }
        if (!pool_n) ui_wrap(24, t + 20, 250, 4, "NOBODY TO ASK YET. MEET PEOPLE IN TOWN.", UI_SOFT);
        if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
        ui_hints(160, 214, "X ASK THEM IN|O BACK", 1);
    } else if (lv == L_VEN) {
        for (int v = 0; v < 7; v++) { int y = t + 2 + v * 22, ok = band.fame >= VEN[v].fame;
            ui_round(18, y, 270, 20, 5, v == gsel ? UI_HILITE : UI_WHITE, v == gsel ? UI_HILITE : UI_PANEL, 0);
            ui_icon(24, y + 4, ok ? IC_PARTY : IC_LOCK, ok ? UI_GREEN : UI_LINE); ui_textfit(44, y + 6, 110, VEN[v].nm, ok ? UI_INK : UI_SOFT);
            if (ok) { sprintf(b, "%d FANS", VEN[v].cap); ui_badge(140, y + 4, b, UI_BLUE, UI_WHITE); sprintf(b, "$%d", VEN[v].pay); ui_badge(224, y + 4, b, UI_GREEN, UI_WHITE); }
            else { sprintf(b, "%d FAME", VEN[v].fame); ui_badge(140, y + 4, b, UI_LINE, UI_SOFT); } }
        if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
        ui_hints(160, 214, band.pad ? "X PLAY HERE|SQ TOUR (3 SHOWS, $60)|O BACK" : "X PLAY HERE|O BACK", 1);
    } else if (lv == L_SET) {
        for (int k = 0; k < 6 && top + k < NBANDSONG; k++) {
            int i = top + k, y = t + 2 + k * 22, at = in_set(i);
            ui_round(18, y, 270, 20, 5, i == sel ? UI_HILITE : UI_WHITE, i == sel ? UI_HILITE : UI_PANEL, 0);
            ui_disc(30, y + 10, 9, at >= 0 ? UI_GREEN : UI_LINE); if (at >= 0) { sprintf(b, "%d", at + 1); ui_textc(30, y + 6, 1, b, UI_WHITE); }
            ui_textfit(44, y + 6, 160, bandsong[i].song, UI_INK); ui_badge(216, y + 4, GN[bandsong[i].genre], bandsong[i].genre == 0 ? UI_AMBER : UI_BLUE, UI_WHITE);
        }
        if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
        sprintf(b, "%d / %d", setn(), BAND_SET); ui_badge(250, 10, b, UI_WHITE, UI_BLUE2);
        ui_hints(160, 214, "X ADD OR TAKE OUT|O BACK", 1);
    } else {
        ui_textc(160, t + 2, 1, bandsong[song].song, UI_BLUE2); ui_textc(160, t + 14, 1, GN[bandsong[song].genre], UI_SOFT);
        for (int r = 1; r <= 4; r++) {
            int sk = ROLE_SK[r - 1], y = t + 30 + (r - 1) * 28, q = band_quality(sk), played = band_n[r], miss = band_miss[r];
            ui_round(18, y, 270, 25, 6, UI_WHITE, UI_PANEL, 0);
            ui_text(24, y + 3, 1, SKN[sk], UI_INK);
            int best = -1, bl = -1; for (int i = 0; i < BAND_MAX; i++) if (band.m[i].kind) { int l = band_level(i, sk), pl = (band.m[i].role[0] && band_role_skill(band.m[i].role[0]) == sk) || (band.m[i].role[1] && band_role_skill(band.m[i].role[1]) == sk); if (pl && l > bl) { bl = l; best = i; } }
            char nm[24]; if (best >= 0) band_member_name(best, nm); else strcpy(nm, "BACKING TRACK");
            ui_textfit(24, y + 14, 90, nm, UI_SOFT); sprintf(b, "LV %d", bl < 0 ? 0 : bl); if (best >= 0) ui_badge(120, y + 12, b, UI_BLUE, UI_WHITE);
            ui_bar(170, y + 5, 108, 6, q * 100 / 256, q > 180 ? UI_GREEN : q > 110 ? UI_AMBER : UI_RED);
            if (played) sprintf(b, "%d%% OF NOTES PLAYED", (played - miss) * 100 / played); else strcpy(b, "RESTING"); ui_textc(224, y + 15, 1, b, UI_SOFT);
        }
        { int e = band_dance(); ui_round(18, t + 144, 270, 18, 6, UI_WHITE, UI_PANEL, 0); ui_text(24, t + 149, 1, "STAGE SHOW", UI_INK); ui_bar(170, t + 149, 108, 6, e * 100 / 256, UI_AMBER); }
        if (msg) { int w = ui_tw(1, msg) + 16; ui_round(160 - w / 2, 190, w, 16, 6, UI_WHITE, UI_PANEL, 0); ui_textc(160, 195, 1, msg, UI_INK); }
        ui_hints(160, 214, "LEFT RIGHT NEXT SONG|X AGAIN|O STOP", 1);
    }
}
__attribute__((section(".ovl_head"), used)) const OvlHead ovl_head = { OVL_MAGIC, super_open, super_update, super_draw };
