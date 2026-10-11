// BORE 2 - the sequel to BORE (GBA), on the PlayStation. True 3D: the voxel people, the rooms and the items are real geometry.
#include "common.h"
#include "gfx.h"
#include "pad.h"
#include "crowd.h"
#include "atlas.h"
#include "look.h"
#include "voxel.h"
#include "items.h"
#include "world.h"
#include "sim.h"
#include "citizens.h"
#include "wants.h"
#include "audio.h"
#include "font.h"

// ---- input ----
static u8 pad_buf[2][34];
static u16 btn, btn_prev;
static int ls_x, ls_y, rs_x, rs_y, analog;
#define HELD(b) (!(btn & (b)))
#define PRESSED(b) (!(btn & (b)) && (btn_prev & (b)))
static void read_pad(void) {
    pad_read(0, pad_buf[0]);
    PADTYPE *pad = (PADTYPE *)pad_buf[0];
    btn_prev = btn; btn = 0xffff; analog = 0; ls_x = ls_y = rs_x = rs_y = 0;
    if (pad->stat == 0 && (pad->type == 0x4 || pad->type == 0x5 || pad->type == 0x7)) {
        btn = pad->btn;
        if (pad->type == 0x5 || pad->type == 0x7) {
            analog = 1;
            ls_x = pad->ls_x - 128; ls_y = pad->ls_y - 128; rs_x = pad->rs_x - 128; rs_y = pad->rs_y - 128;
            if (ABS(ls_x) < 20) ls_x = 0; if (ABS(ls_y) < 20) ls_y = 0; if (ABS(rs_x) < 20) rs_x = 0; if (ABS(rs_y) < 20) rs_y = 0;
        }
    }
}

// ---- colours ----
#define GOLD 255, 200, 40
#define WHITE 255, 255, 255
#define GREY 150, 150, 160
#define DIM 90, 90, 100
#define INK 20, 22, 30

// ---- camera ----
static int cam_yaw = 2600, cam_pitch = 640, cam_dist = 1500;   // yaw 0 = looking north from the south
static int cam_fx, cam_fy, cam_fz;                            // the focus (smoothed)
static int cam_mode = 0;                                       // 0 follow, 1 high, 2 low chase, 3 free
static void camera_apply(int fx, int fy, int fz) {
    cam_fx += (fx - cam_fx) / 6; cam_fy += (fy - cam_fy) / 6; cam_fz += (fz - cam_fz) / 6;
    cam_pitch = CLAMP(cam_pitch, 150, 1000);
    cam_dist = CLAMP(cam_dist, 600, 3600);
    int cp = icos(cam_pitch), sp = isin(cam_pitch);
    int horiz = (cam_dist * cp) >> 12, up = (cam_dist * sp) >> 12;
    cam.at.vx = cam_fx; cam.at.vy = cam_fy; cam.at.vz = cam_fz;
    cam.eye.vx = cam_fx - ((isin(cam_yaw) * horiz) >> 12);
    cam.eye.vz = cam_fz - ((icos(cam_yaw) * horiz) >> 12);
    cam.eye.vy = cam_fy - up;
    gfx_camera_update();
}
static void camera_input(void) {
    if (HELD(PAD_L1)) cam_yaw -= 40;
    if (HELD(PAD_R1)) cam_yaw += 40;
    if (HELD(PAD_L2)) cam_dist += 30;
    if (HELD(PAD_R2)) cam_dist -= 30;
    if (analog) { cam_yaw += rs_x / 3; cam_pitch += rs_y / 4; }
    cam_yaw &= 4095;
}

// ---- the household ----
static Bore edit;              // the creature on the creator's table
static int frame;
static int t_vb;

// ---- states ----
enum { ST_TITLE, ST_MENU, ST_CREATE, ST_LIFE, ST_PAUSE, ST_BUILD, ST_JUKEBOX, ST_HELP, ST_TRAVEL, ST_STORY, ST_WANTS, ST_PIE, ST_PANEL };
static int state = ST_TITLE, prev_state = ST_TITLE;
static int menu_sel;
static int create_tab, create_row, create_from, create_turn;
static int build_tool, build_item, build_x = 8, build_y = 8, build_fl = 1, build_wp = 1, build_lvl, build_peek;
static int juke_sel, juke_scroll;
static int life_started;
static int help_page;
static int hb_now(void) { return VSync(1); }   // hblanks since the last VSync(0) (the flip): 263 per vblank, 525 = one 30 FPS frame
static int t_cpu, t_frame0, t_bub, t_hud, t_upd;
static int show_fps = 0, fps_val, fps_frames, fps_vb, prims_last, t_world, t_sims, t_logic;
static int travel_sel;
static char pie_opt[8][16]; static int pie_n, pie_sel, pie_cit;   // the pie menu
static int panel_tab;                                      // the Bore panel's tab

static void draw_panel(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, 13, 15, 21, 0);   // a solid card: the blend here is additive, so "dark glass" is impossible
    gfx_rect(x, y, w, h, 10, 12, 18, 1);   // twice: the room behind fades to a quarter, so the text owns the panel
    gfx_rect(x, y, w, 1, 255, 200, 40, 0);
    gfx_rect(x, y + h - 1, w, 1, 60, 60, 70, 0);
}

static void menu_row(int x, int y, int w, const char *txt, int sel) {   // one Sims-style menu line: highlight bar, gem, label
    if (sel) { gfx_rect(x, y - 2, w, 11, 28, 48, 62, 0); gfx_rect(x, y - 2, 1, 11, 255, 200, 40, 0); gfx_text(x + 4, y, 1, ">", GOLD); }
    gfx_text(x + 14, y, 1, txt, sel ? 255 : 175, sel ? 255 : 175, sel ? 255 : 185);
}
// which story zones are open (the island comes with the first paycheck, the town with the SPONSORED job level)
static int zone_unlocked(int z) { return z == 0 || z == 3 || (z == 1 && story_paid > 0) || (z == 2 && job_level >= 2); }   // (the city is open from the start)

static void draw_bar(int x, int y, int w, int v, int r, int g, int b) {
    gfx_rect(x, y, w, 6, 30, 30, 36, 0);
    int fw = (w - 2) * v / 100;
    gfx_rect(x + 1, y + 1, fw, 4, r, g, b, 0);
}

// ---- Sims-style widgets: motive bars, the mood meter, portraits, tabs, the notification feed, the action card ----
static void need_rgb(int v, int *r, int *g, int *b) {   // a Sims motive bar goes green, then yellow, then red
    if (v >= 60) { *r = 90; *g = 220; *b = 90; } else if (v >= 30) { *r = 240; *g = 210; *b = 60; } else { *r = 255; *g = 80; *b = 70; }
}
static void draw_motive(int x, int y, int w, int v) {   // ten notches
    int r, g, b; need_rgb(v, &r, &g, &b);
    gfx_rect(x, y, w, 6, 30, 30, 36, 0);
    gfx_rect(x + 1, y + 1, (w - 2) * CLAMP(v, 0, 100) / 100, 4, r, g, b, 0);
    for (int k = 1; k < 10; k++) gfx_rect(x + w * k / 10, y + 1, 1, 4, 30, 30, 36, 0);
}
static void draw_mood_meter(int x, int y, int w, int m) {   // -100..100, growing out from the middle
    gfx_rect(x, y, w, 6, 30, 30, 36, 0);
    int half = (w - 2) / 2, mid = x + 1 + half, len = half * ABS(m) / 100;
    if (m >= 0) gfx_rect(mid, y + 1, len, 4, 90, 220, 90, 0); else gfx_rect(mid - len, y + 1, len, 4, 255, 80, 70, 0);
    gfx_rect(mid, y - 1, 1, 8, 200, 200, 210, 0);
}
static void draw_face_k(int x, int y, int m, int k) {   // the mood face, k times bigger
    int r = m > 15 ? 120 : m > -15 ? 230 : 255, g = m > 15 ? 230 : m > -15 ? 210 : 100, bb = m > 15 ? 90 : m > -15 ? 70 : 90;
    gfx_rect(x + 2 * k, y, 12 * k, 16 * k, r, g, bb, 0); gfx_rect(x, y + 2 * k, 16 * k, 12 * k, r, g, bb, 0);
    gfx_rect(x + 4 * k, y + 5 * k, 2 * k, 3 * k, INK, 0); gfx_rect(x + 10 * k, y + 5 * k, 2 * k, 3 * k, INK, 0);
    if (m > 15) { gfx_rect(x + 4 * k, y + 10 * k, 8 * k, k, INK, 0); gfx_rect(x + 3 * k, y + 9 * k, k, k, INK, 0); gfx_rect(x + 12 * k, y + 9 * k, k, k, INK, 0); }
    else if (m > -15) gfx_rect(x + 4 * k, y + 10 * k, 8 * k, k, INK, 0);
    else { gfx_rect(x + 4 * k, y + 11 * k, 8 * k, k, INK, 0); gfx_rect(x + 3 * k, y + 12 * k, k, k, INK, 0); gfx_rect(x + 12 * k, y + 12 * k, k, k, INK, 0); }
}
static void draw_portrait(int x, int y, const Sim *s, int sel) {   // a household portrait: the mood face in a frame, gold for who you play
    gfx_rect(x, y, 22, 22, sel ? 255 : 60, sel ? 200 : 60, sel ? 40 : 70, 0);
    gfx_rect(x + 1, y + 1, 20, 20, 13, 15, 21, 0);
    draw_face_k(x + 3, y + 3, s->mood, 1);
}
static void draw_tabs(int x, int y, int w, const char *const *nm, int n, int sel) {   // the Sims tab strip
    int tw = w / n;
    for (int i = 0; i < n; i++) {
        int tx = x + i * tw, on = i == sel;
        gfx_rect(tx, y, tw - 2, 11, on ? 28 : 13, on ? 48 : 15, on ? 62 : 21, 0);
        if (on) gfx_rect(tx, y + 11, tw - 2, 1, GOLD, 0);
        gfx_textc(tx + (tw - 2) / 2, y + 2, 1, nm[i], on ? 255 : 150, on ? 255 : 150, on ? 255 : 160);
    }
}
static const char *act_name(int a) {
    switch (a) {
        case ACT_EAT: return "EATING"; case ACT_WC: return "USING THE TOILET"; case ACT_SLEEP: return "SLEEPING"; case ACT_SHOWER: return "SHOWERING";
        case ACT_SIT: return "RELAXING"; case ACT_FUN: return "HAVING FUN"; case ACT_TALK: return "CHATTING"; case ACT_WORK: return "WORKING";
        case ACT_WALK: return "WALKING"; case ACT_COFFEE: return "HAVING COFFEE"; case ACT_READ: return "READING"; case ACT_RUN: return "EXERCISING";
        case ACT_PHONE: return "ON THE PHONE"; case ACT_TRAVEL: return "TRAVELLING"; case ACT_STAIRS: return "ON THE STAIRS";
    }
    return "";
}
static int act_seen, act_total;
static void draw_action_card(const Sim *s) {   // what the Bore is doing, with its progress (the Sims 1 queue, one card)
    if (s->action != act_seen) { act_seen = s->action; act_total = s->act_t; }
    if (s->action == ACT_NONE || s->action == ACT_WALK) return;
    const char *nm = act_name(s->action);
    int w = MAX(gfx_text_w(1, nm) + 12, 84);
    gfx_rect(4, 172, w, 14, 13, 15, 21, 0); gfx_rect(4, 172, 2, 14, GOLD, 0);
    gfx_text(10, 174, 1, nm, 235, 235, 245);
    if (act_total > 0 && act_total < 9999) gfx_rect(10, 183, (w - 10) * CLAMP(100 - s->act_t * 100 / act_total, 0, 100) / 100, 2, 120, 255, 160, 0);
}
#define NFEED 3
static char feed[NFEED][32]; static int feed_t[NFEED], prev_note_t;
static void feed_push(const char *t) {
    for (int i = NFEED - 1; i > 0; i--) { strcpy(feed[i], feed[i - 1]); feed_t[i] = feed_t[i - 1]; }
    strncpy(feed[0], t, 31); feed[0][31] = 0; feed_t[0] = 300;
}
static void feed_poll(void) { if (last_note_t > prev_note_t) feed_push(last_note); prev_note_t = last_note_t; }   // every sim_note lands here
static void feed_draw(void) {   // the notifications stack on the left, newest on top, each fades on its own
    int y = 38;
    for (int i = 0; i < NFEED; i++) {
        if (feed_t[i] <= 0) continue;
        int w = gfx_text_w(1, feed[i]) + 12;
        gfx_rect(4, y, w, 12, 13, 15, 21, 0); gfx_rect(4, y, 2, 12, GOLD, 0);
        gfx_text(10, y + 3, 1, feed[i], 235, 235, 245);
        feed_t[i]--; y += 14;
    }
}

