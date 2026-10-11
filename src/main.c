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
#include "mem8.h"
#include "bigmap.h"
#include "pop.h"
#include "far.h"
#include "ui.h"
#include "places.h"

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
#if defined(AUTO_TOUR) || defined(AUTO_GROW)
    { extern int auto_btn; if (auto_btn) { btn &= (u16)~auto_btn; auto_btn = 0; } }   // (test builds: the tour presses a button for a frame)
#endif
}

// ---- colours ----
#define GOLD 255, 200, 40
#define WHITE 255, 255, 255
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
#define title_bore (sims[MAX_SIMS - 1].bore)   // (the title's Bore borrows the last sim slot: no life is on while the title shows)
static int frame;
static int t_vb;

// ---- states ----
enum { ST_TITLE, ST_MENU, ST_CREATE, ST_LIFE, ST_PAUSE, ST_BUILD, ST_JUKEBOX, ST_HELP, ST_TRAVEL, ST_STORY, ST_WANTS, ST_PIE, ST_PANEL, ST_MAP, ST_DIR, ST_OPTS, ST_JOBS, ST_OVER };
static int state = ST_TITLE, prev_state = ST_TITLE;
static int menu_sel;
static int create_tab, create_row, create_from, create_turn;
static int build_tool, build_item, build_x = 8, build_y = 8, build_fl = 1, build_wp = 1, build_lvl, build_peek;
static int juke_sel, juke_scroll;
static int life_started;
static int help_page;
static int hb_now(void) { return VSync(1); }   // hblanks since the last VSync(0) (the flip): 263 per vblank, 525 = one 30 FPS frame
static int t_cpu, t_frame0, t_bub, t_hud, t_upd;
int avg_cpu16, avg_world16, avg_hh16, hist_cpu[5], avg_prof16[8];
static int show_fps = 0, fps_val, fps_frames, fps_vb, prims_last, t_world, t_sims, t_logic;
static int travel_sel;
int dbg_shift_hb, dbg_shifts;
static void taxi_to_block(int bx, int by, int tx, int ty, const char *where);
static char pie_opt[8][16]; static int pie_n, pie_sel, pie_cit;   // the pie menu
static int panel_tab;                                      // the Bore panel's tab
static int jobs_sel, jobs_n, jobs_scroll, jobs_car[16], jobs_pl[16];   // the JOBS screen
static void jobs_open(void);

// which story zones are open (the island comes with the first paycheck, the town with job level 2)
static int zone_unlocked(int z) { return z == 0 || z == 3 || (z == 1 && story_paid > 0) || (z == 2 && job_level >= 2); }   // (the city is open from the start)

// ---- Sims-style widgets: motive colours, the mood face, the notification feed, the action card ----
static void need_rgb(int v, int *r, int *g, int *b) {   // a Sims motive bar goes green, then yellow, then red
    if (v >= 60) { *r = 90; *g = 220; *b = 90; } else if (v >= 30) { *r = 240; *g = 210; *b = 60; } else { *r = 255; *g = 80; *b = 70; }
}
static void draw_face_k(int x, int y, int m, int k) {   // the mood face, k times bigger
    int r = m > 15 ? 120 : m > -15 ? 230 : 255, g = m > 15 ? 230 : m > -15 ? 210 : 100, bb = m > 15 ? 90 : m > -15 ? 70 : 90;
    gfx_rect(x + 2 * k, y, 12 * k, 16 * k, r, g, bb, 0); gfx_rect(x, y + 2 * k, 16 * k, 12 * k, r, g, bb, 0);
    gfx_rect(x + 4 * k, y + 5 * k, 2 * k, 3 * k, INK, 0); gfx_rect(x + 10 * k, y + 5 * k, 2 * k, 3 * k, INK, 0);
    if (m > 15) { gfx_rect(x + 4 * k, y + 10 * k, 8 * k, k, INK, 0); gfx_rect(x + 3 * k, y + 9 * k, k, k, INK, 0); gfx_rect(x + 12 * k, y + 9 * k, k, k, INK, 0); }
    else if (m > -15) gfx_rect(x + 4 * k, y + 10 * k, 8 * k, k, INK, 0);
    else { gfx_rect(x + 4 * k, y + 11 * k, 8 * k, k, INK, 0); gfx_rect(x + 3 * k, y + 12 * k, k, k, INK, 0); gfx_rect(x + 12 * k, y + 12 * k, k, k, INK, 0); }
}
static const char *act_name(int a) {
    switch (a) {
        case ACT_EAT: return "EATING"; case ACT_WC: return "USING THE TOILET"; case ACT_SLEEP: return "SLEEPING"; case ACT_SHOWER: return "SHOWERING";
        case ACT_SIT: return "RELAXING"; case ACT_FUN: return "HAVING FUN"; case ACT_TALK: return "CHATTING"; case ACT_WORK: return "WORKING";
        case ACT_WALK: return "WALKING"; case ACT_COFFEE: return "HAVING COFFEE"; case ACT_READ: return "READING"; case ACT_RUN: return "EXERCISING";
        case ACT_PHONE: return "ON THE PHONE"; case ACT_TRAVEL: return "TRAVELLING"; case ACT_STAIRS: return "ON THE STAIRS"; case ACT_SWIM: return "SWIMMING";
        case ACT_CARE: return "LOOKING AFTER THE BABY";
    }
    return "";
}
static int act_seen, act_total;
static const char *act_label(const Sim *s) {   // the action's name, the going-out ones in their own words
    switch (s->action) {
        case ACT_SIT: if (s->act_var == 6) return "WATCHING A FILM"; break;
        case ACT_FUN: if (s->act_var == 4) return "BROWSING"; if (s->act_var == 5) return "HAVING A DRINK"; if (s->act_var == 7) return "ADMIRING THE ART"; break;
        case ACT_EAT: if (s->act_var == 2) return "EATING OUT"; if (s->act_var == 5) return "FINE DINING"; if (s->act_var == 8) return "BAKING A CAKE";
                      if (s->act_var == 9) return "BIRTHDAY CAKE"; break;
        case ACT_READ: if (s->act_var == 3) return "DOING HOMEWORK"; if (s->act_var == 5) return "REMEMBERING"; break;
        case ACT_CARE: { static const char *const ck[6] = { "", "FEEDING THE BABY", "CHANGING THE BABY", "PLAYING WITH THE BABY", "CUDDLING THE BABY", "PUTTING BABY TO BED" };
                         if (s->act_var >= 1 && s->act_var <= 5) return ck[s->act_var]; break; }
        case ACT_SLEEP: if (s->act_var == 2) return "A MASSAGE"; if (s->bore.stage == AG_BABY) return "NAPPING"; break;
        case ACT_RUN: if (s->act_var == 2) return "TRAINING"; break;
        case ACT_COFFEE: if (s->act_var == 1) return "A HOT DRINK"; break;
    }
    return act_name(s->action);
}
static void draw_action_card(const Sim *s) {   // what the Bore is doing, with its progress (the Sims 1 queue, one card)
    if (s->action != act_seen) { act_seen = s->action; act_total = s->act_t; }
    if (s->action == ACT_NONE || s->action == ACT_WALK) return;
    const char *nm = act_label(s);
    int w = MAX(ui_tw(1, nm) + 14, 90);
    ui_card(4, 164, w, 17);
    ui_text(10, 167, 1, nm, UI_INK);
    if (s->action == ACT_WORK) ui_bar(9, 175, w - 10, 4, MIN(100, job_worked * 100 / job_shift_len()), UI_GREEN);   // (the shift so far)
    else if (act_total > 0 && act_total < 9999) ui_bar(9, 175, w - 10, 4, CLAMP(100 - s->act_t * 100 / act_total, 0, 100), UI_BLUE);
}
#define NFEED 3
static char feed[NFEED][32]; static int feed_t[NFEED], prev_note_t;
static void feed_push(const char *t) {
    for (int i = NFEED - 1; i > 0; i--) { strcpy(feed[i], feed[i - 1]); feed_t[i] = feed_t[i - 1]; }
    strncpy(feed[0], t, 31); feed[0][31] = 0; feed_t[0] = 300;
}
static void feed_poll(void) { extern char note_q[4][32]; extern int note_qn; for (int i = 0; i < note_qn; i++) feed_push(note_q[i]); note_qn = 0; }   // every sim_note lands here
static void feed_draw(void) {   // the notifications stack on the left, newest on top, each fades on its own
    int y = 34;
    for (int i = 0; i < NFEED; i++) {
        if (feed_t[i] <= 0) continue;
        int w = ui_tw(1, feed[i]) + 14;
        ui_card(4, y, w, 13); gfx_rect(5, y + 2, 2, 9, 38, 146, 222, 0);
        ui_text(10, y + 3, 1, feed[i], UI_INK);
        feed_t[i]--; y += 16;
    }
}

// ---- the life ----
static void start_life(void) {
    if (life_started) return;
    loading("MAKING A NEW LIFE", 20);
    {   // a new world: SAN BOREAS from fresh dice (the land, the towns, every building and room), and its 874 households
        u32 seed = look_rand() * 2654435761u ^ ((u32)VSync(-1) * 40503u) ^ ((u32)frame * 2246822519u); if (!seed) seed = 1;
#if defined(AUTO_CITY) || defined(AUTO_PARTY)
        seed = 0x5EED1234u;                                    // (test builds: the same world every run, so runs compare)
#endif
        bm_new(seed); world_new_seed(seed);
        loading("PEOPLING SAN BOREAS", 45);
        u32 sb; void *sc = crowd_scratch(&sb); pop_new(seed, sc, sb);
        loading("RAISING THE SKYLINE", 60);
        far_new_world();
        loading("MAKING A NEW LIFE", 70);
    }
    world_reset_all();
    for (int i = 0; i < nsims; i++) look_release(&sims[i].bore);   // (the last household's face textures: for this one)
    sim_init();
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 5; py = 12; }
    sim_add(&edit, px, py, 0); look_own_face(&sims[0].bore);     // (you, with faces of your own: the creator keeps its own)
    // two housemates from the dice
    Bore b; look_default(&b); b.stage = AG_ADULT; look_random(&b); sim_add(&b, px + 2, py + 1, 1);
    strcpy(sims[1].bore.first, "MOSS"); sims[1].grump = 1;   // MOSS will not have a thing floating overhead. Do not ask.
    look_default(&b); b.stage = AG_CHILD; look_random(&b); strcpy(b.last, edit.last); b.asp = AS_GROW; look_build(&b); sim_add(&b, px - 1, py + 2, 1);
    strcpy(sims[1].bore.last, edit.last);                     // (one family, one name)
    {   // the family tree: you and MOSS together, the child yours (or, if you made a kid, MOSS's two); a few days into their lives already
        Sim *a = &sims[0], *m = &sims[1], *k = &sims[2];
        if (a->bore.stage >= AG_ADULT) { a->partner = m->uid; m->partner = a->uid; k->par[0] = a->uid; k->par[1] = m->uid; }
        else { a->par[0] = m->uid; k->par[0] = m->uid; }
        a->age_days = (u8)(a->bore.stage >= AG_ADULT ? 3 : 1); m->age_days = 6; k->age_days = (u8)MAX(0, sim_stage_len(AG_CHILD) - 2);   // (the child's birthday comes in a couple of days)
        a->rel[1] = m->rel[0] = 40; a->rel[2] = k->rel[0] = m->rel[2] = k->rel[1] = 30;
    }
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
static int lying_drop(const Bore *b) {   // a Bore lying on its back (voxel.c's sleep pose): how far its back reaches below where it is put
    int mn = 0; for (int i = 0; i < b->ncell; i++) for (int k = 0; k < 8; k++) if (b->cell[i].c[k].vz < mn) mn = b->cell[i].c[k].vz;
    return -mn - 40;
}
int dbg_hh_spr, dbg_hh_full, dbg_hh_miss;
#define LOD_DIST 1280   // beyond this many world units (5 tiles) from the camera's focus, people are drawn with the crowd model
static void draw_sims(void) {
    gfx_set_reserve(RESERVE_UI);   // the people may use what the world left, but not the HUD's share
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->lvl > world_view_lvl || s->away) continue;           // upstairs, out of the floor view; at school
        if (!gfx_visible(s->x, -110 - s->lvl * WALL_FULL, s->z, 150)) continue;   // off the screen
        VECTOR p; setVector(&p, s->x, 0, s->z);
        if (s->action == ACT_SLEEP) { char bc = world_at(s->act_x, s->act_y);   // on the bed, on the crib's mattress, or on the floor
            p.vy = (bc == 'S' || bc == '<' || bc == 'z') ? -items_height('S') + 40 : -((bc == CH_CRIB ? 4 * IZ : 0) + lying_drop(&s->bore)); }
        if (s->action == ACT_SIT || s->action == ACT_WC || (s->action == ACT_EAT && s->gait.sit)) p.vy = -items_height(world_at(s->act_x, s->act_y)) / 2 + 10;
        if (s->action == ACT_SWIM) p.vy = 30 + (isin((frame * 60) & 4095) >> 9);   // afloat, bobbing
        s->gait.mood = s->mood; s->gait.talk = s->action == ACT_TALK; s->gait.seed = i; s->gait.clock = frame;
        p.vy -= s->lvl * WALL_FULL;
        {   // far from where you are looking: the same person at half the voxel resolution (the crowd model); close up, the full model
            int fdx = s->x - cam.at.vx, fdz = s->z - cam.at.vz;
            PROF_BEGIN();
            int edx = s->x - cam.eye.vx, edy = -110 - s->lvl * WALL_FULL - cam.eye.vy, edz = s->z - cam.eye.vz;
            // further off: the same full model as a sprite, rendered again when it has changed enough (one a frame for the household)
            // (the household, not you: a sprite unless it would stand over 90 px tall, where a sprite cell would be too big)
            int far = i != player && (fdx * fdx + fdz * fdz > 3 * TILE * 3 * TILE || edx * edx + edy * edy + edz * edz > 1100 * 1100);
            // (a render is a whole model drawn: only when the last frame left the time for one, or every 6th frame anyway)
            static int hh_fr, hh_used, hh_skip; if (hh_fr != frame) { hh_fr = frame; hh_used = 0; hh_skip++; }
            int drawn = 0;
            if (far) { int id = CROWD_MAX + i, room = gfx_last_cpu + crowd_render_hb + 30 < 522 || (gfx_last_cpu > 540 && gfx_last_cpu + crowd_render_hb + 30 < 785);
                       int want = !hh_used && crowd_sprite_score_of(id, &p, s->yaw, &s->gait) > 5 && (room || hh_skip >= 6);
                       drawn = crowd_sprite_of(id, &s->bore, &p, s->yaw, &s->gait, want);
                       if (want) { hh_used = 1; hh_skip = 0; } }
            if (!drawn) voxel_draw(&s->bore, &p, s->yaw, &s->gait);
            if (drawn) dbg_hh_spr++; else dbg_hh_full++;
            if (far && !drawn) dbg_hh_miss++;
            PROF_END(4);
        }
        if (s->action != ACT_SLEEP && s->action != ACT_SWIM && s->lvl == 0) { VECTOR sp; setVector(&sp, s->x, 0, s->z); gfx_shadow(&sp, 70); }
    }
    citizens_draw();
    if (nsims && !sims[player].grump) draw_marker(&sims[player]);   // MOSS gets nothing. MOSS made that very clear.
    gfx_set_reserve(0);
}

static void draw_bubbles(void) {   // thoughts and notes over heads
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->think < 0 && s->note_t <= 0) continue;
        if (s->lvl > world_view_lvl || s->away) continue;
        VECTOR p; setVector(&p, s->x, -s->lvl * WALL_FULL, s->z);
        gfx_set_object(&p, 0);
        SVECTOR top; setVector(&top, 0, -H * VOX - 40, 0);
        int sx, sy, sz;
        if (!gfx_project(&top, &sx, &sy, &sz)) continue;
        const char *txt = s->note_t > 0 ? s->note : sim_need_name(s->think);
        ui_bubble(sx, sy, txt, s->note_t > 0 ? UI_INK : UI_BADT);
    }
    citizens_draw_bubbles();
}

