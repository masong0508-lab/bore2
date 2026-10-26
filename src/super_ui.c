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

enum { L_HOME, L_MEM, L_PICK, L_ROLE, L_SET, L_REH };
enum { H_NAME, H_GENRE, H_LOGO, H_TONE, H_MEM, H_SET, H_REH, H_N };
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
static void reh_start(int s) {
    song = s; reht = 0; rehacc = 0;
    int j = audio_song_find(bandsong[s].song);
    if (j < 0) { say("THE SONG CANNOT BE FOUND"); rehOn = 0; return; }
    audio_jukebox_play(j);
    int q[6] = { 256, 0, 0, 0, 0, 256 };
    for (int r = 1; r <= 4; r++) q[r] = band_quality(ROLE_SK[r - 1]);
    audio_band_set(bandsong[s].role, q); rehOn = 1;
}
static void reh_train(void) {   // every five seconds of playing: each member learns the parts they play (two parts at once learn more slowly)
    for (int i = 0; i < BAND_MAX; i++) {
        const BandMem *m = &band.m[i]; if (!m->kind) continue;
        int two = m->role[0] && m->role[1] && band_role_skill(m->role[0]) != band_role_skill(m->role[1]);
        for (int k = 0; k < 2; k++) { int sk = band_role_skill(m->role[k]); if (sk < 0) continue; band_train(i, sk, two ? 5 : 8); }
    }
}

void super_open(int arg) {
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
    } else {   // the rehearsal
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
    ui_backdrop();
    char b[64];
    int t = ui_window(10, 6, 300, 228, band.name, LOGO[band.logo & 7]);
    if (lv == L_HOME) {
        static const char *const nm[H_N] = { "BAND NAME", "GENRE", "LOGO", "COLOUR", "MEMBERS", "SETLIST", "REHEARSE" };
        static const s8 ic[H_N] = { IC_BOOK, IC_NOTE, IC_STAR, IC_BRUSH, IC_PEOPLE, IC_NOTE, IC_PLAY };
        for (int i = 0; i < H_N; i++) {
            int y = t + 2 + i * 21, on = i == sel;
            ui_row(18, y, 170, 18, nm[i], ic[i], on);
            if (i == H_NAME) ui_textfit(104, y + 5, 80, band.name, UI_BLUE2);
            else if (i == H_GENRE) ui_textfit(104, y + 5, 80, GN[band.genre % 12], UI_BLUE2);
            else if (i == H_LOGO) ui_icon(104, y + 3, LOGO[band.logo & 7], TONE[band.tone & 7]);
            else if (i == H_TONE) { ui_round(104, y + 4, 40, 10, 4, TONE[band.tone & 7], TONE[band.tone & 7], 0); }
            else if (i == H_MEM) { sprintf(b, "%d / %d", band_count(), BAND_MAX); ui_badge(104, y + 3, b, UI_BLUE, UI_WHITE); }
            else if (i == H_SET) { sprintf(b, "%d / %d", setn(), BAND_SET); ui_badge(104, y + 3, b, UI_BLUE, UI_WHITE); }
        }
        ui_round(196, t + 2, 104, 148, 7, UI_WHITE, UI_PANEL, 0);
        ui_icon(236, t + 8, LOGO[band.logo & 7], TONE[band.tone & 7]); ui_textc(248, t + 30, 1, band.name, UI_INK); ui_textc(248, t + 42, 1, GN[band.genre % 12], UI_SOFT);
        sprintf(b, "FAME %d", band.fame); ui_textc(248, t + 58, 1, b, UI_INK); sprintf(b, "FANS %d", band.fans); ui_textc(248, t + 70, 1, b, UI_INK);
        for (int k = 0; k < 4; k++) { sprintf(b, "%s", SKN[ROLE_SK[k]]); ui_text(204, t + 88 + k * 14, 1, b, UI_SOFT); ui_bar(250, t + 90 + k * 14, 44, 5, band_quality(ROLE_SK[k]) * 100 / 256, UI_GREEN); }
        ui_text(204, t + 144, 1, "PLAY", UI_SOFT);
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