// ---- the life ----
static void start_life(void) {
    if (life_started) return;
    loading("MAKING A NEW LIFE", 40);
    world_reset_all();
    sim_init();
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 5; py = 12; }
    sim_add(&edit, px, py, 0);
    // two housemates from the dice
    Bore b; look_default(&b); b.stage = AG_ADULT; look_random(&b); sim_add(&b, px + 2, py + 1, 1);
    strcpy(sims[1].bore.first, "MOSS"); sims[1].grump = 1;   // MOSS will not have a thing floating overhead. Do not ask.
    look_default(&b); b.stage = AG_CHILD; look_random(&b); sim_add(&b, px - 1, py + 2, 1);
    life_started = 1;
    story_paid = 0; story_talks = 0; story_visit[0] = 1; story_visit[1] = 0; story_visit[2] = 0; story_visit[3] = 0;
    citizens_init();   // a hundred civilians out in the island, the town and the city
    sim_note("WELCOME HOME. X USES THINGS.");
}

static void draw_marker(const Sim *s) {   // who you are playing: a bobbing gold arrow. (It used to be a green diamond. MOSS won.)
    VECTOR p; setVector(&p, s->x, -s->lvl * WALL_FULL, s->z);
    int top = s->bore.w_heady - 34 - (isin((frame * 70) & 4095) >> 9);   // just over this Bore's own head
    gfx_set_object(&p, cam_yaw & 4095);                       // flat to the camera
    SVECTOR a, b, c, d;
    setVector(&a, -12, top - 15, 0); setVector(&b, 12, top - 15, 0); setVector(&c, 0, top, 0);
    gfx_tri_f(&a, &b, &c, 255, 204, 48, 0);
    setVector(&a, -4, top - 34, 0); setVector(&b, 4, top - 34, 0); setVector(&c, -4, top - 15, 0); setVector(&d, 4, top - 15, 0);
    gfx_quad_f(&a, &b, &c, &d, 255, 204, 48, 0);
    setVector(&a, -12, top - 13, -2); setVector(&b, 12, top - 13, -2); setVector(&c, 0, top + 2, -2);
    gfx_tri_f(&a, &b, &c, 90, 60, 10, 0);                     // a dark edge behind, so it reads on bright rooms
}
#define LOD_DIST 1280   // beyond this many world units (5 tiles) from the camera's focus, people are drawn with the crowd model
static void draw_sims(void) {
    gfx_set_reserve(RESERVE_UI);   // the people may use what the world left, but not the HUD's share
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->lvl > world_view_lvl) continue;                      // upstairs, out of the floor view
        if (!gfx_visible(s->x, -110 - s->lvl * WALL_FULL, s->z, 150)) continue;   // off the screen
        VECTOR p; setVector(&p, s->x, 0, s->z);
        if (s->action == ACT_SLEEP) p.vy = -items_height('S') + 40;
        if (s->action == ACT_SIT || s->action == ACT_WC) p.vy = -items_height(world_at(s->act_x, s->act_y)) / 2 + 10;
        s->gait.mood = s->mood; s->gait.talk = s->action == ACT_TALK; s->gait.seed = i; s->gait.clock = frame;
        p.vy -= s->lvl * WALL_FULL;
        {   // far from where you are looking: the same person at half the voxel resolution (the crowd model); close up, the full model
            int fdx = s->x - cam.at.vx, fdz = s->z - cam.at.vz;
            PROF_BEGIN();
            int edx = s->x - cam.eye.vx, edy = -110 - s->lvl * WALL_FULL - cam.eye.vy, edz = s->z - cam.eye.vz;
            if (i != player && (fdx * fdx + fdz * fdz > LOD_DIST * LOD_DIST || edx * edx + edy * edy + edz * edz > 2900 * 2900)) { LodBore lb; voxel_lod_build(&s->bore, &lb); voxel_draw_lod(&lb, &p, s->yaw, &s->gait); }
            else voxel_draw(&s->bore, &p, s->yaw, &s->gait);
            PROF_END(4);
        }
        if (s->action != ACT_SLEEP && s->lvl == 0) { VECTOR sp; setVector(&sp, s->x, 0, s->z); gfx_shadow(&sp, 70); }
    }
    citizens_draw();
    if (nsims && !sims[player].grump) draw_marker(&sims[player]);   // MOSS gets nothing. MOSS made that very clear.
    gfx_set_reserve(0);
}

static void draw_bubbles(void) {   // thoughts and notes over heads
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->think < 0 && s->note_t <= 0) continue;
        if (s->lvl > world_view_lvl) continue;
        VECTOR p; setVector(&p, s->x, -s->lvl * WALL_FULL, s->z);
        gfx_set_object(&p, 0);
        SVECTOR top; setVector(&top, 0, -H * VOX - 40, 0);
        int sx, sy, sz;
        if (!gfx_project(&top, &sx, &sy, &sz)) continue;
        const char *txt = s->note_t > 0 ? s->note : sim_need_name(s->think);
        int w = gfx_text_w(1, txt) + 8;
        gfx_rect(sx - w / 2, sy - 12, w, 12, 245, 245, 250, 0);
        gfx_text(sx - w / 2 + 4, sy - 10, 1, txt, s->note_t > 0 ? 40 : 200, 40, 40);
        gfx_rect(sx - 2, sy, 4, 3, 245, 245, 250, 0);
    }
    citizens_draw_bubbles();
}

static void draw_mood_face(int x, int y, int m) {   // a little round face: the mood, like a Sims portrait's plumbob colour
    int r = m > 15 ? 120 : m > -15 ? 230 : 255, g = m > 15 ? 230 : m > -15 ? 210 : 100, bb = m > 15 ? 90 : m > -15 ? 70 : 90;
    gfx_rect(x + 2, y, 12, 16, r, g, bb, 0); gfx_rect(x, y + 2, 16, 12, r, g, bb, 0);
    gfx_rect(x + 4, y + 5, 2, 3, INK, 0); gfx_rect(x + 10, y + 5, 2, 3, INK, 0);
    if (m > 15) { gfx_rect(x + 4, y + 10, 8, 1, INK, 0); gfx_rect(x + 3, y + 9, 1, 1, INK, 0); gfx_rect(x + 12, y + 9, 1, 1, INK, 0); }
    else if (m > -15) gfx_rect(x + 4, y + 10, 8, 1, INK, 0);
    else { gfx_rect(x + 4, y + 11, 8, 1, INK, 0); gfx_rect(x + 3, y + 12, 1, 1, INK, 0); gfx_rect(x + 12, y + 12, 1, 1, INK, 0); }
}
static const char *const aspLvNm[4] = { "RED", "GREEN", "GOLD", "PLATINUM" };
static const int aspLvCol[4][3] = { {255,80,70}, {90,220,90}, {255,200,40}, {220,235,255} };
static void hud_life(void) {
    Sim *s = &sims[player];
    char b[48];
    for (int i = 0; i < nsims; i++) draw_portrait(4 + i * 22, 4, &sims[i], i == player);   // the household, Sims 2 style
    gfx_text(4, 28, 1, s->bore.first, 255, 230, 120);
    draw_action_card(s);
    // the bottom bar: needs in two columns, the Bore in the middle, the hint on the right
    gfx_rect(0, 188, 320, 52, 8, 10, 22, 1);
    gfx_rect(0, 188, 320, 1, 255, 200, 40, 0);
    static const int col[NEED_N][3] = { {255,160,40}, {80,160,255}, {160,100,255}, {60,220,220}, {255,120,200}, {255,230,60}, {120,255,120}, {200,200,200} };
    for (int n = 0; n < NEED_N; n++) {
        int v = sim_need_val(s, n), cx = (n < 4) ? 6 : 112, cy = 194 + (n & 3) * 11;
        gfx_text(cx, cy, 1, sim_need_name(n), v < 20 ? 255 : 190, v < 20 ? 70 : 190, v < 20 ? 70 : 200);
        draw_motive(cx + 42, cy + 1, 54, v);
    }
    // the Bore: name, mood
    draw_mood_face(218, 194, s->mood);
    gfx_text(238, 192, 2, s->bore.first, WHITE);
    gfx_text(238 + gfx_text_w(2, s->bore.first) + 4, 197, 1, s->bore.last, GREY);
    int m = s->mood;
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    sprintf(b, "%s  LVL %d %s", mood, job_level, (clock_day < 5 && clock_min >= 540 && clock_min < 1020) ? "SHIFT" : "");
    gfx_text(238, 206, 1, b, m > 15 ? 140 : m > -15 ? 220 : 255, m > 15 ? 255 : m > -15 ? 220 : 110, 130);
    draw_mood_meter(238, 214, 76, s->mood);
    const char *hint = sim_hint();
    { const char *ch = citizens_hint(); if (ch[0]) hint = ch; }
    if (hint[0]) { sprintf(b, "X: %s", hint); gfx_text(238, 223, 1, b, GOLD); }
    else gfx_text(238, 223, 1, "START: MENU", DIM);
    // the clock and the money, top right
    gfx_rect(226, 4, 90, 26, 8, 10, 22, 1); gfx_rect(226, 4, 90, 1, 255, 200, 40, 0);
    sprintf(b, "%s %02d:%02d", sim_day_name(clock_day), clock_min / 60, clock_min % 60);
    gfx_text(232, 7, 2, b, 255, 230, 120);
    sprintf(b, "$%ld", cash);
    gfx_text(232, 20, 1, b, 120, 255, 120);
    for (int k = 0; k < 3; k++) { int on = k < sim_speed; gfx_rect(296 + k * 6, 21, 4, 6, on ? 255 : 70, on ? 140 : 70, on ? 60 : 80, 0); }   // speed: 1, 2 or 3 pips
    { int day_ = clock_min >= 360 && clock_min < 1200;   // a sun by day, a moon by night
      gfx_rect(282, 20, 6, 10, day_ ? 255 : 200, day_ ? 210 : 210, day_ ? 60 : 255, 0); gfx_rect(280, 22, 10, 6, day_ ? 255 : 200, day_ ? 210 : 210, day_ ? 60 : 255, 0);
      if (!day_) gfx_rect(285, 21, 4, 5, 13, 15, 21, 0); }
    {   // the wants: the meter beside the face, the current want under the clock
        int lv = wants_level(s);
        gfx_rect(214, 230, 20, 3, 30, 32, 40, 0); gfx_rect(214, 230, 20 * s->asp_pts / ASP_MAX, 3, aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2], 0);
        char t[32]; wants_text(s, (frame / 240) & 3, t);
        sprintf(b, "WANT: %s", t); int w = gfx_text_w(1, b) + 8;
        gfx_rect(316 - w, 32, w, 10, 8, 10, 22, 1); gfx_text(320 - w, 33, 1, b, 200, 220, 255);
        if (wants_flash_t > 0) gfx_textc(160, 60, 2, wants_flash, wants_flash[0] == 'W' ? 120 : 255, wants_flash[0] == 'W' ? 255 : 110, 120);
    }
    feed_draw();
    citizens_hud();
    if (frame < 900) gfx_textc(160, 160, 1, "STICK MOVES   L1 R1 ORBIT   L2 R2 ZOOM   X USES   SQUARE RUNS", GREY);
    if (show_fps) { extern int vox_t_hair, vox_t_block, vox_t_face, vox_n_hair, vox_n_block;
        gfx_rect(2, 57, 316, 86, 0, 0, 0, 0);
        sprintf(b, "%d FPS  %d PRIMS %dK%s", fps_val, gfx_last_prims, gfx_last_bytes >> 10, gfx_last_overflow ? " FULL!" : ""); gfx_text(6, 60, 1, b, 120, 255, 120);
        { extern int gfx_last_wait; static int mx_, mx_shown, mx_n; if (t_cpu > mx_) mx_ = t_cpu; if (++mx_n >= 60) { mx_shown = mx_; mx_ = 0; mx_n = 0; }
          static int mw, ms, ml, mu, mbb, mh, mws, mss, mls, mus, mbs, mhs; if (t_world > mw) mw = t_world; if (t_sims > ms) ms = t_sims; if (t_logic > ml) ml = t_logic;
          if (t_upd > mu) mu = t_upd; if (t_bub > mbb) mbb = t_bub; if (t_hud > mh) mh = t_hud;
          if (mx_n == 0) { mws = mw; mss = ms; mls = ml; mus = mu; mbs = mbb; mhs = mh; mw = ms = ml = mu = mbb = mh = 0; }
          sprintf(b, "MAX%d W%d B%d S%d U%d BB%d H%d", mx_shown, mws, mss, mls, mus, mbs, mhs); }
        gfx_text(6, 69, 2, b, 120, 255, 120);
        { extern int cit_n_full, cit_n_lod; sprintf(b, "FL%d/%d WA%d/%d IT%d/%d RF%d/%d", gfx_prof_last[0], gfx_profn_last[0], gfx_prof_last[1], gfx_profn_last[1], gfx_prof_last[2], gfx_profn_last[2], gfx_prof_last[3], gfx_profn_last[3]);
        gfx_text(6, 88, 2, b, 255, 255, 120); sprintf(b, "SM%d/%d CF%d/%d CL%d/%d X%d", gfx_prof_last[4], gfx_profn_last[4], gfx_prof_last[5], gfx_profn_last[5], gfx_prof_last[6], gfx_profn_last[6], gfx_prof_last[7]); }
        gfx_text(6, 106, 2, b, 255, 255, 120);
        { extern int vox_n_rad; sprintf(b, "JNT%d PREP%d LUT%d RAD%d CELLS%d FH%d", vox_t_face, vox_n_hair, vox_n_block, vox_n_rad, vox_t_block, vox_t_hair); vox_n_rad = 0; } gfx_text(6, 124, 2, b, 255, 200, 200); if (0) gfx_text(6, 69, 2, b, 120, 255, 120);
        { extern int prof_on, prof_total; extern int prof_top(int, u32 *); prof_on = 1;   // the sampling profiler's busiest code (addresses: nm -n bore2.elf)
          static char pl[9][16]; static int pn;
          if ((frame & 127) == 0) { pn = 0; for (int k = 0; k < 9; k++) { u32 ad = 0; int n = prof_top(k, &ad); if (!n) break; sprintf(pl[k], "%05X %d", (unsigned)(ad & 0xfffff), n * 1000 / (prof_total ? prof_total : 1)); pn = k + 1; } }
          gfx_rect(0, 143, 320, 44, 0, 0, 0, 0);
          for (int k = 0; k < pn; k++) gfx_text(4 + (k % 3) * 106, 145 + (k / 3) * 14, 1, pl[k], 255, 255, 255); }
        vox_t_hair = vox_t_block = vox_t_face = vox_n_hair = vox_n_block = 0; }
}