static int venue_t, venue_bx = -1, venue_by = -1; static PlaceInfo venue;
static void venue_watch(void) {   // walking onto a place: its name and its hours, for a few seconds (The Sims 4's lot name)
    PlaceInfo pi; const Sim *s = &sims[player];
    int in = place_at(sim_tile_x(s), sim_tile_y(s), &pi);
    if (in && (pi.bx != venue_bx || pi.by != venue_by)) { venue = pi; venue_t = 300; }
    if (in) { venue_bx = pi.bx; venue_by = pi.by; } else venue_bx = venue_by = -1;
}
static const s8 lotIc[LOT_N] = { -1, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_MONEY, IC_FOOD, IC_BOLT, IC_CASE, IC_BED, IC_DROP, IC_BOOK,
                                 IC_PLAY, IC_CASE, IC_BED, IC_TREE, IC_PEOPLE, IC_FAST, -1, IC_SPROUT,
                                 IC_FOOD, IC_NOTE, IC_CAMERA, IC_SMILE, IC_FOOD, IC_HEART, IC_BOOK, IC_DROP, IC_BRUSH };
static void venue_draw(void) {
    char sub[32], h[8]; int open_ = place_open(venue.lot, clock_min, clock_day);
    if (job_car != CAR_FACTORY && venue.bx == job_bx && venue.by == job_by) strcpy(sub, "YOUR WORKPLACE");
    else if (!open_) sprintf(sub, "CLOSED TILL %s", hour_name(place_opens_at(venue.lot), h));
    else if (place_opens_at(venue.lot) == 0 && place_closes_at(venue.lot) == 24) strcpy(sub, "OPEN ALL DAY");
    else sprintf(sub, "OPEN TILL %s", hour_name(place_closes_at(venue.lot), h));
    int tw_ = MAX(ui_tw(1, venue.name), ui_tw(1, sub)), ic = tw_ + 34 <= 140, w = tw_ + (ic ? 34 : 14), x = 316 - w, y = 52;   // (on the right, under the want)
    ui_card(x, y, w, 27);
    if (ic) ui_icon_disc(x + 13, y + 13, 20, lotIc[venue.lot % LOT_N], UI_BLUE, UI_WHITE);
    ui_text(x + (ic ? 27 : 7), y + 4, 1, venue.name, UI_INK);
    ui_text(x + (ic ? 27 : 7), y + 15, 1, sub, open_ ? UI_GOODT : UI_BADT);
}
static const char *const aspLvNm[4] = { "RED", "GREEN", "GOLD", "PLATINUM" };
static const int aspLvCol[4][3] = { {255,80,70}, {90,220,90}, {255,200,40}, {220,235,255} };
static int cheer_t, cheer_new; static char cheer_a[32], cheer_b[40];
static struct { s16 x, y, vx, vy; u8 c, w; } conf[40];
static u32 conf_rs = 0x9E3779B9u;
static int conf_rn(int n) { conf_rs ^= conf_rs << 13; conf_rs ^= conf_rs >> 17; conf_rs ^= conf_rs << 5; return (int)((conf_rs >> 4) % (u32)n); }
static void cheer_poll(void) {   // sim.c says a birthday or a newcomer happened: the card and the confetti
    if (!sim_cheer) return;
    int i = ABS(sim_cheer) - 1, nw = sim_cheer < 0; sim_cheer = 0;
    if (i < 0 || i >= nsims) return;
    const Sim *o = &sims[i]; const char *st = look_stage_name(o->bore.stage);
    cheer_new = nw; cheer_t = 330;
    if (nw) { sprintf(cheer_a, "WELCOME, %s!", o->bore.first); sprintf(cheer_b, o->bore.stage == AG_BABY ? "A NEW BABY IN THE FAMILY" : "A NEW CHILD IN THE FAMILY"); }
    else { strcpy(cheer_a, "HAPPY BIRTHDAY!"); sprintf(cheer_b, "%s IS NOW %s %s", o->bore.first, (st[0] == 'A' || st[0] == 'E') ? "AN" : "A", st); }
    conf_rs ^= (u32)frame * 2654435761u; if (!conf_rs) conf_rs = 1;
    for (int k = 0; k < 40; k++) { conf[k].x = (s16)(conf_rn(320) << 4); conf[k].y = (s16)(-(conf_rn(140) << 4)); conf[k].vx = (s16)(conf_rn(33) - 16); conf[k].vy = (s16)(16 + conf_rn(24));
                                   conf[k].c = (u8)conf_rn(6); conf[k].w = (u8)(2 + conf_rn(3)); }
}
static void cheer_draw(void) {
    static const u8 pal[6][3] = { {255, 90, 110}, {255, 200, 50}, {90, 200, 255}, {120, 220, 100}, {200, 120, 255}, {255, 150, 60} };
    if (cheer_t <= 0) return;
    cheer_t--;
    if (state != ST_LIFE) return;
    if (!cheer_new || cheer_t > 200) for (int k = 0; k < 40; k++) {   // the confetti, tumbling down (a newcomer gets a short shower)
        conf[k].x += conf[k].vx + (isin(((frame + k * 37) * 90) & 4095) >> 10); conf[k].y += conf[k].vy;
        int x = conf[k].x >> 4, y = conf[k].y >> 4;
        if (y < 0 || y > 240 || x < 0 || x > 318) continue;
        const u8 *c = pal[conf[k].c]; int tall = ((frame + k) >> 3) & 1;   // (it flutters: flat, then edge on)
        gfx_rect(x, y, tall ? 2 : conf[k].w, tall ? conf[k].w : 2, c[0], c[1], c[2], 0);
    }
    int w = MAX(ui_tw(2, cheer_a), ui_tw(1, cheer_b)) + 46, x = 160 - w / 2, y = 50;
    ui_card(x, y, w, 31);
    ui_icon_disc(x + 18, y + 15, 24, cheer_new ? IC_BABY : IC_CAKE, cheer_new ? UI_RGB(236, 120, 160) : UI_RGB(255, 180, 40), UI_WHITE);
    ui_text(x + 34, y + 3, 2, cheer_a, cheer_new ? UI_RGB(200, 70, 120) : UI_RGB(200, 120, 10));
    ui_text(x + 34, y + 19, 1, cheer_b, UI_INK);
}
static void hud_life(void) {
    if (!nsims) return;
    Sim *s = &sims[player];
    char b[48];
    int m = s->mood;
    u32 moodc = m > 15 ? UI_GREEN : m > -15 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70);
    u32 moodt = m > 15 ? UI_RGB(40, 140, 40) : m > -15 ? UI_RGB(170, 110, 10) : UI_RGB(200, 60, 50);   // (the same, deep enough to read on white)
    // the household, top left: round portraits, a ring in each one's mood colour, the one you play ringed blue with the plumbob
    for (int i = 0; i < nsims; i++) {
        const Sim *o = &sims[i];
        int x = 15 + i * 24, y = 16, me = i == player, mm = o->mood;
        u32 ring = me ? UI_BLUE : o->away ? UI_RGB(176, 186, 200) : mm > 15 ? UI_GREEN : mm > -15 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70);
        ui_disc(x, y, 23, UI_WHITE); ui_disc(x, y, 21, ring); ui_disc(x, y, 17, o->away ? UI_RGB(222, 228, 236) : UI_RGB(234, 240, 248));
        if (o->away) ui_icon(x - 6, y - 6, IC_BOOK, UI_RGB(112, 126, 146));   // at school till 3
        else draw_face_k(x - 8, y - 8, mm, 1);
        if (o->bore.stage == AG_BABY) { ui_disc(x + 8, y + 8, 11, UI_WHITE); ui_icon_s(x + 4, y + 4, IC_BABY, 8, UI_RGB(236, 120, 160)); }   // (a baby)
        if (o->bday && !o->away) { ui_disc(x - 8, y + 8, 12, UI_RGB(255, 200, 60)); ui_icon_s(x - 12, y + 4, IC_CAKE, 8, UI_WHITE); }   // a birthday today
        if (me) ui_icon(x - 6, y - 19 + ((frame >> 4) & 1), IC_PLUMBOB, UI_GREEN);
    }
    draw_action_card(s);
    // the needs, bottom left: a white card, an icon and a bar for each (green, then yellow, then red)
    static const s8 nic[NEED_N] = { IC_FOOD, IC_DROP, IC_BED, IC_BUBBLES, IC_SOFA, IC_SMILE, IC_CHAT, IC_HOUSE };
    ui_card(4, 184, 166, 52);
    for (int n = 0; n < NEED_N; n++) {
        int v = sim_need_val(s, n), cx = 10 + (n >> 2) * 80, cy = 189 + (n & 3) * 11;
        int r, g, bb; need_rgb(v, &r, &g, &bb);
        ui_icon(cx, cy - 1, nic[n], v < 20 ? UI_RGB(220, 60, 50) : UI_SOFT);
        ui_bar(cx + 14, cy + 1, 60, 7, v, UI_RGB(r, g, bb));
    }
    // the Bore, bottom right: the mood on a big round badge, the name, how they feel, what X would do
    ui_card(174, 184, 142, 52);
    ui_disc(192, 202, 28, moodc); ui_disc(192, 202, 24, UI_RGB(250, 252, 255));
    draw_face_k(184, 194, m, 1);
    ui_text(210, 188, 2, s->bore.first, UI_INK);
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    ui_text(210, 203, 1, mood, moodt);
    if (s->bore.stage == AG_CHILD || s->bore.stage == AG_TEEN) sprintf(b, "GRADE %s", sim_grade(s->grade)); else sprintf(b, "LVL %d", job_level);   // (a kid: the school grade)
    ui_text(212 + ui_tw(1, mood) + 4, 203, 1, b, UI_SOFT);
    { int lv = wants_level(s);                                   // the aspiration meter, under the mood
      ui_bar(210, 213, 100, 5, 100 * s->asp_pts / ASP_MAX, UI_RGB(aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2])); }
    const char *hint = sim_hint();
    { const char *ch = citizens_hint(); if (ch[0] && !sim_front_service() && (s->action == ACT_NONE || s->action == ACT_WALK)) hint = ch; }   // (busy: X gets up)
    if (hint[0]) { sprintf(b, "X %s", hint); ui_round(180, 222, 132, 11, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_text(185, 224, 1, b, UI_WHITE); }
    else ui_text(182, 224, 1, "START: MENU", UI_SOFT);
    // the clock and the money, top right
    ui_card(220, 4, 96, 28);
    sprintf(b, "%s %02d:%02d", sim_day_name(clock_day), clock_min / 60, clock_min % 60);
    ui_text(226, 7, 2, b, UI_INK);
    sprintf(b, "$%ld", cash); ui_text(226, 21, 1, b, UI_RGB(40, 150, 50));
    { int ic = sim_speed <= 1 ? IC_PLAY : IC_FAST; ui_icon_disc(305, 24, 12, ic, sim_speed >= 3 ? UI_RGB(236, 150, 40) : UI_BLUE, UI_WHITE); }
    { int day_ = clock_min >= 360 && clock_min < 1200; ui_icon(286, 19, IC_SUN, day_ ? UI_RGB(236, 170, 30) : UI_RGB(120, 140, 200)); }
    {   // the want, under the clock
        char t[32]; wants_text(s, (frame / 240) & 3, t);
        int w = ui_tw(1, t) + 22;
        ui_card(316 - w, 35, w, 14); ui_icon(320 - w, 36, IC_STAR, UI_RGB(236, 170, 30)); ui_text(334 - w, 38, 1, t, UI_INK);
        if (wants_flash_t > 0 && state == ST_LIFE) { int fw = ui_tw(2, wants_flash) + 16; ui_card(160 - fw / 2, 86, fw, 18); ui_textc(160, 89, 2, wants_flash, wants_flash[0] == 'W' ? UI_RGB(40, 150, 50) : UI_RGB(200, 60, 50)); wants_flash_t--; }
    }
    feed_draw();
    if (venue_t > 0) { venue_draw(); venue_t--; }
    cheer_poll(); cheer_draw();
    citizens_hud();
    if (frame < 900) ui_hintbar(144, "L1 R1 TURN|L2 R2 ZOOM|X USE|SQ RUN");
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
        else { pie_cit = 1; pie_n = sim_front_service() ? 0 : citizens_pie_open(pie_opt);   // a civilian in front of you first, then the things (a counter right ahead wins)
               if (!pie_n) { pie_cit = 0; pie_n = sim_pie_open(pie_opt); }
               if (pie_n == 1) { if (pie_cit) citizens_pie_do(0); else sim_pie_do(0); } else if (pie_n > 1) { pie_sel = 0; state = ST_PIE; } else audio_voice(VOICE_NAH); }
    }
    if (PRESSED(PAD_CIRCLE)) sim_player_cancel();
    if (PRESSED(PAD_TRIANGLE)) { sim_speed = sim_speed % 3 + 1; }
    if (PRESSED(PAD_L3)) show_fps ^= 1;
    { extern int getenv_noshadow, dbg_noao, dbg_nograd; if (PRESSED(PAD_R3)) { static int mode; mode = (mode + 1) & 3; getenv_noshadow = mode == 1; dbg_noao = mode == 2; dbg_nograd = mode == 3; } }
    if (PRESSED(PAD_SELECT)) { sim_switch_player(); sim_note("NOW PLAYING AS THE NEXT BORE"); }
    { int t0 = hb_now(); for (int k = 0; k < steps; k++) sim_step(); citizens_update(steps); t_logic = hb_now() - t0; }
    {   // SAN BOREAS: the map window follows you a block at a time (everything slides with it)
        int dtx, dty;
#ifdef AUTO_CITY
        { static int fr_, done_; fr_ += steps;   // (test builds: by the clock, 60 a second, so runs at any frame rate line up)
#if defined(AUTO_TOUR)
          (void)0;
#elif defined(AUTO_PLACES)
          { extern int sim_debug_goto(char c, int lot);   // (test builds: a job at the nearest diner, a ride there, work; a swim; a meal; a shop)
            static const struct { u16 at; s8 lot; char c; } stp[4] = { { 400, LOT_DINER, 'y' }, { AUTO_PLACES, LOT_POOL, '~' }, { AUTO_PLACES + 700, LOT_DINER, 'y' }, { AUTO_PLACES + 1300, LOT_SHOP, 'y' } };
            if (fr_ >= 120 && !(done_ & 1)) { done_ |= 1; jobs_open(); for (int k = 0; k < jobs_n; k++) if (jobs_car[k] == CAR_FOOD) { jobs_sel = k + 1; break; }
                const Place *p = &pop_place[jobs_pl[jobs_sel - 1]]; char nm[24]; bm_place_name(p->lot, bm_hash(p->bx, p->by, 3), nm); sim_take_job(CAR_FOOD, p->bx, p->by, nm);
                taxi_to_block(job_bx, job_by, -1, -1, job_place); }
            for (int k = 0; k < 4; k++) {
                int b0 = 2 << (k * 2), b1 = b0 << 1;
                if (fr_ >= stp[k].at && !(done_ & b0)) { done_ |= b0;      // there, or a taxi to the nearest one first
                    if (sim_debug_goto(stp[k].c, stp[k].lot)) done_ |= b1;
                    else { int bi = -1, bd = 1 << 30, pbx = world_wbx + sim_tile_x(s) / BM_BLK, pby = world_wby + sim_tile_y(s) / BM_BLK;
                           for (int i = 0; i < pop_nplace; i++) if (pop_place[i].lot == stp[k].lot) { int d = ABS(pop_place[i].bx - pbx) + ABS(pop_place[i].by - pby); if (d < bd) { bd = d; bi = i; } }
                           if (bi >= 0) taxi_to_block(pop_place[bi].bx, pop_place[bi].by, -1, -1, "TEST"); } }
                if (fr_ >= stp[k].at + 200 && !(done_ & b1)) { done_ |= b1; sim_debug_goto(stp[k].c, stp[k].lot); }
                if (fr_ >= stp[k].at && fr_ < stp[k].at + 30) { cam_pitch = 760; cam_dist = 1800; }
            } }
#elif defined(AUTO_TOWER)
          if (fr_ >= 120 && !(done_ & 1)) { done_ |= 1; const Settle *cp = bm_set(bm_capital()); taxi_to_block(cp->x + AUTO_TOWER, cp->y + 2, 8, 1, "TEST"); }
          if (fr_ > 120) { cam_pitch = 760; cam_dist = 3600; cam_yaw = 2048 + 300; }
          if (fr_ > 180 && fr_ < 800 && s->action == ACT_NONE) s->x += 3 * steps;
#endif
#if !defined(AUTO_TOWER) && !defined(AUTO_PLACES) && !defined(AUTO_TOUR)
          if (fr_ > 240 && s->action == ACT_NONE) s->x += 11 * steps;   // (stroll east along the street)
          if (fr_ > 2000) { cam_pitch = 700; cam_dist = 4200; cam_yaw = 2048 + 500; }   // (then look over the roofs)
#ifdef AUTO_LOW
          if (fr_ > 600) { cam_pitch = AUTO_LOW; cam_dist = 2200; cam_yaw = (cam_yaw + 3 * steps) & 4095; }   // (a low camera turning round: the horizon)
#endif
          if (fr_ >= 400 && !(done_ & 2)) { done_ |= 2; extern int prof_on; prof_on = 1; }
#endif
        }
#endif
        int ts_ = hb_now();
        if (world_stream(sim_tile_x(s), sim_tile_y(s), &dtx, &dty)) { sim_shift(dtx, dty); citizens_shift(dtx, dty); cam_fx -= dtx * TILE; cam_fz += dty * TILE;
            { extern int dbg_shift_hb, dbg_shifts; int d = hb_now() - ts_; if (d > dbg_shift_hb) dbg_shift_hb = d; dbg_shifts++; } }
    }
    feed_poll();
    if (!(frame & 7)) venue_watch();
    camera_input();
    if (PRESSED(PAD_START)) { state = ST_PAUSE; menu_sel = 0; }
    if (travel_asked) { travel_asked = 0; travel_sel = world_zone; state = ST_TRAVEL; }
    if (sim_story_over) { sim_story_over = 0; state = ST_OVER; }   // the last of the family is gone
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
        u32 top = (u32)tr | ((u32)tg << 8) | ((u32)tb << 16), hor = (u32)br | ((u32)bg << 8) | ((u32)bb << 16);
        { extern u32 world_horizon; extern int world_far_range; world_horizon = hor; world_far_range = (world_zone == ZONE_CITY && far_view) ? 60 : 34; }
        if (world_zone == ZONE_CITY && far_view) far_sky(top, hor);   // the sky meets the land at the real horizon
        else gfx_sky(top, hor);
    }
    gfx_prim_count = 0;
    { int t0 = hb_now(); world_draw(sim_tile_x(s), sim_tile_y(s), frame, 1); t_world = hb_now() - t0;
      t0 = hb_now(); gfx_prim_count = 0; draw_sims(); t_sims = hb_now() - t0; }
    { int t0 = hb_now(); draw_bubbles(); t_bub = hb_now() - t0; }
    prims_last = gfx_prim_count;
    if (state != ST_OVER) { int t0 = hb_now(); hud_life(); t_hud = hb_now() - t0; }
}