static void life_update(int steps) {
    Sim *s = &sims[player];
    // movement: the stick relative to the camera
    int mx = 0, mz = 0;
    if (analog && (ls_x || ls_y)) { mx = ls_x * 32; mz = -ls_y * 32; }
    else {
        if (HELD(PAD_LEFT)) mx = -4096; if (HELD(PAD_RIGHT)) mx = 4096;
        if (HELD(PAD_UP)) mz = 4096; if (HELD(PAD_DOWN)) mz = -4096;
    }
    if (mx || mz) {   // turn the push by the camera yaw: "up" = away from the camera
        int c = icos(cam_yaw), sn = isin(cam_yaw);
        int wx = (mx * c + mz * sn) >> 12, wz = (mz * c - mx * sn) >> 12;
        for (int k = 0; k < steps; k++) sim_player_move(wx, wz, HELD(PAD_SQUARE));
    } else sim_player_move(0, 0, 0);
    if (PRESSED(PAD_CROSS)) {   // X: busy - stop; otherwise the pie menu (one option: just do it)
        Sim *me = &sims[player];
        if (me->action != ACT_NONE && me->action != ACT_WALK) sim_player_interact();
        else if (citizens_punch()) { }   // a fight is on and someone is in reach
        else { pie_cit = 1; pie_n = citizens_pie_open(pie_opt);   // a civilian in front of you first, then the things
               if (!pie_n) { pie_cit = 0; pie_n = sim_pie_open(pie_opt); }
               if (pie_n == 1) { if (pie_cit) citizens_pie_do(0); else sim_pie_do(0); } else if (pie_n > 1) { pie_sel = 0; state = ST_PIE; } else audio_voice(VOICE_NAH); }
    }
    if (PRESSED(PAD_CIRCLE)) sim_player_cancel();
    if (PRESSED(PAD_TRIANGLE)) { sim_speed = sim_speed % 3 + 1; }
    if (PRESSED(PAD_L3)) show_fps ^= 1;
    { extern int getenv_noshadow, dbg_noao, dbg_nograd; if (PRESSED(PAD_R3)) { static int mode; mode = (mode + 1) & 3; getenv_noshadow = mode == 1; dbg_noao = mode == 2; dbg_nograd = mode == 3; } }
    if (PRESSED(PAD_SELECT)) { sim_switch_player(); sim_note("NOW PLAYING AS THE NEXT BORE"); }
    { int t0 = hb_now(); for (int k = 0; k < steps; k++) sim_step(); citizens_update(steps); t_logic = hb_now() - t0; }
    feed_poll();
    camera_input();
    if (PRESSED(PAD_START)) { state = ST_PAUSE; menu_sel = 0; }
    if (travel_asked) { travel_asked = 0; travel_sel = world_zone; state = ST_TRAVEL; }
}

static void life_draw(void) {
    Sim *s = &sims[player];
    camera_apply(s->x, -200 - s->lvl * WALL_FULL, s->z);
    world_view_lvl = s->lvl; world_roof_show = 0;
    // the day-night sky and light
    int t = clock_min;
    int day = (t > 360 && t < 1200) ? 255 : (t >= 1200 && t < 1320) ? 255 - (t - 1200) * 2 : (t >= 300 && t <= 360) ? (t - 300) * 4 : 20;
    day = CLAMP(day, 20, 255);
    gfx_set_clear((20 * 8 * day) >> 8, (26 * 8 * day) >> 8, (31 * 8 * day) >> 8);
    gfx_set_ambient(54 + (day * 36 >> 8), 48 + (day * 60 >> 8));
    {   // the sky: deep blue up top down to a warm haze at the horizon, dimmed by the night
        int tr = (70 * day) >> 8, tg = (120 * day) >> 8, tb = (220 * day) >> 8;
        int br = (215 * day) >> 8, bg = (205 * day) >> 8, bb = (190 * day) >> 8;
        if (world_zone == ZONE_TOWN) { tr = (110 * day) >> 8; tg = (140 * day) >> 8; tb = (200 * day) >> 8; br = (240 * day) >> 8; bg = (190 * day) >> 8; bb = (120 * day) >> 8; }
        if (world_zone == ZONE_ISLAND) { tr = (40 * day) >> 8; tg = (140 * day) >> 8; tb = (230 * day) >> 8; br = (200 * day) >> 8; bg = (230 * day) >> 8; bb = (240 * day) >> 8; }
        gfx_sky((u32)tr | ((u32)tg << 8) | ((u32)tb << 16), (u32)br | ((u32)bg << 8) | ((u32)bb << 16));
    }
    gfx_prim_count = 0;
    { int t0 = hb_now(); world_draw(sim_tile_x(s), sim_tile_y(s), frame, 1); t_world = hb_now() - t0;
      t0 = hb_now(); gfx_prim_count = 0; draw_sims(); t_sims = hb_now() - t0; }
    { int t0 = hb_now(); draw_bubbles(); t_bub = hb_now() - t0; }
    prims_last = gfx_prim_count;
    { int t0 = hb_now(); hud_life(); t_hud = hb_now() - t0; }
}

// ---- travel between the zones ----
static void travel_go(int z) {
    { char b[40]; sprintf(b, "TRAVELLING TO %s", world_zone_name(z)); loading(b, 50); }
    world_switch(z);
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 20; py = 20; }
    for (int i = 0; i < nsims; i++) {
        Sim *sm = &sims[i];
        sim_player_cancel_sim(sm);
        sm->x = (px + (i % 3)) * TILE + TILE / 2; sm->z = -((py + (i / 3)) * TILE + TILE / 2);
        sm->path_n = 0;
    }
    citizens_enter(z);
    char b[40]; sprintf(b, "WELCOME TO %s", world_zone_name(z)); sim_note(b);
    audio_voice(VOICE_YAHOO);
}
static void travel_update(void) {
    if (PRESSED(PAD_UP)) travel_sel = (travel_sel + ZONE_N - 1) % ZONE_N;
    if (PRESSED(PAD_DOWN)) travel_sel = (travel_sel + 1) % ZONE_N;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) {
        if (!zone_unlocked(travel_sel)) { audio_voice(VOICE_NAH); return; }
        if (travel_sel != world_zone) { travel_go(travel_sel); story_visit[travel_sel] = 1; for (int i = 0; i < nsims; i++) { if (travel_sel < 3) wants_event(&sims[i], travel_sel == 1 ? W_ISLAND : W_TOWN, 0); wants_reroll(&sims[i]); } }
        state = ST_LIFE;
    }
}
static void travel_draw(void) {
    life_draw();
    draw_panel(70, 46, 180, 118);
    gfx_textc(160, 52, 2, "WHERE TO?", GOLD);
    static const char *const blurb[ZONE_N] = { "HOME: THE HOUSE, THE FACTORY, THE RAIL PARK", "A SAND ISLAND: A CAMP, PALMS, THE SEA", "A DESERT MAIN STREET: DINER, TRAILER, THE LAB", "THE CITY: 25 BLOCKS, 23 HOMES, PARK, DINER, GYM" };
    static const char *const lockTxt[ZONE_N] = { "", "LOCKED: EARN YOUR FIRST PAYCHECK", "LOCKED: GET SPONSORED AT WORK", "" };
    for (int i = 0; i < ZONE_N; i++) {
        int sel = i == travel_sel, here = i == world_zone, open_ = zone_unlocked(i);
        if (!open_) gfx_text(86, 74 + i * 20, 1, world_zone_name(i), 110, 110, 120);
        else gfx_text(86, 74 + i * 20, 1, world_zone_name(i), sel ? 255 : here ? 120 : 190, sel ? 255 : here ? 120 : 190, sel ? 255 : 130);
        gfx_text(86, 83 + i * 20, 1, !open_ ? lockTxt[i] : here ? "YOU ARE HERE" : blurb[i], DIM);
        if (sel) gfx_text(76, 74 + i * 20, 1, ">", GOLD);
    }
}

// ---- the pause menu ----
static const char *const pauseNm[] = { "RESUME", "BORE PANEL", "THE STORY", "TRAVEL", "CREATE A BORE", "ADD A BORE", "THROW A PARTY", "BUILD MODE", "JUKEBOX", "HOW TO PLAY", "NEW LIFE", "QUIT TO TITLE" };
#define NPAUSE 12
void throw_party(void) {   // (also the phone's THROW A PARTY)
    if (citizens_party_on()) { sim_note("A PARTY IS ALREADY ON"); return; }
    if (cash < 100) { sim_note("A PARTY COSTS $100"); audio_voice(VOICE_NAH); return; }
    int n = citizens_party();
    if (n <= 0) { sim_note("NO ROOM TO DANCE HERE"); audio_voice(VOICE_NAH); return; }
    cash -= 100;
    char b[40]; sprintf(b, "PARTY! %d GUESTS ARE COMING", n); sim_note(b);
    audio_station_play(1);                                            // RAVE 101
    audio_voice(VOICE_YAHOO);
    for (int i = 0; i < nsims; i++) sim_moodlet(&sims[i], 15);        // FUN TIME
}
static void pause_update(void) {
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + NPAUSE - 1) % NPAUSE;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % NPAUSE;
    if (PRESSED(PAD_START) || PRESSED(PAD_CIRCLE)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) {
        switch (menu_sel) {
            case 0: state = ST_LIFE; break;
            case 1: panel_tab = 0; state = ST_PANEL; break;
            case 2: state = ST_STORY; break;
            case 3: travel_sel = world_zone; state = ST_TRAVEL; break;
            case 4: edit = sims[player].bore; create_tab = 0; create_row = 0; prev_state = ST_LIFE; state = ST_CREATE; world_backup(); world_gen_studio(); break;
            case 5: { Bore b; look_default(&b); b.stage = (u8)(AG_CHILD + look_rand() % 3); look_random(&b);
                      sim_add(&b, sim_tile_x(&sims[player]), sim_tile_y(&sims[player]) + 1, 1); sim_note("A NEW BORE MOVED IN"); state = ST_LIFE; } break;
            case 6: throw_party(); state = ST_LIFE; break;
            case 7: build_x = sim_tile_x(&sims[player]); build_y = sim_tile_y(&sims[player]); state = ST_BUILD; break;
            case 8: prev_state = ST_LIFE; state = ST_JUKEBOX; break;
            case 9: prev_state = ST_PAUSE; help_page = 0; state = ST_HELP; break;
            case 10: life_started = 0; start_life(); state = ST_LIFE; break;
            case 11: audio_jukebox_stop(); life_started = 0; state = ST_TITLE; break;
        }
    }
}
static void pause_draw(void) {
    life_draw();
    draw_panel(88, 24, 144, 158);
    gfx_textc(160, 28, 2, "PAUSED", GOLD);
    for (int i = 0; i < NPAUSE; i++) menu_row(94, 48 + i * 11, 132, pauseNm[i], i == menu_sel);
}

// ---- the pie menu (Sims 1 & 2): the choices round a circle, the d-pad points at one ----
static void pie_update(void) {
    int dx = HELD(PAD_RIGHT) - HELD(PAD_LEFT), dy = HELD(PAD_DOWN) - HELD(PAD_UP);
    if (analog && (ABS(ls_x) > 60 || ABS(ls_y) > 60)) { dx = ls_x; dy = ls_y; }
    if ((dx || dy) && (PRESSED(PAD_LEFT) || PRESSED(PAD_RIGHT) || PRESSED(PAD_UP) || PRESSED(PAD_DOWN) || analog)) {   // the option nearest to where the d-pad points
        int best = pie_sel, bd = 1 << 30;
        for (int i = 0; i < pie_n; i++) {
            int a = (i * 4096 / pie_n) & 4095, ox = isin(a), oy = -icos(a);
            int d = -(ox * dx + oy * dy) / (ABS(dx) + ABS(dy));
            if (d < bd) { bd = d; best = i; }
        }
        pie_sel = best;
    }
    if (PRESSED(PAD_CROSS)) { state = ST_LIFE; if (pie_cit) citizens_pie_do(pie_sel); else sim_pie_do(pie_sel); }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START) || PRESSED(PAD_TRIANGLE)) state = ST_LIFE;
}
static void pie_draw(void) {
    life_draw();
    int cx = 160, cy = 100, R = pie_n > 4 ? 56 : 46;
    for (int k = 0; k < 24; k++) {   // the ring, and a spoke out to the choice
        int a0 = k * 4096 / 24, a1 = (k + 1) * 4096 / 24;
        gfx_line(cx + ((isin(a0) * R) >> 12), cy - ((icos(a0) * R * 3 / 4) >> 12), cx + ((isin(a1) * R) >> 12), cy - ((icos(a1) * R * 3 / 4) >> 12), 70, 80, 100);
    }
    { int a = (pie_sel * 4096 / pie_n) & 4095; gfx_line(cx, cy, cx + ((isin(a) * R) >> 12), cy - ((icos(a) * R * 3 / 4) >> 12), GOLD); }
    { const char *t = pie_cit ? citizens_pie_title() : sim_pie_title(); int w = gfx_text_w(1, t) + 10; gfx_rect(cx - w / 2, cy - 6, w, 12, 13, 15, 21, 0); gfx_rect(cx - w / 2, cy + 5, w, 1, 255, 200, 40, 0); gfx_textc(cx, cy - 4, 1, t, GOLD); }
    for (int i = 0; i < pie_n; i++) {
        int a = (i * 4096 / pie_n) & 4095, x = cx + ((isin(a) * R) >> 12), y = cy - ((icos(a) * R * 3 / 4) >> 12);
        int w = gfx_text_w(1, pie_opt[i]) + 14, sel = i == pie_sel;
        gfx_rect(x - w / 2, y - 7, w, 14, sel ? 40 : 13, sel ? 70 : 15, sel ? 50 : 21, 0);
        gfx_rect(x - w / 2, y - 7, w, 1, sel ? 255 : 80, sel ? 200 : 90, sel ? 40 : 110, 0); gfx_rect(x - w / 2, y + 6, w, 1, sel ? 255 : 50, sel ? 200 : 55, sel ? 40 : 70, 0);
        gfx_textc(x, y - 3, 1, pie_opt[i], sel ? 255 : 210, sel ? 255 : 210, sel ? 255 : 220);
    }
    gfx_textc(160, 168, 1, "D-PAD PICKS   X DO IT   O NEVER MIND", GREY);
}

// ---- the wants and fears page ----
static void wants_update(void) {
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_CROSS) || PRESSED(PAD_START)) state = ST_PAUSE;
}
static void wants_draw(void) {
    life_draw();
    Sim *s = &sims[player];
    draw_panel(30, 14, 260, 178);
    char b[64], t[32];
    sprintf(b, "%s WANTS", s->bore.first); gfx_textc(160, 18, 2, b, GOLD);
    { const char *tr[3]; int n = sim_traits(s, tr); sprintf(b, "%s", look_asp_name(s->bore.asp));
      for (int i = 0; i < n; i++) { strcat(b, i ? ", " : "  -  "); strcat(b, tr[i]); }
      gfx_text(40, 36, 1, b, DIM); }
    int lv = wants_level(s);
    gfx_rect(54, 46, 212, 6, 30, 32, 40, 0);
    gfx_rect(54, 46, 212 * s->asp_pts / ASP_MAX, 6, aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2], 0);
    for (int k = 1; k < 4; k++) { int x = 54 + 212 * (k == 1 ? 3000 : k == 2 ? 7000 : 10000) / ASP_MAX; gfx_rect(x, 45, 1, 8, 13, 15, 21, 0); }
    sprintf(b, "%s%s", aspLvNm[lv], lv == 3 ? " - NOTHING CAN BRING THEM DOWN" : wants_desperate(s) ? " - DESPERATE" : ""); gfx_text(54, 55, 1, b, aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2]);
    gfx_text(40, 68, 1, "WANTS", GOLD);
    for (int i = 0; i < 4; i++) { wants_text(s, i, t); gfx_text(48, 78 + i * 10, 1, t, 230, 230, 240); sprintf(b, "%d", s->want[i].pts); gfx_text(128, 78 + i * 10, 1, b, 90, 230, 90); }
    gfx_text(40, 122, 1, "FEARS", 255, 120, 110);
    for (int i = 0; i < 2; i++) { fears_text(s, i, t); gfx_text(48, 132 + i * 10, 1, t, 230, 200, 200); sprintf(b, "-%d", s->fear[i].pts); gfx_text(128, 132 + i * 10, 1, b, 255, 100, 90); }
    gfx_rect(160, 66, 1, 90, 60, 60, 70, 0);
    gfx_text(168, 68, 1, "MOODLETS", 200, 220, 255);
    { int y = 78, any = 0;
      for (int i = 0; i < 6; i++) if (s->ml[i].id) { sprintf(b, "%s", sim_moodlet_name(s->ml[i].id)); int v = s->ml[i].val;
          gfx_text(176, y, 1, b, v >= 0 ? 150 : 255, v >= 0 ? 240 : 140, v >= 0 ? 160 : 130); sprintf(b, "%d", v); gfx_text(262, y, 1, b, v >= 0 ? 90 : 255, v >= 0 ? 230 : 100, v >= 0 ? 90 : 90); y += 10; any = 1; }
      if (!any) gfx_text(176, 78, 1, "NOTHING MUCH", DIM); }
    gfx_textc(160, 180, 1, "O BACK", GREY);
}

// ---- the story page: the goals of this life, Bustin' Out style ----
static void story_update(void) {
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_CROSS) || PRESSED(PAD_START)) state = ST_PAUSE;
}
static void story_draw(void) {
    life_draw();
    draw_panel(52, 34, 216, 142);
    gfx_textc(160, 38, 2, "THE STORY SO FAR", GOLD);
    struct { const char *txt; int done; } g[6] = {
        { "EARN A PAYCHECK (WORK 9 TO 5, MON-FRI)", story_paid > 0 },
        { "SAIL TO CAPTAVAT ISLAND", story_visit[1] },
        { "GET SPONSORED AT WORK", job_level >= 2 },
        { "SEE THE DESERT TOWN", story_visit[2] },
        { "MAKE FRIENDS: CHAT 5 TIMES", story_talks >= 5 },
        { "SAVE UP 20000", cash >= 20000 },
    };
    int done = 0;
    for (int i = 0; i < 6; i++) {
        int y = 58 + i * 14;
        gfx_rect(62, y, 8, 8, 70, 70, 80, 0); gfx_rect(63, y + 1, 6, 6, 13, 15, 21, 0);
        if (g[i].done) gfx_rect(64, y + 2, 4, 4, 90, 230, 90, 0);
        gfx_text(74, y, 1, g[i].txt, g[i].done ? 120 : 200, g[i].done ? 255 : 200, g[i].done ? 160 : 210);
        if (g[i].done) { gfx_text(236, y, 1, "DONE", 90, 230, 90); done++; }
    }
    char b[40]; sprintf(b, "%d OF 6 - A NEW GOAL OPENS EACH DOOR", done);
    gfx_textc(160, 148, 1, b, DIM);
    gfx_textc(160, 162, 1, "O BACK", GREY);
}