// ---- travel between the zones ----
static void travel_go(int z) {
    { char b[40]; sprintf(b, "TRAVELLING TO %s", world_zone_name(z)); loading(b, 50); }
    world_switch(z);
    int px, py;
    if (z == ZONE_CITY) { world_arrival(bm_capital(), &px, &py); far_reset(); }
    else { world_find('P', &px, &py); if (px < 0) { px = 20; py = 20; } }
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
static void travel_draw(void) {   // where to: the four places, each on a row with its picture, the locked ones greyed with what opens them
    static const char *const blurb[ZONE_N] = { "HOUSE, FACTORY, RAIL PARK", "A CAMP, PALMS AND THE SEA", "DINER, TRAILER AND THE LAB", "" };
    static const char *const lockTxt[ZONE_N] = { "", "EARN YOUR FIRST PAYCHECK", "GET PROMOTED TWICE", "" };
    static const s8 zic[ZONE_N] = { IC_HOUSE, IC_TREE, IC_SUN, IC_PIN };
    char b[48];
    life_draw();
    ui_dim();
    int top = ui_window(36, 24, 248, 192, "WHERE TO?", IC_CAR);
    for (int i = 0; i < ZONE_N; i++) {
        int y = top + 2 + i * 34, sel = i == travel_sel, here = i == world_zone, open_ = zone_unlocked(i);
        if (sel) { ui_round(42, y, 236, 31, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(42, y + 4, 2, 23, 38, 146, 222, 0); }
        ui_icon_disc(61, y + 15, 22, open_ ? zic[i] : IC_LOCK, !open_ ? UI_RGB(176, 184, 196) : sel ? UI_BLUE : UI_RGB(110, 146, 190), UI_WHITE);
        ui_text(78, y + 6, 1, world_zone_name(i), !open_ ? UI_SOFT : sel ? UI_BLUE2 : UI_INK);
        const char *t = !open_ ? lockTxt[i] : blurb[i];
        if (i == ZONE_CITY) { int nf = 0; while (nf < NFAM && pop_fam[nf].n) nf++; sprintf(b, "%d TOWNS, %d HOUSEHOLDS", bm_nset(), nf); t = b; }
        if (here) t = "YOU ARE HERE";
        ui_text(78, y + 18, 1, t, here ? UI_GOODT : open_ ? UI_SOFT : UI_BADT);
    }
    ui_hints(160, top + 145, "X GO|O BACK", 1);
}

// ---- the pause menu ----
// ---- SAN BOREAS: the map (and a taxi anywhere on it) ----
// The map is a 128x128 picture of the whole state (8x8 blocks a pixel), made from the generator when it is opened and put
// in the VRAM the people's sprites use (they are rendered again after).
static int map_sel, map_t;
static void put_household(int px, int py) {     // everyone at home stands round (px, py)
    for (int i = 0; i < nsims; i++) {
        Sim *sm = &sims[i];
        sim_player_cancel_sim(sm);
        sm->x = (px + (i % 3)) * TILE + TILE / 2; sm->z = -((py + (i / 3)) * TILE + TILE / 2);
        sm->path_n = 0; sm->lvl = 0;
    }
    cam_fx = sims[player].x; cam_fz = sims[player].z;
}
static void taxi_to_block(int bx, int by, int tx, int ty, const char *where) {   // tx, ty: the tile to stand on in that block (-1: its street)
    loading("TAKING A TAXI", 60);
    if (world_zone != ZONE_CITY) world_switch(ZONE_CITY);
    world_open_at(bx, by);
    loading("TAKING A TAXI", 85); far_reset();                  // the land round you, out to the horizon
    int px = (bx - world_wbx) * BM_BLK + (tx >= 0 ? tx : 8), py = (by - world_wby) * BM_BLK + (ty >= 0 ? ty : 1);
    put_household(px, py);
    citizens_enter(ZONE_CITY);
    char b[48]; snprintf(b, sizeof b, "WELCOME TO %s", where); sim_note(b);
    audio_voice(VOICE_YAHOO);
}
static void map_open(void) {
    u16 *px = atlas_scratch();
    static const u16 tcol[6] = { RGB(4, 9, 19), RGB(25, 23, 15), RGB(10, 18, 7), RGB(5, 12, 5), RGB(24, 20, 12), RGB(15, 15, 15) };
    for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) px[y * 128 + x] = tcol[bm_terrain(x * 8 + 4, y * 8 + 4)] | 0x8000;
    for (int i = 0; i < bm_nseg(); i++) {         // the highways
        int c, a, b, v; bm_seg(i, &c, &a, &b, &v);
        for (int k = a; k <= b; k += 4) { int x = v ? c / 8 : k / 8, y = v ? k / 8 : c / 8; px[y * 128 + x] = RGB(9, 9, 10) | 0x8000; }
    }
    for (int i = 0; i < bm_nset(); i++) {          // the towns
        const Settle *st = bm_set(i); int r = MAX(1, st->r / 8);
        u16 col = st->kind == ST_CITY ? RGB(30, 22, 12) : st->kind == ST_TOWN ? RGB(28, 25, 16) : RGB(24, 22, 18);
        for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x * x + y * y <= r * r + 1) {
            int X = st->x / 8 + x, Y = st->y / 8 + y; if (X >= 0 && Y >= 0 && X < 128 && Y < 128) px[Y * 128 + X] = col | 0x8000; }
    }
    atlas_vram_load(0, 256, 128, 128, px);
    crowd_flush();
    map_sel = 0; map_t = 0;
    if (world_zone == ZONE_CITY && world_wbx >= 0) {   // start on the nearest town
        int pbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK, pby = world_wby + sim_tile_y(&sims[player]) / BM_BLK, bd = 1 << 30;
        for (int i = 0; i < bm_nset(); i++) { const Settle *st = bm_set(i); int d = (st->x - pbx) * (st->x - pbx) + (st->y - pby) * (st->y - pby); if (d < bd) { bd = d; map_sel = i; } }
    }
}
static void map_update(void) {
    if (map_t < 0) map_open();
    map_t++;
    int n = bm_nset();
    if (PRESSED(PAD_UP)) map_sel = (map_sel + n - 1) % n;
    if (PRESSED(PAD_DOWN)) map_sel = (map_sel + 1) % n;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) {
        if (cash < 20) { sim_note("A TAXI IS $20"); audio_voice(VOICE_NAH); return; }
        cash -= 20;
        const Settle *st = bm_set(map_sel);
        taxi_to_block(st->x, st->y, -1, -1, st->name);
        state = ST_LIFE;
    }
}
static void map_draw(void) {   // the state on a card: the map on the left (the towns as dots, the picked one named), the list on the right
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    char b[48];
    int top = ui_window(4, 4, 312, 232, bm_state_name(), IC_PIN);
    ui_badge(306 - ui_tw(1, "TAXI $20") - 10, 8, "TAXI $20", UI_WHITE, UI_BLUE2);
    const int MX = 9, MY = top, MS = 178;
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, 128, 128, 128, 128, 128);   // the picture (on the card: drawn with the 2D, not the scene)
    int n = bm_nset();
    for (int i = 0; i < n; i++) {                  // every town a dot: a city big, a village small
        const Settle *st = bm_set(i); if (i == map_sel) continue;
        int x = MX + st->x * MS / BM_N, y = MY + st->y * MS / BM_N, d = st->kind == ST_CITY ? 7 : st->kind == ST_TOWN ? 5 : 3;
        ui_disc(x, y, d + 2, UI_RGB(40, 46, 60)); ui_disc(x, y, d, UI_WHITE);
    }
    if (world_zone == ZONE_CITY && world_wbx >= 0) {   // you are here
        int wx = world_wbx * BM_BLK + sim_tile_x(&sims[player]), wy = world_wby * BM_BLK + sim_tile_y(&sims[player]);
        int x = MX + wx * MS / (BM_N * BM_BLK), y = MY + wy * MS / (BM_N * BM_BLK);
        ui_disc(x, y, (map_t >> 3) & 1 ? 11 : 9, UI_WHITE); ui_disc(x, y, 7, UI_GREEN);
    }
    {   // the picked town: a ring, and its name on a blue pill
        const Settle *st = bm_set(map_sel); int x = MX + st->x * MS / BM_N, y = MY + st->y * MS / BM_N;
        ui_disc(x, y, (map_t >> 3) & 1 ? 13 : 11, UI_BLUE); ui_disc(x, y, 6, UI_WHITE);
        int w = ui_tw(1, st->name) + 10, lx = x + 9, ly = CLAMP(y - 6, MY + 1, MY + MS - 13);
        if (lx + w > MX + MS - 1) lx = MAX(x - 9 - w, MX + 1);
        ui_shadow(lx, ly, w, 12, 4); ui_round(lx, ly, w, 12, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_text(lx + 5, ly + 2, 1, st->name, UI_WHITE);
    }
    int rows = 15, first = CLAMP(map_sel - 7, 0, MAX(0, n - rows));
    for (int k = 0; k < rows && first + k < n; k++) {   // the list
        int i = first + k, y = top + 1 + k * 12, on = i == map_sel; const Settle *st = bm_set(i);
        if (on) { ui_round(192, y - 2, 118, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(192, y, 2, 8, 38, 146, 222, 0); }
        ui_disc(199, y + 3, st->kind == ST_CITY ? 7 : st->kind == ST_TOWN ? 5 : 4, on ? UI_BLUE : st->kind == ST_CITY ? UI_BLUE2 : UI_RGB(150, 164, 184));
        ui_text(206, y, 1, st->name, on ? UI_BLUE2 : UI_INK);
    }
    const Settle *st = bm_set(map_sel);
    static const char *const kn[3] = { "CITY", "TOWN", "VILLAGE" };
    sprintf(b, "%s, %d BLOCKS ACROSS", kn[st->kind % 3], st->r * 2 + 1);
    gfx_rect(10, 211, 300, 1, 200, 210, 222, 0);
    ui_text(12, 218, 1, b, UI_SOFT);
    ui_hints(310, 215, "X TAXI|O BACK", 2);
}

// ---- SAN BOREAS: the people (every household, who they are, where they are right now) ----
static int dir_sel, dir_fam = -1, dir_msel, dir_t, dir_counts[5];
static void dir_open(void) { dir_fam = -1; dir_msel = 0; dir_t = 0; for (int i = 0; i < 5; i++) dir_counts[i] = pop_count_where(i); }
static void dir_update(void) {
    dir_t++;
    int nf = 0; while (nf < NFAM && pop_fam[nf].n) nf++;
    if (!nf) { if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = ST_LIFE; return; }
    if (dir_fam < 0) {
        if (PRESSED(PAD_UP)) dir_sel = (dir_sel + nf - 1) % nf;
        if (PRESSED(PAD_DOWN)) dir_sel = (dir_sel + 1) % nf;
        if (PRESSED(PAD_L1)) dir_sel = MAX(0, dir_sel - 15);
        if (PRESSED(PAD_R1)) dir_sel = MIN(nf - 1, dir_sel + 15);
        if (PRESSED(PAD_L2)) dir_sel = MAX(0, dir_sel - 150);
        if (PRESSED(PAD_R2)) dir_sel = MIN(nf - 1, dir_sel + 150);
        if (PRESSED(PAD_CROSS)) { dir_fam = dir_sel; dir_msel = 0; }
        if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = ST_LIFE;
        return;
    }
    const Fam *f = &pop_fam[dir_fam];
    if (PRESSED(PAD_UP)) dir_msel = (dir_msel + f->n - 1) % f->n;
    if (PRESSED(PAD_DOWN)) dir_msel = (dir_msel + 1) % f->n;
    if (PRESSED(PAD_CIRCLE)) { dir_fam = -1; return; }
    if (PRESSED(PAD_CROSS)) {                      // go and knock: a taxi to their front door
        if (cash < 20) { sim_note("A TAXI IS $20"); audio_voice(VOICE_NAH); return; }
        cash -= 20;
        char sur[NAME_LEN], b[40]; pop_surname(dir_fam, sur); snprintf(b, sizeof b, "THE %sS' HOUSE", sur);
        taxi_to_block(f->bx, f->by, 8, 1, b);
        int h = world_home_find(ZONE_CITY, f->bx, f->by, f->unit);
        if (h >= 0) { const Home *hm = world_home(ZONE_CITY, h); put_household(hm->dx, hm->dy + 1); }
        state = ST_LIFE; dir_fam = -1;
    }
}
static void dir_draw(void) {
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    char b[64], sur[NAME_LEN], fn[NAME_LEN], addr[32];
    int nf = 0; while (nf < NFAM && pop_fam[nf].n) nf++;
    if (dir_fam < 0) {
        int top = ui_window(4, 4, 312, 232, "THE PEOPLE OF SAN BOREAS", IC_PEOPLE);
        sprintf(b, "%d HOUSEHOLDS, %d PEOPLE", nf, dir_counts[0] + dir_counts[1] + dir_counts[2] + dir_counts[3] + dir_counts[4]); ui_text(12, top - 1, 1, b, UI_SOFT);
        {   // where everyone is now, as little coloured pills
            static const char *const nm[5] = { "ASLEEP", "HOME", "WORK", "SCHOOL", "OUT" };
            static const s8 ic[5] = { IC_BED, IC_HOUSE, IC_CASE, IC_BOOK, IC_CAR };
            static const u32 cc[5] = { UI_RGB(120, 120, 190), UI_RGB(70, 160, 90), UI_RGB(38, 146, 222), UI_RGB(220, 150, 40), UI_RGB(200, 90, 150) };
            static const u8 at[5] = { AT_SLEEP, AT_HOME, AT_WORK, AT_SCHOOL, AT_OUT };
            int x = 12, w = 0;
            for (int k = 0; k < 5; k++) { sprintf(b, "%s %d", nm[k], dir_counts[at[k]]); w += ui_tw(1, b) + 27; }
            int words = w <= 300;                       // (the words if they fit; the pictures alone if not)
            for (int k = 0; k < 5; k++) { if (words) sprintf(b, "%s %d", nm[k], dir_counts[at[k]]); else sprintf(b, "%d", dir_counts[at[k]]);
                x += ui_badge_i(x, top + 9, ic[k], b, cc[k], UI_WHITE) + 4; }
        }
        int ly = top + 25, rows = 13;
        int first = CLAMP(dir_sel - 6, 0, MAX(0, nf - rows));
        for (int k = 0; k < rows && first + k < nf; k++) {
            int i = first + k, y = ly + k * 12, on = i == dir_sel; const Fam *f = &pop_fam[i];
            pop_surname(i, sur);
            const Settle *st = bm_set(f->set);
            if (on) { ui_round(8, y - 2, 304, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(8, y, 2, 8, 38, 146, 222, 0); }
            sprintf(b, "THE %sS", sur); ui_text(14, y, 1, b, on ? UI_BLUE2 : UI_INK);
            ui_text(124, y, 1, pop_kind_name(f->kind), UI_SOFT);
            sprintf(b, "%d", pop_alive(i)); ui_text(222, y, 1, b, UI_INK);
            ui_text(236, y, 1, st ? st->name : "?", UI_SOFT);
        }
        const Fam *f = &pop_fam[dir_sel]; const Settle *st = bm_set(f->set);
        bm_street(f->set, f->bx, f->by, f->unit, addr);
        gfx_rect(10, 212, 300, 1, 200, 210, 222, 0);
        ui_icon(12, 215, IC_HOUSE, UI_BLUE);
        sprintf(b, "%s, %s", addr, st ? st->name : ""); ui_text(28, 218, 1, b, UI_BLUE2);
        ui_hints(310, 215, "X SEE THEM|O BACK", 2);
        return;
    }
    const Fam *f = &pop_fam[dir_fam]; const Settle *st = bm_set(f->set);
    pop_surname(dir_fam, sur);
    sprintf(b, "THE %sS", sur);
    int top = ui_window(4, 4, 312, 232, b, IC_HOUSE);
    bm_street(f->set, f->bx, f->by, f->unit, addr);
    sprintf(b, "%s - %s, %s", pop_kind_name(f->kind), addr, st ? st->name : ""); ui_text(12, top - 1, 1, b, UI_SOFT);
    for (int k = 0; k < f->n && k < 6; k++) {
        int p = f->first + k, y = top + 12 + k * 28, on = k == dir_msel;
        static const char *const ag[5] = { "BABY", "CHILD", "TEEN", "ADULT", "ELDER" };
        if (on) { ui_round(8, y - 3, 304, 27, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(8, y, 2, 20, 38, 146, 222, 0); }
        ui_icon_disc(22, y + 9, 18, IC_PLUMBOB, on ? UI_BLUE : UI_RGB(150, 166, 186), UI_WHITE);
        pop_first(p, fn);
        sprintf(b, "%s %s", fn, sur); ui_text(36, y, 1, b, on ? UI_BLUE2 : UI_INK);
        sprintf(b, "%s, %s", ag[pop_per[p].stage % 5], pop_trait_name(pop_per[p].trait)); ui_text(310 - ui_tw(1, b), y, 1, b, UI_SOFT);
        char jb[48], db[48]; pop_job_name(p, jb); pop_doing(p, db);
        int dw = ui_tw(1, db);
        if (36 + ui_tw(1, jb) + 8 > 310 - dw) { char *c = strchr(jb, ','); if (c) *c = 0; }   // (no room for both: the job without its place)
        ui_text(36, y + 11, 1, jb, UI_RGB(26, 96, 160));
        ui_text(310 - dw, y + 11, 1, db, UI_RGB(40, 130, 60));
    }
    gfx_rect(10, 212, 300, 1, 200, 210, 222, 0);
    ui_icon(12, 215, IC_CAR, UI_BLUE);
    ui_hints(28, 215, "X TAXI TO THEIR HOUSE $20", 0);
    ui_hints(310, 215, "O BACK", 2);
}

// ---- JOBS: your job (and a free ride to it), and the jobs going at real places near you ----
static const s8 carIc[CAR_N] = { IC_HAMMER, IC_FOOD, IC_MONEY, IC_BOLT, IC_DROP, IC_BED, IC_PLAY, IC_CASE, IC_BOOK, IC_NOTE, IC_HEART, IC_CAMERA, IC_SMILE, IC_BRUSH };
static void jobs_open(void) {
    int rbx, rby;
    if (world_zone == ZONE_CITY && world_wbx >= 0) { rbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK; rby = world_wby + sim_tile_y(&sims[player]) / BM_BLK; }
    else { const Settle *cp = bm_set(bm_capital()); rbx = cp->x; rby = cp->y; }
    jobs_n = job_openings(rbx, rby, jobs_car, jobs_pl, 16);
    jobs_sel = 0; jobs_scroll = 0;
}
static void shift_text(char *out, int car) {   // "7 AM - 3 PM, MON - FRI"
    char h1[8], h2[8], d[12]; const Career *c = &careers[car];
    sprintf(out, "%s - %s, %s", hour_name(c->from, h1), hour_name(c->to, h2), days_name(c->days, d));
}
static void shift_short(char *out, int car) {  // "7AM-3PM MON-FRI"
    char h1[8], h2[8], d[12], *p; const Career *c = &careers[car];
    sprintf(out, "%s-%s %s", hour_name(c->from, h1), hour_name(c->to, h2), days_name(c->days, d));
    for (p = out; *p; p++) if (p[0] == ' ' && (p[1] == 'A' || p[1] == 'P' || p[1] == '-' || (p > out && p[-1] == '-'))) memmove(p, p + 1, strlen(p));
}
static void jobs_update(void) {
    int n = jobs_n + 1;   // (row 0: your own job)
    if (PRESSED(PAD_UP)) jobs_sel = (jobs_sel + n - 1) % n;
    if (PRESSED(PAD_DOWN)) jobs_sel = (jobs_sel + 1) % n;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = ST_LIFE; return; }
    if (!PRESSED(PAD_CROSS)) return;
    if (jobs_sel == 0) {   // go to work: the boss pays the taxi
        state = ST_LIFE;
        if (job_car == CAR_FACTORY) { if (world_zone != ZONE_HOME) travel_go(ZONE_HOME); sim_note("THE FACTORY: THE GOLD MARKER"); }
        else taxi_to_block(job_bx, job_by, -1, -1, job_place);
        return;
    }
    int k = jobs_sel - 1;
    if (jobs_pl[k] < 0) sim_take_job(CAR_FACTORY, -1, -1, "THE FACTORY");
    else { const Place *p = &pop_place[jobs_pl[k]]; char nm[24]; bm_place_name(p->lot, bm_hash(p->bx, p->by, 3), nm); sim_take_job(jobs_car[k], p->bx, p->by, nm); }
    jobs_open();
}
static void jobs_draw(void) {
    life_draw();
    ui_dim();
    char b[64], t[40];
    int top = ui_window(4, 4, 312, 232, "JOBS", IC_CASE);
    sprintf(b, "$%ld", cash); ui_badge(306 - ui_tw(1, b) - 10, 8, b, UI_WHITE, UI_GOODT);
    {   // your job, on its own card, with the button to go
        int on = jobs_sel == 0, len = job_shift_len();
        ui_round(10, top, 300, 55, 6, on ? UI_RGB(226, 242, 254) : UI_WHITE, on ? UI_HILITE : UI_PANEL, 0);
        if (on) gfx_rect(10, top + 4, 2, 47, 38, 146, 222, 0);
        ui_icon_disc(32, top + 19, 30, carIc[job_car], UI_BLUE, UI_WHITE);
        ui_text(54, top + 3, 2, job_title(), UI_INK);
        sprintf(b, "LEVEL %d OF 5", job_level); ui_badge(304 - ui_tw(1, b) - 10, top + 4, b, UI_BLUE, UI_WHITE);
        sprintf(b, "%s - %s", careers[job_car].name, job_place); ui_text(54, top + 18, 1, b, UI_SOFT);
        shift_text(t, job_car); sprintf(b, "%s   $%d A SHIFT", t, job_pay()); ui_text(54, top + 28, 1, b, UI_INK);
        ui_text(54, top + 42, 1, "TODAY", UI_SOFT); ui_bar(90, top + 42, 108, 7, MIN(100, job_worked * 100 / len), UI_GREEN);
        ui_button(208, top + 38, 96, 14, "GO TO WORK", IC_CAR, on);
    }
    ui_section(12, top + 59, 296, "JOBS GOING NEAR YOU");
    if (jobs_sel > 0 && jobs_sel - 1 < jobs_scroll) jobs_scroll = jobs_sel - 1;
    if (jobs_sel > 0 && jobs_sel - 1 >= jobs_scroll + 5) jobs_scroll = jobs_sel - 5;
    for (int r = 0; r < 5 && jobs_scroll + r < jobs_n; r++) {
        int k = jobs_scroll + r, c = jobs_car[k], y = top + 71 + r * 22, on = jobs_sel == k + 1;
        const Career *cr = &careers[c];
        if (on) { ui_round(10, y - 2, 300, 21, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(10, y + 1, 2, 15, 38, 146, 222, 0); }
        ui_icon_disc(26, y + 8, 18, carIc[c], on ? UI_BLUE : UI_RGB(130, 156, 190), UI_WHITE);
        int lv = c == job_car ? job_level : 0;
        ui_text(40, y, 1, cr->title[lv], on ? UI_BLUE2 : UI_INK);
        sprintf(b, "$%d A SHIFT", cr->pay0 + cr->pay_lvl * lv); ui_text(306 - ui_tw(1, b), y, 1, b, UI_GOODT);
        shift_short(t, c); int tw2 = ui_tw(1, t);
        if (jobs_pl[k] < 0) strcpy(b, "THE FACTORY, THE MAN BASE");
        else { const Place *p = &pop_place[jobs_pl[k]]; char nm[24]; const Settle *st = bm_set(p->set);
               sprintf(b, "%s, %s", bm_place_name(p->lot, bm_hash(p->bx, p->by, 3), nm), st ? st->name : "");
               if (40 + ui_tw(1, b) + 8 > 306 - tw2) strcpy(b, nm); }   // (no room for the town: the place alone)
        ui_text(40, y + 10, 1, b, UI_SOFT);
        ui_text(306 - tw2, y + 10, 1, t, UI_SOFT);
    }
    gfx_rect(10, 212, 300, 1, 200, 210, 222, 0);
    ui_hints(160, 217, jobs_sel ? "DPAD PICK|X TAKE THIS JOB|O BACK" : "DPAD PICK|X GO TO WORK|O BACK", 1);
}

#ifdef AUTO_TOUR
// (test builds) a ride to each new kind of place, up to the thing to use there, the clock set to when it is open, and the buttons
// pressed by the clock (real time, so the loading screens do not throw it): the pie menus open and their options run, as from the pad
int auto_btn;
static void tour_tick(void) {
    extern int sim_debug_goto(char c, int lot);
    static const struct { s8 lot; char c; u8 hour; char keys[4]; } tour[10] = {
        { LOT_CAFE, 'y', 9, "XX" }, { LOT_RESTAURANT, 't', 13, "X" }, { LOT_SALON, 'y', 13, "XX" }, { LOT_CINEMA, ';', 14, "XDX" },
        { LOT_GALLERY, ')', 14, "X" }, { LOT_SPA, 'S', 15, "X" }, { LOT_CLINIC, 'y', 15, "X" }, { LOT_LIBRARY, 'b', 16, "XX" }, { LOT_BAR, 'y', 18, "XX" },
        { LOT_POOL, '~', 18, "XDX" } };
    static int k = -1, phase, t0;
    if (k < 0) { if (state != ST_LIFE || !life_started || world_zone != ZONE_CITY || frame < 200) return; k = 0; phase = 0; clock_day = 1; }
    if (k >= 10 || (state != ST_LIFE && state != ST_PIE)) return;
    int t = VSync(-1) - t0; const Sim *s = &sims[player];
    if (phase == 0) {   // a taxi to the nearest one
        int bi = -1, bd = 1 << 30, pbx = world_wbx + sim_tile_x(s) / BM_BLK, pby = world_wby + sim_tile_y(s) / BM_BLK;
        for (int i = 0; i < pop_nplace; i++) if (pop_place[i].lot == tour[k].lot) { int d = ABS(pop_place[i].bx - pbx) + ABS(pop_place[i].by - pby); if (d < bd) { bd = d; bi = i; } }
        if (bi >= 0) { char nm[24]; bm_place_name(tour[k].lot, bm_hash(pop_place[bi].bx, pop_place[bi].by, 3), nm); taxi_to_block(pop_place[bi].bx, pop_place[bi].by, -1, -1, nm); }
        clock_min = tour[k].hour * 60; phase = 1; t0 = VSync(-1); return;
    }
    if (phase == 1) { if (t < 150) return; sim_debug_goto(tour[k].c, tour[k].lot); cam_pitch = 700; cam_dist = 1700; phase = 2; t0 = VSync(-1); return; }
    char key = tour[k].keys[phase - 2];
    if (key) { if (t >= 50) { auto_btn = key == 'X' ? PAD_CROSS : key == 'D' ? PAD_DOWN : PAD_CIRCLE; phase++; t0 = VSync(-1); } return; }
    if (t >= 360) { k++; phase = 0; }
}
#endif

#ifdef AUTO_GROW
int auto_btn;
static int pie_find(const char *t) { for (int i = 0; i < pie_n; i++) if (!strcmp(pie_opt[i], t)) return i; return -1; }
static void grow_tick(void) {   // (test builds: a family's life in two minutes: a birthday and its cake, a baby, school, old age, the screens)
    extern int sim_debug_grow(int what, int arg), sim_debug_goto(char c, int lot);
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); cam_dist = 1300; }
    static const struct { s16 at; char op; s8 arg; } plan[] = {
        { 60, 'm', 2 },                                   // the child's birthday comes at midnight
        { 400, 'f', 'F' }, { 430, 'b', 'X' }, { 480, 'p', 0 }, { 510, 'b', 'X' },   // to the fridge: BIRTHDAY CAKE
        { 880, 'f', (s8)0x82 }, { 910, 'b', 'X' }, { 950, 'p', 1 }, { 980, 'b', 'X' },   // the cake: BLOW CANDLES
        { 1380, 'r', 1 }, { 1390, 'g', 1 }, { 1420, 'b', 'X' }, { 1460, 'p', 2 }, { 1490, 'b', 'X' },   // MOSS: START A FAMILY
        { 1700, 'n', 0 },                                 // the baby comes home (asleep on the floor: there is no crib)
        { 1760, 'g', -1 }, { 1761, 'k', 1 }, { 1830, 'b', 'X' }, { 1870, 'p', 3 }, { 1900, 'b', 'X' },   // close up: CUDDLE
        { 2100, 'k', 0 }, { 2110, 'h', -1 },              // the baby cries; the grown-ups come
        { 2800, 'c', -1 }, { 2810, 'g', -1 }, { 2840, 'b', 'X' }, { 2880, 'p', 4 }, { 2910, 'b', 'X' },   // a crib: PUT TO BED
        { 3060, 'y', -1 }, { 3065, 'g', -1 }, { 3080, 'k', 2 }, { 3170, 'k', 1 }, { 3250, 'k', 0 },   // (close up, from the side and from above: the baby in its crib)
        { 3300, 's', 0 },                                 // school, TUESDAY 8 AM
        { 3600, 'e', 0 },                                 // home at 3
        { 3800, 'w', 2 }, { 3830, 'b', 'S' }, { 3860, 'b', 'D' }, { 3890, 'b', 'X' }, { 3930, 'b', '<' }, { 4050, 'b', '<' }, { 4170, 'b', 'S' },   // the teen's panel: SCHOOL, PEOPLE
        { 4250, 'w', 0 }, { 4280, 'b', 'S' }, { 4310, 'b', '>' }, { 4330, 'b', 'D' }, { 4350, 'b', 'D' }, { 4370, 'b', 'D' }, { 4390, 'b', 'D' }, { 4420, 'b', 'X' },
        { 4450, 'b', 'R' }, { 4470, 'b', 'R' }, { 4490, 'b', 'R' }, { 4510, 'b', 'R' }, { 4640, 'b', 'O' }, { 4670, 'b', 'O' },   // OPTIONS: LIFE
        { 4800, 'o', 1 },                                 // MOSS: the end of a long life (at midnight)
        { 5200, 'f', (s8)0x83 }, { 5230, 'b', 'X' },     // the gravestone: REMEMBER (its only choice: no pie)
        { 5600, 'b', 'S' }, { 5630, 'b', '>' }, { 5650, 'b', 'D' }, { 5670, 'b', 'D' }, { 5690, 'b', 'D' }, { 5710, 'b', 'D' }, { 5730, 'b', 'D' }, { 5760, 'b', 'X' },
        { 5800, 'b', 'R' }, { 5830, 'b', 'R' }, { 5860, 'b', 'R' }, { 5990, 'b', 'O' },   // HOW TO PLAY: GROWING UP
        { 6150, 'z', 0 }, { 6400, 'b', 'X' }, { 0, 0, 0 } };   // THE END, then X: a new life
    static const char *const pick[5] = { "BIRTHDAY CAKE", "BLOW CANDLES", "START A FAMILY", "CUDDLE", "PUT TO BED" };
    int t = VSync(-1) - t0;
    while (plan[k].op && t >= plan[k].at) {
        int a = plan[k].arg, baby = nsims - 1;
        switch (plan[k].op) {
            case 'm': clock_day = 0; clock_min = 1436; sim_debug_grow(1, a); break;
            case 'f': sim_debug_goto((char)a, -1); cam_pitch = 700; break;
            case 'b': auto_btn = a == 'X' ? PAD_CROSS : a == 'O' ? PAD_CIRCLE : a == 'S' ? PAD_START : a == 'D' ? PAD_DOWN : a == 'R' ? PAD_R1 : a == '<' ? PAD_L1 : PAD_RIGHT; break;
            case 'p': { int i = pie_find(pick[a]); if (i >= 0) pie_sel = i; } break;
            case 'r': sim_debug_grow(3, a); break;
            case 'g': sim_debug_grow(2, a < 0 ? baby : a); break;
            case 'n': sim_debug_grow(4, 0); break;
            case 'h': sim_debug_grow(5, baby); break;
            case 'c': sim_debug_grow(6, baby); break;
            case 's': clock_day = 1; clock_min = 476; break;
            case 'e': clock_min = 896; break;
            case 'w': sim_debug_grow(8, a); break;
            case 'o': sim_debug_grow(7, a); clock_min = 1436; break;
            case 'z': sim_story_over = 1; break;
            case 'y': sim_debug_grow(9, baby); break;
            case 'k': cam_pitch = a == 2 ? 170 : a ? 420 : 700; cam_dist = a == 2 ? 700 : a ? 800 : 1300; break;
        }
        k++;
    }
}
#endif

// ---- OPTIONS: the picture and the speed, as many knobs as a PC game, with presets; the game runs behind as you change them ----
static void title_draw(void);
enum { O_PRESET, O_VIEW, O_DETAIL, O_FARB, O_TREES, O_HAZE, O_CROWD, O_LIVE, O_SPR, O_SHADOW, O_AO, O_WALLS, O_FPSCAP, O_DITHER, O_CAM, O_SHOWFPS,
       O_AGING, O_OLDAGE, O_TOWNAGE, O_N };
static const char *const optNm[O_N] = { "PRESET", "VIEW DISTANCE", "FULL DETAIL", "FAR BUILDINGS", "FAR TREES", "HAZE", "CROWD SIZE",
    "PEOPLE IN FULL 3D", "PEOPLE SPRITES", "SHADOWS", "FLOOR SHADING", "WALL LIGHT", "FRAME RATE", "DITHERING", "CAMERA", "SHOW FPS",
    "LIFESPAN", "OLD AGE", "THE TOWN GROWS UP" };
static const char *const optV[O_N][6] = {
    { "CUSTOM", "LOW", "MEDIUM", "HIGH", "ULTRA" }, { "50 M", "250 M", "500 M", "1 KM", "1.5 KM", "2 KM" },
    { "10 M", "12 M", "15 M", "17 M" }, { "TALL ONLY", "ALL", "WITH WINDOWS" }, { "OFF", "100 M", "200 M", "350 M" },
    { "OFF", "LIGHT", "THICK" }, { "16", "32", "48", "64" }, { "1", "2", "4", "6" }, { "SLOW", "NORMAL", "FAST" },
    { "OFF", "PEOPLE", "EVERYTHING" }, { "OFF", "ON" }, { "FLAT", "SOFT" }, { "AS FAST AS IT CAN", "STEADY 30", "STEADY 20" },
    { "OFF", "ON" }, { "HIGH (SIMS)", "LOW (SEE FAR)" }, { "OFF", "ON" },
    { "NOBODY AGES", "SHORT", "NORMAL", "LONG" }, { "LIVE ON", "PASS ON" }, { "OFF", "ON" } };
static const u8 optN[O_N] = { 5, 6, 4, 3, 4, 3, 4, 4, 3, 3, 2, 2, 3, 2, 2, 2, 4, 2, 2 };
static const char *const optBlurb[O_N] = {
    "LOW, MEDIUM, HIGH OR ULTRA SETS THEM ALL", "HOW FAR SAN BOREAS GOES BEFORE THE SKY", "WHERE ROOMS, WALLS AND THINGS ARE DRAWN IN FULL",
    "THE TOWNS PAST THE MAP: TALL ONES, ALL, OR WINDOWS TOO", "TREES PAST THE NEAR DETAIL, OUT TO", "THE AIR BETWEEN YOU AND THE HORIZON",
    "HOW MANY PEOPLE CAN BE DRAWN AT ONCE", "THE NEAREST, DRAWN AS REAL 3D (THE REST ARE SPRITES)", "HOW OFTEN THE CROWD'S SPRITES ARE REDRAWN",
    "SOFT SHADOWS UNDER PEOPLE AND THINGS", "FLOORS DARKEN WHERE THEY MEET WALLS", "WALLS LIGHTER UP TOP, DARKER AT THE FOOT",
    "A STEADY RATE FEELS SMOOTHER THAN A JUMPY ONE", "THE PLAYSTATION'S SMOOTHING OF COLOURS", "LOW TO SEE THE SKYLINE (THE RIGHT STICK TILTS TOO)",
    "THE FRAME RATE IN A CORNER (L3 IN PLAY)",
    "HOW FAST BORES GROW UP, FROM BABY TO ELDER", "AT THE END OF THE ELDER YEARS: GO ON, OR SAY GOODBYE",
    "EVERYONE IN SAN BOREAS GETS OLDER AND HAS BABIES TOO" };
static const u8 presets[4][O_N] = {   // LOW, MEDIUM, HIGH, ULTRA (the graphics knobs; the frame rate, dithering, camera and the counter are left be)
    { 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 }, { 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1 },
    { 3, 3, 2, 2, 2, 1, 3, 3, 1, 2, 1, 1 }, { 4, 5, 3, 2, 3, 2, 3, 3, 2, 2, 1, 1 } };
#define O_PRESET_LAST O_WALLS                 // (presets set O_VIEW .. O_WALLS)
static u8 opt[O_N] = { 3, 3, 2, 2, 2, 1, 3, 3, 1, 2, 1, 1, 0, 1, 0, 0, 2, 1, 1 };   // HIGH: a kilometre of view; NORMAL lives
static int opt_sel, opt_back, opt_tab;
static void opts_apply(void) {
    extern int getenv_noshadow, dbg_noao, dbg_nograd, world_detail_r, cit_live_max, cit_maxdraw, cit_sprite_rate;
    far_view = opt[O_VIEW]; far_detail = opt[O_FARB]; far_trees = opt[O_TREES]; far_haze = opt[O_HAZE];
    static const u8 rr[4] = { 8, 10, 12, 14 }, cr[4] = { 16, 32, 48, 64 }, lv[4] = { 1, 2, 4, 6 };
    world_detail_r = rr[opt[O_DETAIL] & 3]; cit_maxdraw = cr[opt[O_CROWD] & 3]; cit_live_max = lv[opt[O_LIVE] & 3]; cit_sprite_rate = opt[O_SPR];
    gfx_noshadow = opt[O_SHADOW] == 0; getenv_noshadow = opt[O_SHADOW] < 2;
    dbg_noao = !opt[O_AO]; dbg_nograd = !opt[O_WALLS];
    gfx_min_vsyncs = opt[O_FPSCAP] == 0 ? 1 : opt[O_FPSCAP] == 1 ? 2 : 3;
    gfx_set_dither(opt[O_DITHER]);
    show_fps = opt[O_SHOWFPS];
    aging = opt[O_AGING]; old_age = opt[O_OLDAGE]; pop_aging = opt[O_TOWNAGE];
}
static void opts_open(int back) { opt_back = back; opt_sel = 0; opt_tab = 0; state = ST_OPTS; }
// the knobs in four tabs (L1 R1), like The Sims 4's game options: sliders for amounts, switches for on and off, pills for choices
#define NOTAB 5
static const char *const optTabNm[NOTAB] = { "GRAPHICS", "PEOPLE", "LIGHT", "PLAY", "LIFE" };
static const s8 optTabIc[NOTAB] = { IC_EYE, IC_PEOPLE, IC_SUN, IC_BOLT, IC_SPROUT };
static const s8 optTab[NOTAB][7] = { { O_PRESET, O_VIEW, O_DETAIL, O_FARB, O_TREES, O_HAZE, -1 }, { O_CROWD, O_LIVE, O_SPR, -1 },
                                     { O_SHADOW, O_AO, O_WALLS, O_DITHER, -1 }, { O_FPSCAP, O_CAM, O_SHOWFPS, -1 }, { O_AGING, O_OLDAGE, O_TOWNAGE, -1 } };
static int opt_kind(int i) {   // 0 a choice, 1 a slider, 2 a switch
    if (i == O_AO || i == O_DITHER || i == O_SHOWFPS || i == O_TOWNAGE) return 2;
    if (i == O_VIEW || i == O_DETAIL || i == O_TREES || i == O_HAZE || i == O_CROWD || i == O_LIVE || i == O_SPR) return 1;
    return 0;
}
static int opt_rows(int t) { int n = 0; while (n < 7 && optTab[t][n] >= 0) n++; return n; }
static void opts_update(void) {
    int nr = opt_rows(opt_tab);
    if (PRESSED(PAD_L1)) { opt_tab = (opt_tab + NOTAB - 1) % NOTAB; opt_sel = 0; }
    if (PRESSED(PAD_R1)) { opt_tab = (opt_tab + 1) % NOTAB; opt_sel = 0; }
    nr = opt_rows(opt_tab);
    if (PRESSED(PAD_UP)) opt_sel = (opt_sel + nr - 1) % nr;
    if (PRESSED(PAD_DOWN)) opt_sel = (opt_sel + 1) % nr;
    if (opt_sel >= nr) opt_sel = 0;
    int d = PRESSED(PAD_RIGHT) || PRESSED(PAD_CROSS) ? 1 : PRESSED(PAD_LEFT) ? -1 : 0;
    if (d) {
        int i = optTab[opt_tab][opt_sel], n = optN[i];
        if (i == O_PRESET) {
            int p = opt[O_PRESET] + d; if (p < 1) p = 4; if (p > 4) p = 1;       // (CUSTOM is not picked: it is what you get by hand)
            opt[O_PRESET] = (u8)p; for (int k = O_VIEW; k <= O_PRESET_LAST; k++) opt[k] = presets[p - 1][k - 1];
        } else {
            if (opt_kind(i) == 1) opt[i] = (u8)CLAMP(opt[i] + d, 0, n - 1);     // (a slider stops at its ends)
            else opt[i] = (u8)((opt[i] + d + n) % n);
            if (i <= O_PRESET_LAST) opt[O_PRESET] = 0;
            if (i == O_CAM) cam_pitch = opt[O_CAM] ? 250 : 640;
        }
        opts_apply(); audio_voice(VOICE_YAHOO);
    }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START) || PRESSED(PAD_TRIANGLE)) state = opt_back;
}
static void opts_draw(void) {
    if (opt_back == ST_MENU) title_draw(); else life_draw();
    ui_dim();
    int top = ui_window(8, 8, 304, 224, "OPTIONS", IC_GEAR);
    { char b[40]; sprintf(b, "%d FPS%s", fps_val, ram8 ? "  8 MB" : "");
      ui_badge(304 - ui_tw(1, b) - 10, 12, b, fps_val >= 25 ? UI_GREEN : fps_val >= 18 ? UI_RGB(236, 180, 40) : UI_RGB(228, 84, 70), UI_WHITE); }
    // the tabs down the left
    for (int t = 0; t < NOTAB; t++) {
        int y = top + 2 + t * 24, on = t == opt_tab;
        ui_round(14, y, 78, 20, 6, on ? UI_RGB(70, 176, 238) : UI_RGB(238, 243, 249), on ? UI_BLUE2 : UI_RGB(224, 231, 240), 0);
        ui_icon_disc(25, y + 10, 15, optTabIc[t], on ? UI_WHITE : UI_BLUE, on ? UI_BLUE : UI_WHITE);
        ui_text(36, y + 7, 1, optTabNm[t], on ? UI_WHITE : UI_INK);
    }
    ui_hints(16, top + 170, "L1 R1 TABS", 0);
    // the knobs
    gfx_rect(98, top, 1, 168, 214, 222, 232, 0);
    int nr = opt_rows(opt_tab);
    for (int r = 0; r < nr; r++) {
        int i = optTab[opt_tab][r], sel = r == opt_sel, y = top + r * 26, k = opt_kind(i);
        if (sel) { ui_round(104, y, 202, 24, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(104, y + 3, 2, 18, 38, 146, 222, 0); }
        ui_text(110, y + 3, 1, optNm[i], sel ? UI_BLUE2 : UI_INK);
        const char *v = optV[i][opt[i] % optN[i]];
        if (k == 2) ui_toggle(278, y + 3, opt[i], sel);
        else ui_text(300 - ui_tw(1, v), y + 3, 1, v, UI_BLUE2);
        if (k == 1) ui_slider(110, y + 13, 188, opt[i], optN[i], sel);
        else if (k == 0) {   // a choice: its arrows
            int w = ui_tw(1, v);
            ui_text(300 - w - 10, y + 3, 1, "<", sel ? UI_BLUE : UI_LINE);
            if (sel) { ui_text(110, y + 13, 1, "LEFT RIGHT TO CHANGE", UI_SOFT); }
        }
    }
    // what the knob does
    ui_round(104, top + 166, 202, 32, 6, UI_RGB(234, 240, 248), UI_RGB(224, 231, 240), 0);
    ui_icon(110, top + 176, IC_HELP, UI_BLUE);
    ui_wrap(126, top + 171, 176, 2, optBlurb[optTab[opt_tab][opt_sel]], UI_INK);
    ui_hints(16, top + 185, "O BACK", 0);
}

static const char *const pauseNm[] = { "RESUME", "BORE PANEL", "THE STORY", "TRAVEL", "MAP AND TAXI", "THE PEOPLE", "JOBS", "CREATE A BORE", "ADD A BORE", "THROW A PARTY", "BUILD MODE", "JUKEBOX", "OPTIONS", "HOW TO PLAY", "NEW LIFE", "QUIT TO TITLE" };
#define NPAUSE 16
#ifdef AUTO_PARTY
int ap_frames, ap_vs, ap_cpu;
#endif
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
#define PCOL 8                                  // the pause menu: two columns, eight down the left
static void pause_update(void) {
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + NPAUSE - 1) % NPAUSE;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % NPAUSE;
    if (PRESSED(PAD_RIGHT) && menu_sel < PCOL) menu_sel = MIN(menu_sel + PCOL, NPAUSE - 1);
    if (PRESSED(PAD_LEFT) && menu_sel >= PCOL) menu_sel -= PCOL;
    if (PRESSED(PAD_START) || PRESSED(PAD_CIRCLE)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) {
        switch (menu_sel) {
            case 0: state = ST_LIFE; break;
            case 1: panel_tab = 0; state = ST_PANEL; break;
            case 2: state = ST_STORY; break;
            case 3: travel_sel = world_zone; state = ST_TRAVEL; break;
            case 4: map_t = -1; state = ST_MAP; break;   // (opened on the map's own first frame: this frame still draws the people, whose sprites share that VRAM)
            case 5: dir_open(); state = ST_DIR; break;
            case 6: jobs_open(); state = ST_JOBS; break;
            case 7: { TexRef te = edit.tex_eye, tm = edit.tex_mouth; edit = sims[player].bore; edit.tex_eye = te; edit.tex_mouth = tm; look_build(&edit); }   // (the creator draws on its own faces)
                    create_tab = 0; create_row = 0; prev_state = ST_LIFE; state = ST_CREATE; world_backup(); world_gen_studio(); break;
            case 8: { Bore b; look_default(&b); b.stage = (u8)(AG_CHILD + look_rand() % 3); look_random(&b);
                      sim_add(&b, sim_tile_x(&sims[player]), sim_tile_y(&sims[player]) + 1, 1); sim_note("A NEW BORE MOVED IN"); state = ST_LIFE; } break;
            case 9: throw_party(); state = ST_LIFE; break;
            case 10: build_x = sim_tile_x(&sims[player]); build_y = sim_tile_y(&sims[player]); state = ST_BUILD; break;
            case 11: prev_state = ST_LIFE; state = ST_JUKEBOX; break;
            case 12: opts_open(ST_PAUSE); break;
            case 13: prev_state = ST_PAUSE; help_page = 0; state = ST_HELP; break;
            case 14: life_started = 0; start_life(); state = ST_LIFE; break;
            case 15: audio_jukebox_stop(); life_started = 0; state = ST_TITLE; look_release(&title_bore); look_default(&title_bore); look_random(&title_bore); break;
        }
    }
}
static void pause_draw(void) {
    static const s8 ic[NPAUSE] = { IC_PLAY, IC_PLUMBOB, IC_BOOK, IC_CAR, IC_PIN, IC_PEOPLE, IC_CASE, IC_STAR, IC_ADD, IC_PARTY, IC_HAMMER, IC_NOTE, IC_GEAR, IC_HELP, IC_SPROUT, IC_DOOR };
    life_draw();
    ui_dim();
    int top = ui_window(14, 10, 292, 220, "PAUSED", IC_PAUSE);
    { char b[48]; sprintf(b, "%s %02d:%02d", sim_day_name(clock_day), clock_min / 60, clock_min % 60);   // the household's day, on the header
      gfx_text(300 - gfx_text_w(1, b), 13, 1, b, 255, 255, 255); sprintf(b, "$%ld", cash); gfx_text(300 - gfx_text_w(1, b), 22, 1, b, 210, 255, 200); }
    for (int i = 0; i < NPAUSE; i++) {
        int col = i >= PCOL, row = col ? i - PCOL : i;
        ui_button(22 + col * 142, top + row * 24, 134, 19, pauseNm[i], ic[i], i == menu_sel);
    }
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
    int cx = 160, cy = 100, R = pie_n > 4 ? 60 : 50;
    { const char *t = pie_cit ? citizens_pie_title() : sim_pie_title();   // the centre: who or what it is, on a round badge
      ui_shadow(cx - 9, cy - 9, 18, 18, 6); ui_icon_disc(cx, cy, 20, pie_cit ? IC_CHAT : IC_PLUMBOB, UI_BLUE, UI_WHITE);
      int w = ui_tw(1, t) + 12; ui_round(cx - w / 2, cy + 12, w, 13, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_textc(cx, cy + 15, 1, t, UI_WHITE); }
    for (int i = 0; i < pie_n; i++) {
        int a = (i * 4096 / pie_n) & 4095, x = cx + ((isin(a) * R) >> 12), y = cy - ((icos(a) * R * 3 / 4) >> 12);
        int w = ui_tw(1, pie_opt[i]) + 16, sel = i == pie_sel;
        ui_shadow(x - w / 2, y - 7, w, 15, 6);
        ui_round(x - w / 2, y - 7, w, 15, 6, sel ? UI_RGB(70, 176, 238) : UI_WHITE, sel ? UI_BLUE2 : UI_RGB(230, 236, 244), 0);
        ui_textc(x, y - 3, 1, pie_opt[i], sel ? UI_WHITE : UI_INK);
    }
    ui_hintbar(172, "DPAD PICK|X DO IT|O NEVER MIND");
}


// ---- the end: the last of the family has gone ----
static void over_update(void) {
    if (PRESSED(PAD_CROSS)) { life_started = 0; start_life(); state = ST_LIFE; }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { audio_jukebox_stop(); life_started = 0; state = ST_TITLE; look_release(&title_bore); look_default(&title_bore); look_random(&title_bore); }
}
static void over_draw(void) {
    life_draw();
    ui_dim();
    char b[48];
    int top = ui_window(36, 56, 248, 112, "THE END", IC_HEART);
    snprintf(b, sizeof b, "THE %s FAMILY", sim_end_name); ui_textc(160, top + 6, 2, b, UI_INK);
    ui_textc(160, top + 26, 1, "THEIR STORY HAS COME TO AN END.", UI_SOFT);
    sprintf(b, "%d DAYS IN SAN BOREAS", day_count + 1); ui_textc(160, top + 39, 1, b, UI_SOFT);
    ui_hints(160, top + 62, "X A NEW LIFE|O TITLE", 1);
}

// ---- the story page: the goals of this life, Bustin' Out style ----
static void story_update(void) {
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_CROSS) || PRESSED(PAD_START)) state = ST_PAUSE;
}
static void story_draw(void) {   // the goals of this life, ticked off as they are done
    life_draw();
    ui_dim();
    char b[48], isl[40];
    sprintf(isl, "SAIL TO %s", world_zone_name(ZONE_ISLAND));
    struct { const char *txt; int done; } g[6] = {
        { "EARN A PAYCHECK (START, JOBS)", story_paid > 0 },
        { isl, story_visit[1] },
        { "GET PROMOTED TWICE", job_level >= 2 },
        { "SEE THE DESERT TOWN", story_visit[2] },
        { "MAKE FRIENDS: CHAT 5 TIMES", story_talks >= 5 },
        { "SAVE UP $20000", cash >= 20000 },
    };
    int top = ui_window(30, 24, 260, 194, "THE STORY SO FAR", IC_BOOK), done = 0;
    for (int i = 0; i < 6; i++) {
        int y = top + 2 + i * 22, d = g[i].done;
        ui_round(38, y, 244, 19, 6, d ? UI_RGB(234, 249, 234) : UI_WHITE, d ? UI_RGB(212, 238, 212) : UI_PANEL, 0);
        if (d) ui_icon_disc(49, y + 9, 15, IC_TICK, UI_GREEN, UI_WHITE);
        else { ui_disc(49, y + 9, 15, UI_RGB(186, 196, 210)); ui_disc(49, y + 9, 11, UI_WHITE); }
        ui_text(62, y + 6, 1, g[i].txt, d ? UI_GOODT : UI_INK);
        done += d;
    }
    sprintf(b, "%d OF 6 DONE", done);
    ui_text(40, top + 137, 1, b, UI_INK);
    ui_bar(124, top + 137, 156, 7, done * 100 / 6, UI_GREEN);
    ui_hints(160, top + 152, "O BACK", 1);
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
static const s8 panelIc[5] = { IC_SMILE, IC_STAR, IC_BOOK, IC_HEART, IC_CASE };
static void panel_draw(void) {   // the Bore panel: who they are and how they feel up top, then five tabs
    life_draw();
    ui_dim();
    Sim *s = &sims[player];
    char b[64], t[32];
    sprintf(b, "%s %s", s->bore.first, s->bore.last);
    int top = ui_window(4, 4, 312, 232, b, IC_PLUMBOB);
    int m = s->mood;
    u32 moodc = m > 15 ? UI_GREEN : m > -15 ? UI_AMBER : UI_RED, moodt = m > 15 ? UI_GOODT : m > -15 ? UI_GOLDT : UI_BADT;
    ui_disc(24, top + 10, 24, moodc); ui_disc(24, top + 10, 20, UI_RGB(250, 252, 255));
    draw_face_k(16, top + 2, m, 1);
    sprintf(b, "%s - %s", look_stage_name(s->bore.stage), look_asp_name(s->bore.asp)); ui_text(42, top, 1, b, UI_SOFT);
    {   // growing up: the next birthday
        int L = sim_stage_len(s->bore.stage), x = 42 + ui_tw(1, b) + 8;
        if (s->bday) ui_badge_i(x, top - 2, IC_CAKE, "BIRTHDAY TODAY!", UI_RGB(255, 186, 40), UI_WHITE);
        else if (L && s->bore.stage < AG_ELDER) { int d = MAX(1, L - s->age_days); sprintf(t, "BIRTHDAY IN %d DAY%s", d, d > 1 ? "S" : ""); ui_text(x, top, 1, t, UI_BLUE2); }
    }
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    ui_text(42, top + 11, 1, mood, moodt);
    ui_meter(120, top + 11, 186, m);
    int kid = s->bore.stage == AG_CHILD || s->bore.stage == AG_TEEN;   // (a kid's fifth tab is SCHOOL)
    for (int i = 0; i < 5; i++) {   // the tabs: a picture and a word, the open one blue
        const char *tn = i == 4 && kid ? "SCHOOL" : panelTab[i];
        int x = 10 + i * 60, on = i == panel_tab, w = 15 + ui_tw(1, tn), x0 = x + (59 - w) / 2;
        ui_round(x, top + 25, 59, 17, 6, on ? UI_RGB(70, 176, 238) : UI_RGB(238, 243, 249), on ? UI_BLUE2 : UI_RGB(222, 229, 238), 0);
        ui_icon(x0, top + 27, i == 4 && kid ? IC_BOOK : panelIc[i], on ? UI_WHITE : UI_BLUE);
        ui_text(x0 + 15, top + 30, 1, tn, on ? UI_WHITE : UI_INK);
    }
    int y0 = top + 50;
    if (panel_tab == 0) {   // MOOD: every motive, and what is lifting or dragging the mood
        static const s8 nic[NEED_N] = { IC_FOOD, IC_DROP, IC_BED, IC_BUBBLES, IC_SOFA, IC_SMILE, IC_CHAT, IC_HOUSE };
        ui_section(12, y0, 138, "MOTIVES");
        for (int n = 0; n < NEED_N; n++) { int v = sim_need_val(s, n), y = y0 + 12 + n * 13, r, g, bb; need_rgb(v, &r, &g, &bb);
            ui_icon(12, y - 2, nic[n], v < 20 ? UI_BADT : UI_SOFT);
            ui_text(28, y, 1, sim_need_name(n), v < 20 ? UI_BADT : UI_INK);
            ui_bar(84, y, 66, 7, v, UI_RGB(r, g, bb)); }
        ui_section(162, y0, 144, "MOODLETS");
        int y = y0 + 13, any = 0;
        for (int i = 0; i < 6; i++) if (s->ml[i].id) { int v = s->ml[i].val;
            ui_round(162, y - 3, 144, 13, 4, v >= 0 ? UI_RGB(234, 249, 234) : UI_RGB(252, 234, 232), v >= 0 ? UI_RGB(214, 240, 214) : UI_RGB(246, 214, 210), 0);
            ui_text(168, y, 1, sim_moodlet_name(s->ml[i].id), UI_INK);
            sprintf(b, "%s%d", v > 0 ? "+" : "", v); ui_text(300 - ui_tw(1, b), y, 1, b, v >= 0 ? UI_GOODT : UI_BADT);
            y += 15; any = 1; }
        if (!any) ui_text(168, y, 1, "NOTHING MUCH", UI_SOFT);
        { const char *tr[3]; int n = sim_traits(s, tr), x = 12;
          if (n) { ui_text(x, 200, 1, "TRAITS", UI_SOFT); x += ui_tw(1, "TRAITS") + 6;
                   for (int i = 0; i < n; i++) { ui_badge(x, 198, tr[i], UI_RGB(206, 226, 248), UI_BLUE2); x += ui_tw(1, tr[i]) + 14; } } }
    } else if (panel_tab == 1) {   // WANTS: the aspiration meter, four wants, two fears
        static const u32 lvT[4] = { UI_BADT, UI_GOODT, UI_GOLDT, UI_RGB(70, 100, 150) };
        int lv = wants_level(s);
        ui_section(12, y0, 294, "ASPIRATION");
        ui_bar(12, y0 + 11, 294, 9, 100 * s->asp_pts / ASP_MAX, UI_RGB(aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2]));
        for (int k = 1; k < 4; k++) gfx_rect(12 + 294 * (k == 1 ? 3000 : k == 2 ? 7000 : 10000) / ASP_MAX, y0 + 10, 1, 11, 120, 132, 150, 0);
        sprintf(b, "%s%s", aspLvNm[lv], lv == 3 ? " - NOTHING CAN BRING THEM DOWN" : wants_desperate(s) ? " - DESPERATE" : "");
        ui_text(12, y0 + 24, 1, b, lvT[lv]);
        ui_section(12, y0 + 37, 294, "WANTS");
        for (int i = 0; i < 4; i++) { int y = y0 + 49 + i * 13; wants_text(s, i, t);
            ui_icon(12, y - 2, IC_STAR, UI_AMBER); ui_text(28, y, 1, t, UI_INK);
            sprintf(b, "+%d", s->want[i].pts); ui_badge(306 - ui_tw(1, b) - 10, y - 2, b, UI_GREEN, UI_WHITE); }
        ui_section(12, y0 + 102, 294, "FEARS");
        for (int i = 0; i < 2; i++) { int y = y0 + 114 + i * 13; fears_text(s, i, t);
            ui_icon(12, y - 2, IC_BOLT, UI_RED); ui_text(28, y, 1, t, UI_INK);
            sprintf(b, "-%d", s->fear[i].pts); ui_badge(306 - ui_tw(1, b) - 10, y - 2, b, UI_RED, UI_WHITE); }
    } else if (panel_tab == 2) {   // SKILLS and PERSONALITY
        static const char *const skNm[3] = { "COOKING", "BODY", "FUN" };
        int raw[3] = { s->skill_cook, s->skill_body, s->skill_fun };
        ui_section(12, y0, 294, "SKILLS");
        for (int i = 0; i < 3; i++) { int y = y0 + 12 + i * 14, lv = wants_skill_level(raw[i]);
            ui_text(12, y, 1, skNm[i], UI_INK); ui_pips(96, y, lv, 10, UI_AMBER); sprintf(b, "LEVEL %d", lv); ui_text(224, y, 1, b, UI_SOFT); }
        ui_section(12, y0 + 58, 294, "PERSONALITY");
        for (int i = 0; i < TR_N; i++) { int y = y0 + 70 + i * 14;
            ui_text(12, y, 1, look_trait_name(i), UI_INK); ui_pips(96, y, s->bore.tr[i], 10, UI_BLUE); }
    } else if (panel_tab == 3) {   // PEOPLE: how everyone else in the house feels about you (the daily relationship)
        int row = 0;
        for (int j = 0; j < nsims; j++) {
            if (j == player) continue;
            int y = y0 + row * 25, rs = s->rel[j], mm = sims[j].mood;
            if (y > 172) break;
            ui_round(10, y, 300, 23, 6, UI_WHITE, UI_PANEL, 0);
            ui_disc(23, y + 11, 21, mm > 15 ? UI_GREEN : mm > -15 ? UI_AMBER : UI_RED); ui_disc(23, y + 11, 17, UI_RGB(250, 252, 255));
            draw_face_k(15, y + 3, mm, 1);
            {   // the name, what they are to you (the family tree), and their stage of life
                const char *rl = sim_relation(player, j), *sg = sims[j].away ? "AT SCHOOL" : look_stage_name(sims[j].bore.stage);
                sprintf(b, "%s %s", sims[j].bore.first, sims[j].bore.last);
                if (40 + ui_tw(1, b) + (rl ? ui_tw(1, rl) + 6 : 0) + 6 + ui_tw(1, sg) > 214) strcpy(b, sims[j].bore.first);
                int x = 40; ui_text(x, y + 3, 1, b, UI_INK); x += ui_tw(1, b) + 6;
                if (rl) { ui_text(x, y + 3, 1, rl, UI_BLUE2); x += ui_tw(1, rl) + 6; }
                if (x + ui_tw(1, sg) <= 214) ui_text(x, y + 3, 1, sg, UI_SOFT);
            }
            ui_meter(40, y + 13, 170, rs);
            gfx_rect(40 + 1 + 84 + 42, y + 12, 1, 9, 236, 176, 40, 0);   // (a friend at 50)
            const char *lab = rs >= 50 ? "FRIEND" : rs >= 15 ? "PALS" : rs > -15 ? "STRANGERS" : rs > -50 ? "DISLIKED" : "ENEMY";
            u32 lc = rs >= 50 ? UI_GREEN : rs >= 15 ? UI_RGB(110, 190, 110) : rs > -15 ? UI_RGB(150, 160, 176) : rs > -50 ? UI_AMBER : UI_RED;
            ui_badge(220, y + 6, lab, lc, UI_WHITE);
            row++;
        }
        if (!row) ui_text(14, y0, 1, "NOBODY ELSE LIVES HERE", UI_SOFT);
        if (baby_due >= 0) { int d = MAX(1, (baby_due - life_minutes() + 1439) / 1440);   // a baby on the way
            sprintf(b, "A BABY IS ON THE WAY: %d DAY%s TO GO", d, d > 1 ? "S" : "");
            ui_icon(12, 199, IC_BABY, UI_RGB(236, 120, 160)); ui_text(28, 201, 1, b, UI_RGB(200, 70, 120)); }
        else { ui_icon(12, 199, IC_HEART, UI_RED);
            ui_text(28, 201, 1, "CHAT, JOKE AND HUG TO MAKE FRIENDS. 50 IS A FRIEND.", UI_SOFT); }
    } else if (kid) {   // SCHOOL: the grade, today at school, the homework, the report card
        int g = s->grade, sd = clock_day < 5 && day_count > 0;   // (no school on the day you move in)
        u32 gc = g >= 85 ? UI_GREEN : g >= 70 ? UI_RGB(70, 176, 238) : g >= 55 ? UI_AMBER : UI_RED;
        ui_icon_disc(30, y0 + 13, 28, IC_BOOK, UI_BLUE, UI_WHITE);
        ui_text(52, y0 + 1, 2, s->bore.stage == AG_CHILD ? "GRADE SCHOOL" : "HIGH SCHOOL", UI_INK);
        sprintf(b, "GRADE %s", sim_grade(g)); ui_badge(52, y0 + 16, b, gc, UI_WHITE); int bw = ui_tw(1, b) + 10;
        ui_text(52 + bw + 8, y0 + 18, 1, g >= 85 ? "TOP OF THE CLASS" : g >= 70 ? "DOING WELL" : g >= 55 ? "GETTING BY" : g >= 40 ? "FALLING BEHIND" : "FAILING", UI_SOFT);
        ui_text(12, y0 + 34, 1, "THE BUS AT 8 AM, HOME AT 3 PM, MON - FRI", UI_SOFT);
        ui_section(12, y0 + 48, 294, "TODAY");
        ui_text(12, y0 + 60, 1, "SCHOOL", UI_INK);
        ui_text(96, y0 + 60, 1, !sd ? "NO SCHOOL TODAY" : s->away ? "AT SCHOOL NOW" : clock_min < 480 ? "THE BUS COMES AT 8" : clock_min >= 900 ? "DONE FOR THE DAY" : "SKIPPED TODAY",
                sd && !s->away && clock_min >= 480 && clock_min < 900 ? UI_BADT : UI_INK);
        ui_text(12, y0 + 74, 1, "HOMEWORK", UI_INK);
        if (s->homework) ui_badge_i(96, y0 + 72, IC_TICK, "DONE", UI_GREEN, UI_WHITE);
        else ui_text(96, y0 + 74, 1, sd ? "NOT YET: X ON A DESK OR A BOOKSHELF" : "NONE TODAY", sd ? UI_BADT : UI_SOFT);
        ui_section(12, y0 + 88, 294, "REPORT CARD");
        ui_bar(12, y0 + 100, 294, 9, g, gc);
        { static const u8 cut[5] = { 0, 40, 55, 70, 85 }; static const char *const gl[5] = { "F", "D", "C", "B", "A" };
          for (int k = 0; k < 5; k++) { int x = 12 + 294 * cut[k] / 100; if (k) gfx_rect(x, y0 + 99, 1, 11, 120, 132, 150, 0);
              int nx = 12 + 294 * (k < 4 ? cut[k + 1] : 100) / 100; ui_textc((x + nx) / 2, y0 + 112, 1, gl[k], k == (g >= 85 ? 4 : g >= 70 ? 3 : g >= 55 ? 2 : g >= 40 ? 1 : 0) ? UI_INK : UI_SOFT); } }
        ui_text(12, y0 + 124, 1, "HOMEWORK EVERY SCHOOL DAY KEEPS THE GRADES UP", UI_SOFT);
    } else {   // JOB: the career, the level, the pay, today's shift, the run of good days and strikes
        char t2[40]; int len = job_shift_len();
        ui_icon_disc(30, y0 + 13, 28, carIc[job_car], UI_BLUE, UI_WHITE);
        ui_text(52, y0 + 1, 2, job_title(), UI_INK);
        sprintf(b, "LEVEL %d OF 5", job_level); ui_badge(52, y0 + 16, b, UI_BLUE, UI_WHITE);
        sprintf(b, "$%d A SHIFT", job_pay()); ui_text(140, y0 + 18, 1, b, UI_GOODT);
        shift_text(t2, job_car); sprintf(b, "%s, %s", job_place, t2); ui_text(12, y0 + 34, 1, b, UI_SOFT);
        ui_section(12, y0 + 48, 294, "TODAY");
        ui_text(12, y0 + 60, 1, "SHIFT DONE", UI_INK); ui_bar(96, y0 + 60, 210, 7, MIN(100, job_worked * 100 / len), UI_GREEN);
        ui_text(12, y0 + 74, 1, "GOOD DAYS", UI_INK); ui_pips(96, y0 + 74, job_days_good, 3, UI_GREEN); ui_text(140, y0 + 74, 1, "3 IN A ROW: PROMOTED", UI_SOFT);
        ui_text(12, y0 + 88, 1, "STRIKES", UI_INK); ui_pips(96, y0 + 88, job_strikes, 3, UI_RED); ui_text(140, y0 + 88, 1, "3 STRIKES: DEMOTED", UI_SOFT);
        ui_section(12, y0 + 102, 294, "MONEY");
        sprintf(b, "$%ld", cash); ui_text(12, y0 + 114, 2, b, UI_GOODT);
        ui_text(110, y0 + 117, 1, "BILLS: $800 EVERY MIDNIGHT", UI_SOFT);
    }
    gfx_rect(10, 212, 300, 1, 200, 210, 222, 0);
    ui_hints(160, 217, "L1 R1 TAB|SEL NEXT BORE|O BACK", 1);
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
        if (r->kind == ROW_AGE) { int st = edit.stage; do st = (st + AG_N + dir) % AG_N; while (st == AG_BABY); edit.stage = (u8)st; look_fix(&edit); look_build(&edit); }   // (a baby cannot be played)
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
        if (prev_state == ST_LIFE) { Sim *me = &sims[player]; if (me->bore.stage != edit.stage) { me->age_days = 0; me->bday = 0; }
            TexRef te = me->bore.tex_eye, tm = me->bore.tex_mouth; me->bore = edit; me->bore.tex_eye = te; me->bore.tex_mouth = tm; look_build(&me->bore);   // (on the Bore's own faces)
            crowd_forget(CROWD_MAX + player); state = ST_LIFE; }
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
    // the card: the tab on a blue header, its rows, the buttons at the foot
    static const s8 createIc[NTAB] = { IC_PEOPLE, IC_SMILE, IC_BRUSH, IC_SHIRT, IC_STAR, IC_SPROUT, IC_PLUMBOB };
    int top = ui_window(166, 4, 150, 232, look_tab_name[create_tab], createIc[create_tab]);
    int first = MAX(0, create_row - 7), shown = MIN(n, first + 9);
    for (int i = first; i < shown; i++) {
        const LookRow *r = &rows[i]; int y = top + (i - first) * 19;
        int sel = i == create_row;
        if (sel) { ui_round(170, y - 1, 142, 19, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(170, y + 2, 2, 13, 38, 146, 222, 0); }
        ui_text(176, y + 1, 1, r->label, sel ? UI_BLUE2 : UI_INK);
        if (sel) { ui_text(292, y + 1, 1, "<", UI_BLUE); ui_text(302, y + 1, 1, ">", UI_BLUE); }
        char val[24] = "";
        if (r->kind == ROW_AGE) strcpy(val, look_stage_name(edit.stage));
        else if (r->id == LK_ASP) strcpy(val, look_asp_name(edit.asp));
        else if (r->id == LK_LTW) sprintf(val, "GOAL %d", edit.ltw + 1);
        else if (r->id >= LK_TRAIT0 && r->id < LK_TRAIT0 + 5) ui_pips(176, y + 10, edit.tr[r->id - LK_TRAIT0], 10, UI_AMBER);
        else if (r->kind == ROW_SLIDER) ui_slider(176, y + 9, 128, (edit.look[r->id] + 4) % 9, 9, sel);
        else if (r->kind == ROW_COLOUR) {
            int cnt = look_count(r->id), sp = cnt > 12 ? 8 : 11, sw = sp - 2;
            for (int k = 0; k < cnt && k < 16; k++) {
                if (k == edit.look[r->id]) gfx_rect(176 + k * sp - 2, y + 7, sw + 4, 12, 26, 112, 190, 0);
                gfx_rect_c(176 + k * sp, y + 9, sw, 8, look_option_colour(r->id, k), 0);
            }
        }
        else { const char *nm = look_option_name(r->id, edit.look[r->id]); if (nm) strncpy(val, nm, 23); }
        if (val[0]) ui_text(176, y + 10, 1, val, sel ? UI_BLUE2 : UI_SOFT);
    }
    for (int t = 0; t < NTAB; t++) ui_disc(220 + t * 9, 204, 6, t == create_tab ? UI_BLUE : UI_LINE);   // which tab of the seven
    ui_hints(241, 210, "L1 R1 TAB|TRI DICE", 1);
    ui_hints(241, 222, prev_state == ST_LIFE ? "START DONE|O BACK" : "START LIVE|O BACK", 1);
    // the Bore's name card, and the title
    char nm[32]; sprintf(nm, "%s %s", edit.first, edit.last);
    int big = ui_tw(2, nm) <= 140, w = MAX(big ? ui_tw(2, nm) : ui_tw(1, nm), 100) + 14;
    ui_card(6, 6, w, 31);
    ui_text(12, big ? 9 : 11, big ? 2 : 1, nm, UI_INK);
    char sb[48]; sprintf(sb, "%s, %s", look_stage_name(edit.stage), look_sign_name(look_sign_of(&edit))); ui_text(12, 25, 1, sb, UI_SOFT);
    { const char *t = "CREATE A BORE"; int tw_ = ui_tw(1, t) + 30;
      ui_shadow(6, 214, tw_, 18, 6); ui_round(6, 214, tw_, 18, 6, UI_RGB(70, 176, 238), UI_BLUE2, 0);
      ui_icon(11, 217, IC_STAR, UI_WHITE); ui_text(26, 220, 1, t, UI_WHITE); }
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
static const char *const catNm[7] = { "SEATS, BEDS", "TABLES, STORAGE", "KITCHEN, BATH", "FUN AND TECH", "DECOR, PLANTS", "SKATE", "SPECIAL" };
static const char *const catSet[6] = { "CUh;S<N%z\x81", "tdibyK?#O", "F/:cTH,->&Y`", "vARj[!$()QImn]_{", "rlpZVqo}gus*0", "=LJXM159G" };
static int item_cat(char ch) { if (ch == CH_CAKE || ch == CH_GRAVE) return -1; for (int c = 0; c < 6; c++) if (strchr(catSet[c], ch)) return c; return 6; }
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
    int top = ui_window_s(196, 4, 120, 168, catNm[cat]);
    for (int c = 0; c < 7; c++) ui_disc(220 + c * 12, top + 2, 6, c == cat ? UI_BLUE : UI_LINE);   // which of the seven tabs
    int first = CLAMP(p - 5, 0, MAX(0, n - 10));
    for (int k = first; k < n && k < first + 10; k++) {
        char ch = items_at(idx[k]); int y = top + 10 + (k - first) * 12, sel = k == p, cost = items_cost(ch);
        if (sel) { ui_round(199, y - 2, 114, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(199, y, 2, 8, 38, 146, 222, 0); }
        ui_text(204, y, 1, items_name(ch), sel ? UI_BLUE2 : UI_INK);
        sprintf(b, "%d", cost); ui_text(311 - ui_tw(1, b), y, 1, b, cost > cash ? UI_BADT : UI_GOODT);
    }
    sprintf(b, "%d OF %d", p + 1, n); ui_textc(256, top + 135, 1, b, UI_SOFT);
}
static void draw_pick_list(const char *title, int count, int sel, const char *(*nm)(int)) {   // floors and wallpapers: seven at a time, the pick in the middle
    char b[24];
    int top = ui_window_s(196, 4, 120, 116, title);
    for (int k = -3; k <= 3; k++) {
        int i = (sel + k + count * 4) % count, y = top + 4 + (k + 3) * 12;
        if (!k) { ui_round(199, y - 2, 114, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(199, y, 2, 8, 38, 146, 222, 0); }
        ui_text(204, y, 1, nm(i), !k ? UI_BLUE2 : UI_SOFT);
    }
    sprintf(b, "%d OF %d", sel + 1, count); ui_textc(256, top + 87, 1, b, UI_SOFT);
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
        if (build_tool == 2) { if (*cell == ' ' && !floor_supported(L, build_x, build_y)) sim_note("NOTHING UNDER IT TO HOLD IT UP");
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
    {   // the tools as round buttons, the status card, the catalog on the right, the buttons along the foot
        static const s8 toolIc[NTOOL] = { IC_SOFA, IC_BRICK, IC_TILES, IC_BRUSH, IC_HOUSE };
        char b[64];
        ui_card(4, 4, 190, 52);
        for (int i = 0; i < NTOOL; i++) { int on = i == build_tool; ui_icon_disc(17 + i * 22, 16, 19, toolIc[i], on ? UI_BLUE : UI_RGB(222, 230, 240), on ? UI_WHITE : UI_BLUE2); }
        ui_text(126, 8, 1, toolNm[build_tool], UI_BLUE2);
        sprintf(b, "$%ld", cash); ui_text(126, 19, 1, b, UI_GOODT);
        sprintf(b, "STOREY %d OF %d", build_lvl + 1, NLEVELS); ui_text(10, 31, 1, b, UI_SOFT);
        sprintf(b, "AT %d,%d", build_x, build_y); ui_text(188 - ui_tw(1, b), 31, 1, b, UI_SOFT);
        if (build_tool == 0) sprintf(b, "%s  $%d", items_name(items_at(build_item)), items_cost(items_at(build_item)));
        else if (build_tool == 1) sprintf(b, "WALL $50  (%s)", world_wall_name(build_wp));
        else if (build_tool == 2) sprintf(b, "%s  $10 A TILE", world_floor_name(build_fl));
        else if (build_tool == 3) sprintf(b, "%s", world_wall_name(build_wp));
        else sprintf(b, "%s ROOF, %s PITCH", world_roof_style_name(world_roof_style), world_roof_pitch_name(world_roof_pitch));
        ui_text(10, 42, 1, b, UI_INK);
        if (build_tool == 0) draw_catalog();
        else if (build_tool == 1) draw_pick_list("WALLS  $50", NWALLS, build_wp, world_wall_name);
        else if (build_tool == 2) draw_pick_list("FLOORS  $10", NFLOORS, build_fl, world_floor_name);
        else if (build_tool == 3) draw_pick_list("WALLPAPER", NWALLS, build_wp, world_wall_name);
        else { int top = ui_window_s(196, 4, 120, 62, "THE ROOF");
               ui_text(204, top + 2, 1, world_roof_style_name(world_roof_style), UI_INK);
               ui_text(204, top + 14, 1, world_roof_pitch_name(world_roof_pitch), UI_SOFT);
               ui_text(204, top + 26, 1, world_roof_col_name(world_roof_col), UI_SOFT); }
        ui_card(4, 206, 312, 30);
        if (build_tool == 4) { ui_hints(160, 209, "X STYLE|SQ PITCH|L1 R1 COLOUR|SEL TOOL", 1); ui_hints(160, 221, "L2 R2 STOREY|O DONE", 1); }
        else { ui_hints(160, 209, build_tool == 0 ? "L1 R1 PICK|R3 NEXT TAB|X BUY|SQ SELL" : "L1 R1 PICK|X PLACE|SQ ERASE", 1);
               ui_hints(160, 221, "SEL TOOL|L2 R2 STOREY|L3 ROOF|O DONE", 1); }
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
static void juke_draw(void) {   // the song playing on its own card, every song in a list under it
    if (prev_state == ST_LIFE) life_draw(); else title_draw();
    ui_dim();
    int top = ui_window(10, 6, 300, 228, "JUKEBOX", IC_NOTE);
    int cur = audio_jukebox_current(), n = audio_song_count();
    char b[64];
    { const char *m = audio_jukebox_shuffle() ? "SHUFFLE" : "IN ORDER"; ui_badge(304 - ui_tw(1, m) - 10, 10, m, UI_WHITE, UI_BLUE2); }
    ui_round(16, top, 288, 38, 6, UI_WHITE, UI_PANEL, 0);
    ui_icon_disc(34, top + 19, 26, cur >= 0 ? IC_PLAY : IC_PAUSE, cur >= 0 ? UI_GREEN : UI_RGB(170, 180, 194), UI_WHITE);
    if (cur >= 0) {
        ui_text(54, top + 5, 1, audio_song_name(cur), UI_INK);
        int e = audio_jukebox_elapsed(), l = audio_song_len(cur);
        const char *ar = audio_song_artist(cur);
        b[0] = 0;
        if (audio_station() >= 0) { strcat(b, audio_station_name(audio_station())); strcat(b, "   "); }
        if (ar && ar[0]) { strcat(b, ar); strcat(b, "   "); }
        sprintf(b + strlen(b), "%d:%02d / %d:%02d", e / 60, e % 60, l / 60, l % 60);
        ui_text(54, top + 16, 1, b, UI_SOFT);
        ui_bar(54, top + 27, 244, 6, l ? MIN(100, e * 100 / l) : 0, UI_BLUE);
    } else ui_text(54, top + 15, 1, audio_cd_ok() ? "SILENT. X PLAYS A SONG." : "NO MUSIC ON THIS DISC", UI_SOFT);
    if (juke_sel < juke_scroll) juke_scroll = juke_sel;
    if (juke_sel >= juke_scroll + 12) juke_scroll = juke_sel - 11;
    for (int i = juke_scroll; i < n && i < juke_scroll + 12; i++) {
        int y = top + 45 + (i - juke_scroll) * 12, sel = i == juke_sel, pl = i == cur;
        if (sel) { ui_round(16, y - 2, 288, 12, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(16, y, 2, 8, 38, 146, 222, 0); }
        if (pl) ui_icon(22, y - 2, IC_NOTE, UI_GREEN);
        ui_text(38, y, 1, audio_song_name(i), pl ? UI_GOODT : sel ? UI_BLUE2 : UI_INK);
        int l = audio_song_len(i); sprintf(b, "%d:%02d", l / 60, l % 60); ui_text(298 - ui_tw(1, b), y, 1, b, UI_SOFT);
    }
    gfx_rect(16, 214, 288, 1, 200, 210, 222, 0);
    ui_hints(160, 218, "X PLAY|L1 R1 SKIP|SQ SHUFFLE|O BACK", 1);
}

// ---- help ----
typedef struct { s8 ic; const char *t; } HelpTip;
#define NHELP 4
static const struct { const char *title; HelpTip it[5]; } help_pg[NHELP] = {
    { "HOW TO PLAY", { { IC_PLAY, "MOVE WITH THE STICK OR THE D-PAD. SQUARE RUNS." },
        { IC_FOOD, "X USES THE THING IN FRONT OF YOU: THE FRIDGE, THE BED, THE SHOWER, THE SOFA, THE TV." },
        { IC_CHAT, "X ON A PERSON TO TALK. O STOPS WHAT YOU ARE DOING." },
        { IC_FAST, "TRIANGLE CHANGES HOW FAST TIME GOES." },
        { IC_PEOPLE, "SELECT SWITCHES TO THE NEXT BORE IN THE HOUSE." } } },
    { "THE CAMERA", { { IC_CAMERA, "L1 R1 TURN THE CAMERA. L2 R2 ZOOM. THE RIGHT STICK TURNS AND TILTS IT." },
        { IC_EYE, "WALLS IN THE WAY DROP DOWN, LIKE THE SIMS." },
        { IC_CASE, "WORK: START, THEN JOBS. PICK A JOB AT A REAL PLACE, GO THERE, AND X ON A COUNTER, DESK OR MACHINE." },
        { IC_MONEY, "BILLS ARE $800, EVERY MIDNIGHT. KEEP SOME MONEY BY." },
        { IC_PLUMBOB, "START OPENS THE MENU: THE BORE PANEL, THE MAP, BUILD MODE AND MORE." } } },
    { "BUILD AND CREATE", { { IC_HAMMER, "BUILD MODE: ITEMS, WALLS, FLOORS AND 103 WALLPAPERS." },
        { IC_BRUSH, "SELECT PICKS THE TOOL. L1 R1 PICK THE THING. X PLACES IT, SQUARE TAKES IT AWAY." },
        { IC_HOUSE, "L2 R2 CHANGE THE STOREY. L3 SHOWS THE ROOF." },
        { IC_STAR, "CREATE A BORE: EVERY BODY, FACE, HAIR, CLOTHES AND PART, IN 3D." },
        { IC_DICE, "TRIANGLE ROLLS THE DICE FOR A RANDOM BORE." } } },
    { "GROWING UP", { { IC_SPROUT, "BORES GROW UP: BABY, CHILD, TEEN, ADULT, ELDER. OPTIONS SETS HOW FAST." },
        { IC_CAKE, "A BIRTHDAY? BAKE A CAKE AT THE FRIDGE OR STOVE, THEN BLOW CANDLES." },
        { IC_HEART, "TWO GROWN-UPS WHO GET ON: X ON ONE, START A FAMILY. OR ADOPT BY PHONE." },
        { IC_BABY, "A BABY CRIES WHEN IT NEEDS YOU: FEED, CHANGE, PLAY, CUDDLE. BUY A CRIB." },
        { IC_BOOK, "SCHOOL IS 8 AM TO 3 PM, MON - FRI. HOMEWORK AT A DESK OR SHELF." } } } };
static void help_update(void) {
    if (PRESSED(PAD_R1) || PRESSED(PAD_CROSS)) help_page = (help_page + 1) % NHELP;
    if (PRESSED(PAD_L1)) help_page = (help_page + NHELP - 1) % NHELP;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = prev_state == ST_PAUSE ? ST_LIFE : ST_MENU;
}
static void help_draw(void) {   // three pages of tips, each with its picture
    if (prev_state == ST_PAUSE) life_draw(); else title_draw();
    ui_dim();
    int top = ui_window(14, 8, 292, 224, help_pg[help_page].title, IC_HELP), y = top + 2;
    char b[16]; sprintf(b, "PAGE %d OF %d", help_page + 1, NHELP); ui_badge(300 - ui_tw(1, b) - 10, 12, b, UI_WHITE, UI_BLUE2);
    for (int i = 0; i < 5; i++) {
        const HelpTip *h = &help_pg[help_page].it[i];
        int nl = ui_wrap_n(236, h->t), hh = MAX(28, nl * 10 + 8);
        ui_round(20, y, 280, hh - 3, 6, UI_WHITE, UI_PANEL, 0);
        ui_icon_disc(36, y + (hh - 3) / 2, 22, h->ic, UI_BLUE, UI_WHITE);
        ui_wrap(54, y + (hh - 3 - nl * 10 + 3) / 2, 236, 3, h->t, UI_INK);
        y += hh;
    }
    for (int k = 0; k < NHELP; k++) ui_disc(160 - (NHELP - 1) * 6 + k * 12, 205, 7, k == help_page ? UI_BLUE : UI_LINE);
    ui_hints(160, 212, "L1 R1 PAGE|O BACK", 1);
}

// ---- title and menu ----
static int title_t;
static void title_update(void) {
    title_t++;
#if defined(AUTO_PARTY) || defined(AUTO_CITY)
    if (title_t == 30) { life_started = 0; start_life(); cam_yaw = 2600; cam_pitch = 640; cam_dist = 1500; state = ST_LIFE; frame = 0;
#ifdef AUTO_CITY
        travel_go(ZONE_CITY);
#endif
        return; }
#endif
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
    { int w = gfx_text_w(3, "BORE 2"); gfx_text_raw(162 - w / 2, 16, 3, "BORE 2", 60, 30, 10); }   // (a shadow under the logo)
    gfx_textc(160, 14, 3, "BORE 2", GOLD);
    { int w = gfx_text_w(1, "DIPPINN PRODUCTIONS"); gfx_text_raw(161 - w / 2, 53, 1, "DIPPINN PRODUCTIONS", 40, 30, 30); }
    gfx_textc(160, 52, 1, "DIPPINN PRODUCTIONS", 230, 230, 236);
    if (state == ST_TITLE) {   // the tagline on a card, and what to press
        ui_card(66, 196, 188, 36);
        ui_textc(160, 201, 1, "THE VOXEL LIFE, NOW IN FULL 3D", UI_BLUE2);
        if ((title_t / 20) & 1) ui_hints(160, 214, "START PLAY", 1);
    }
}
static const char *const menuNm[] = { "STORY MODE", "CREATE A BORE", "JUKEBOX", "OPTIONS", "HOW TO PLAY" };
static const char *const menuBlurb[] = { "HOME, THE ISLAND, THE TOWN", "SCULPT YOUR BORE", "ALL 33 SONGS", "PICTURE AND SPEED", "CONTROLS AND TIPS" };
static void menu_update(void) {
    title_t++;
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + 4) % 5;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % 5;
    if (PRESSED(PAD_CIRCLE)) state = ST_TITLE;
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_START)) {
        switch (menu_sel) {
            case 0: life_started = 0; start_life(); cam_yaw = 2600; cam_pitch = 640; cam_dist = 1500; state = ST_LIFE; frame = 0; break;
            case 1: prev_state = ST_MENU; create_tab = 0; create_row = 0; state = ST_CREATE; world_backup(); world_gen_studio(); break;
            case 2: prev_state = ST_MENU; state = ST_JUKEBOX; break;
            case 3: opts_open(ST_MENU); break;
            case 4: prev_state = ST_MENU; help_page = 0; state = ST_HELP; break;
        }
    }
}
static void menu_draw(void) {
    title_draw();
    static const s8 ic[5] = { IC_PLAY, IC_STAR, IC_NOTE, IC_GEAR, IC_HELP };
    ui_card(92, 64, 136, 126);
    for (int i = 0; i < 5; i++) ui_button(98, 70 + i * 20, 124, 17, menuNm[i], ic[i], i == menu_sel);
    ui_textc(160, 175, 1, menuBlurb[menu_sel], UI_SOFT);
}

// ---- the loading screen: a bar and what is being made, shown before any long job (start-up, a new life, travel) ----
static void loading_draw(const char *what, int pct) {   // The Sims 4's way: a big plumbob, the name, a white card with the bar, and a tip
    static const char *const tips[] = { "BILLS COME AT MIDNIGHT: KEEP $800 BY.", "START, THEN MAP: A TAXI ANYWHERE FOR $20.",
        "L2 ZOOMS OUT: SAN BOREAS GOES ON FOR KILOMETRES.", "A STEADY 30 (OPTIONS) FEELS SMOOTHER.", "MOSS DOES NOT WANT A PLUMBOB. DO NOT ASK.",
        "CHAT 5 TIMES TO MAKE A FRIEND.", "BUILD MODE HAS 103 WALLPAPERS." };
    pct = CLAMP(pct, 0, 100);
    gfx_set_clear(60, 120, 190);
    gfx_grad(0, 0, SCREEN_W, SCREEN_H, UI_RGB(130, 196, 244), UI_RGB(130, 196, 244), UI_RGB(34, 96, 170), UI_RGB(34, 96, 170), 0);
    ui_icon_s(144, 22, IC_PLUMBOB, 36, UI_RGB(40, 120, 30));   // (a darker copy under it, for depth)
    ui_icon_s(142, 20, IC_PLUMBOB, 36, UI_GREEN);
    { int w = gfx_text_w(3, "BORE 2"); gfx_text_raw(162 - w / 2, 66, 3, "BORE 2", 40, 60, 110); gfx_text_raw(160 - w / 2, 64, 3, "BORE 2", 255, 255, 255); }
    ui_card(60, 124, 200, 40);
    ui_text(70, 131, 1, what, UI_INK);
    { char b[8]; sprintf(b, "%d%%", pct); ui_text(250 - ui_tw(1, b), 131, 1, b, UI_SOFT); }
    ui_bar(70, 146, 180, 9, pct, UI_BLUE);
    { static int tip = -1, tip_t; int now = VSync(-1); if (tip < 0 || now - tip_t > 300) { tip = (tip + 1 + (now & 3)) % 7; tip_t = now; }
      const char *t = tips[tip]; int w = ui_tw(1, t) + 40;
      ui_round(160 - w / 2, 180, w, 15, 6, UI_RGB(24, 70, 130), UI_RGB(24, 70, 130), 0);
      ui_text(160 - w / 2 + 6, 184, 1, "TIP", UI_RGB(255, 210, 90)); ui_text(160 - w / 2 + 30, 184, 1, t, UI_WHITE); }
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
    mem8_init();                                     // (8 MB of RAM? one disc for both)
    gfx_init();
    atlas_init();
    font_init();
    ui_init();          boot("GFX ATLAS FONT OK");   // (the boot text needs the font, so the first line comes after it)
    // each stage is announced BEFORE it runs ("> NAME"), so the last line on screen names the stage that is running when the game stops
    boot("> LOOK");        look_init();
    boot("> VOXEL");       voxel_init();
    boot("> ITEMS");       items_init();
    boot("> WORLD TEX");   world_init();
    boot("> AUDIO+DISC");  audio_init();
    pad_init();                                      // (the pad is read directly in read_pad: see pad.c)
    boot("> CROWD");       crowd_init();             // the voice clips are in the sound chip now: their RAM holds the crowd's models
    opts_apply();                                    // (the options' starting choices: HIGH)
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
#ifdef AUTO_TOUR
        tour_tick();
#endif
#ifdef AUTO_GROW
        grow_tick();
#endif
        ui_light = 0;
        switch (state) {
            case ST_TITLE: title_update(); title_draw(); break;
            case ST_MENU: menu_update(); menu_draw(); break;
            case ST_CREATE: create_update(); create_draw(); break;
            case ST_LIFE:
#ifdef AUTO_PARTY
                { static int ap; if (++ap == 120) { cash += 100; throw_party(); } if (ap == 200) { extern int prof_on; prof_on = 1; }
                  if (ap > 200) { extern int ap_frames, ap_vs, ap_cpu; ap_frames++; ap_vs += steps; ap_cpu += t_cpu; } }   // (test builds: a party a couple of seconds in)
#endif
                { int t0 = hb_now(); life_update(steps); t_upd = hb_now() - t0; } life_draw(); break;
            case ST_PAUSE: pause_update(); pause_draw(); break;
            case ST_BUILD: build_update(); build_draw(); break;
            case ST_JUKEBOX: juke_update(); juke_draw(); break;
            case ST_HELP: help_update(); help_draw(); break;
            case ST_TRAVEL: travel_update(); travel_draw(); break;
            case ST_STORY: story_update(); story_draw(); break;
            case ST_PIE: pie_update(); pie_draw(); break;
            case ST_PANEL: panel_update(); panel_draw(); break;
            case ST_MAP: map_update(); map_draw(); break;
            case ST_DIR: dir_update(); dir_draw(); break;
            case ST_OPTS: opts_update(); opts_draw(); break;
            case ST_JOBS: jobs_update(); jobs_draw(); break;
            case ST_OVER: over_update(); over_draw(); break;
        }
        t_cpu = hb_now() - t_frame0;
        if (state == ST_LIFE) { avg_cpu16 += t_cpu - (avg_cpu16 >> 4); avg_world16 += t_world - (avg_world16 >> 4);   // (the test readouts)
            avg_hh16 += gfx_prof[4] - (avg_hh16 >> 4); hist_cpu[MIN(4, t_cpu / 262)]++;
            for (int k = 0; k < 8; k++) avg_prof16[k] += gfx_prof[k] - (avg_prof16[k] >> 4); }
        gfx_flip();
    }
    return 0;
}