// ---- the Bore panel: MOOD / WANTS / SKILLS / PEOPLE / JOB tabs (the Sims 2 panel) ----
static const char *const panelTab[5] = { "MOOD", "WANTS", "SKILLS", "PEOPLE", "JOB" };
static void panel_update(void) {
    if (PRESSED(PAD_R1) || PRESSED(PAD_RIGHT)) panel_tab = (panel_tab + 1) % 5;
    if (PRESSED(PAD_L1) || PRESSED(PAD_LEFT)) panel_tab = (panel_tab + 4) % 5;
    if (PRESSED(PAD_SELECT)) sim_switch_player();
    if (PRESSED(PAD_CIRCLE)) state = ST_PAUSE;
    if (PRESSED(PAD_START)) state = ST_LIFE;
}
static void pips(int x, int y, int v, int n, int r, int g, int b) { for (int k = 0; k < n; k++) gfx_rect(x + k * 12, y, 10, 7, k < v ? r : 40, k < v ? g : 42, k < v ? b : 50, 0); }
static void panel_draw(void) {
    life_draw();
    Sim *s = &sims[player];
    char b[64], t[32];
    draw_panel(4, 4, 312, 232);
    draw_face_k(14, 12, s->mood, 2);
    gfx_text(52, 12, 2, s->bore.first, WHITE);
    gfx_text(52 + gfx_text_w(2, s->bore.first) + 4, 17, 1, s->bore.last, GREY);
    sprintf(b, "%s  -  %s", look_stage_name(s->bore.stage), look_asp_name(s->bore.asp)); gfx_text(52, 29, 1, b, GREY);
    int m = s->mood;
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    gfx_text(52, 40, 1, mood, m > 15 ? 140 : m > -15 ? 220 : 255, m > 15 ? 255 : m > -15 ? 220 : 110, 130);
    draw_mood_meter(110, 41, 190, m);
    draw_tabs(10, 56, 300, panelTab, 5, panel_tab);
    int y0 = 78;
    if (panel_tab == 0) {   // MOOD: every motive, and what is lifting or dragging the mood
        gfx_text(14, y0 - 4, 1, "MOTIVES", GOLD);
        for (int n = 0; n < NEED_N; n++) { int v = sim_need_val(s, n), y = y0 + 8 + n * 13;
            gfx_text(14, y, 1, sim_need_name(n), v < 20 ? 255 : 200, v < 20 ? 90 : 200, v < 20 ? 90 : 210); draw_motive(66, y + 1, 84, v); }
        gfx_rect(160, y0 - 4, 1, 120, 60, 60, 70, 0);
        gfx_text(170, y0 - 4, 1, "MOODLETS", 200, 220, 255);
        int y = y0 + 8, any = 0;
        for (int i = 0; i < 6; i++) if (s->ml[i].id) { int v = s->ml[i].val;
            gfx_text(170, y, 1, sim_moodlet_name(s->ml[i].id), v >= 0 ? 150 : 255, v >= 0 ? 240 : 140, v >= 0 ? 160 : 130);
            sprintf(b, "%s%d", v > 0 ? "+" : "", v); gfx_text(284, y, 1, b, v >= 0 ? 90 : 255, v >= 0 ? 230 : 100, v >= 0 ? 90 : 90); y += 13; any = 1; }
        if (!any) gfx_text(170, y, 1, "NOTHING MUCH", DIM);
        { const char *tr[3]; int n = sim_traits(s, tr); if (n) { b[0] = 0; for (int i = 0; i < n; i++) { if (i) strcat(b, ", "); strcat(b, tr[i]); } gfx_text(14, 196, 1, b, DIM); } }
    } else if (panel_tab == 1) {   // WANTS: the aspiration meter, four wants, two fears
        int lv = wants_level(s);
        gfx_text(14, y0 - 4, 1, "ASPIRATION", GOLD);
        gfx_rect(14, y0 + 8, 292, 6, 30, 32, 40, 0);
        gfx_rect(14, y0 + 8, 292 * s->asp_pts / ASP_MAX, 6, aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2], 0);
        for (int k = 1; k < 4; k++) gfx_rect(14 + 292 * (k == 1 ? 3000 : k == 2 ? 7000 : 10000) / ASP_MAX, y0 + 7, 1, 8, 13, 15, 21, 0);
        sprintf(b, "%s%s", aspLvNm[lv], lv == 3 ? " - NOTHING CAN BRING THEM DOWN" : wants_desperate(s) ? " - DESPERATE" : "");
        gfx_text(14, y0 + 18, 1, b, aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2]);
        gfx_text(14, y0 + 36, 1, "WANTS", GOLD);
        for (int i = 0; i < 4; i++) { int y = y0 + 48 + i * 13; wants_text(s, i, t);
            gfx_rect(14, y, 7, 7, 70, 70, 80, 0); gfx_rect(15, y + 1, 5, 5, 13, 15, 21, 0);
            gfx_text(26, y, 1, t, 230, 230, 240); sprintf(b, "+%d", s->want[i].pts); gfx_text(270, y, 1, b, 90, 230, 90); }
        gfx_text(14, y0 + 108, 1, "FEARS", 255, 120, 110);
        for (int i = 0; i < 2; i++) { int y = y0 + 120 + i * 13; fears_text(s, i, t);
            gfx_text(26, y, 1, t, 230, 200, 200); sprintf(b, "-%d", s->fear[i].pts); gfx_text(270, y, 1, b, 255, 100, 90); }
    } else if (panel_tab == 2) {   // SKILLS and PERSONALITY
        static const char *const skNm[3] = { "COOKING", "BODY", "FUN" };
        int raw[3] = { s->skill_cook, s->skill_body, s->skill_fun };
        gfx_text(14, y0 - 4, 1, "SKILLS", GOLD);
        for (int i = 0; i < 3; i++) { int y = y0 + 8 + i * 13, lv = wants_skill_level(raw[i]);
            gfx_text(14, y, 1, skNm[i], 210, 210, 220); pips(80, y, lv, 10, 255, 200, 40); sprintf(b, "LV %d", lv); gfx_text(214, y, 1, b, WHITE); }
        gfx_text(14, y0 + 56, 1, "PERSONALITY", GOLD);
        for (int i = 0; i < TR_N; i++) { int y = y0 + 68 + i * 13;
            gfx_text(14, y, 1, look_trait_name(i), 210, 210, 220); pips(80, y, s->bore.tr[i], 10, 120, 180, 255); }
    } else if (panel_tab == 3) {   // PEOPLE: how everyone else in the house feels about you (the daily relationship)
        int row = 0;
        for (int j = 0; j < nsims; j++) {
            if (j == player) continue;
            int y = y0 - 4 + row * 28, rs = s->rel[j];
            draw_portrait(14, y, &sims[j], 0);
            sprintf(b, "%s %s", sims[j].bore.first, sims[j].bore.last); gfx_text(42, y + 2, 1, b, WHITE);
            draw_mood_meter(42, y + 14, 170, rs);
            gfx_rect(42 + 1 + 84 + 42, y + 12, 1, 10, GOLD, 0);   // friend at 50
            const char *lab = rs >= 50 ? "FRIEND" : rs >= 15 ? "PALS" : rs > -15 ? "STRANGERS" : rs > -50 ? "DISLIKED" : "ENEMY";
            gfx_text(222, y + 14, 1, lab, rs >= 15 ? 150 : rs > -15 ? 200 : 255, rs >= 15 ? 240 : rs > -15 ? 200 : 140, rs >= 15 ? 160 : rs > -15 ? 210 : 130);
            row++;
        }
        if (!row) gfx_text(14, y0, 1, "NOBODY ELSE LIVES HERE", DIM);
        gfx_text(14, 211, 1, "CHAT, JOKE AND HUG TO GROW A FRIENDSHIP. 50 IS A FRIEND.", DIM);
    } else {   // JOB
        gfx_text(14, y0 - 4, 1, "THE FACTORY", GOLD);
        sprintf(b, "LEVEL %d OF 5", job_level); gfx_text(14, y0 + 8, 1, b, WHITE);
        sprintf(b, "A FULL SHIFT PAYS %d", 1400 + 800 * job_level); gfx_text(110, y0 + 8, 1, b, 120, 255, 120);   // PAY0 + PAY_LVL * level (sim.c)
        gfx_text(14, y0 + 20, 1, "9 TO 5, MON TO FRI, ON THE GOLD WORK MARKER", GREY);
        gfx_text(14, y0 + 38, 1, "TODAY'S SHIFT", 200, 220, 255); draw_motive(110, y0 + 39, 190, MIN(100, job_worked * 100 / 300));
        gfx_text(14, y0 + 54, 1, "GOOD DAYS", 200, 220, 255); pips(110, y0 + 54, job_days_good, 3, 90, 220, 90); gfx_text(160, y0 + 54, 1, "3 IN A ROW GETS A PROMOTION", DIM);
        gfx_text(14, y0 + 70, 1, "STRIKES", 200, 220, 255); pips(110, y0 + 70, job_strikes, 3, 255, 80, 70); gfx_text(160, y0 + 70, 1, "3 GETS YOU DEMOTED", DIM);
        gfx_text(14, y0 + 92, 1, "MONEY", GOLD);
        sprintf(b, "$%ld", cash); gfx_text(14, y0 + 104, 1, b, 120, 255, 120);
        gfx_text(110, y0 + 104, 1, "BILLS: 800 EVERY MIDNIGHT", GREY);
    }
    gfx_textc(160, 222, 1, "L1 R1 TAB   SELECT NEXT BORE   O BACK   START PLAY", GREY);
}

// ---- the creator ----
static void create_update(void) {
    const LookRow *rows; int n = look_tab_rows(create_tab, &rows);
    if (PRESSED(PAD_L1)) { create_tab = (create_tab + NTAB - 1) % NTAB; create_row = 0; }
    if (PRESSED(PAD_R1)) { create_tab = (create_tab + 1) % NTAB; create_row = 0; }
    if (PRESSED(PAD_UP)) create_row = (create_row + n - 1) % n;
    if (PRESSED(PAD_DOWN)) create_row = (create_row + 1) % n;
    int dir = 0;
    if (PRESSED(PAD_LEFT)) dir = -1;
    if (PRESSED(PAD_RIGHT) || PRESSED(PAD_CROSS)) dir = 1;
    if (dir) {
        const LookRow *r = &rows[create_row];
        if (r->kind == ROW_AGE) { edit.stage = (u8)((edit.stage + AG_N + dir) % AG_N); look_build(&edit); }
        else if (r->id == LK_ASP) edit.asp = (u8)((edit.asp + AS_PICK + dir) % AS_PICK);
        else if (r->id == LK_LTW) edit.ltw = (u8)((edit.ltw + 4 + dir) % 4);
        else if (r->id >= LK_TRAIT0 && r->id < LK_TRAIT0 + 5) { int t = r->id - LK_TRAIT0; int v = edit.tr[t] + dir; edit.tr[t] = (u8)CLAMP(v, 0, 10); }
        else {
            int cnt = look_count(r->id), v = edit.look[r->id];
            for (int k = 0; k < cnt; k++) { v = (v + cnt + dir) % cnt; if (look_allowed(&edit, r->id, v)) break; }
            edit.look[r->id] = (u8)v;
            look_build(&edit);
        }
        audio_voice(VOICE_AGREE);
    }
    if (PRESSED(PAD_TRIANGLE)) { look_random(&edit); audio_voice(VOICE_LAUGH); }
    if (HELD(PAD_L2)) create_turn -= 40;
    if (HELD(PAD_R2)) create_turn += 40;
    if (analog) create_turn += rs_x / 3;
    if (PRESSED(PAD_START) || PRESSED(PAD_SELECT)) {
        world_restore();
        if (prev_state == ST_LIFE) { sims[player].bore = edit; state = ST_LIFE; }
        else { life_started = 0; start_life(); state = ST_LIFE; }
        audio_voice(VOICE_YAHOO);
    }
    if (PRESSED(PAD_CIRCLE)) { world_restore(); state = prev_state == ST_LIFE ? ST_LIFE : ST_MENU; }
}
static void create_draw(void) {
    const LookRow *rows; int n = look_tab_rows(create_tab, &rows);
    // the studio: a little parlour with the Bore in the middle
    gfx_set_clear(14, 16, 24);
    gfx_set_ambient(86, 100);
    gfx_sky(0x6a5040u, 0xb09070u);                                           // a warm backdrop behind the windows
    int close = (create_tab == 1 || create_tab == 2);                       // closer for the face and the hair
    int bx = 4 * TILE + TILE / 2, bz = -(5 * TILE + TILE / 2);              // the Bore stands on the rug
    cam_fx = bx - (close ? 190 : 230); cam_fy = close ? -290 : -200; cam_fz = bz;   // the focus sits right of the Bore: it shows in the left half, beside the card
    int yaw = 2048, pitch = close ? 260 : 380, dist = close ? 560 : 820;
    cam.at.vx = cam_fx; cam.at.vy = cam_fy; cam.at.vz = cam_fz;
    cam.eye.vx = cam_fx - ((isin(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vz = cam_fz - ((icos(yaw) * ((dist * icos(pitch)) >> 12)) >> 12);
    cam.eye.vy = cam_fy - ((dist * isin(pitch)) >> 12);
    gfx_camera_update();
    gfx_set_fog(0, 0);
    world_view_lvl = 0; world_roof_show = 0;
    world_draw(4, 5, frame, 1);
    VECTOR p; setVector(&p, bx, 0, bz);
    Gait g = { 0 }; g.clock = frame; g.mood = 60; g.phase = (frame * 90) & 4095; g.amp = 0;
    voxel_draw(&edit, &p, create_turn & 4095, &g);
    gfx_shadow(&p, 70);
    // the card
    draw_panel(166, 4, 150, 232);
    { char tb[32]; sprintf(tb, "< %s >", look_tab_name[create_tab]); gfx_textc(241, 6, 2, tb, GOLD);
      for (int t = 0; t < NTAB; t++) gfx_rect(220 + t * 8, 3, 5, 2, t == create_tab ? 255 : 90, t == create_tab ? 200 : 90, t == create_tab ? 40 : 100, 0); }
    gfx_rect(166, 18, 150, 1, 60, 60, 70, 0);
    int first = MAX(0, create_row - 7), shown = MIN(n, first + 9);
    for (int i = first; i < shown; i++) {
        const LookRow *r = &rows[i]; int y = 24 + (i - first) * 20;
        int sel = i == create_row;
        gfx_text(172, y, 1, r->label, sel ? 255 : 170, sel ? 230 : 170, sel ? 120 : 180);
        char val[24] = "";
        if (r->kind == ROW_AGE) strcpy(val, look_stage_name(edit.stage));
        else if (r->id == LK_ASP) strcpy(val, look_asp_name(edit.asp));
        else if (r->id == LK_LTW) sprintf(val, "GOAL %d", edit.ltw + 1);
        else if (r->id >= LK_TRAIT0 && r->id < LK_TRAIT0 + 5) {
            int v = edit.tr[r->id - LK_TRAIT0];
            for (int k = 0; k < 10; k++) gfx_rect(172 + k * 9, y + 10, 7, 6, k < v ? 255 : 50, k < v ? 200 : 50, k < v ? 40 : 60, 0);
        }
        else if (r->kind == ROW_SLIDER) {
            int pos = (edit.look[r->id] + 4) % 9;
            for (int k = 0; k < 9; k++) gfx_rect(172 + k * 10, y + 10, 8, 6, k == pos ? 255 : 50, k == pos ? 200 : 50, k == pos ? 40 : 60, 0);
        }
        else if (r->kind == ROW_COLOUR) {
            int cnt = look_count(r->id);
            { int sp = cnt > 12 ? 8 : 11, sw = sp - 2; for (int k = 0; k < cnt && k < 16; k++) { u16 c = look_option_colour(r->id, k); gfx_rect_c(172 + k * sp, y + 9, sw, 8, c, 0); if (k == edit.look[r->id]) gfx_rect(172 + k * sp - 1, y + 8, sp, 1, GOLD, 0); } }
        }
        else { const char *nm = look_option_name(r->id, edit.look[r->id]); if (nm) strncpy(val, nm, 23); }
        if (val[0]) gfx_text(172, y + 9, 1, val, WHITE);
        if (sel) gfx_text(164, y, 1, ">", GOLD);
    }
    if (create_tab == NTAB - 1) {
        char b[32]; sprintf(b, "SIGN: %s", look_sign_name(look_sign_of(&edit))); gfx_text(172, 206, 1, b, GREY);
    }
    gfx_textc(241, 216, 1, "L1 R1 TAB  TRIANGLE DICE", GREY);
    gfx_textc(241, 226, 1, prev_state == ST_LIFE ? "START DONE  O BACK" : "START GO LIVE LIFE", GOLD);
    char nm[32]; sprintf(nm, "%s %s", edit.first, edit.last); gfx_text(8, 8, 1, nm, GOLD);
    gfx_text(8, 18, 1, look_stage_name(edit.stage), GREY);
    gfx_text(8, 226, 1, "CREATE A BORE", WHITE);
}

// ---- build mode ----
static const char *const toolNm[5] = { "BUY", "WALL", "FLOOR", "WALLPAPER", "ROOF" };
#define NTOOL 5
static int floor_supported(int l, int x, int y) {   // an upper floor needs something under it, or a floor beside it on its own storey
    if (l == 0) return 1;
    char below = world_at_l(l - 1, x, y);
    if (below != ' ' && below != '|') return 1;
    static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    for (int k = 0; k < 4; k++) { char c = world_at_l(l, x + dx[k], y + dy[k]); if (c != ' ') return 1; }
    return 0;
}
// ---- the catalog (Sims buy mode): the items in tabs, a scrolling list with prices ----
static const char *const catNm[7] = { "SEATS AND BEDS", "TABLES AND STORAGE", "KITCHEN AND BATH", "FUN AND TECH", "DECOR AND PLANTS", "SKATE", "SPECIAL" };
static const char *const catSet[6] = { "CUh;S<N%z", "tdibyK?#O", "F/:cTH,->&Y`", "vARj[!$()QImn]_{", "rlpZVqo}gus*0", "=LJXM159G" };
static int item_cat(char ch) { for (int c = 0; c < 6; c++) if (strchr(catSet[c], ch)) return c; return 6; }
static int cat_list(int cat, int *idx) { int n = 0; for (int i = 0; i < items_count() && n < 96; i++) if (item_cat(items_at(i)) == cat) idx[n++] = i; return n; }
static void catalog_step(int dir) {   // L1 R1: through the tab, and on into the next tab at the end
    int idx[96], cat = item_cat(items_at(build_item)), n = cat_list(cat, idx), p = 0;
    for (int k = 0; k < n; k++) if (idx[k] == build_item) p = k;
    p += dir;
    if (p >= 0 && p < n) { build_item = idx[p]; return; }
    int nc = cat;
    for (int g = 0; g < 7; g++) { nc = (nc + 7 + dir) % 7; n = cat_list(nc, idx); if (n) break; }
    if (n) build_item = idx[dir > 0 ? 0 : n - 1];
}
static void catalog_next_cat(void) {   // R3: jump to the next tab
    int idx[96], cat = item_cat(items_at(build_item));
    for (int g = 1; g <= 7; g++) { int nc = (cat + g) % 7; if (cat_list(nc, idx)) { build_item = idx[0]; return; } }
}
static void draw_catalog(void) {
    int idx[96], cat = item_cat(items_at(build_item)), n = cat_list(cat, idx), p = 0;
    char b[24];
    for (int k = 0; k < n; k++) if (idx[k] == build_item) p = k;
    draw_panel(196, 4, 120, 156);
    for (int c = 0; c < 7; c++) gfx_rect(204 + c * 16, 8, 12, 3, c == cat ? 255 : 80, c == cat ? 200 : 80, c == cat ? 40 : 95, 0);
    gfx_textc(256, 14, 1, catNm[cat], GOLD);
    int first = CLAMP(p - 5, 0, MAX(0, n - 10));
    for (int k = first; k < n && k < first + 10; k++) {
        char ch = items_at(idx[k]); int y = 28 + (k - first) * 12, sel = k == p, cost = items_cost(ch);
        if (sel) { gfx_rect(199, y - 2, 114, 11, 28, 48, 62, 0); gfx_rect(199, y - 2, 1, 11, GOLD, 0); }
        gfx_text(204, y, 1, items_name(ch), sel ? 255 : 175, sel ? 255 : 175, sel ? 255 : 185);
        sprintf(b, "%d", cost); gfx_text(310 - gfx_text_w(1, b), y, 1, b, cost > cash ? 255 : 120, cost > cash ? 90 : 230, cost > cash ? 90 : 120);
    }
    sprintf(b, "%d OF %d", p + 1, n); gfx_textc(256, 148, 1, b, DIM);
}
static void draw_pick_list(const char *title, int count, int sel, const char *(*nm)(int)) {   // floors and wallpapers: seven at a time, the pick in the middle
    char b[24];
    draw_panel(196, 4, 120, 104);
    gfx_textc(256, 8, 1, title, GOLD);
    for (int k = -3; k <= 3; k++) {
        int i = (sel + k + count * 4) % count, y = 24 + (k + 3) * 11;
        if (!k) { gfx_rect(199, y - 2, 114, 11, 28, 48, 62, 0); gfx_rect(199, y - 2, 1, 11, GOLD, 0); }
        gfx_text(204, y, 1, nm(i), !k ? 255 : 150, !k ? 255 : 150, !k ? 255 : 160);
    }
    sprintf(b, "%d OF %d", sel + 1, count); gfx_textc(256, 98, 1, b, DIM);
}
static void build_update(void) {
    static int rep;
    int dx = 0, dy = 0;
    int l = HELD(PAD_LEFT) || ls_x < -40, r = HELD(PAD_RIGHT) || ls_x > 40, u = HELD(PAD_UP) || ls_y < -40, d = HELD(PAD_DOWN) || ls_y > 40;
    if (l || r || u || d) { if (rep == 0 || rep > 12) { if (l) dx = -1; if (r) dx = 1; if (u) dy = -1; if (d) dy = 1; if (rep > 12) rep = 9; } rep++; } else rep = 0;
    if (dx || dy) {   // move with the camera: "up" is away from it
        int c = icos(cam_yaw), sn = isin(cam_yaw);
        int vx = dx * c - (-dy) * sn, vz = (-dy) * c + dx * sn;
        if (ABS(vx) >= ABS(vz)) build_x += vx > 0 ? 1 : -1; else build_y += vz > 0 ? -1 : 1;
        build_x = CLAMP(build_x, 1, MW - 2); build_y = CLAMP(build_y, 1, MH - 2);
    }
    if (PRESSED(PAD_SELECT)) build_tool = (build_tool + 1) % NTOOL;
    if (PRESSED(PAD_R2) && build_lvl < NUP) { build_lvl++; audio_voice(VOICE_AGREE); }     // the storey you build on
    if (PRESSED(PAD_L2) && build_lvl > 0) { build_lvl--; audio_voice(VOICE_AGREE); }
    if (PRESSED(PAD_L3)) build_peek ^= 1;                                                  // see the roof
    if (analog) { cam_yaw += rs_x / 3; cam_pitch += rs_y / 4; cam_pitch = CLAMP(cam_pitch, 300, 1000); }
    if (build_tool == 0) { if (PRESSED(PAD_L1)) catalog_step(-1); if (PRESSED(PAD_R1)) catalog_step(1); if (PRESSED(PAD_R3)) catalog_next_cat(); }
    if (build_tool == 2) { if (PRESSED(PAD_L1)) build_fl = (build_fl + NFLOORS - 1) % NFLOORS; if (PRESSED(PAD_R1)) build_fl = (build_fl + 1) % NFLOORS; }
    if (build_tool == 3) { if (PRESSED(PAD_L1)) build_wp = (build_wp + NWALLS - 1) % NWALLS; if (PRESSED(PAD_R1)) build_wp = (build_wp + 1) % NWALLS; }
    if (build_tool == 1) { if (PRESSED(PAD_L1)) build_wp = (build_wp + NWALLS - 1) % NWALLS; if (PRESSED(PAD_R1)) build_wp = (build_wp + 1) % NWALLS; }
    if (build_tool == 4) {   // the roof: L1 R1 colour, X style, SQUARE pitch
        if (PRESSED(PAD_L1)) { world_roof_col = (world_roof_col + NROOF_COL - 1) % NROOF_COL; world_touched(); }
        if (PRESSED(PAD_R1)) { world_roof_col = (world_roof_col + 1) % NROOF_COL; world_touched(); }
        if (PRESSED(PAD_CROSS)) { world_roof_style = (world_roof_style + 1) % NROOF_STYLE; world_touched(); audio_voice(VOICE_FINISHED); }
        if (PRESSED(PAD_SQUARE)) { world_roof_pitch = (world_roof_pitch + 1) % NROOF_PITCH; world_touched(); audio_voice(VOICE_FINISHED); }
        if (PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; state = ST_LIFE; }
        return;
    }
    int L = build_lvl;
    char *cell = world_cell(L, build_x, build_y);
    int occupied = 0; for (int i = 0; i < nsims; i++) if (sims[i].lvl == L && sim_tile_x(&sims[i]) == build_x && sim_tile_y(&sims[i]) == build_y) occupied = 1;
    if (PRESSED(PAD_CROSS) && !occupied) {
        int ok = 0;
        if (build_tool == 0) { char ch = items_at(build_item); int cost = items_cost(ch);
            if (*cell == '.' && cash >= cost) {
                if (ch == '|') {   // stairs: there must be a storey above, and its tile over the stairs becomes the stairwell
                    if (L >= NUP) { sim_note("NO STOREY ABOVE THE TOP ONE"); }
                    else { char *up = world_cell(L + 1, build_x, build_y); if (*up == '.' || *up == ' ') { *up = ' '; *cell = ch; cash -= cost; ok = 1; } }
                } else { *cell = ch; cash -= cost; ok = 1; wants_event(&sims[player], W_BUY, 0); }
            } }
        if (build_tool == 1) { if (*cell == '.' && cash >= 50) { *cell = 'W'; *world_wall_cell(L, build_x, build_y) = (u8)build_wp; cash -= 50; ok = 1; } }
        if (build_tool == 2) { if (*cell == ' ' && !floor_supported(L, build_x, build_y)) sim_note("A FLOOR UP HERE NEEDS SOMETHING UNDER IT");
                               else if (cash >= 10) { if (*cell == ' ') *cell = '.'; *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; ok = 1; } }
        if (build_tool == 3) { if (*cell == 'W' || *cell == 'w') { *world_wall_cell(L, build_x, build_y) = (u8)build_wp; ok = 1; } }
        audio_voice(ok ? VOICE_FINISHED : VOICE_NAH);
    }
    if (HELD(PAD_CROSS) && build_tool == 2 && cash >= 10 && *cell != ' ' && *world_floor_cell(L, build_x, build_y) != build_fl) { *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; }
    if (HELD(PAD_CROSS) && build_tool == 2 && L > 0 && *cell == ' ' && cash >= 10 && floor_supported(L, build_x, build_y)) { *cell = '.'; *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; }
    if (PRESSED(PAD_SQUARE) && !occupied) {
        if (*cell != '.' && *cell != 'P' && *cell != ' ') { int back = items_is_item(*cell) ? items_cost(*cell) * 3 / 4 : (*cell == 'W' ? 25 : 0);
            if (*cell == '|' && L < NUP) { char *up = world_cell(L + 1, build_x, build_y); if (*up == ' ' && floor_supported(L + 1, build_x, build_y)) *up = '.'; }   // close the stairwell again
            *cell = '.'; cash += back; }
        else if (*cell == '.' && L > 0) { *cell = ' '; cash += 5; }                       // take the floor itself away (upper storeys)
    }
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_SQUARE) || HELD(PAD_CROSS)) world_touched();
    if (PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; state = ST_LIFE; }
}
static void build_draw(void) {
    {   // looking at the roof: lift the camera above the ridge and pull back, so the whole roof is in view
        int roofv = build_tool == 4 || build_peek, keep = cam_dist;
        if (roofv) cam_dist = MAX(cam_dist, 2600);
        camera_apply(build_x * TILE + TILE / 2, -100 - build_lvl * WALL_FULL - (roofv ? WALL_FULL + 300 : 0), -(build_y * TILE + TILE / 2));
        cam_dist = keep;
    }
    gfx_set_clear(20 * 8, 26 * 8, 31 * 8);
    gfx_set_ambient(100, 84);
    world_view_lvl = build_lvl; world_roof_show = build_tool == 4 || build_peek;
    world_draw(build_x, build_y, frame, build_tool != 4 && !build_peek);
    draw_sims();
    // the cursor: a frame around the tile, and a ghost of the item
    gfx_set_identity();
    { int x0 = build_x * TILE, z1 = -build_y * TILE, yy = -4 - build_lvl * WALL_FULL; SVECTOR a, b, c, d;
      setVector(&a, x0, yy, z1); setVector(&b, x0 + TILE, yy, z1); setVector(&c, x0, yy, z1 - TILE); setVector(&d, x0 + TILE, yy, z1 - TILE);
      int pulse = 160 + (isin((frame * 120) & 4095) >> 6);
      gfx_quad_f(&a, &b, &c, &d, pulse, pulse, 40, 0); }
    if (build_tool == 0 && world_at_l(build_lvl, build_x, build_y) == '.') {
        char ch = items_at(build_item);
        world_lvl = build_lvl; world_cam_push(build_lvl);
        items_draw(ch, build_x, build_y, world_item_facing(build_x, build_y), items_along(ch) ? world_along(build_x, build_y, ch) : 0, frame);
        world_cam_pop(); world_lvl = 0;
    }
    {   // the tab strip, the status card, and the catalog on the right
        static const char *const toolTab[NTOOL] = { "BUY", "WALL", "FLOOR", "PAPER", "ROOF" };
        char b[64];
        draw_tabs(4, 4, 190, toolTab, NTOOL, build_tool);
        draw_panel(4, 18, 190, 34);
        sprintf(b, "STOREY %d/%d   AT %d,%d", build_lvl + 1, NLEVELS, build_x, build_y); gfx_text(10, 22, 1, b, GREY);
        sprintf(b, "FUNDS $%ld", cash); gfx_text(10, 32, 1, b, 120, 255, 120);
        if (build_tool == 0) sprintf(b, "%s  $%d", items_name(items_at(build_item)), items_cost(items_at(build_item)));
        else if (build_tool == 1) sprintf(b, "WALL $50  (%s)", world_wall_name(build_wp));
        else if (build_tool == 2) sprintf(b, "%s  $10 A TILE", world_floor_name(build_fl));
        else if (build_tool == 3) sprintf(b, "%s", world_wall_name(build_wp));
        else sprintf(b, "%s ROOF, %s PITCH", world_roof_style_name(world_roof_style), world_roof_pitch_name(world_roof_pitch));
        gfx_text(10, 42, 1, b, WHITE);
        if (build_tool == 0) draw_catalog();
        else if (build_tool == 1) draw_pick_list("WALLS  $50", NWALLS, build_wp, world_wall_name);
        else if (build_tool == 2) draw_pick_list("FLOORS  $10 A TILE", NFLOORS, build_fl, world_floor_name);
        else if (build_tool == 3) draw_pick_list("WALLPAPERS", NWALLS, build_wp, world_wall_name);
        else { draw_panel(196, 4, 120, 50); gfx_textc(256, 8, 1, "THE ROOF", GOLD); gfx_text(204, 22, 1, world_roof_style_name(world_roof_style), WHITE);
               gfx_text(204, 32, 1, world_roof_pitch_name(world_roof_pitch), 200, 200, 210); gfx_text(204, 42, 1, world_roof_col_name(world_roof_col), 200, 200, 210); }
        gfx_rect(0, 214, 320, 26, 8, 10, 22, 1); gfx_rect(0, 214, 320, 1, 255, 200, 40, 0);
        if (build_tool == 4) gfx_textc(160, 218, 1, "X STYLE   SQ PITCH   L1 R1 COLOUR   SEL TOOL   O DONE", GREY);
        else gfx_textc(160, 218, 1, build_tool == 0 ? "L1 R1 PICK   R3 NEXT TAB   X PLACE   SQ ERASE   SEL TOOL" : "L1 R1 PICK   X PLACE   SQ ERASE   SEL TOOL", GREY);
        gfx_textc(160, 228, 1, "L2 R2 STOREY   L3 SEE THE ROOF   R-STICK TURN   O DONE", GREY);
    }
}

// ---- the jukebox ----
static void juke_update(void) {
    int n = audio_song_count();
    if (n) {
        if (PRESSED(PAD_UP)) juke_sel = (juke_sel + n - 1) % n;
        if (PRESSED(PAD_DOWN)) juke_sel = (juke_sel + 1) % n;
        if (PRESSED(PAD_CROSS)) { audio_station_play(-1); if (audio_jukebox_current() == juke_sel) audio_jukebox_stop(); else audio_jukebox_play(juke_sel); }
        if (PRESSED(PAD_L1)) audio_jukebox_play((audio_jukebox_current() + n - 1) % n);
        if (PRESSED(PAD_R1)) audio_jukebox_next();
        if (PRESSED(PAD_SQUARE)) audio_jukebox_toggle_shuffle();
    }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = prev_state == ST_LIFE ? ST_LIFE : ST_MENU;
}
static void juke_draw(void) {
    gfx_set_clear(8, 8, 14);
    draw_panel(10, 6, 300, 228);
    gfx_textc(160, 8, 2, "JUKEBOX", GOLD);
    int cur = audio_jukebox_current();
    char b[64];
    if (cur >= 0) {
        sprintf(b, "%s: %s", audio_station() >= 0 ? audio_station_name(audio_station()) : "NOW PLAYING", audio_song_name(cur)); gfx_text(16, 22, 1, b, 120, 255, 160);
        int e = audio_jukebox_elapsed(), l = audio_song_len(cur);
        sprintf(b, "%s   %d:%02d / %d:%02d", audio_song_artist(cur), e / 60, e % 60, l / 60, l % 60); gfx_text(16, 32, 1, b, GREY);
        draw_bar(16, 42, 288, l ? MIN(100, e * 100 / l) : 0, 120, 255, 160);
    } else {
        gfx_text(16, 22, 1, audio_cd_ok() ? "SILENT. X PLAYS A SONG." : "NO MUSIC ON THIS DISC", GREY);
    }
    int n = audio_song_count();
    if (juke_sel < juke_scroll) juke_scroll = juke_sel;
    if (juke_sel >= juke_scroll + 16) juke_scroll = juke_sel - 15;
    for (int i = juke_scroll; i < n && i < juke_scroll + 16; i++) {
        int y = 54 + (i - juke_scroll) * 10, sel = i == juke_sel, pl = i == cur;
        gfx_text(16, y, 1, sel ? ">" : " ", GOLD);
        gfx_text(26, y, 1, audio_song_name(i), pl ? 120 : sel ? 255 : 180, pl ? 255 : sel ? 255 : 180, pl ? 160 : sel ? 255 : 190);
        int l = audio_song_len(i); sprintf(b, "%d:%02d", l / 60, l % 60); gfx_text(270, y, 1, b, GREY);
    }
    sprintf(b, "%d SONGS   %s", n, audio_jukebox_shuffle() ? "SHUFFLE" : "IN ORDER");
    gfx_text(16, 216, 1, b, GREY);
    gfx_text(16, 226, 1, "X PLAY/STOP  L1 R1 SKIP  SQ SHUFFLE  O BACK", GREY);
}

// ---- help ----
static const char *const help_txt[3][12] = {
    { "HOW TO PLAY", "", "LIVE: MOVE WITH THE STICK OR D-PAD.", "X USES THE THING IN FRONT OF YOU:", "FRIDGE EATS, BED SLEEPS, SHOWER", "WASHES, SOFA LOUNGES, TV IS FUN.", "TALK TO A HOUSEMATE WITH X.", "O GETS YOU UP. SQUARE RUNS.", "TRIANGLE CHANGES THE SPEED.", "SELECT SWITCHES BORE.", "", "R1: NEXT PAGE" },
    { "THE CAMERA", "", "FULL 3D: L1 AND R1 ORBIT,", "L2 AND R2 ZOOM, THE RIGHT STICK", "ORBITS AND TILTS.", "WALLS BETWEEN YOU AND THE CAMERA", "DROP LOW, LIKE A SIMS GAME.", "PAUSE: BORE PANEL, MOOD AND PEOPLE.", "THE JOB: STAND ON THE GOLD WORK", "MARKER IN THE FACTORY 9 TO 5.", "BILLS: 800 EVERY MIDNIGHT.", "R1: NEXT PAGE" },
    { "BUILD AND CREATE", "", "BUILD MODE (PAUSE MENU): PLACE", "BORE'S ITEMS, WALLS, FLOORS AND", "103 WALLPAPERS. SELECT PICKS THE", "TOOL, L1 R1 PICK, X PLACES.", "R3 JUMPS TO THE NEXT CATALOG TAB.", "CREATE A BORE: EVERY BODY, FACE,", "HAIR, CLOTHES AND SPORE PART OF", "THE GBA GAME, NOW IN 3D.", "", "O: BACK" } };
static void help_update(void) {
    if (PRESSED(PAD_R1) || PRESSED(PAD_CROSS)) help_page = (help_page + 1) % 3;
    if (PRESSED(PAD_L1)) help_page = (help_page + 2) % 3;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = prev_state == ST_PAUSE ? ST_LIFE : ST_MENU;
}
static void help_draw(void) {
    gfx_set_clear(8, 8, 14);
    draw_panel(10, 10, 300, 220);
    for (int i = 0; i < 12; i++) gfx_text(18, 16 + i * 14, 1, help_txt[help_page][i], i == 0 ? 255 : 210, i == 0 ? 200 : 210, i == 0 ? 40 : 220);
}

// ---- title and menu ----
static Bore title_bore;
static int title_t;
static void title_update(void) {
    title_t++;
    if (PRESSED(PAD_START) || PRESSED(PAD_CROSS)) { state = ST_MENU; menu_sel = 0; audio_voice(VOICE_YAHOO); }
}
static void title_draw(void) {
    gfx_set_clear(10, 8, 30);
    gfx_set_ambient(96, 86);
    gfx_sky(0x8a3a20u, 0xe0a060u);                                           // an evening sky over the lot
    int tx = 9, ty = 13;                                                     // the house's lounge
    int fx = tx * TILE + TILE / 2, fz = -(ty * TILE + TILE / 2);
    cam_fx = fx; cam_fy = -220; cam_fz = fz;
    cam_yaw = (2048 + (isin((title_t * 3) & 4095) >> 3)) & 4095; cam_pitch = 420; cam_dist = 1500;
    camera_apply(fx, -220, fz);
    gfx_set_fog(2000, 5000);
    world_view_lvl = NUP; world_roof_show = 0;
    world_draw(tx, ty, title_t, 1);
    VECTOR p; setVector(&p, fx, 0, fz);
    Gait g = { 0 }; g.clock = frame; g.mood = 60; g.phase = (title_t * 120) & 4095; g.amp = 4096;
    voxel_draw(&title_bore, &p, (title_t * 9) & 4095, &g);
    gfx_shadow(&p, 70);
    gfx_textc(160, 14, 3, "BORE 2", GOLD);
    gfx_textc(160, 52, 1, "DIPPINN PRODUCTIONS", GREY);
    gfx_textc(160, 200, 1, "THE VOXEL LIFE, NOW IN FULL 3D", 180, 200, 255);
    if ((title_t / 20) & 1) gfx_textc(160, 222, 1, "PRESS START", WHITE);
}
static const char *const menuNm[] = { "STORY MODE", "CREATE A BORE", "JUKEBOX", "HOW TO PLAY" };
static const char *const menuBlurb[] = { "HOME, THE ISLAND, THE TOWN", "SCULPT YOUR BORE", "ALL 33 SONGS", "CONTROLS AND TIPS" };
static void menu_update(void) {
    title_t++;
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + 3) % 4;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % 4;
    if (PRESSED(PAD_CIRCLE)) state = ST_TITLE;
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_START)) {
        switch (menu_sel) {
            case 0: life_started = 0; start_life(); cam_yaw = 2600; cam_pitch = 640; cam_dist = 1500; state = ST_LIFE; frame = 0; break;
            case 1: prev_state = ST_MENU; create_tab = 0; create_row = 0; state = ST_CREATE; world_backup(); world_gen_studio(); break;
            case 2: prev_state = ST_MENU; state = ST_JUKEBOX; break;
            case 3: prev_state = ST_MENU; help_page = 0; state = ST_HELP; break;
        }
    }
}
static void menu_draw(void) {
    title_draw();
    draw_panel(94, 66, 132, 90);
    for (int i = 0; i < 4; i++) menu_row(100, 76 + i * 14, 120, menuNm[i], i == menu_sel);
    gfx_rect(100, 134, 120, 1, 60, 60, 70, 0);
    gfx_textc(160, 140, 1, menuBlurb[menu_sel], DIM);
}

// ---- the loading screen: a bar and what is being made, shown before any long job (start-up, a new life, travel) ----
static void loading_draw(const char *what, int pct) {
    pct = CLAMP(pct, 0, 100);
    gfx_set_clear(20, 12, 8);
    gfx_sky(0x501810u, 0xa06030u);
    gfx_textc(160, 64, 3, "BORE 2", GOLD);
    gfx_textc(160, 104, 1, "LOADING", WHITE);
    gfx_textc(160, 120, 1, what, 180, 200, 255);
    gfx_rect(58, 144, 204, 12, 10, 8, 20, 0);
    gfx_rect(60, 146, 200 * pct / 100, 8, 255, 200, 60, 0);
}
void loading(const char *what, int pct) { loading_draw(what, pct); gfx_flip(); DrawSync(0); }

// ---- start-up: the loading screen, plus a small trace of the stages (the last one printed is where a hang would be) ----
extern char _end[];
#define BOOT_KEEP 32
#define BOOT_SHOW 6
#define BOOT_STEPS 10
static char boot_log[BOOT_KEEP][20]; static int boot_n;
static void boot_frame(void) {
#ifdef BOOT_TRACE
    char b[32];
    int free_k = (int)((0x801FFFF0u - 0x10000u - (u32)_end) / 1024);   // (the last 64 KB below the stack top is left for the stack itself)
    sprintf(b, "RAM FREE %dK", free_k);
    gfx_text(8, 228, 1, b, free_k < 32 ? 255 : 140, free_k < 32 ? 60 : 140, free_k < 32 ? 60 : 150);
    int first = boot_n > BOOT_SHOW ? boot_n - BOOT_SHOW : 0;
    for (int k = first; k < boot_n; k++) gfx_text(8, 176 + (k - first) * 8, 1, boot_log[k % BOOT_KEEP], 110, 200, 110);
#endif
    gfx_flip();
    DrawSync(0);   // let the GPU finish the frame before the next texture upload
}
void boot(const char *stage) {
    char *dst = boot_log[boot_n % BOOT_KEEP]; int i = 0; while (stage[i] && i < 19) { dst[i] = stage[i]; i++; } dst[i] = 0; boot_n++;
    // drawn twice: the screen shows the frame BEFORE the one just flipped, so the first flip alone would leave the stage on screen one step late
    loading_draw(stage, boot_n * 100 / BOOT_STEPS); boot_frame();
    loading_draw(stage, boot_n * 100 / BOOT_STEPS); boot_frame();
}

int main(void) {
    gfx_init();
    atlas_init();
    font_init();        boot("GFX ATLAS FONT OK");   // (the boot text needs the font, so the first line comes after it)
    // each stage is announced BEFORE it runs ("> NAME"), so the last line on screen names the stage that is running when the game stops
    boot("> LOOK");        look_init();
    boot("> VOXEL");       voxel_init();
    boot("> ITEMS");       items_init();
    boot("> WORLD TEX");   world_init();
    boot("> AUDIO+DISC");  audio_init();
    pad_init();                                      // (the pad is read directly in read_pad: see pad.c)
    boot("> CROWD");       crowd_init();             // the voice clips are in the sound chip now: their RAM holds the crowd's models
    look_default(&edit);
    look_default(&title_bore); look_random(&title_bore);
    boot("> HOME LOT");    world_gen();
    strcpy(edit.first, "DIPPA"); strcpy(edit.last, "DIPPINN");
    boot("> TITLE SONG");  audio_jukebox_play(0);   // THE DIPPER MAN, the title song (as on the GBA)
    boot("> TITLE");
    t_vb = VSync(-1);
    while (1) {
        t_frame0 = hb_now();
        read_pad();
        int now = VSync(-1), steps = now - t_vb; t_vb = now;
        steps = CLAMP(steps, 1, 4);
        frame++;
        fps_frames++; if (now - fps_vb >= 60) { fps_val = fps_frames * 60 / (now - fps_vb); fps_frames = 0; fps_vb = now; }
        audio_update();
        switch (state) {
            case ST_TITLE: title_update(); title_draw(); break;
            case ST_MENU: menu_update(); menu_draw(); break;
            case ST_CREATE: create_update(); create_draw(); break;
            case ST_LIFE:
#ifdef AUTO_PARTY
                { static int ap; if (++ap == 120) { cash += 100; throw_party(); } }   // (test builds: a party a couple of seconds in)
#endif
                { int t0 = hb_now(); life_update(steps); t_upd = hb_now() - t0; } life_draw(); break;
            case ST_PAUSE: pause_update(); pause_draw(); break;
            case ST_BUILD: build_update(); build_draw(); break;
            case ST_JUKEBOX: juke_update(); juke_draw(); break;
            case ST_HELP: help_update(); help_draw(); break;
            case ST_TRAVEL: travel_update(); travel_draw(); break;
            case ST_STORY: story_update(); story_draw(); break;
            case ST_WANTS: wants_update(); wants_draw(); break;
            case ST_PIE: pie_update(); pie_draw(); break;
            case ST_PANEL: panel_update(); panel_draw(); break;
        }
        t_cpu = hb_now() - t_frame0;
        gfx_flip();
    }
    return 0;
}
