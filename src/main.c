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
#include "ghost.h"
#include "arcade.h"
#include "gotchi.h"
#include "realm.h"
#include "ovl.h"
static const OvlHead *ovl;   // the overlay that is in (KINGDOM.OVL, ARCADE.OVL)
#include "far.h"
#include "ui.h"
#include "places.h"
#include "home.h"
#include "island.h"
#include "save.h"
#include "mcard.h"
#include "story.h"
#include "skate.h"
#include "traffic.h"

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
        btn = pad->btn & pad_latch_take();   // (and any tap seen between frames)
        if (pad->type == 0x5 || pad->type == 0x7) {
            analog = 1;
            ls_x = pad->ls_x - 128; ls_y = pad->ls_y - 128; rs_x = pad->rs_x - 128; rs_y = pad->rs_y - 128;
            if (ABS(ls_x) < 20) ls_x = 0; if (ABS(ls_y) < 20) ls_y = 0; if (ABS(rs_x) < 20) rs_x = 0; if (ABS(rs_y) < 20) rs_y = 0;
        }
    }
#if defined(AUTO_TOUR) || defined(AUTO_GROW) || defined(AUTO_HOME) || defined(AUTO_ISLE) || defined(AUTO_SCENE) || defined(AUTO_PHONE)
    { extern int auto_btn; if (auto_btn) { btn &= (u16)~auto_btn; auto_btn = 0; } }   // (test builds: the tour presses a button for a frame)
#endif
#ifdef AUTO_PHONE
    { extern int auto_hold; btn &= (u16)~auto_hold; }   // (... or holds some down)
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
static int wall_view;                                          // WALL VIEW (options): 0 AUTO (cut away, but not in first person), 1 CUT AWAY, 2 FULL WALLS
static int cam_view;                                           // CAMERA: 0 THE SIMS (it orbits you), 1 BEHIND (it swings round behind you), 2 FIRST PERSON
static int fp_pitch;                                           // first person: looking up (+) or down (-)
static int cam_turn_rate = 40, cam_zoom_tilt = 1;
static void camera_apply(int fx, int fy, int fz) {
    { int k_ = (on_board || veh_on || sims[player].gait.run) ? 3 : 6; cam_fx += (fx - cam_fx) / k_; cam_fy += (fy - cam_fy) / k_; cam_fz += (fz - cam_fz) / k_; }   // (a rider: kept up with)
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
static void camera_input(int steps) {
    if (cam_view == 2) {   // first person: L2 R2 look down and up (L1 R1 and the d-pad turn: life_update)
        if (HELD(PAD_L2)) fp_pitch -= 24 * steps;
        if (HELD(PAD_R2)) fp_pitch += 24 * steps;
        if (analog) fp_pitch -= rs_y * steps / 6;
        fp_pitch = CLAMP(fp_pitch, -700, 600);
        if (!HELD(PAD_L2) && !HELD(PAD_R2) && !(analog && ABS(rs_y) > 8) && fp_pitch && sims[player].gait.amp > 2000) {   // walking on, not looking: the head levels out again
            int d = MIN(ABS(fp_pitch), 14 * steps); fp_pitch += fp_pitch > 0 ? -d : d; }
        return;
    }
    if (HELD(PAD_L1) && !skate_air()) cam_yaw -= cam_turn_rate * steps;   // (in the air on a board, L1 R1 spin you)
    if (HELD(PAD_R1) && !skate_air()) cam_yaw += cam_turn_rate * steps;
    if (HELD(PAD_L2)) { cam_dist += 30 * steps; if (cam_zoom_tilt) cam_pitch += 9 * steps; }    // out: up and away
    if (HELD(PAD_R2)) { cam_dist -= 30 * steps; if (cam_zoom_tilt) cam_pitch -= 9 * steps; }    // in: down to their level
    if (analog) { cam_yaw += rs_x * steps / 3; cam_pitch += rs_y * steps / 4; }
    cam_yaw &= 4095;
}
static const char *const camViewNm[3] = { "THE SIMS CAMERA", "CAMERA BEHIND YOU", "FIRST PERSON" };

// ---- the household ----
static Bore edit;              // the creature on the creator's table
#define title_bore (sims[MAX_SIMS - 1].bore)   // (the title's Bore borrows the last sim slot: no life is on while the title shows)
static int frame;
static int t_vb;

// ---- states ----
static void party_begin(void);
enum { ST_TITLE, ST_MENU, ST_CREATE, ST_LIFE, ST_PAUSE, ST_BUILD, ST_JUKEBOX, ST_HELP, ST_TRAVEL, ST_STORY, ST_WANTS, ST_PIE, ST_PANEL, ST_MAP, ST_DIR, ST_OPTS, ST_JOBS, ST_OVER, ST_HOMES, ST_CRAFT, ST_PHONE, ST_SLOTS, ST_NHOOD, ST_WORLDS, ST_NAME, ST_SPICK, ST_SCARD, ST_WMAKE, ST_CONFIRM, ST_ARCADE, ST_REALM, ST_GOTCHI, ST_PARTY, ST_DJ, ST_SUPER, ST_DOG, ST_PRISON };
#ifdef AUTO_GOTCHI
static void opts_dbg(int tab, int sel, int back);
#endif
static int state = ST_TITLE, prev_state = ST_TITLE;
static int menu_sel;
static int sub_back = ST_LIFE;
enum { PH_CALL, PH_PIZZA, PH_PARTY, PH_MAP, PH_TRAVEL, PH_JOBS, PH_HOMES, PH_PEOPLE, PH_FAMILY, PH_ADOPT, PH_MUSIC, PH_GOALS, PH_TEXT, PH_PHOTO, PH_GAMES, PH_N };   // the phone's apps
static int ph_sel; void phone_open(void);              // (the phone: below)                       // where O goes back to from the phone's screens (the phone itself)
#define BACK_TO() (PRESSED(PAD_START) ? ST_LIFE : sub_back)
static int create_tab, create_row, create_from, create_turn;
static int build_tool, build_item, build_x = 8, build_y = 8, build_fl = 1, build_wp = 1, build_lvl, build_peek;
static int build_open, room_on, room_ax, room_ay;   // the DOORS tool's pick; the ROOM tool's first corner
enum { BT_BUY, BT_ROOM, BT_WALL, BT_DOOR, BT_FLOOR, BT_PAINT, BT_ROOF, NTOOL };   // build mode's tools
static int build_rot = -1, build_msg_ok, build_crot = -1, build_cx, build_cy, build_cl; static char build_carry;   // the turn picked for the next item (-1: automatic); the item being moved and where it came from
static int room_open;   // the ROOM tool: 0 no door, 1 a door, 2 a door and windows (SQUARE)
static const char *const roomOpenNm[3] = { "NO DOOR", "A DOOR", "DOOR + WINDOWS" };
static struct { int on, L, x0, y0, x1, y1, spent; char m[256]; u8 f[256], w[256]; } room_undo;   // the last room built, to take back (TRIANGLE)
extern u16 bfs_q[];
static u8 fill_seen[MW * MH / 8 + 1];
static char build_msg[32]; static int build_msg_t;   // what build mode has to say (the life's notes are not on screen there)
static void home_go(void);
static void travel_go(int z);
static void name_open(char *buf, int cap, const char *title, void (*done)(void));
// ---- ARE YOU SURE: before anything that cannot be undone (a new life, quitting, moving house). NO is picked to start with ----
static const char *cf_title; static char cf_body[160]; static void (*cf_yes)(void); static int cf_back, cf_sel, cf_t;
static void confirm_open(const char *title, const char *body, void (*yes)(void)) {
    cf_title = title; strncpy(cf_body, body, sizeof cf_body - 1); cf_body[sizeof cf_body - 1] = 0; cf_yes = yes; cf_back = state; cf_sel = 1; cf_t = 0; state = ST_CONFIRM;
}
static void life_draw(void);
static int map_bus, map_t;   // the map is the bus map (a stop on the street): $ a ride, and the wait, instead of the $20 taxi
static void wo_name_for(u32 seed);   // (THE WORLDS: a set world's capital is named for it)
static void worlds_open(void);
static void worlds_go(u32 seed, int mode);
static void story_taxi(void);
static void worlds_draw(void);
static void wm_open(void);
static void wm_apply(void);
static void wm_check_dials(void);
static int wm_used;
static void create_draw(void);
static void nhood_open(int first);   // THE NEIGHBORHOOD: 48 lots to pick a home from (first: a new life, before they have one)
static int nh_pending;                // a new life has begun: the neighborhood opens on its first frame
static u32 life_seed;                 // the world the next new life is made in (0: fresh dice)
static char life_cap[18];             // ... and its capital's name, if the player gave it one
static int life_story, sp_sel = 1, sc_t, goals_old, sp_join;   // the story the next new life is in (0: none); the STORY MISSIONS pick; the card's clock; GOALS showing THE STORY SO FAR
static int life_mode, mode_pending;   // 0 SAN BOREAS, 1 CASTAWAY (they begin on the island), 2 RAGS TO RICHES ($1,000 to start)
static int craft_sel, craft_msg_t; static char craft_msg[32];   // the CRAFT list's pick; why it could not be made (a while)
extern char _end[];
static int juke_sel, juke_scroll;
static int life_started;
static int help_page;
static int hb_now(void) { return VSync(1); }   // hblanks since the last VSync(0) (the flip): 263 per vblank, 525 = one 30 FPS frame
static int t_cpu, t_frame0, t_bub, t_hud, t_upd;
static int det_opt = 12;                                      // the FULL DETAIL option's range (the range in use dips under it when frames run long)
int dbg_slow[16][8], dbg_slow_n;
int avg_cpu16, avg_world16, avg_hh16, hist_cpu[5], avg_prof16[8];
static int show_fps = 0, fps_val, fps_frames, fps_vb, prims_last, t_world, t_sims, t_logic;
static int travel_sel;
int dbg_shift_hb, dbg_shifts;
static void taxi_to_block(int bx, int by, int tx, int ty, const char *where);
static char pie_opt[8][16]; static int pie_n, pie_sel, pie_cit;   // the pie menu
static int panel_tab;                                      // the Bore panel's tab
static int jobs_sel, jobs_n, jobs_scroll, jobs_car[16], jobs_pl[16];   // the JOBS screen
static int jobs_board;   // 1: the JOB BOARD (the computer, or a smartphone): every opening; 0: the morning paper's three
static void jobs_open(void);

// which story zones are open (the island comes with the first paycheck, the town with job level 2)
static int zone_unlocked(int z) { return z == 0 || z == 3 || (z == 1 && story_paid > 0) || (z == 2 && (job_level >= 2 || story_town_open())); }   // (the city is open from the start)

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
        case ACT_GATHER: { static const char *const gn[14] = { "GATHERING", "CHOPPING WOOD", "GATHERING STONE", "GATHERING LEAVES", "PICKING BERRIES", "SHAKING A PALM",
                               "FISHING", "DIGGING", "SEARCHING THE WRECK", "READING THE NOTE", "READING THE JOURNAL", "LISTENING", "PICKING UP A SHELL", "TAKING THE IDOL" };
                           return gn[s->act_var % 14]; }
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
    ui_card(4, 158, w, 17);
    ui_text(10, 161, 1, nm, UI_INK);
    if (s->action == ACT_WORK) ui_bar(9, 169, w - 10, 4, MIN(100, job_worked * 100 / job_shift_len()), UI_GREEN);   // (the shift so far)
    else if (act_total > 0 && act_total < 9999) ui_bar(9, 169, w - 10, 4, CLAMP(100 - s->act_t * 100 / act_total, 0, 100), UI_BLUE);
}
#define NFEED 3
#define FEED_VB 330                                     // a note shows this long (vblanks: five and a half seconds, whatever the frame rate)
static char feed[NFEED][32]; static int feed_t[NFEED], prev_note_t;   // feed_t: when it came (VSync count), 0 none
static void feed_push(const char *t) {
    for (int i = 0; i < NFEED; i++) if (feed_t[i] && !strncmp(feed[i], t, 31)) { feed_t[i] = VSync(-1) | 1; return; }   // (the same again: just kept up)
    for (int i = NFEED - 1; i > 0; i--) { strcpy(feed[i], feed[i - 1]); feed_t[i] = feed_t[i - 1]; }
    strncpy(feed[0], t, 31); feed[0][31] = 0; feed_t[0] = VSync(-1) | 1;
}
static void feed_poll(void) { extern char note_q[4][32]; extern int note_qn; for (int i = 0; i < note_qn; i++) feed_push(note_q[i]); note_qn = 0; }   // every sim_note lands here
static void feed_draw(int y) {   // the notifications stack on the left, newest on top; each slides away after a few seconds
    int now = VSync(-1);
    for (int i = 0; i < NFEED; i++) {
        if (!feed_t[i]) continue;
        int age = now - feed_t[i];
        if (age > FEED_VB || age < 0) { feed_t[i] = 0; continue; }
        int w = ui_tw(1, feed[i]) + 14, x = 4 - (age > FEED_VB - 24 ? (age - (FEED_VB - 24)) * (w + 8) / 24 : 0);   // (out to the left at the end)
        ui_card(x, y, w, 13); gfx_rect(x + 1, y + 2, 2, 9, 38, 146, 222, 0);
        ui_text(x + 6, y + 3, 1, feed[i], UI_INK);
        y += 16;
    }
}

// ---- the life ----
static void start_life(void) {
    if (life_started) return;
    loading("MAKING A NEW LIFE", 20);
    {   // a new world: SAN BOREAS from fresh dice (the land, the towns, every building and room), and its 874 households
        u32 seed = look_rand() * 2654435761u ^ ((u32)VSync(-1) * 40503u) ^ ((u32)frame * 2246822519u); if (!seed) seed = 1;
        if (life_seed) { seed = life_seed; life_seed = 0; } else bm_cap_name[0] = 0;   // (fresh dice: BORE CITY again)
        bm_rename_clear(); if (life_cap[0]) { bm_rename(0, life_cap); life_cap[0] = 0; }   // (a capital named on PICK A WORLD)   // (a preset world, or one made on the CREATE A WORLD screen)
#if defined(AUTO_CITY) || defined(AUTO_PARTY)
        seed = 0x5EED1234u;                                    // (test builds: the same world every run, so runs compare)
#endif
        bm_gen = 4; bm_new(seed); world_new_seed(seed);
        loading("PEOPLING SAN BOREAS", 45);
        u32 sb; void *sc = crowd_scratch(&sb); pop_new(seed, sc, sb);
        loading("RAISING THE SKYLINE", 60);
        far_new_world();
        loading("MAKING A NEW LIFE", 70);
    }
    world_reset_all(); home_reset(); isl_reset();
    for (int i = 0; i < nsims; i++) look_release(&sims[i].bore);   // (the last household's face textures: for this one)
    sim_init();
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 5; py = 12; }
    if (life_story) story_household(life_story, px, py); else {   // a story: its own people (you play its lead)
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
    story_reset(); }
#if !defined(AUTO_CITY) && !defined(AUTO_PARTY) && !defined(AUTO_SUPER) && !defined(AUTO_PLACES) && !defined(AUTO_TOUR) && !defined(AUTO_SCENE) && !defined(AUTO_PHONE) && !defined(AUTO_ISLE) && !defined(AUTO_HOME) && !defined(AUTO_GROW) && !defined(AUTO_SAVE) && !defined(AUTO_TOWER) && !defined(AUTO_FLOAT) && !defined(AUTO_STREAM) && !defined(AUTO_STORY) && !defined(AUTO_GOTCHI) && !defined(AUTO_DJ) && !defined(AUTO_DJ)
    nh_pending = life_mode == 0; cash = life_mode == 2 ? 1000 : 20000; mode_pending = life_mode; life_mode = 0;   // the family picks its home on THE NEIGHBORHOOD screen (The Sims: $20,000 to start), and life_update moves them in
#endif
    life_started = 1;
    story_paid = 0; story_talks = 0; story_visit[0] = 1; story_visit[1] = 0; story_visit[2] = 0; story_visit[3] = 0;
    citizens_init();   // a hundred civilians out in the island, the town and the city
    sim_note("WELCOME HOME. X USES THINGS.");
    if (life_story) { story_begin(); life_story = 0; }
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
    int mn = 0; for (int i = 0; i < b->ncell; i++) { SVECTOR cw[8]; voxel_corners(b, i, cw); for (int k = 0; k < 8; k++) if (cw[k].vz < mn) mn = cw[k].vz; }
    return -mn - 40;
}
int dbg_hh_spr, dbg_hh_full, dbg_hh_miss;
#define LOD_DIST 1280   // beyond this many world units (5 tiles) from the camera's focus, people are drawn with the crowd model
static void draw_sims(void) {
    gfx_set_reserve(RESERVE_UI);   // the people may use what the world left, but not the HUD's share
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        { Gait *g_ = &s->gait; int tg_ = g_->sleep ? 4096 : 0; g_->lie += CLAMP(tg_ - g_->lie, -170, 170); }   // lying down and getting up take about a second: lie follows where sleep is heading (voxel.c works the pose out from it)
        if (s->lvl > world_view_lvl || s->away) continue;           // upstairs, out of the floor view; at school
        if (!gfx_visible(s->x, -110 - s->lvl * WALL_FULL, s->z, 150)) continue;   // off the screen
        VECTOR p; setVector(&p, s->x, 0, s->z);
        if (s->gait.lie > 0) {   // lying (or just getting up): on the bed, the bunk, the crib's mattress, or on the floor; the height eases in with the pose
            char bc = world_at(s->act_x, s->act_y); int sm = voxel_lie_ease(s->gait.lie), bed = bc == 'S' ? 1 : bc == '<' ? 2 : 0, to;
            if (bed) to = -(bc == 'S' ? 6 * IZ : 4 * IZ);                                  // the back rests right on the pillow's top (the bunk's mattress)
            else if (bc == 'z') to = -items_height('S') + 40;                              // (the tent: as ever)
            else to = -((bc == CH_CRIB ? 4 * IZ : 0) + lying_drop(&s->bore));
            p.vy = (to * sm) >> 12; s->gait.bed = bed;
        } else s->gait.bed = 0;
        if (s->action == ACT_SIT || s->action == ACT_WC || (s->action == ACT_EAT && s->gait.sit)) p.vy = -items_height(world_at(s->act_x, s->act_y)) / 2 + 10;
        if (s->action == ACT_SWIM) p.vy = 30 + (isin((frame * 60) & 4095) >> 9);   // afloat, bobbing
        if (i == player && on_board && !skate_bailing()) p.vy -= 24 + board_lift;   // standing on the deck
        if (i == player && veh_on) p.vy -= veh_seat(veh_on - 1) + 10;               // up on the seat
        if (i == player && on_board) skate_pose(&s->gait); else s->gait.skate = 0;
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
            if (!drawn) { if (!(i == player && cam_view == 2)) voxel_draw(&s->bore, &p, i == player && on_board && !skate_bailing() ? skate_body_yaw(s->yaw) : s->yaw, &s->gait); voxel_draw_covers(&s->bore, &p, s->yaw, &s->gait); }   // (first person: the eye is inside your own head, so your body is not drawn)
            if (drawn) dbg_hh_spr++; else dbg_hh_full++;
            if (far && !drawn) dbg_hh_miss++;
            PROF_END(4);
        }
        if (i == player && veh_on) items_draw_vehicle(s->x, s->z, s->yaw, -s->lvl * WALL_FULL, veh_on - 1, veh_colour(veh_on - 1), veh_wph);
        if (i == player && on_board) { int ro, pi, sp; skate_board(&ro, &pi, &sp); items_draw_board(s->x, s->z, s->yaw, -board_lift - skate_board_up() - s->lvl * WALL_FULL, ro, pi, sp); }
        if (s->action != ACT_SLEEP && s->action != ACT_SWIM && s->lvl == 0) { VECTOR sp; setVector(&sp, s->x, 0, s->z); gfx_shadow(&sp, 70); }
    }
    traffic_draw();
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
        ui_bubble(gfx_ui_pre(sx), sy, txt, s->note_t > 0 ? UI_INK : UI_BADT);
    }
    citizens_draw_bubbles();
}

static int venue_t, venue_bx = -1, venue_by = -1; static PlaceInfo venue;
static void venue_watch(void) {   // walking onto a place: its name and its hours, for a few seconds (The Sims 4's lot name)
    PlaceInfo pi; const Sim *s = &sims[player];
    int in = place_at(sim_tile_x(s), sim_tile_y(s), &pi);
    if (in && (pi.bx != venue_bx || pi.by != venue_by)) { venue = pi; venue_t = VSync(-1) | 1; }   // (shown four seconds, whatever the frame rate)
    if (in) { venue_bx = pi.bx; venue_by = pi.by; } else venue_bx = venue_by = -1;
}
static const s8 lotIc[LOT_N] = { -1, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_HOUSE, IC_MONEY, IC_FOOD, IC_BOLT, IC_CASE, IC_BED, IC_DROP, IC_BOOK,
                                 IC_PLAY, IC_CASE, IC_BED, IC_TREE, IC_PEOPLE, IC_FAST, -1, IC_SPROUT,
                                 IC_FOOD, IC_NOTE, IC_CAMERA, IC_SMILE, IC_FOOD, IC_HEART, IC_BOOK, IC_DROP, IC_BRUSH, IC_BOLT, IC_FOOD, -1 };
static void venue_draw(void) {
    char sub[32], h[8]; int open_ = place_open(venue.lot, clock_min, clock_day);
    if (job_car != CAR_FACTORY && venue.bx == job_bx && venue.by == job_by) strcpy(sub, "YOUR WORKPLACE");
    else if (!open_) sprintf(sub, "CLOSED TILL %s", hour_name(place_opens_at(venue.lot), h));
    else if (place_opens_at(venue.lot) == 0 && place_closes_at(venue.lot) == 24) strcpy(sub, "OPEN ALL DAY");
    else sprintf(sub, "OPEN TILL %s", hour_name(place_closes_at(venue.lot), h));
    int tw_ = MAX(ui_tw(1, venue.name), ui_tw(1, sub)), ic = tw_ + 34 <= 140, w = tw_ + (ic ? 34 : 14), x = 316 - w, y = story_id && story_ch < 5 && world_zone != ZONE_ISLAND ? 73 : 57;   // (on the right, under the want and the story)
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
    // the household, top left: round portraits, a ring in each one's mood colour, the one you play ringed blue with the gold arrow
    for (int i = 0; i < nsims; i++) {
        const Sim *o = &sims[i];
        int x = 17 + i * 24, y = 21, me = i == player, mm = o->mood;
        u32 ring = me ? UI_BLUE : o->away ? UI_RGB(176, 186, 200) : mm > 15 ? UI_GREEN : mm > -15 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70);
        ui_disc(x, y, 23, UI_WHITE); ui_disc(x, y, 21, ring); ui_disc(x, y, 17, o->away ? UI_RGB(222, 228, 236) : UI_RGB(234, 240, 248));
        if (o->away) ui_icon(x - 6, y - 6, IC_BOOK, UI_RGB(112, 126, 146));   // at school till 3
        else draw_face_k(x - 8, y - 8, mm, 1);
        if (o->bore.stage == AG_BABY) { ui_disc(x + 8, y + 8, 11, UI_WHITE); ui_icon_s(x + 4, y + 4, IC_BABY, 8, UI_RGB(236, 120, 160)); }   // (a baby)
        if (o->bday && !o->away) { ui_disc(x - 8, y + 8, 12, UI_RGB(255, 200, 60)); ui_icon_s(x - 12, y + 4, IC_CAKE, 8, UI_WHITE); }   // a birthday today
        if (me && o->bore.stage != AG_BABY) { ui_disc(x + 8, y + 8, 11, UI_WHITE); ui_icon_s(x + 4, y + 4, IC_ARROW, 8, UI_RGB(236, 176, 40)); }
    }
    draw_action_card(s);
    // the needs, bottom left: a white card, an icon and a bar for each (green, then yellow, then red)
    static const s8 nic[NEED_N] = { IC_FOOD, IC_DROP, IC_BED, IC_BUBBLES, IC_SOFA, IC_SMILE, IC_CHAT, IC_HOUSE };
    ui_card(4, 178, 166, 52);
    for (int n = 0; n < NEED_N; n++) {
        int v = sim_need_val(s, n), cx = 10 + (n >> 2) * 80, cy = 183 + (n & 3) * 11;
        int r, g, bb; need_rgb(v, &r, &g, &bb);
        ui_icon(cx, cy - 1, nic[n], v < 20 ? UI_RGB(220, 60, 50) : UI_SOFT);
        ui_bar(cx + 14, cy + 1, 60, 7, v, UI_RGB(r, g, bb));
    }
    // the Bore, bottom right: the mood on a big round badge, the name, how they feel, what X would do
    ui_card(174, 178, 142, 52);
    ui_disc(192, 196, 28, moodc); ui_disc(192, 196, 24, UI_RGB(250, 252, 255));
    draw_face_k(184, 188, m, 1);
    ui_text(210, 182, 2, s->bore.first, UI_INK);
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    ui_text(210, 197, 1, mood, moodt);
    if (s->bore.stage == AG_CHILD || s->bore.stage == AG_TEEN) sprintf(b, "GRADE %s", sim_grade(s->grade)); else sprintf(b, "LVL %d", job_level);   // (a kid: the school grade)
    ui_text(212 + ui_tw(1, mood) + 4, 197, 1, b, UI_SOFT);
    { int lv = wants_level(s);                                   // the aspiration meter, under the mood
      ui_bar(210, 207, 100, 5, 100 * s->asp_pts / ASP_MAX, UI_RGB(aspLvCol[lv][0], aspLvCol[lv][1], aspLvCol[lv][2])); }
    const char *hint = on_board ? skate_hint() : sim_hint();
    { const char *ch = on_board ? "" : citizens_hint(); if (ch[0] && !sim_front_service() && (s->action == ACT_NONE || s->action == ACT_WALK)) hint = ch; }   // (busy: X gets up)
    if (hint[0]) { sprintf(b, "X %s", hint); ui_round(180, 215, 132, 11, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_text(185, 217, 1, b, UI_WHITE); }
    else if (veh_on) { ui_round(150, 215, 162, 11, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_text(155, 217, 1, veh_hud(), UI_WHITE); }   // (at the wheel: what it is, and the fuel)
    else ui_hints(246, 214, "SEL PHONE", 1);
    if (cam_view == 2 && (s->action == ACT_NONE || s->action == ACT_WALK)) {   // FIRST PERSON: a dot in the middle of the view; with something to use in front, it grows and says what X does
        if (hint[0]) { int w = ui_tw(1, hint) + 14; ui_disc(160, 120, 9, UI_WHITE); ui_disc(160, 120, 7, UI_BLUE);
                       ui_round(160 - w / 2, 131, w, 11, 4, UI_RGB(70, 176, 238), UI_BLUE2, 0); ui_text(160 - w / 2 + 5, 133, 1, "X", UI_WHITE); ui_text(160 - w / 2 + 14, 133, 1, hint, UI_WHITE); }
        else { ui_disc(160, 120, 5, UI_RGB(40, 40, 40)); ui_disc(160, 120, 3, UI_WHITE); } }
    // the clock and the money, top right
    ui_card(220, 9, 96, 28);
    sprintf(b, "%s %02d:%02d", sim_day_name(clock_day), clock_min / 60, clock_min % 60);
    ui_text(226, 12, 2, b, UI_INK);
    sprintf(b, "$%ld", cash); ui_text(226, 26, 1, b, UI_RGB(40, 150, 50));
    { static const u32 SC[4] = { UI_RGB(236, 120, 170), UI_RGB(236, 170, 30), UI_RGB(214, 110, 30), UI_RGB(80, 150, 220) };   // the season, and its festival on the last day
      int se = sim_season(), fe = sim_festival(); { const char *sn = fe ? sim_festival_name(se) : sim_season_name(se); ui_badge(160 - (ui_tw(1, sn) + 10) / 2, 7, sn, fe ? UI_AMBER : SC[se], UI_WHITE); }
      { int wx = world_zone == ZONE_ISLAND ? 0 : sim_weather();   // the weather: rain in streaks, hail in white pellets, snow in thick flakes (out of doors, and lighter through a window)
        int ou = sim_outdoors(s);
        if (wx == 1) for (int i = 0; i < (ou ? 90 : 30); i++) { int x = (i * 37 + frame * 3) % 340 - 10, y = (i * 53 + frame * (14 + i % 5)) % 240 - 8; gfx_rect_c(x, y, 1, 9, RGB(14, 20, 31), 1); gfx_rect_c(x + 1, y, 1, 7, RGB(20, 26, 31), 1); }
        if (wx == 2) for (int i = 0; i < (ou ? 45 : 15); i++) { int x = (i * 61 + frame * 4) % 330 - 5, y = (i * 47 + frame * (16 + i % 4)) % 236 - 6; ui_disc(x, y, 6, UI_RGB(240, 248, 255)); ui_disc(x, y, 4, UI_WHITE); }
        if (wx == 3) for (int i = 0; i < (ou ? 110 : 40); i++) { int sp = 1 + i % 3, x = (i * 43 + isin((frame * 7 + i * 300) & 4095) / 90 + 320 * 4 + frame / 2) % 330 - 5, y = (i * 29 + frame * sp * 2 / 3) % 236 - 6; ui_disc(x, y, 4 + (i & 1) * 2, UI_WHITE); } }
      if (world_zone != ZONE_ISLAND && sim_outdoors(s) && !sim_weather()) {   // petals, leaves and snow drift down outdoors; confetti at the festival
          static const u32 PC[4] = { UI_RGB(255, 190, 210), UI_RGB(255, 255, 255), UI_RGB(230, 130, 40), UI_RGB(250, 250, 255) };
          int n = fe ? 24 : se == 1 ? 0 : 18;
          for (int i = 0; i < n; i++) { int sp = 1 + (i % 3), x = (i * 53 + frame * (fe ? 0 : 1) / 2 * (i & 1 ? 1 : -1) + isin((frame * 9 + i * 400) & 4095) / 140 + 320 * 4) % 320, y = (i * 41 + frame * sp / 2) % 230 - 4;
              ui_disc(x, y, 5, fe ? SC[i & 3] ^ (u32)(i * 0x331) : PC[se]); } } }
    { int ic = sim_speed <= 1 ? IC_PLAY : IC_FAST; ui_icon_disc(305, 29, 12, ic, sim_speed >= 3 ? UI_RGB(236, 150, 40) : UI_BLUE, UI_WHITE); }
    { int day_ = clock_min >= 360 && clock_min < 1200; ui_icon(286, 24, IC_SUN, day_ ? UI_RGB(236, 170, 30) : UI_RGB(120, 140, 200)); }
    {   // the want, under the clock
        char t[32]; wants_text(s, (frame / 240) & 3, t);
        int w = ui_tw(1, t) + 22;
        ui_card(316 - w, 40, w, 14); ui_icon(320 - w, 41, IC_STAR, UI_RGB(236, 170, 30)); ui_text(334 - w, 43, 1, t, UI_INK);
        if (wants_flash_t > 0 && state == ST_LIFE) { int fw = ui_tw(2, wants_flash) + 16; ui_card(160 - fw / 2, 86, fw, 18); ui_textc(160, 89, 2, wants_flash, wants_flash[0] == 'W' ? UI_RGB(40, 150, 50) : UI_RGB(200, 60, 50)); wants_flash_t--; }
    }
    if (story_id && story_ch < 5 && world_zone != ZONE_ISLAND) {   // the story: this chapter's next task, under the want
        char t[48]; if (story_hud(t)) { int w = ui_tw(1, t) + 22; ui_card(316 - w, 56, w, 14); ui_icon(320 - w, 57, IC_BOOK, UI_BLUE2); ui_text(334 - w, 59, 1, t, UI_INK); } }
    { extern int citizens_card_on(void); feed_draw(citizens_card_on() ? 86 : 38); }   // (under the person card when it shows)
    if (venue_t && VSync(-1) - venue_t < 240) venue_draw(); else venue_t = 0;
    if (world_zone == ZONE_ISLAND) {   // the castaway's stores, and the secrets found
        int x = 222, y = 57; static const s8 ic[4] = { IC_WOOD, IC_STONE, IC_SPROUT, IC_FOOD };
        const int v[4] = { isl_wood, isl_stone, isl_leaves, isl_food };
        static const u32 icc[4] = { UI_RGB(150, 100, 50), UI_RGB(120, 124, 132), UI_RGB(70, 160, 70), UI_RGB(220, 120, 40) };
        ui_card(x, y, 94, 40);
        for (int k = 0; k < 4; k++) { int cx = x + 4 + (k & 1) * 46, cy = y + 3 + (k >> 1) * 12;
            ui_icon(cx, cy, ic[k], icc[k]); sprintf(b, "%d", v[k]); ui_text(cx + 15, cy + 2, 1, b, k == 3 && !isl_food ? UI_BADT : UI_INK); }
        ui_icon(x + 4, y + 27, IC_STAR, UI_RGB(236, 170, 30)); sprintf(b, "SECRETS %d/%d", isl_secret_count(), SEC_N); ui_text(x + 19, y + 29, 1, b, UI_INK);
    }
    skate_hud();
    cheer_poll(); cheer_draw();
    citizens_hud();
    { static int hint_vb = -1; if (hint_vb < 0) hint_vb = VSync(-1);   // (the controls, for the first few seconds of play only)
      if (VSync(-1) - hint_vb < 480) ui_hintbar(150, "L1 R1 TURN|L2 R2 ZOOM|X USE|SQ RUN"); }
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

static int alone_opt = 1;   // (OPTIONS > LIFE > YOUR BORE ALONE, from opts_apply)
static void life_update(int steps) {
    if (nh_pending) { nh_pending = 0; nhood_open(1); return; }
    if (mode_pending == 1) { mode_pending = 0; travel_go(ZONE_ISLAND); story_visit[1] = 1; sim_note("STRANDED! THE BOAT HOME IS ON THE BEACH"); return; }
    if (mode_pending == 2) { mode_pending = 0; if (!story_id) { sim_note("RAGS TO RICHES: $1,000 TO YOUR NAME"); sim_note("GOAL: A HOME OF YOUR OWN"); } }
#ifdef AUTO_GHOST
    { static int done; extern int sim_debug_grave(void); if (!done && frame > 60 && home_is_here()) done = sim_debug_grave(); ghost_mode = 2; }
#endif
    ghost_tick(steps, frame);
#ifdef AUTO_REALM
    { static int ar; if (++ar == 40 && (ovl = ovl_load("KINGDOM.OVL"))) { ovl->open(0); state = ST_REALM; return; } }   // (test builds: straight into KINGDOM MODE)
#endif
#ifdef AUTO_GOTCHI
    { static int ag; if (++ag == 40) {
        for (int k = 0; k < nsims && k < 3; k++) { gotchi_buy(k); Toy *t = gotchi_toy(k); static const u8 st[8] = { 4, 4, 3, 4, 2, 1, 0, 4 }, fm[8] = { 0, 3, 1, 2, 0, 0, 0, 1 };
            for (int i = 0; i < (k == 0 ? 8 : k == 1 ? 5 : 2); i++) { gotchi_egg(t, i); Blob *b = &t->b[i]; b->sp = (u8)((i * 2 + k * 3 + (AUTO_GOTCHI & 1)) % 16); b->stage = st[i]; b->form = fm[i]; b->hun = 3; b->hap = 4; b->bond = (u8)(i * 14); b->age = 16; b->poop = (u8)(i == 1 && k == 0); if (i == 2) b->hun = 0;
                if (st[i] == 4) gotchi_dex_see(b->sp, b->form); } }
        for (int k = 0; k < 40; k += 3) gotchi_dex_see(k / 4, k & 3);
        if (AUTO_GOTCHI < 20) sim_want_gotchi = (AUTO_GOTCHI >> 1) + 1;
        else if (AUTO_GOTCHI >= 26) {   // (the party pack: its things about the house, a guest list, a party going)
            int px = sim_tile_x(&sims[player]), py = sim_tile_y(&sims[player]), put = 0; static const u8 ch[8] = { AUTO_GOTCHI >= 32 ? 0xCF : 0xC7, AUTO_GOTCHI >= 32 ? 0xD0 : 0xC8, AUTO_GOTCHI >= 32 ? 0xD1 : 0xC9, AUTO_GOTCHI >= 32 ? 0xD2 : 0xCA, 0xCB, 0xCC, 0xCD, 0xCE };
            for (int r = 3; r < 9 && put < 8; r++) for (int dy = -r; dy <= r && put < 8; dy++) for (int dx = -r; dx <= r && put < 8; dx++) { if (ABS(dx) != r && ABS(dy) != r) continue; int x = px + dx, y = py + dy; if (x > 0 && y > 0 && x < MW && y < MH && lifeMap[y][x] == '.' && ((x + y) % 3) == 0 && (x & 1)) lifeMap[y][x] = (char)ch[put++]; }
            world_touched(); for (int i = 0; i < 90; i++) { pop_per[i * 7].rel = (s8)(20 + i % 50); }
            if (AUTO_GOTCHI == 26) sim_want_party = 1;
            if (AUTO_GOTCHI == 28) { party_open = 1; cash += 500; party_begin(); }
            if (AUTO_GOTCHI == 30 || AUTO_GOTCHI == 33) { int i; for (i = 0; i < items_count(); i++) if ((u8)items_at(i) == (AUTO_GOTCHI == 33 ? 0xCF : 0xC7)) break; build_item = i; build_x = px; build_y = py; build_tool = 0; room_on = 0; build_msg_t = 0; state = ST_BUILD; }
        }
        else {   // (the guide's pictures: the toy's things about the house, the catalog, the options)
            int px = sim_tile_x(&sims[player]), py = sim_tile_y(&sims[player]), put = 0; static const u8 ch[9] = { 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6 };
            for (int r = 1; r < 8 && put < 9; r++) for (int dy = -r; dy <= r && put < 9; dy++) for (int dx = -r; dx <= r && put < 9; dx++) { if (ABS(dx) != r && ABS(dy) != r) continue; int x = px + dx, y = py + dy; if (x > 0 && y > 0 && x < MW && y < MH && lifeMap[y][x] == '.' && ((x + y) & 1) == 0) lifeMap[y][x] = (char)ch[put++]; }
            world_touched(); sim_gotchi_mood(player, 2); sim_gotchi_mood(player, 1); sim_note("A BOREAGOTCHI HATCHED!");
            if (AUTO_GOTCHI == 22) { int i; for (i = 0; i < items_count(); i++) if ((u8)items_at(i) == 0xBE) break; build_item = i; build_x = px; build_y = py; build_tool = 0; room_on = 0; build_msg_t = 0; state = ST_BUILD; }
            if (AUTO_GOTCHI == 24) opts_dbg(5, 5, ST_LIFE); } } }
#endif
#ifdef AUTO_SUPER
    { static int as_; if (++as_ == 40) {   // (test builds: a band of the family, skilled by AUTO_SUPER, straight into the band screens)
        extern void band_test_setup(int lv); band_test_setup(AUTO_SUPER); sim_want_super = 1; } }
#endif
#ifdef AUTO_DJ
    { static int ad; if (++ad == 40) { dj_xp = AUTO_DJ; sim_want_dj = 1; } }   // (test builds: straight to the DJ booth, at that skill)
#endif
#ifdef AUTO_FIGHT
    { static int af; if (++af == 40) sim_want_arcade = AUTO_FIGHT; }   // (test builds: straight to a cabinet)
#endif
    if (dog_want_name) { dog_want_name = 0; name_open(dog_edit.name, 12, "NAME YOUR DOG", 0); return; }   // (the dog maker's typing screen; then it comes back)
    if (dog_resume && !sim_want_dog) { dog_resume = 0; sim_want_dog = 3; }
    if (sim_want_prison) { int n = sim_want_prison; sim_want_prison = 0; if ((ovl = ovl_load("PRISON.OVL"))) { ovl->open(n); state = ST_PRISON; } return; }   // (HARD TIME: its own file on the disc)
    if (sim_want_dog) { int rs = sim_want_dog == 3; sim_want_dog = 0; if ((ovl = ovl_load("DOG.OVL"))) { ovl->open(rs); state = ST_DOG; } return; }   // (CREATE A DOG: its own file on the disc)
    if (sim_want_bus) { sim_want_bus = 0; map_bus = 1; map_t = -1; state = ST_MAP; return; }   // (a bus stop: the bus map)
    if (sim_want_super) { sim_want_super = 0; if ((ovl = ovl_load("SUPER.OVL"))) { ovl->open(0); state = ST_SUPER; } return; }   // (SUPERSTAR: the band)
    if (sim_want_dj) { sim_want_dj = 0; if ((ovl = ovl_load("DJ.OVL"))) { ovl->open(0); state = ST_DJ; } return; }   // (the DJ booth: a file of its own on the disc)
    if (sim_want_party) { sim_want_party = 0; if ((ovl = ovl_load("PARTY.OVL"))) { ovl->open(0); state = ST_PARTY; } return; }   // (the PARTY pack: the guest list, a file of its own on the disc)
    if (sim_want_gotchi) { int g_ = sim_want_gotchi - 1; sim_want_gotchi = 0; if ((ovl = ovl_load("GOTCHI.OVL"))) { ovl->open(g_); state = ST_GOTCHI; } return; }   // (the BOREAGOTCHI! pack: a file of its own on the disc)
    if (sim_want_arcade) { int g_ = sim_want_arcade - 1; sim_want_arcade = 0; if ((ovl = ovl_load("ARCADE.OVL"))) { ovl->open(g_); state = ST_ARCADE; } else cash += arcade_price(g_); return; }   // (an arcade cabinet, the prize counter)
    if (sim_want_jobs) { sim_want_jobs = 0; jobs_board = 1; jobs_open(); sub_back = ST_LIFE; state = ST_JOBS; return; }   // (the computer's JOB BOARD)
    if (story_card()) { sc_t = 0; state = ST_SCARD; audio_voice(VOICE_FINISHED); return; }   // a chapter begins, or one is done
    Sim *s = &sims[player];
    {   // YOUR BORE ALONE (borrowed from BORE on the GBA): the pad left alone a while, your Bore looks after itself like the rest of the
        // household (needs first, a chat now and then; never a fight or a proposal: those stay yours). Any button gives it back at once.
        static int quiet, alone; int touched = (u16)~btn != 0 || (analog && (ABS(ls_x) > 20 || ABS(ls_y) > 20 || ABS(rs_x) > 20 || ABS(rs_y) > 20));
        int wait = alone_opt == 2 ? 600 : 1500;
        if (touched || !alone_opt || on_board || s->bore.stage == AG_BABY) {
            if (alone) { alone = 0; s->ai = 0; if (s->action == ACT_WALK) { s->action = ACT_NONE; s->path_n = 0; } }
            quiet = 0;
        } else if ((quiet += steps) >= wait && !alone) { alone = 1; s->ai = 1; sim_note("ON ITS OWN NOW: ANY BUTTON TAKES OVER"); }
        if (alone) { s->ai = 1; goto moved_alone; }
    }
    // movement: the stick relative to the camera
    int mx = 0, mz = 0;
    if (analog && (ls_x || ls_y)) { mx = ls_x * 32; mz = -ls_y * 32; }
    else {
        if (HELD(PAD_LEFT)) mx = -4096; if (HELD(PAD_RIGHT)) mx = 4096;
        if (HELD(PAD_UP)) mz = 4096; if (HELD(PAD_DOWN)) mz = -4096;
    }
#ifdef AUTO_SKATE
    extern int tst_on, tst_wx, tst_wz;
    if (tst_on) { for (int k = 0; k < steps; k++) sim_player_move(tst_wx, tst_wz, 0); goto moved_; }   // (test builds: the stick, in world space)
#endif
    if (cam_view == 2 || veh_on) {   // first person: up and down walk (and at the wheel: up is gas, down reverses, left and right steer), left and right (and L1 R1) turn
        int turn = -((mx * 50) >> 12); if (cam_view == 2) { if (HELD(PAD_L1)) turn += 60; if (HELD(PAD_R1)) turn -= 60; }   // (yaw grows to the LEFT of a south-facing eye: right is a negative turn)
        { static int fp_turn; if (veh_on && cam_view != 2) turn = turn * 2 / 3; fp_turn += (turn - fp_turn) / 3; if (!turn && ABS(fp_turn) < 6) fp_turn = 0; turn = fp_turn;   // (a turn eases in and out, not a jerk)
          if (cam_view == 2 && ((PRESSED(PAD_L1) && HELD(PAD_R1)) || (PRESSED(PAD_R1) && HELD(PAD_L1)))) { s->yaw = (s->yaw + 2048) & 4095; fp_turn = turn = 0; } }   // L1 + R1 together: about face
        if (!mz && !turn) { int n_ = on_board ? steps : 1; for (int k = 0; k < n_; k++) sim_player_tank(0, 0, 0); }
        else for (int k = 0; k < steps; k++) sim_player_tank(mz, turn, HELD(PAD_SQUARE));
    } else if (mx || mz) {   // turn the push by the camera yaw: "up" = away from the camera
        int c = icos(cam_yaw), sn = isin(cam_yaw);
        int wx = (mx * c + mz * sn) >> 12, wz = (mz * c - mx * sn) >> 12;
        for (int k = 0; k < steps; k++) sim_player_move(wx, wz, HELD(PAD_SQUARE));
    } else { int n_ = on_board ? steps : 1; for (int k = 0; k < n_; k++) sim_player_move(0, 0, 0); }   // (a board rolls on every step)
    moved_alone: ;
#ifdef AUTO_SKATE
    moved_: ;
#endif
    if ((PRESSED(PAD_L2) && HELD(PAD_R2)) || (PRESSED(PAD_R2) && HELD(PAD_L2))) {   // L2 + R2 together: the next camera
        extern void cam_view_set(int v); cam_view_set((cam_view + 1) % 3);
    }
    if (cam_view != 2 && ((PRESSED(PAD_L1) && HELD(PAD_R1)) || (PRESSED(PAD_R1) && HELD(PAD_L1)))) cam_yaw = (2048 - s->yaw) & 4095;   // L1 + R1: round behind you
    if (on_board && cam_view == 0 && skate_speed() > 6 && !skate_air()) {   // riding: the camera follows round behind the way you are rolling (a skate game's chase camera)
        int d = (((2048 - skate_dir()) - cam_yaw + 2048) & 4095) - 2048;
        cam_yaw = (cam_yaw + d * MIN(steps, 4) / 24) & 4095;
    }
    if (veh_on && cam_view != 2 && !HELD(PAD_L1) && !HELD(PAD_R1) && !(analog && (rs_x || rs_y))) {   // at the wheel: the camera swings round behind the car
        int d = (((2048 - s->yaw) - cam_yaw + 2048) & 4095) - 2048;
        cam_yaw = (cam_yaw + d * MIN(steps, 4) / 20) & 4095;
    }
    if (veh_on && PRESSED(PAD_TRIANGLE)) { sim_note("BEEP BEEP!"); audio_voice(VOICE_FINISHED); traffic_honk(); }
    if (cam_view == 1 && s->gait.amp > 2000 && s->action == ACT_NONE) {   // BEHIND: the camera swings round behind as you walk
        int d = (((2048 - s->yaw) - cam_yaw + 2048) & 4095) - 2048;
        cam_yaw = (cam_yaw + d * MIN(steps, 4) / 12) & 4095;
    }
    if (on_board) {   // riding: the buttons are the board's (skate.c)
        int h = (HELD(PAD_CROSS) ? SKB_X : 0) | (HELD(PAD_SQUARE) ? SKB_SQ : 0) | (HELD(PAD_CIRCLE) ? SKB_O : 0) | (HELD(PAD_TRIANGLE) ? SKB_TRI : 0) | (HELD(PAD_L1) ? SKB_L1 : 0) | (HELD(PAD_R1) ? SKB_R1 : 0);
        int p_ = (PRESSED(PAD_CROSS) ? SKB_X : 0) | (PRESSED(PAD_SQUARE) ? SKB_SQ : 0) | (PRESSED(PAD_CIRCLE) ? SKB_O : 0) | (PRESSED(PAD_TRIANGLE) ? SKB_TRI : 0) | (PRESSED(PAD_L1) ? SKB_L1 : 0) | (PRESSED(PAD_R1) ? SKB_R1 : 0);
#ifdef AUTO_SKATE
        { extern int tst_held, tst_pr; h |= tst_held; p_ |= tst_pr; tst_pr = 0; }
#endif
        skate_input(h, p_);
    } else
    if (PRESSED(PAD_CROSS)) {   // X: busy - stop; otherwise the pie menu (one option: just do it)
        Sim *me = &sims[player];
        if (me->action != ACT_NONE && me->action != ACT_WALK) sim_player_interact();
        else if (citizens_punch()) { }   // a fight is on and someone is in reach
        else { pie_cit = 1; pie_n = sim_front_service() ? 0 : citizens_pie_open(pie_opt);   // a civilian in front of you first, then the things (a counter right ahead wins)
               if (!pie_n) { pie_cit = 0; pie_n = sim_pie_open(pie_opt); }
               if (pie_n == 1) { if (pie_cit) citizens_pie_do(0); else sim_pie_do(0); } else if (pie_n > 1) { pie_sel = 0; state = ST_PIE; } else audio_voice(VOICE_NAH); }
    }
    if (PRESSED(PAD_CIRCLE) && !on_board) sim_player_cancel();
    if (PRESSED(PAD_TRIANGLE) && !on_board && !veh_on) { sim_speed = sim_speed % 3 + 1; }
    if (PRESSED(PAD_L3)) show_fps ^= 1;
    { extern int getenv_noshadow, dbg_noao, dbg_nograd; if (PRESSED(PAD_R3)) { static int mode; mode = (mode + 1) & 3; getenv_noshadow = mode == 1; dbg_noao = mode == 2; dbg_nograd = mode == 3; } }
    if (PRESSED(PAD_SELECT)) { extern void phone_open(void); phone_open(); }   // the phone in your pocket
    { int t0 = hb_now(); for (int k = 0; k < steps; k++) sim_step(); citizens_update(steps); traffic_update(steps); t_logic = hb_now() - t0; }
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
#if defined(AUTO_FLOAT)
          {   // (test builds: ULTRA, a walk round a square of blocks with a high camera turning: the storeys must stand on their buildings)
            static const u8 pre[] = { 4, 5, 3, 2, 3, 2, 3, 3, 2, 2, 1, 1 }; extern void float_opts(const u8 *p, int n); if (fr_ < 4) float_opts(pre, sizeof pre);
            if (fr_ >= 60 && !(done_ & 4)) { done_ |= 4; const Settle *cp = bm_set(bm_capital()); taxi_to_block(cp->x + 1, cp->y + 1, 6, 6, "TEST"); }
            if (fr_ > 120 && s->action == ACT_NONE) { int ph = ((fr_ - 120) / 700) & 3; int v = 9 * steps; if (ph == 0) s->x += v; else if (ph == 1) s->z -= v; else if (ph == 2) s->x -= v; else s->z += v; }
            cam_pitch = 470; cam_dist = 3200; cam_yaw = (fr_ * 3) & 4095;
          }
#elif !defined(AUTO_TOWER) && !defined(AUTO_PLACES) && !defined(AUTO_TOUR)
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
    story_tick(steps);
    feed_poll();
    if (!(frame & 7)) venue_watch();
    camera_input(steps);
    if (PRESSED(PAD_START)) { state = ST_PAUSE; menu_sel = 0; }
    if (travel_asked) { travel_asked = 0; travel_sel = home_city && home_is_here() ? 0 : world_zone + home_city; state = ST_TRAVEL; }
    if (sim_story_over) { sim_story_over = 0; state = ST_OVER; }   // the last of the family is gone
    if (home_move_asked) { home_move_asked = 0; home_go(); }          // a home just bought: they move in
    if (isl_craft_asked) { isl_craft_asked = 0; craft_sel = 0; craft_msg_t = 0; state = ST_CRAFT; }   // the campfire: make something
}

static void life_draw(void) {
#ifdef AUTO_DAY
    { static int once; if (++once == 30) { day_count = AUTO_DAY; clock_min = 12 * 60; } }
#endif
#ifdef AUTO_DOGMK
    { static int o3; if (++o3 == 40) sim_want_dog = 1; }
#endif
#ifdef AUTO_PRISON
    { static int o4; if (++o4 == 40) { sim_crime(CR_ASSAULT, 60); sim_crime(CR_ASSAULT, 20); sim_crime(CR_ASSAULT, 20); sim_crime(CR_RESIST, 10); sim_crime(CR_RESIST, 10); rap_sheet = 4; sim_arrest(0); } }
#endif
#ifdef AUTO_PETBUS
    { static int once2; if (++once2 == 40) { Sim *pl = &sims[player]; int px = sim_tile_x(pl), py = sim_tile_y(pl), put = 0; pet_kind = AUTO_PETBUS; pet_col = 3; bus_pass = day_count + 4;
        for (int r = 2; r < 9 && !put; r++) for (int dy = -r; dy <= r && !put; dy++) for (int dx = -r; dx <= r && !put; dx++) { if (ABS(dx) != r && ABS(dy) != r) continue; int x = px + dx, y = py + dy;
            if (x > 0 && y > 0 && x < MW && y < MH && lifeMap[y][x] == '.') { lifeMap[y][x] = '+'; wallMap[y][x] = 0x80 | (AUTO_PETBUS & 3); clock_min = 12 * 60 + (20 - (12 * 60 + x * 7 + y * 13) % 20) % 20; put = 1; } }
        world_touched(); } }
#endif
    Sim *s = &sims[player];
    gfx_set_fog(0, 0);                                   // (the title's fog stays on the title)
    camera_apply(s->x, -200 - s->lvl * WALL_FULL - (on_board ? board_lift : 0), s->z);   // (up with a rider in the air)
    world_view_lvl = s->lvl; world_roof_show = 0;
    { extern int world_fp; world_fp = cam_view == 2; }
    if (cam_view == 2) {   // FIRST PERSON: the eye in the Bore's head, looking the way it faces (fp_pitch up or down)
        int fx = isin(s->yaw), fz = -icos(s->yaw), eh = H * VOX - 60;   // (the face is (sin, -cos): sim.c's yaw)
        if (s->bore.stage == AG_CHILD) eh = eh * 3 / 4; else if (s->bore.stage == AG_BABY) eh /= 2;
        cam.eye.vx = s->x - (fx * 40 >> 12); cam.eye.vz = s->z - (fz * 40 >> 12); cam.eye.vy = -eh - s->lvl * WALL_FULL - (s->gait.amp > 2000 ? (ABS(isin(s->gait.phase * 2)) >> (HELD(PAD_SQUARE) ? 8 : 9)) : 0);   // (a step's bob, bigger running)
        if (s->gait.amp > 2000) { int sw = (isin(s->gait.phase) * (HELD(PAD_SQUARE) ? 26 : 16)) >> 12; cam.eye.vx += (fz * sw) >> 12; cam.eye.vz -= (fx * sw) >> 12; }   // (and a sway side to side, one way per foot)
        int cp = icos(fp_pitch), sp = isin(fp_pitch);
        cam.at.vx = cam.eye.vx + ((fx * cp >> 12) * 600 >> 12); cam.at.vz = cam.eye.vz + ((fz * cp >> 12) * 600 >> 12); cam.at.vy = cam.eye.vy - (sp * 600 >> 12);
        gfx_camera_update();
        cam_fx = s->x; cam_fy = -200 - s->lvl * WALL_FULL; cam_fz = s->z;   // (the third-person camera picks up from here)
        cam_yaw = (2048 - s->yaw) & 4095;
        world_view_lvl = MIN(NUP, s->lvl + 1);                             // (the storey above: its floor is the ceiling)
    }
    if (world_zone == ZONE_CITY || world_zone == ZONE_HOME || world_zone == ZONE_TOWN) { int wx = sim_weather(); world_season_skin(sim_season(), sim_season() == 3 ? 2 : wx == 1 || wx == 2 ? 1 : 0); { extern int far_season, far_wet; far_season = sim_season(); far_wet = wx == 1 || wx == 2; } }
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
        if (world_zone == ZONE_CITY || world_zone == ZONE_HOME) {   // the season tints the sky
            static const u8 ST[4][6] = { { 130, 160, 235, 240, 215, 220 }, { 60, 130, 240, 250, 225, 170 }, { 120, 110, 170, 240, 160, 90 }, { 150, 160, 190, 225, 230, 245 } };
            const u8 *q = ST[sim_season()]; tr = (tr + ((q[0] * day) >> 8)) / 2; tg = (tg + ((q[1] * day) >> 8)) / 2; tb = (tb + ((q[2] * day) >> 8)) / 2; br = (br + ((q[3] * day) >> 8)) / 2; bg = (bg + ((q[4] * day) >> 8)) / 2; bb = (bb + ((q[5] * day) >> 8)) / 2; }
        { int wx = world_zone == ZONE_ISLAND ? 0 : sim_weather(); if (wx) { int k = wx == 3 ? 10 : 12;   // weather greys the sky and dims the day
              tr = (tr * (16 - k) + ((wx == 3 ? 170 : 90) * day >> 8) * k) / 16; tg = (tg * (16 - k) + ((wx == 3 ? 176 : 98) * day >> 8) * k) / 16; tb = (tb * (16 - k) + ((wx == 3 ? 190 : 112) * day >> 8) * k) / 16;
              br = (br * (16 - k) + ((wx == 3 ? 200 : 120) * day >> 8) * k) / 16; bg = (bg * (16 - k) + ((wx == 3 ? 205 : 126) * day >> 8) * k) / 16; bb = (bb * (16 - k) + ((wx == 3 ? 214 : 138) * day >> 8) * k) / 16; } }
        u32 top = (u32)tr | ((u32)tg << 8) | ((u32)tb << 16), hor = (u32)br | ((u32)bg << 8) | ((u32)bb << 16);
        { extern u32 world_horizon; extern int world_far_range; world_horizon = hor; world_far_range = (world_zone == ZONE_CITY && far_view) ? 60 : 34; }
        if (world_zone == ZONE_CITY && far_view) far_sky(top, hor);   // the sky meets the land at the real horizon
        else if (world_zone == ZONE_ISLAND) far_sky_sea(top, hor, (u32)((40 * day) >> 8) | ((u32)((112 * day) >> 8) << 8) | ((u32)((168 * day) >> 8) << 16));   // (the endless sea)
        else if (world_zone == ZONE_HOME) far_sky_sea(top, hor, (u32)((64 * day) >> 8) | ((u32)((120 * day) >> 8) << 8) | ((u32)((52 * day) >> 8) << 16));     // (the fields going on)
        else if (world_zone == ZONE_TOWN) far_sky_sea(top, hor, (u32)((196 * day) >> 8) | ((u32)((160 * day) >> 8) << 8) | ((u32)((108 * day) >> 8) << 16));  // (the desert going on)
        else gfx_sky(top, hor);
    }
    gfx_prim_count = 0;
    world_doors_reset(); for (int i = 0; i < nsims; i++) world_door_mark(sim_tile_x(&sims[i]), sim_tile_y(&sims[i])); citizens_door_marks();
    { int t0 = hb_now(); world_draw(sim_tile_x(s), sim_tile_y(s), frame, wall_view == 1 || (wall_view == 0 && cam_view != 2)); t_world = hb_now() - t0;   // (WALL VIEW: AUTO cuts the walls between you and the camera, but not in first person; CUT AWAY always; FULL WALLS never)
      t0 = hb_now(); gfx_prim_count = 0; draw_sims(); if (pet_kind && pet_zone == world_zone) { if (pet_kind == 1) items_draw_dog(pet_x, pet_z, pet_yaw, -sims[player].lvl * WALL_FULL, &pet_dog, pet_ph); else items_draw_pet(pet_x, pet_z, pet_yaw, -sims[player].lvl * WALL_FULL, pet_kind, pet_col, pet_ph); } ghost_draw(cam_yaw, frame); t_sims = hb_now() - t0; }
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
    if (z == ZONE_ISLAND) { sim_note("FIND FOOD: BERRIES, FISH"); sim_note("X ON TREES AND ROCKS: GATHER"); sim_note("8 SECRETS ARE HIDDEN HERE"); }
    audio_voice(VOICE_YAHOO);
}
static void travel_update(void) {   // (with a home in SAN BOREAS, the first row is HOME: travel_sel 0; the zones after it)
    int nr = ZONE_N + home_city;
    if (PRESSED(PAD_UP)) travel_sel = (travel_sel + nr - 1) % nr;
    if (PRESSED(PAD_DOWN)) travel_sel = (travel_sel + 1) % nr;
    if (travel_sel >= nr) travel_sel = 0;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = BACK_TO(); return; }
    if (PRESSED(PAD_CROSS)) {
        if (home_city && travel_sel == 0) { state = ST_LIFE; if (!home_is_here()) home_go(); return; }
        int z = travel_sel - home_city;
        if (!zone_unlocked(z)) { audio_voice(VOICE_NAH); return; }
        if (z != world_zone) { travel_go(z); story_visit[z] = 1; for (int i = 0; i < nsims; i++) { if (z < 3) wants_event(&sims[i], z == 1 ? W_ISLAND : W_TOWN, 0); wants_reroll(&sims[i]); } }
        state = ST_LIFE;
    }
}
static void travel_draw(void) {   // where to: the four places, each on a row with its picture, the locked ones greyed with what opens them
    static const char *const blurb[ZONE_N] = { "HOUSE, FACTORY, RAIL PARK", "SURVIVE, BUILD, FIND 8 SECRETS", "DINER, TRAILER AND THE LAB", "" };
    static const char *const lockTxt[ZONE_N] = { "", "EARN YOUR FIRST PAYCHECK", "GET PROMOTED TWICE", "" };
    static const s8 zic[ZONE_N] = { IC_HOUSE, IC_TREE, IC_SUN, IC_PIN };
    char b[48];
    life_draw();
    ui_dim();
    int top = ui_window(36, home_city ? 8 : 24, 248, home_city ? 224 : 192, "WHERE TO?", IC_CAR);
    if (home_city) {   // home: the family's own, in SAN BOREAS
        int y = top + 2, sel = travel_sel == 0, here = home_is_here(); char ad[24];
        if (sel) { ui_round(42, y, 236, 31, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(42, y + 4, 2, 23, 38, 146, 222, 0); }
        ui_icon_disc(61, y + 15, 22, IC_HOUSE, sel ? UI_GREEN : UI_RGB(90, 170, 110), UI_WHITE);
        ui_text(78, y + 6, 1, "HOME", sel ? UI_BLUE2 : UI_INK);
        ui_text(78, y + 18, 1, here ? "YOU ARE HERE" : home_address(home_bx, home_by, ad), here ? UI_GOODT : UI_SOFT);
        top += 34;
    }
    for (int i = 0; i < ZONE_N; i++) {
        int y = top + 2 + i * 34, sel = i + home_city == travel_sel, here = i == world_zone && !(home_city && i == ZONE_CITY && home_is_here()), open_ = zone_unlocked(i);
        if (sel) { ui_round(42, y, 236, 31, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(42, y + 4, 2, 23, 38, 146, 222, 0); }
        ui_icon_disc(61, y + 15, 22, open_ ? zic[i] : IC_LOCK, !open_ ? UI_RGB(176, 184, 196) : sel ? UI_BLUE : UI_RGB(110, 146, 190), UI_WHITE);
        ui_text(78, y + 6, 1, world_zone_name(i), !open_ ? UI_SOFT : sel ? UI_BLUE2 : UI_INK);
        const char *t = !open_ ? lockTxt[i] : blurb[i];
        if (i == ZONE_CITY) { int nf = 0; while (nf < NFAM && pop_fam[nf].n) nf++; sprintf(b, "%d TOWNS, %d HOUSEHOLDS", bm_nset(), nf); t = b; }
        if (here) t = "YOU ARE HERE";
        ui_text(78, y + 18, 1, t, here ? UI_GOODT : open_ ? UI_SOFT : UI_BADT);
    }
    ui_hints(160, top + 143, "X GO|O BACK", 1);
}

// ---- the pause menu ----
// ---- SAN BOREAS: the map (and a taxi anywhere on it) ----
// The map is a 128x128 picture of the whole state (8x8 blocks a pixel), made from the generator when it is opened and put
// in the VRAM the people's sprites use (they are rendered again after).
static int map_sel;
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
    int px = (bx - world_wbx) * BM_BLK + (tx >= 0 ? tx : 9), py = (by - world_wby) * BM_BLK + (ty >= 0 ? ty : BM_LOT0 / 2);
    put_household(px, py);
    citizens_enter(ZONE_CITY);
    char b[48]; snprintf(b, sizeof b, "WELCOME TO %s", where); sim_note(b);
    audio_voice(VOICE_YAHOO);
}
static void home_go(void) {
    char ad[24]; home_address(home_bx, home_by, ad);
    int tx, ty;
    if (!world_own_door(&tx, &ty)) { taxi_to_block(home_bx, home_by, -1, -1, ad); if (!world_own_door(&tx, &ty)) return; }
    put_household(tx, ty);
    char b[40]; snprintf(b, sizeof b, "HOME: %s", ad); sim_note(b); sim_note("WELCOME HOME!");
    for (int i = 0; i < nsims; i++) sim_moodlet(&sims[i], 15);   // (FUN TIME: a new home)
    audio_voice(VOICE_YAHOO);
}
static u8 mi_n, mi_x[4], mi_y[4], mb_x, mb_y;   // the island map: the secrets found that stay put (the idol, the wreck, the journal); the boat home
static void map_island_open(void) {   // CASTAWAY ISLAND from above, a pixel a tile: the sea (paler by the shore), sand, meadow, jungle, rock, the huts
    u16 *px = atlas_scratch();
    for (int i = 0; i < 128 * 128; i++) px[i] = RGB(6, 14, 24) | 0x8000;
    mi_n = 0; mb_x = MW / 2; mb_y = MH - 4;
    for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) {
        u8 f = floorMap[y][x]; char c = lifeMap[y][x]; u16 col;
        if (f == FL_WATER) {
            int shore = 0; for (int k = 0; k < 4; k++) { int a = x + (k == 0) - (k == 1), b = y + (k == 2) - (k == 3); if (a >= 0 && b >= 0 && a < MW && b < MH && floorMap[b][a] != FL_WATER) shore = 1; }
            col = shore ? RGB(11, 21, 28) : RGB(6, 14, 24);
        }
        else if (c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f' || c == 'D') col = RGB(27, 25, 22);
        else if (c == 'g' || c == '3' || c == '6' || c == (char)CH_BUSH) col = RGB(4, 12, 4);
        else if (c == '*') col = RGB(13, 13, 12);
        else if (f == FL_SAND) col = RGB(30, 27, 18);
        else if (f == FL_GRASS) col = RGB(11, 21, 7);
        else if (f == FL_ROCK) col = RGB(19, 18, 16);
        else if (f == FL_PAVING) col = RGB(21, 15, 9);
        else col = RGB(24, 19, 13);   // (a floor indoors)
        px[y * 128 + x] = col | 0x8000;
        if (c == '@') { mb_x = (u8)x; mb_y = (u8)y; }
        if (mi_n < 4 && ((c == (char)CH_IDOL && (isl_secrets >> SEC_IDOL & 1)) || (c == (char)CH_WRECK && (isl_secrets >> SEC_WRECK & 1)) || (c == (char)CH_JOURNAL && (isl_secrets >> SEC_JOURNAL & 1)))) { mi_x[mi_n] = (u8)x; mi_y[mi_n] = (u8)y; mi_n++; }
    }
    atlas_vram_load(0, 256, 128, 128, px);
    crowd_flush();
    map_t = 0;
}
static void map_island_draw(void) {   // the island on a card: you, the camp, the boat home, the secrets found; the list: what is found, and a hint to the rest
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    char b[32];
    int top = ui_window(4, 4, 312, 232, world_zone_name(ZONE_ISLAND), IC_PIN);
    sprintf(b, "SECRETS %d OF %d", isl_secret_count(), SEC_N);
    ui_badge(306 - ui_tw(1, b) - 10, 8, b, UI_WHITE, UI_BLUE2);
    const int MX = 9, MY = top, MS = 178;
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, MW, MH, 128, 128, 128);
    #define MAPX(t) (MX + (t) * MS / MW + 1)
    #define MAPY(t) (MY + (t) * MS / MH + 1)
    for (int k = 0; k < mi_n; k++) ui_icon_disc(MAPX(mi_x[k]), MAPY(mi_y[k]), 11, IC_STAR, UI_AMBER, UI_WHITE);
    ui_icon_disc(MAPX(mb_x), MAPY(mb_y), 13, IC_CAR, UI_BLUE2, UI_WHITE);
    ui_icon_disc(MAPX(isl_fire_x), MAPY(isl_fire_y), 13, IC_HOUSE, UI_RGB(236, 120, 40), UI_WHITE);
    { int x = MAPX(sim_tile_x(&sims[player])), y = MAPY(sim_tile_y(&sims[player])); ui_disc(x, y, (map_t >> 3) & 1 ? 11 : 9, UI_WHITE); ui_disc(x, y, 7, UI_GREEN); }
    #undef MAPX
    #undef MAPY
    static const char *const found[SEC_N] = { "THE BOTTLE", "BURIED TREASURE", "THE TIKI IDOL", "THE SHIPWRECK", "SIX SEASHELLS", "HERMIT'S JOURNAL", "THE ISLET", "THE GOLDEN IDOL" };
    static const char *const hint[SEC_N] = { "A FAR-OFF BEACH", "BY A BIG ROCK", "DEEP IN THE JUNGLE", "A FAR SHORE", 0, "AN OLD CAMP", "OUT AT SEA", "ON THE ISLET" };
    ui_section(192, top, 118, "THE SECRETS");
    for (int i = 0; i < SEC_N; i++) {
        int y = top + 14 + i * 14, d = (isl_secrets >> i) & 1;
        if (d) ui_icon_disc(198, y + 4, 11, IC_TICK, UI_GREEN, UI_WHITE);
        else { ui_disc(198, y + 4, 11, UI_RGB(186, 196, 210)); ui_disc(198, y + 4, 7, UI_WHITE); }
        const char *t = d ? found[i] : hint[i];
        if (!d && i == SEC_SHELLS) { sprintf(b, "SEASHELLS %d OF %d", isl_shells, ISL_SHELLS); t = b; }
        if (!d && i == SEC_TREASURE && !(isl_secrets & (1 << SEC_BOTTLE))) t = "ASK THE BOTTLE";   // (the bottle's note says where)
        ui_text(206, y, 1, t, d ? UI_GOODT : UI_SOFT);
    }
    int ly = top + 20 + SEC_N * 14;
    gfx_rect(192, ly - 5, 118, 1, 214, 222, 232, 0);
    ui_disc(198, ly + 4, 11, UI_WHITE); ui_disc(198, ly + 4, 7, UI_GREEN); ui_text(206, ly, 1, "YOU", UI_INK);
    ui_icon_disc(198, ly + 18, 11, IC_HOUSE, UI_RGB(236, 120, 40), UI_WHITE); ui_text(206, ly + 14, 1, "THE CAMP", UI_INK);
    ui_icon_disc(198, ly + 32, 11, IC_CAR, UI_BLUE2, UI_WHITE); ui_text(206, ly + 28, 1, "THE BOAT HOME", UI_INK);
    gfx_rect(10, 211, 300, 1, 200, 210, 222, 0);
    ui_text(12, 218, 1, "THE CAMPFIRE: COOK, EAT, CRAFT", UI_SOFT);
    ui_hints(310, 215, "O BACK", 2);
}
static int bus_pass_ok(void) { return day_count < bus_pass; }
static void bus_trip(const Settle *st, int *fare, int *mins) {   // the ride to a town: by distance. The wait for the bus (one every 20 minutes) is on top
    int pbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK, pby = world_wby + sim_tile_y(&sims[player]) / BM_BLK, d = ABS(st->x - pbx) + ABS(st->y - pby);
    *fare = bus_pass_ok() ? 0 : MIN(15, 3 + d / 40); *mins = 15 + MIN(150, d / 3) + ((clock_min % 20) < 4 ? 0 : 20 - clock_min % 20);
}
static void map_open(void) {
    if (world_zone == ZONE_ISLAND) { map_island_open(); return; }
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
static char map_nm[18];
static void map_renamed(void) { bm_rename(map_sel, map_nm); char b[40]; snprintf(b, sizeof b, "NOW CALLED %s", map_nm); sim_note(b); }
static void map_update(void) {
    if (map_t < 0) map_open();
    map_t++;
    if (world_zone == ZONE_ISLAND) { if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START) || PRESSED(PAD_CROSS)) state = PRESSED(PAD_CIRCLE) ? sub_back : ST_LIFE; return; }   // (no taxis out here)
    int n = bm_nset();
    if (PRESSED(PAD_UP)) map_sel = (map_sel + n - 1) % n;
    if (PRESSED(PAD_DOWN)) map_sel = (map_sel + 1) % n;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { map_bus = 0; state = BACK_TO(); return; }
    if (PRESSED(PAD_TRIANGLE)) { strcpy(map_nm, bm_set(map_sel)->name); name_open(map_nm, 17, "RENAME THIS TOWN", map_renamed); return; }
    if (PRESSED(PAD_CROSS) && map_bus) {   // the bus: cheaper, slower; a pass rides free
        int fa, mi; const Settle *bs = bm_set(map_sel); bus_trip(bs, &fa, &mi);
        if (cash < fa) { char nb[40]; sprintf(nb, "THE FARE IS $%d", fa); sim_note(nb); audio_voice(VOICE_NAH); return; }
        cash -= fa; clock_min += mi; map_bus = 0;
        { char nb[48]; if (fa) sprintf(nb, "BUS TO %s: $%d, %d MIN", bs->name, fa, mi); else sprintf(nb, "BUS TO %s: PASS, %d MIN", bs->name, mi); sim_note(nb); }
        taxi_to_block(bs->x, bs->y, -1, -1, bs->name); state = ST_LIFE; return;
    }
    if (PRESSED(PAD_CROSS)) {
        if (cash < 20) { sim_note("A TAXI RIDE COSTS $20"); audio_voice(VOICE_NAH); return; }
        cash -= 20;
        const Settle *st = bm_set(map_sel);
        taxi_to_block(st->x, st->y, -1, -1, st->name);
        state = ST_LIFE;
    }
}
static void map_draw(void) {   // the state on a card: the map on the left (the towns as dots, the picked one named), the list on the right
    if (world_zone == ZONE_ISLAND) { map_island_draw(); return; }
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    char b[48];
    int top = ui_window(4, 4, 312, 232, bm_state_name(), IC_PIN);
    if (map_bus) { char bb[40]; int fa, mi; bus_trip(bm_set(map_sel), &fa, &mi);
        if (fa) sprintf(bb, "BUS $%d, %d MIN", fa, mi); else sprintf(bb, "PASS, %d MIN", mi);
        ui_badge(306 - ui_tw(1, bb) - 10, 8, bb, UI_GREEN, UI_WHITE);
        if (bus_pass_ok()) { sprintf(bb, "PASS: %d DAYS LEFT", bus_pass - day_count); ui_badge(10, 8, bb, UI_AMBER, UI_WHITE); } }
    else ui_badge(306 - ui_tw(1, "TAXI $20") - 10, 8, "TAXI $20", UI_WHITE, UI_BLUE2);
    const int MX = 9, MY = top, MS = 178;
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, 128, 128, 128, 128, 128);   // the picture (on the card: drawn with the 2D, not the scene)
    int n = bm_nset();
    for (int i = 0; i < n; i++) {                  // every town a dot: a city big, a village small
        const Settle *st = bm_set(i); if (i == map_sel) continue;
        int x = MX + st->x * MS / BM_N, y = MY + st->y * MS / BM_N, d = st->kind == ST_CITY ? 7 : st->kind == ST_TOWN ? 5 : 3;
        ui_disc(x, y, d + 2, UI_RGB(40, 46, 60)); ui_disc(x, y, d, UI_WHITE);
    }
    if (home_city) {   // home: a house on a green disc
        int x = MX + home_bx * MS / BM_N, y = MY + home_by * MS / BM_N;
        ui_disc(x, y, 15, UI_WHITE); ui_icon_disc(x, y, 13, IC_HOUSE, UI_GREEN, UI_WHITE);
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
    ui_hints(310, 215, map_bus ? "X RIDE THE BUS|TRI RENAME|O BACK" : "X TAXI|TRI RENAME|O BACK", 2);
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
        if (cash < 20) { sim_note("A TAXI RIDE COSTS $20"); audio_voice(VOICE_NAH); return; }
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
        ui_icon_disc(22, y + 9, 18, IC_ARROW, on ? UI_BLUE : UI_RGB(150, 166, 186), UI_WHITE);
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

// ---- HOMES: the homes for sale near you (houses, cottages, two-storeys, empty lots) and the one you have ----
static HomeAd hm_ad[16]; static int hm_n, hm_sel, hm_scroll;
static void homes_open(void) {
    int cbx, cby;
    if (world_zone == ZONE_CITY && world_wbx >= 0) { cbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK; cby = world_wby + sim_tile_y(&sims[player]) / BM_BLK; }
    else { const Settle *cp = bm_set(bm_capital()); cbx = cp->x; cby = cp->y; }
    hm_n = home_list(cbx, cby, hm_ad, 16); hm_sel = 0; hm_scroll = 0; state = ST_HOMES;
}
static void homes_update(void) {
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = BACK_TO(); return; }
    if (!hm_n) return;
    if (PRESSED(PAD_UP)) hm_sel = (hm_sel + hm_n - 1) % hm_n;
    if (PRESSED(PAD_DOWN)) hm_sel = (hm_sel + 1) % hm_n;
    const HomeAd *a = &hm_ad[hm_sel];
    if (PRESSED(PAD_CROSS)) { if (home_buy(a->bx, a->by)) state = ST_LIFE; }   // (life_update moves the family in)
    if (PRESSED(PAD_SQUARE)) {                                                 // go and see it first
        if (cash < 20) { audio_voice(VOICE_NAH); return; }
        cash -= 20; char ad[24]; home_address(a->bx, a->by, ad); taxi_to_block(a->bx, a->by, -1, -1, ad); state = ST_LIFE;
    }
}
static void homes_draw(void) {
    life_draw();
    ui_dim();
    char b[48], ad[24];
    int top = ui_window(8, 6, 304, 228, "HOMES FOR SALE", IC_HOUSE);
    if (home_city) { home_address(home_bx, home_by, ad); snprintf(b, sizeof b, "YOURS: %s", ad); } else strcpy(b, "YOURS: THE MAN BASE");
    ui_icon(12, top - 2, IC_HOUSE, UI_GREEN); ui_text(28, top, 1, b, UI_GOODT);
    sprintf(b, "$%ld", cash); ui_text(306 - ui_tw(1, b), top, 1, b, UI_GOODT);
    gfx_rect(12, top + 12, 296, 1, 214, 222, 232, 0);
    if (!hm_n) { ui_textc(160, top + 60, 1, "NOTHING FOR SALE NEAR HERE", UI_SOFT); ui_hints(160, 214, "O BACK", 1); return; }
    int refund = home_city ? home_paid : 0, rows = 9;
    if (hm_sel < hm_scroll) hm_scroll = hm_sel;
    if (hm_sel >= hm_scroll + rows) hm_scroll = hm_sel - rows + 1;
    for (int k = 0; k < rows && hm_scroll + k < hm_n; k++) {   // the list: what, where, how much (red: more than you have)
        int i = hm_scroll + k, y = top + 18 + k * 13, sel = i == hm_sel; const HomeAd *a = &hm_ad[i];
        if (sel) { ui_round(12, y - 2, 296, 13, 4, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(12, y, 2, 9, 38, 146, 222, 0); }
        ui_icon(17, y - 2, a->lot == LOT_EMPTY ? IC_SPROUT : IC_HOUSE, sel ? UI_BLUE : UI_SOFT);
        ui_text(33, y, 1, home_address(a->bx, a->by, ad), sel ? UI_BLUE2 : UI_INK);
        ui_text(162, y, 1, home_kind(a->lot), UI_SOFT);
        sprintf(b, "$%d", a->price); ui_text(304 - ui_tw(1, b), y, 1, b, cash + refund >= a->price ? UI_GOODT : UI_BADT);
    }
    {   // the one picked: where, what you get
        const HomeAd *a = &hm_ad[hm_sel]; const Settle *st = a->set != 255 ? bm_set(a->set) : 0;
        int y = top + 18 + rows * 13 + 4;
        ui_round(12, y, 296, 40, 6, UI_RGB(234, 240, 248), UI_RGB(224, 231, 240), 0);
        ui_icon_disc(30, y + 20, 26, a->lot == LOT_EMPTY ? IC_SPROUT : IC_HOUSE, UI_BLUE, UI_WHITE);
        snprintf(b, sizeof b, "%s IN %s", home_kind(a->lot), st ? st->name : "SAN BOREAS"); ui_text(48, y + 5, 1, b, UI_INK);
        ui_text(48, y + 17, 1, home_blurb(a->lot), UI_SOFT);
        if (cash + refund < a->price) ui_text(48, y + 28, 1, "NOT ENOUGH MONEY YET", UI_BADT);
        else if (home_city) { sprintf(b, "YOUR HOME NOW SELLS FOR $%d", home_paid); ui_text(48, y + 28, 1, b, UI_SOFT); }
        else ui_text(48, y + 28, 1, "THE MAN BASE STAYS YOURS TOO", UI_SOFT);
    }
    ui_hints(160, 214, "X BUY AND MOVE IN|SQ SEE IT $20|O BACK", 1);
}

#ifdef AUTO_HOME
int auto_btn;
static void home_tick(void) {   // (test builds: buy a house and move in, build a storey on it, go away and come back, then buy an empty lot and build there)
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); cash = 40000; }
    static const struct { s16 at; char op; s8 arg; } plan[] = {
        { 60, 'Q', PH_HOMES }, { 160, 'b', 'X' },                            // the phone: HOMES
        { 300, 'h', 0 }, { 400, 'b', 'X' },                                  // an affordable house: buy it and move in
        { 800, 'k', 1 }, { 900, 'i', 0 }, { 1100, 'k', 0 },                  // outside, then inside: furnished for them
        { 1300, 'B', BT_ROOM }, { 1310, 'l', 1 }, { 1320, 'c', 0 }, { 1360, 'b', 'X' }, { 1400, 'c', 1 }, { 1500, 'b', 'X' },   // a storey over the house: the ROOM tool
        { 1600, 'B', BT_DOOR }, { 1610, 'w', 0 }, { 1640, 'R', 0 }, { 1660, 'b', 'X' },   // a window in it
        { 1750, 'z', 0 }, { 1800, 'B', BT_ROOF }, { 1900, 'b', 'O' },         // the stairs; the roof over it all
        { 2000, 'k', 2 }, { 2300, 'k', 0 },
        { 2380, 'v', 0 },                                                     // from the street: the new storey and its roof
        { 2400, 'f', 0 },                                                     // off across town
        { 2700, 't', 0 }, { 2720, 'b', 'X' },                                 // TRAVEL: HOME
        { 3000, 'v', 0 }, { 3300, 'k', 0 },
        { 3400, 'Q', PH_HOMES }, { 3500, 'b', 'X' },                          // HOMES again
        { 3600, 'h', 1 }, { 3700, 'b', 'X' },                                 // an empty lot this time (the house sells)
        { 4100, 'k', 1 }, { 4200, 'B', BT_ROOM }, { 4210, 'c', 2 }, { 4240, 'b', 'X' }, { 4280, 'c', 3 }, { 4350, 'b', 'X' },   // a room on the lot
        { 4450, 'B', BT_DOOR }, { 4460, 'c', 4 }, { 4480, 'b', 'X' }, { 4560, 'b', 'O' }, { 4600, 'v', 1 },
        { 4900, 'Q', PH_TRAVEL }, { 4960, 'b', 'X' },                         // TRAVEL: home is on the list
        { 5100, 'b', 'O' },
        { 5300, 'g', 0 }, { 5340, 'b', 'X' }, { 5420, 'p', 0 }, { 5440, 'b', 'X' },   // a FOR SALE sign down the road: WHAT IS IT?
        { 5700, 'Q', PH_MAP }, { 5770, 'b', 'X' }, { 5950, 'b', 'O' },       // the map: home is on it
        { 0, 0, 0 } };
    int t = VSync(-1) - t0;
    while (plan[k].op && t >= plan[k].at) {
        int a = plan[k].arg, wx, wy; const Home *hm = 0;
        if (world_own_block(&wx, &wy)) { int hi = world_home_find(ZONE_CITY, home_bx, home_by, 0); if (hi >= 0) hm = world_home(ZONE_CITY, hi); }
        switch (plan[k].op) {
            case 'b': auto_btn = a == 'X' ? PAD_CROSS : a == 'O' ? PAD_CIRCLE : a == 'S' ? PAD_START : a == 'D' ? PAD_DOWN : a == 'R' ? PAD_R1 : a == 'U' ? PAD_UP : PAD_RIGHT; break;
            case 'R': auto_btn = PAD_R1; break;
            case 'Q': phone_open(); ph_sel = a; break;
            case 'h': for (int i = 0; i < hm_n; i++) if ((a ? hm_ad[i].lot == LOT_EMPTY : hm_ad[i].lot != LOT_EMPTY) && hm_ad[i].price <= cash + (home_city ? home_paid : 0)) { hm_sel = i; break; } break;
            case 'k': cam_pitch = a == 2 ? 520 : a ? 620 : 700; cam_dist = a == 2 ? 2600 : a ? 1700 : 1300; cam_yaw = a == 2 ? 2600 + 400 : 2600; break;
            case 'i': if (hm) { Sim *s = &sims[player]; s->x = ((hm->x0 + hm->x1) / 2) * TILE + TILE / 2; s->z = -(((hm->y0 + hm->y1) / 2) * TILE + TILE / 2); s->path_n = 0; s->action = ACT_NONE; } break;
            case 'B': state = ST_BUILD; build_tool = a; room_on = 0; build_msg_t = 0; build_lvl = 0; if (hm) { build_x = hm->dx; build_y = hm->dy; } break;
            case 'l': build_lvl = a; break;
            case 'c': if (a < 2 && hm) { build_x = a ? hm->x1 + 1 : hm->x0 - 1; build_y = a ? hm->y1 + 1 : hm->y0 - 1; }
                      else if (world_own_block(&wx, &wy)) { int ox = wx * BM_BLK + BM_LOT0, oy = wy * BM_BLK + BM_LOT0; build_x = ox + (a == 2 ? 2 : a == 3 ? 9 : 5); build_y = oy + (a == 2 ? 2 : a == 3 ? 8 : 8); } break;
            case 'w': if (hm) { build_x = (hm->x0 + hm->x1) / 2; build_y = hm->y0 - 1; } break;
            case 'z': if (hm) { int sx = hm->x0 + 1, sy = hm->y0 + 2; *world_cell(0, sx, sy) = '|'; *world_cell(1, sx, sy) = ' '; *world_cell(0, sx, sy - 1) = '.'; *world_cell(0, sx, sy + 1) = '.'; world_touched(); } break;
            case 'f': taxi_to_block(home_bx + 5, home_by + 1, -1, -1, "ACROSS TOWN"); break;
            case 't': state = ST_TRAVEL; travel_sel = 0; break;
            case 'g': { extern int sim_debug_goto(char c, int lot); sim_debug_goto('8', -1); cam_pitch = 640; cam_dist = 1500; } break;
            case 'p': for (int i = 0; i < pie_n; i++) if (!strcmp(pie_opt[i], "WHAT IS IT?")) pie_sel = i; break;
            case 'v': { int tx, ty; if (world_own_door(&tx, &ty)) { Sim *s = &sims[player]; s->x = tx * TILE + TILE / 2; s->z = -((ty + (a ? 3 : 5)) * TILE + TILE / 2); s->path_n = 0; s->action = ACT_NONE; }
                        cam_yaw = 0; cam_pitch = 430; cam_dist = 2300; } break;
        }
        k++;
    }
}
#endif


// ---- THE NEIGHBORHOOD (The Sims' neighborhood screen): the blocks round a town from above, and 48 lots picked at random from what
// is for sale there, furnished homes and empty lots to build on. The d-pad hops from lot to lot; X buys one and the family moves in ----
#define NH_MAX 48
static int nh_b = 25, nh_p = 5;                     // the view: 25 x 25 blocks, 5 pixels a block (32 x 32 at 4, when there is less for sale)
#define NH_B nh_b
#define NH_P nh_p
static u16 nh_bx[NH_MAX], nh_by[NH_MAX]; static u8 nh_lot[NH_MAX]; static int nh_price[NH_MAX];
static int nh_n, nh_sel, nh_first, nh_t, nh_x0, nh_y0, nh_set;
static void nhood_pick_in(int cbx, int cby);
static void nhood_pick(int cbx, int cby) {         // 48 lots from what is for sale round there (a wider view if the town is short of them)
    nh_b = 25; nh_p = 5; nhood_pick_in(cbx, cby);
    if (nh_n < NH_MAX) { nh_b = 32; nh_p = 4; nhood_pick_in(cbx, cby); }
}
static void nhood_pick_in(int cbx, int cby) {      // ... about a third empty, the rest furnished
    nh_x0 = CLAMP(cbx - NH_B / 2, 0, BM_N - NH_B); nh_y0 = CLAMP(cby - NH_B / 2, 0, BM_N - NH_B);
    const int want_e = NH_MAX / 3, want_h = NH_MAX - want_e;
    int ne = 0, nh = 0, seen_e = 0, seen_h = 0;   // two reservoirs (a fair draw, with no list of every lot kept)
    static u16 eb[NH_MAX][2], hb[NH_MAX][2];
    for (int by = nh_y0; by < nh_y0 + NH_B; by++) for (int bx = nh_x0; bx < nh_x0 + NH_B; bx++) {
        if (!home_for_sale(bx, by)) continue;
        BInfo b; bm_block(bx, by, &b);
        if (b.lot == LOT_EMPTY) { seen_e++; if (ne < NH_MAX) { eb[ne][0] = (u16)bx; eb[ne][1] = (u16)by; ne++; } else { int j = (int)(look_rand() % (u32)seen_e); if (j < NH_MAX) { eb[j][0] = (u16)bx; eb[j][1] = (u16)by; } } }
        else { seen_h++; if (nh < NH_MAX) { hb[nh][0] = (u16)bx; hb[nh][1] = (u16)by; nh++; } else { int j = (int)(look_rand() % (u32)seen_h); if (j < NH_MAX) { hb[j][0] = (u16)bx; hb[j][1] = (u16)by; } } }
    }
    int te = MIN(ne, want_e), th = MIN(nh, want_h);
    if (te < want_e) th = MIN(nh, NH_MAX - te); else if (th < want_h) te = MIN(ne, NH_MAX - th);
    for (int i = 0; i < ne; i++) { int j = i + (int)(look_rand() % (u32)(ne - i)); u16 a = eb[i][0], b = eb[i][1]; eb[i][0] = eb[j][0]; eb[i][1] = eb[j][1]; eb[j][0] = a; eb[j][1] = b; }   // (a shuffle: which of the sample)
    for (int i = 0; i < nh; i++) { int j = i + (int)(look_rand() % (u32)(nh - i)); u16 a = hb[i][0], b = hb[i][1]; hb[i][0] = hb[j][0]; hb[i][1] = hb[j][1]; hb[j][0] = a; hb[j][1] = b; }
    nh_n = 0;
    for (int i = 0; i < te; i++) { nh_bx[nh_n] = eb[i][0]; nh_by[nh_n] = eb[i][1]; nh_n++; }
    for (int i = 0; i < th; i++) { nh_bx[nh_n] = hb[i][0]; nh_by[nh_n] = hb[i][1]; nh_n++; }
    for (int i = 1; i < nh_n; i++) {               // in reading order (top row first), so L1 and R1 step through them in order
        u16 x = nh_bx[i], y = nh_by[i]; int j = i;
        while (j > 0 && (nh_by[j - 1] > y || (nh_by[j - 1] == y && nh_bx[j - 1] > x))) { nh_bx[j] = nh_bx[j - 1]; nh_by[j] = nh_by[j - 1]; j--; }
        nh_bx[j] = x; nh_by[j] = y;
    }
    for (int i = 0; i < nh_n; i++) { BInfo b; bm_block(nh_bx[i], nh_by[i], &b); nh_lot[i] = b.lot; nh_price[i] = home_price(nh_bx[i], nh_by[i]); }
    nh_sel = 0;
    int bd = 1 << 30;                              // start on the lot nearest the middle
    for (int i = 0; i < nh_n; i++) { int dx = nh_bx[i] - cbx, dy = nh_by[i] - cby, d = dx * dx + dy * dy + (nh_price[i] > cash + (home_city ? home_paid : 0) ? 1 << 20 : 0); if (d < bd) { bd = d; nh_sel = i; } }   // (one you can afford, if there is one)
}
static void nhood_paint(void) {                    // the blocks from above, 5 pixels each: the land, the streets, what stands on every lot
    u16 *px = atlas_scratch();
    static const u16 tcol[6] = { RGB(6, 13, 23), RGB(27, 25, 17), RGB(11, 20, 8), RGB(6, 14, 6), RGB(25, 21, 13), RGB(16, 16, 15) };
    for (int by = 0; by < NH_B; by++) for (int bx = 0; bx < NH_B; bx++) {
        BInfo b; bm_block(nh_x0 + bx, nh_y0 + by, &b);
        u16 c = tcol[b.terrain % 6];
        int l = b.lot;
        if (b.terrain == BT_SEA) l = LOT_WILD;
        if (l == LOT_HOUSE || l == LOT_COTTAGE || l == LOT_TWO || l == LOT_TOWN || l == LOT_APART) c = RGB(22, 11, 9);
        else if (l == LOT_PARK || l == LOT_SKATE) c = RGB(9, 22, 9);
        else if (l == LOT_SQUARE) c = RGB(24, 23, 20);
        else if (l == LOT_EMPTY) c = RGB(22, 19, 11);
        else if (l == LOT_FARM) c = RGB(20, 22, 8);
        else if (l != LOT_WILD && bm_is_place(l)) c = RGB(13, 16, 22);
        for (int y = 0; y < NH_P; y++) for (int x = 0; x < NH_P; x++) {
            u16 v = c;
            if (b.terrain != BT_SEA && ((y == 0 && (b.road & 1)) || (x == 0 && (b.road & 2)) || (x == 0 && y == 0 && (b.road & 4)))) v = RGB(10, 10, 11);   // the streets
            else if (l != LOT_WILD && l != LOT_PARK && l != LOT_EMPTY && b.terrain != BT_SEA && x >= 2 && x <= 3 && y >= 2 && y <= 3) v = (u16)((c & 0x7bde) >> 1);   // (a roof's middle: darker)
            px[(by * NH_P + y) * 128 + bx * NH_P + x] = v | 0x8000;
        }
    }
    atlas_vram_load(0, 256, 128, 128, px);
    crowd_flush();
}
static void nhood_open(int first) {
    int cbx, cby;
    if (!first && world_zone == ZONE_CITY && world_wbx >= 0) { cbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK; cby = world_wby + sim_tile_y(&sims[player]) / BM_BLK; }
    else { const Settle *cp = bm_set(bm_capital()); cbx = cp->x; cby = cp->y; }
    nh_set = bm_set_at(cbx, cby); if (nh_set == 255) nh_set = bm_capital();
    nhood_pick(cbx, cby);
    nh_first = first; nh_t = -1; state = ST_NHOOD;
}
static void nhood_hop(int dx, int dy) {            // the nearest lot that way (the d-pad)
    int best = -1, bs = 1 << 30;
    for (int i = 0; i < nh_n; i++) {
        if (i == nh_sel) continue;
        int ex = nh_bx[i] - nh_bx[nh_sel], ey = nh_by[i] - nh_by[nh_sel];
        int along = ex * dx + ey * dy, side = ABS(ex * dy - ey * dx);
        if (along <= 0 || side > along * 2) continue;
        int sc = along * along + side * side * 3;
        if (sc < bs) { bs = sc; best = i; }
    }
    if (best >= 0) { nh_sel = best; audio_voice(VOICE_FINISHED); }
}
static int nh_buy_sel;
static void nhood_buy_yes(void) { if (home_buy(nh_bx[nh_buy_sel], nh_by[nh_buy_sel])) state = ST_LIFE; }
static void nhood_update(void) {
    if (nh_t < 0) nhood_paint();
    nh_t++;
    if (PRESSED(PAD_CIRCLE) && !nh_first) { state = BACK_TO(); return; }
    if (!nh_n) { if (PRESSED(PAD_CROSS) || PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE)) state = ST_LIFE; return; }
    if (PRESSED(PAD_LEFT)) nhood_hop(-1, 0);
    if (PRESSED(PAD_RIGHT)) nhood_hop(1, 0);
    if (PRESSED(PAD_UP)) nhood_hop(0, -1);
    if (PRESSED(PAD_DOWN)) nhood_hop(0, 1);
    if (PRESSED(PAD_L1)) nh_sel = (nh_sel + nh_n - 1) % nh_n;
    if (PRESSED(PAD_R1)) nh_sel = (nh_sel + 1) % nh_n;
    if (nh_first && PRESSED(PAD_TRIANGLE)) { sim_note("THE MAN BASE IT IS. HOME SWEET HOME"); state = ST_LIFE; return; }   // (stay in the old place)
    if (!nh_first && PRESSED(PAD_SQUARE)) {        // go and see it first
        if (cash < 20) { audio_voice(VOICE_NAH); return; }
        cash -= 20; char ad[24]; home_address(nh_bx[nh_sel], nh_by[nh_sel], ad); taxi_to_block(nh_bx[nh_sel], nh_by[nh_sel], -1, -1, ad); state = ST_LIFE; return;
    }
    if (!nh_first && PRESSED(PAD_SELECT)) { homes_open(); return; }   // (the plain list)
    if (PRESSED(PAD_CROSS)) {
        if (!home_for_sale(nh_bx[nh_sel], nh_by[nh_sel])) { sim_note("TOO LATE, THAT ONE IS SOLD"); audio_voice(VOICE_NAH); return; }
        if (home_city && !nh_first) {   // you have a home already: what moving costs, and what stays behind, first
            if (nh_bx[nh_sel] == home_bx && nh_by[nh_sel] == home_by) { sim_note("THAT ONE IS YOUR HOME ALREADY"); return; }
            char ad[24], old[24]; home_address(nh_bx[nh_sel], nh_by[nh_sel], ad); home_address(home_bx, home_by, old);
            int net = nh_price[nh_sel] - home_paid; char b[160];
            if (cash + home_paid < nh_price[nh_sel]) { sprintf(b, "NOT ENOUGH MONEY: IT IS $%d, AND YOURS SELLS FOR $%d", nh_price[nh_sel], home_paid); sim_note(b); audio_voice(VOICE_NAH); return; }
            snprintf(b, sizeof b, "MOVE TO %s? %s SELLS FOR $%d. ANYTHING YOU BUILT OR BOUGHT THERE STAYS WITH THE HOUSE. %s $%d.", ad, old, home_paid, net >= 0 ? "AFTER THE SALE YOU PAY" : "AFTER THE SALE YOU GET", net >= 0 ? net : -net);
            nh_buy_sel = nh_sel; confirm_open("MOVE HOUSE?", b, nhood_buy_yes); return;
        }
        if (home_buy(nh_bx[nh_sel], nh_by[nh_sel])) state = ST_LIFE;   // (life_update moves the family in)
    }
}
static void nhood_draw(void) {
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    char b[48], ad[24];
    const Settle *ns = bm_set(nh_set);
    snprintf(b, sizeof b, "THE NEIGHBORHOOD: %s", ns ? ns->name : "SAN BOREAS");
    int top = ui_window(4, 4, 312, 232, b, IC_HOUSE);
    const int MX = 10, MY = top + 1, MS = 182;
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    if (nh_t >= 0) gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, NH_B * NH_P, NH_B * NH_P, 128, 128, 128);
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < nh_n; i++) {   // every lot a dot: green a furnished home, gold an empty lot (the picked one last, on top)
        if ((i == nh_sel) != pass) continue;
        int x = MX + ((nh_bx[i] - nh_x0) * NH_P + 3) * MS / (NH_B * NH_P), y = MY + ((nh_by[i] - nh_y0) * NH_P + 3) * MS / (NH_B * NH_P);
        int empty = nh_lot[i] == LOT_EMPTY, sold = !home_for_sale(nh_bx[i], nh_by[i]);
        u32 c = sold ? UI_RGB(150, 150, 150) : empty ? UI_RGB(236, 176, 40) : UI_RGB(60, 190, 90);
        if (i == nh_sel) { ui_disc(x, y, (nh_t >> 3) & 1 ? 17 : 15, UI_WHITE); ui_icon_disc(x, y, 13, empty ? IC_SPROUT : IC_HOUSE, UI_BLUE, UI_WHITE); }
        else { ui_disc(x, y, 8, UI_WHITE); ui_disc(x, y, 6, c); }
    }
    if (home_city && home_bx >= nh_x0 && home_by >= nh_y0 && home_bx < nh_x0 + NH_B && home_by < nh_y0 + NH_B) {   // yours
        int x = MX + ((home_bx - nh_x0) * NH_P + 3) * MS / (NH_B * NH_P), y = MY + ((home_by - nh_y0) * NH_P + 3) * MS / (NH_B * NH_P);
        ui_disc(x, y, 13, UI_WHITE); ui_icon_disc(x, y, 11, IC_HOUSE, UI_GREEN, UI_WHITE);
    }
    const int PX = 198, PW = 110;                   // the picked lot, on the right
    if (!nh_n) { ui_wrap(PX, top + 10, PW, 4, "NO HOMES FOR SALE ROUND HERE RIGHT NOW", UI_SOFT); ui_hints(310, 215, nh_first ? "X STAY AT THE MAN BASE" : "O BACK", 2); return; }
    int i = nh_sel, empty = nh_lot[i] == LOT_EMPTY, refund = home_city ? home_paid : 0;
    sprintf(b, "LOT %d OF %d", i + 1, nh_n); ui_text(PX, top + 1, 1, b, UI_SOFT);
    ui_round(PX - 2, top + 13, PW + 4, 50, 6, UI_RGB(234, 240, 248), UI_RGB(224, 231, 240), 0);
    ui_icon_disc(PX + 12, top + 27, 22, empty ? IC_SPROUT : IC_HOUSE, empty ? UI_RGB(220, 150, 30) : UI_GREEN, UI_WHITE);
    ui_textfit(PX + 26, top + 18, PW - 26, home_kind(nh_lot[i]), UI_INK);
    ui_textfit(PX + 26, top + 29, PW - 26, empty ? "BUILD ON IT" : "FURNISHED", empty ? UI_GOLDT : UI_GOODT);
    home_address(nh_bx[i], nh_by[i], ad); ui_textfit(PX + 2, top + 44, PW - 2, ad, UI_BLUE2);
    { BInfo bi; bm_block(nh_bx[i], nh_by[i], &bi); const Settle *st = bi.set != 255 ? bm_set(bi.set) : 0; ui_textfit(PX + 2, top + 54, PW - 2, st ? st->name : "OPEN COUNTRY", UI_SOFT); }
    int can = cash + refund >= nh_price[i], sold = !home_for_sale(nh_bx[i], nh_by[i]);
    sprintf(b, "$%d", nh_price[i]); ui_text(PX, top + 70, 2, b, sold ? UI_SOFT : can ? UI_GOODT : UI_BADT);
    ui_text(PX, top + 88, 1, sold ? "SOLD!" : can ? "YOU CAN AFFORD IT" : "NOT ENOUGH MONEY", sold || !can ? UI_BADT : UI_SOFT);
    sprintf(b, "YOU HAVE $%ld", cash); ui_textfit(PX, top + 100, PW, b, UI_INK);
    if (home_city && !nh_first) { sprintf(b, "YOURS: $%d", home_paid); ui_textfit(PX, top + 111, PW, b, UI_SOFT); }
    {   // the key
        int ky = top + 128;
        ui_disc(PX + 4, ky + 3, 7, UI_RGB(60, 190, 90)); ui_text(PX + 12, ky, 1, "HOME", UI_INK);
        ui_disc(PX + 54, ky + 3, 7, UI_RGB(236, 176, 40)); ui_text(PX + 62, ky, 1, "EMPTY", UI_INK);
    }
    gfx_rect(10, 211, 300, 1, 200, 210, 222, 0);
    ui_hints(160, 215, nh_first ? "DPAD PICK|X MOVE IN|TRI THE MAN BASE" : "DPAD PICK|X BUY|SQ SEE $20|SEL LIST|O BACK", 1);
}

// ---- CRAFT: what the castaways can make at the campfire from what they have gathered ----
static void craft_update(void) {
    if (PRESSED(PAD_UP)) { craft_sel = (craft_sel + ISL_NCRAFT - 1) % ISL_NCRAFT; craft_msg_t = 0; }
    if (PRESSED(PAD_DOWN)) { craft_sel = (craft_sel + 1) % ISL_NCRAFT; craft_msg_t = 0; }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = ST_LIFE; return; }
    if (craft_msg_t > 0) craft_msg_t--;
    if (PRESSED(PAD_CROSS)) {
        const Craft *c = &isl_crafts[craft_sel];
        int r = isl_make(craft_sel);
        if (r > 0) { state = ST_LIFE; return; }   // (made: its note is in the feed)
        audio_voice(VOICE_NAH); craft_msg_t = 150;
        if (r < 0) strcpy(craft_msg, "NO ROOM FOR IT BY THE FIRE");
        else if ((craft_sel == 3 && isl_spear) || (craft_sel == 5 && world_count(CH_RAFT))) strcpy(craft_msg, "YOU HAVE ONE ALREADY");
        else if (isl_wood < c->wood) sprintf(craft_msg, "NEED %d MORE WOOD", c->wood - isl_wood);
        else if (isl_stone < c->stone) sprintf(craft_msg, "NEED %d MORE STONE", c->stone - isl_stone);
        else sprintf(craft_msg, "NEED %d MORE LEAVES", c->leaves - isl_leaves);
    }
}
static void craft_draw(void) {
    life_draw();
    ui_dim();
    char b[32];
    int top = ui_window(20, 10, 280, 222, "MAKE SOMETHING", IC_HAMMER);
    {   // what there is to make it from: four boxes, each its picture, how many, and its name
        static const s8 ic[4] = { IC_WOOD, IC_STONE, IC_SPROUT, IC_FOOD }; static const char *const nm[4] = { "WOOD", "STONE", "LEAVES", "FOOD" };
        const int v[4] = { isl_wood, isl_stone, isl_leaves, isl_food };
        for (int k = 0; k < 4; k++) {
            int x = 26 + k * 68;
            ui_round(x, top, 64, 26, 6, UI_WHITE, UI_PANEL, 0);
            ui_icon_disc(x + 13, top + 13, 18, ic[k], v[k] ? UI_BLUE : UI_RGB(176, 184, 196), UI_WHITE);
            sprintf(b, "%d", v[k]); ui_text(x + 26, top + 4, 1, b, UI_INK); ui_text(x + 26, top + 14, 1, nm[k], UI_SOFT);
        }
    }
    for (int i = 0; i < ISL_NCRAFT; i++) {
        const Craft *c = &isl_crafts[i]; int y = top + 34 + i * 20, sel = i == craft_sel, ok = isl_can_make(i);
        if (sel) { ui_round(26, y - 3, 268, 19, 6, UI_RGB(226, 242, 254), UI_HILITE, 0); gfx_rect(26, y, 2, 13, 38, 146, 222, 0); }
        ui_icon_disc(40, y + 6, 16, c->icon, ok ? UI_GREEN : UI_RGB(176, 184, 196), UI_WHITE);
        ui_text(54, y + 3, 1, c->name, ok ? (sel ? UI_BLUE2 : UI_INK) : UI_SOFT);
        int x = 150; const int need[3] = { c->wood, c->stone, c->leaves }, have[3] = { isl_wood, isl_stone, isl_leaves }; static const s8 nic[3] = { IC_WOOD, IC_STONE, IC_SPROUT };
        for (int k = 0; k < 3; k++) if (need[k]) { ui_icon(x, y + 1, nic[k], have[k] >= need[k] ? UI_GOODT : UI_BADT); sprintf(b, "%d", need[k]); ui_text(x + 14, y + 3, 1, b, have[k] >= need[k] ? UI_GOODT : UI_BADT); x += 36; }
        if ((i == 3 && isl_spear) || (i == 5 && world_count(CH_RAFT))) ui_text(150, y + 3, 1, "MADE", UI_GOODT);
    }
    ui_round(26, top + 155, 268, 24, 6, UI_RGB(234, 240, 248), UI_RGB(224, 231, 240), 0);
    if (craft_msg_t > 0) { ui_icon(32, top + 161, IC_LOCK, UI_BADT); ui_text(48, top + 163, 1, craft_msg, UI_BADT); }   // (why it could not be made)
    else { ui_icon(32, top + 161, IC_HELP, UI_BLUE); ui_text(48, top + 163, 1, isl_crafts[craft_sel].blurb, UI_INK); }
    ui_hints(160, top + 184, "X MAKE IT|O BACK", 1);
}

#ifdef AUTO_ISLE
int auto_btn;
static void isle_tick(void) {   // (test builds: CASTAWAY ISLAND: its map, gathering, the campfire's crafts, every secret, the raft to the islet, foraging, the story)
    extern int sim_debug_goto(char c, int lot);
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); story_paid = 1; clock_min = 9 * 60; travel_go(ZONE_ISLAND); story_visit[1] = 1; }
    static const struct { s16 at; char op; s16 arg; } plan[] = {
        { 30, 'm', 0 }, { 200, 'b', 'O' }, { 260, 'k', 0 },                                       // the island's map: no secrets yet, the hints
        { 300, 'g', (s16)(u8)CH_BUSH }, { 330, 'b', 'X' }, { 380, 'b', 'X' },                       // berries
        { 650, 'g', 'g' }, { 680, 'b', 'X' }, { 730, 'p', 1 }, { 750, 'b', 'X' },                 // a palm: CHOP WOOD
        { 1100, 'g', (s16)(u8)CH_FISH }, { 1130, 'b', 'X' },                                      // fishing
        { 1600, 'g', 's' }, { 1630, 'b', 'X' }, { 1670, 'p', 2 }, { 1690, 'b', 'X' },             // the campfire: CRAFT
        { 1750, 'c', 4 }, { 1780, 'b', 'X' },                                                     // a SHELTER: not enough yet
        { 1900, 'r', 0 }, { 1920, 'b', 'X' },                                                     // now there is: made, by the fire
        { 2000, 'g', 'D' }, { 2010, 'k', 2 }, { 2250, 'k', 0 },                                   // the shelter
        { 2300, 'g', 's' }, { 2330, 'b', 'X' }, { 2370, 'p', 2 }, { 2390, 'b', 'X' }, { 2440, 'c', 5 }, { 2470, 'b', 'X' },   // a RAFT
        { 2600, 'g', (s16)(u8)CH_BOTTLE }, { 2630, 'b', 'X' },                                    // the secrets
        { 2900, 'g', (s16)(u8)CH_DIG }, { 2930, 'b', 'X' },
        { 3350, 'g', (s16)(u8)CH_IDOL }, { 3380, 'b', 'X' },
        { 3650, 'g', (s16)(u8)CH_WRECK }, { 3680, 'b', 'X' },
        { 4050, 'g', (s16)(u8)CH_SHELL }, { 4080, 'b', 'X' }, { 4130, 'g', (s16)(u8)CH_SHELL }, { 4160, 'b', 'X' }, { 4210, 'g', (s16)(u8)CH_SHELL }, { 4240, 'b', 'X' },
        { 4290, 'g', (s16)(u8)CH_SHELL }, { 4320, 'b', 'X' }, { 4370, 'g', (s16)(u8)CH_SHELL }, { 4400, 'b', 'X' }, { 4450, 'g', (s16)(u8)CH_SHELL }, { 4480, 'b', 'X' },
        { 4550, 'g', (s16)(u8)CH_JOURNAL }, { 4580, 'b', 'X' },
        { 4850, 'm', 0 }, { 5000, 'b', 'O' },                                                     // the map again: what is found
        { 5050, 'g', (s16)(u8)CH_RAFT }, { 5080, 'b', 'X' }, { 5120, 'p', 3 }, { 5140, 'b', 'X' },   // the raft: out to the islet
        { 5300, 'k', 1 },
        { 5450, 'g', (s16)(u8)CH_GOLD }, { 5480, 'b', 'X' },                                      // the golden idol
        { 5750, 'g', (s16)(u8)CH_RAFT }, { 5780, 'b', 'X' }, { 5820, 'p', 3 }, { 5840, 'b', 'X' },   // and back
        { 5950, 'f', 0 }, { 5960, 'k', 2 },                                                       // an empty larder: the others forage
        { 6600, 'm', 0 }, { 6750, 'b', 'O' },
        { 6800, 'Q', PH_GOALS }, { 6870, 'b', 'X' },                                            // the phone: GOALS
        { 7050, 'b', 'O' }, { 7080, 'h', 5 }, { 7250, 'b', 'O' }, { 0, 0, 0 } };                  // and the island's help page
    static const char *const pick[4] = { "", "CHOP WOOD", "CRAFT", "SAIL ACROSS" };
    int t = VSync(-1) - t0;
    while (plan[k].op && t >= plan[k].at) {
        int a = plan[k].arg;
        switch (plan[k].op) {
            case 'b': auto_btn = a == 'X' ? PAD_CROSS : a == 'O' ? PAD_CIRCLE : a == 'S' ? PAD_START : a == 'D' ? PAD_DOWN : a == 'U' ? PAD_UP : PAD_RIGHT; break;
            case 'g': sim_debug_goto((char)a, -1); cam_pitch = 640; cam_dist = 1400; break;
            case 'p': for (int i = 0; i < pie_n; i++) if (!strcmp(pie_opt[i], pick[a])) pie_sel = i; break;
            case 'c': craft_sel = a; break;
            case 'Q': phone_open(); ph_sel = a; break;
            case 'm': map_t = -1; state = ST_MAP; break;
            case 'h': prev_state = ST_PAUSE; help_page = a; state = ST_HELP; break;
            case 'k': cam_pitch = a == 2 ? 760 : a ? 520 : 640; cam_dist = a == 2 ? 3000 : a ? 2200 : 1400; break;
            case 'r': isl_wood += 30; isl_stone += 6; isl_leaves += 20; break;
            case 'f': isl_food = 0; for (int i = 0; i < nsims; i++) if (i != player) sims[i].need[N_FOOD] = 12 << 10; break;
        }
        k++;
    }
}
#endif

#ifdef AUTO_PHONE
int auto_btn, auto_hold;
static void phone_tick(void) {   // (test builds: the HUD, the phone and its apps, a call, the pause menu, THE MAN BASE and the desert town to the horizon)
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); }
    static const struct { s16 at; char op; s16 arg; } plan[] = {
        { 10, 'c', 0 }, { 40, 'H', 2 }, { 240, 'H', 1 }, { 440, 'H', 4 }, { 640, 'H', 3 }, { 840, 'H', 5 }, { 1040, 'H', 6 }, { 1240, 'H', 0 },   // the walk: toward the camera, away, right, left, diagonals
        { 1310, 'k', 0 },
        { 1500, 'b', 'E' }, { 1560, 'b', 'R' }, { 1600, 'b', 'R' }, { 1640, 'b', 'D' }, { 1680, 'b', 'D' }, { 1720, 'b', 'X' },   // the phone; FAMILY
        { 1780, 'b', 'D' }, { 1840, 'b', 'O' }, { 1880, 'b', 'L' }, { 1920, 'b', 'U' }, { 1980, 'b', 'O' },
        { 2060, 'b', 'E' }, { 2100, 'b', 'X' },                                                      // CALL: on the phone where you stand
        { 2400, 'b', 'O' }, { 2430, 'b', 'O' }, { 2460, 'b', 'S' }, { 2550, 'b', 'O' },            // back to the phone, closed; the pause menu
        { 2600, 'k', 1 }, { 2800, 'k', 2 },                                                        // THE MAN BASE: fields to the horizon
        { 3000, 't', ZONE_TOWN }, { 3020, 'k', 1 }, { 3200, 'k', 2 },                              // the desert town
        { 3400, 't', ZONE_CITY }, { 3440, 'k', 0 },
        { 3600, 'b', 'E' }, { 3630, 'b', 'R' }, { 3660, 'b', 'R' }, { 3690, 'b', 'R' }, { 3720, 'b', 'X' }, { 3900, 'b', 'O' }, { 4000, 'b', 'O' },   // the phone: MAP
        { 4100, 'b', 'E' }, { 4130, 'b', 'R' }, { 4160, 'b', 'X' },                              // PIZZA
        { 4400, 't', ZONE_HOME }, { 4500, 'g', 'S' }, { 4530, 'b', 'X' }, { 4560, 'b', 'X' }, { 4590, 'b', 'X' }, { 4620, 'K', 0 }, { 4700, 'K', 1 },   // to bed: it lies in it the right way round
        { 4800, 'b', 'O' }, { 4850, 'g', 'C' }, { 4880, 'b', 'X' }, { 4910, 'b', 'X' }, { 4950, 'K', 0 }, { 5050, 'K', 1 },       // a seat
        { 5150, 'b', 'O' }, { 5200, 'g', 'T' }, { 5230, 'b', 'X' }, { 5260, 'b', 'X' }, { 5300, 'K', 0 }, { 5400, 'K', 1 },   // the toilet
        { 5500, 'b', 'O' }, { 5550, 'P', 0 }, { 5650, 'P', 1 }, { 5750, 'P', 2 }, { 5850, 'P', 3 }, { 5950, 'P', 4 }, { 6050, 'P', 5 },
        { 6150, 'P', 0 }, { 6170, 'V', 2 }, { 6200, 'H', 1 }, { 6500, 'H', 4 }, { 6600, 'H', 1 }, { 6850, 'H', 0 }, { 6880, 'F', -400 }, { 6960, 'F', 300 },   // FIRST PERSON: walk through the house, look down, up
        { 7000, 't', ZONE_CITY }, { 7040, 'V', 2 }, { 7100, 'H', 1 }, { 7500, 'H', 0 }, { 7550, 'V', 1 }, { 7600, 'H', 4 }, { 7800, 'H', 1 }, { 8100, 'H', 0 }, { 8200, 'V', 0 }, { 0, 0, 0 } };   // the city, then the camera BEHIND YOU   // poses, from the side: standing, sitting, lying, from above, talking, waving
    int t = VSync(-1) - t0;
    while (plan[k].op && t >= plan[k].at) {
        int a = plan[k].arg;
        switch (plan[k].op) {
            case 'b': auto_btn = a == 'X' ? PAD_CROSS : a == 'O' ? PAD_CIRCLE : a == 'S' ? PAD_START : a == 'D' ? PAD_DOWN : a == 'U' ? PAD_UP : a == 'L' ? PAD_LEFT : a == 'E' ? PAD_SELECT : PAD_RIGHT; break;
            case 'k': cam_pitch = a == 2 ? 300 : a ? 520 : 640; cam_dist = a == 2 ? 3600 : a ? 3200 : 1500; cam_yaw = a == 2 ? 1400 : 2600; break;
            case 't': story_paid = 1; job_level = 2; travel_go(a); break;
            case 'c': cam_yaw = 0; cam_pitch = 420; cam_dist = 1300; break;
            case 'g': { extern int sim_debug_goto(char c, int lot); sim_debug_goto((char)a, -1); } break;
            case 'K': cam_pitch = 600; cam_dist = 1100; cam_yaw = a ? 1024 : 0; break;
            case 'P': { Sim *me = &sims[player];
                        if (!a) { me->x = 55 * TILE + TILE / 2; me->z = -(55 * TILE + TILE / 2); me->lvl = 0; me->path_n = 0; }
                        me->action = (a == 2 || a == 3) ? ACT_SLEEP : a == 1 ? ACT_SIT : a == 4 ? ACT_TALK : ACT_NONE; me->act_t = 9999; me->yaw = 1024;
                        me->gait.sit = a == 1; me->gait.sleep = a == 2 || a == 3; me->gait.amp = 0; me->gait.talk = a == 4; me->gait.wave = a == 5 ? 20 : 0;
                        cam_pitch = a == 3 ? 900 : 200; cam_dist = 800; cam_yaw = 0; } break;
            case 'V': { extern void cam_view_set(int v); cam_view_set(a); } break;
            case 'F': fp_pitch = a; break;
            case 'H': { extern int auto_hold; static const u16 hm[7] = { 0, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_UP | PAD_RIGHT, PAD_DOWN | PAD_LEFT }; auto_hold = hm[a]; cam_yaw = 0; } break;
        }
        k++;
    }
}
#endif

#ifdef AUTO_SCENE
int auto_btn;
static void scene_tick(void) {   // (test builds: CASTAWAY ISLAND's sights, each from four angles: the camp, a palm, the jungle, the pines and rocks, a bush, the pond, a hut, the islet)
    extern int sim_debug_goto(char c, int lot);
    static int t0 = -1, n;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); clock_min = 10 * 60; travel_go(ZONE_ISLAND); }
    static const char where[9] = { 's', 'g', '3', '6', '*', (char)CH_BUSH, '0', 'D', (char)CH_GOLD };
    static const s16 view[4][3] = { { 640, 1500, 2600 }, { 380, 1900, 3300 }, { 800, 3400, 600 }, { 520, 2600, 1800 } };
    if (n < 36 && VSync(-1) - t0 >= 60 + n * 60) {
        int stop = n >> 2, k = n & 3;
        if (k == 0) sim_debug_goto(where[stop], -1);
        cam_pitch = view[k][0]; cam_dist = view[k][1]; cam_yaw = view[k][2]; clock_min = 10 * 60;
        n++;
    }
}
#endif

// ---- JOBS: your job (and a free ride to it), and the jobs going at real places near you ----
static const s8 carIc[CAR_N] = { IC_HAMMER, IC_FOOD, IC_MONEY, IC_BOLT, IC_DROP, IC_BED, IC_PLAY, IC_CASE, IC_BOOK, IC_NOTE, IC_HEART, IC_CAMERA, IC_SMILE, IC_BRUSH };
static void jobs_open(void) {
    int rbx, rby;
    if (world_zone == ZONE_CITY && world_wbx >= 0) { rbx = world_wbx + sim_tile_x(&sims[player]) / BM_BLK; rby = world_wby + sim_tile_y(&sims[player]) / BM_BLK; }
    else { const Settle *cp = bm_set(bm_capital()); rbx = cp->x; rby = cp->y; }
    jobs_n = job_openings(rbx, rby, jobs_car, jobs_pl, 16);
    if (!jobs_board && phone_tier < 2 && jobs_n > 3) {   // TODAY'S CLASSIFIEDS: three of them, a different three each day (the JOB BOARD has them all)
        int a = (int)((day_count * 7 + day_count / 5) % jobs_n); s8 c[3]; s16 p[3];
        for (int k = 0; k < 3; k++) { int i = (a + k * jobs_n / 3) % jobs_n; c[k] = (s8)jobs_car[i]; p[k] = (s16)jobs_pl[i]; }
        for (int k = 0; k < 3; k++) { jobs_car[k] = c[k]; jobs_pl[k] = p[k]; }
        jobs_n = 3;
    }
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
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = BACK_TO(); return; }
    if (!PRESSED(PAD_CROSS)) return;
    if (jobs_sel == 0 && job_none) { sim_note("NO JOB YET: PICK ONE FROM THE LIST"); audio_voice(VOICE_NAH); return; }
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
    int top = ui_window(4, 4, 312, 232, jobs_board || phone_tier >= 2 ? "THE JOB BOARD" : "TODAY'S CLASSIFIEDS", IC_CASE);
    sprintf(b, "$%ld", cash); ui_badge(306 - ui_tw(1, b) - 10, 8, b, UI_WHITE, UI_GOODT);
    {   // your job, on its own card, with the button to go
        int on = jobs_sel == 0, len = job_shift_len();
        ui_round(10, top, 300, 55, 6, on ? UI_RGB(226, 242, 254) : UI_WHITE, on ? UI_HILITE : UI_PANEL, 0);
        if (on) gfx_rect(10, top + 4, 2, 47, 38, 146, 222, 0);
        if (job_none) {   // no job yet: what to do about it
            ui_icon_disc(32, top + 19, 30, IC_CASE, UI_RGB(150, 156, 170), UI_WHITE);
            ui_text(54, top + 3, 2, "NO JOB YET", UI_INK);
            ui_wrap(54, top + 20, 248, 3, "NO PAY, BUT NO BILLS EITHER. PICK ONE BELOW AND X TAKES IT. NEW ONES EVERY MORNING.", UI_SOFT);
        } else {
        ui_icon_disc(32, top + 19, 30, carIc[job_car], UI_BLUE, UI_WHITE);
        ui_text(54, top + 3, 2, job_title(), UI_INK);
        sprintf(b, "LEVEL %d OF 5", job_level); ui_badge(304 - ui_tw(1, b) - 10, top + 4, b, UI_BLUE, UI_WHITE);
        sprintf(b, "%s - %s", careers[job_car].name, job_place); ui_text(54, top + 18, 1, b, UI_SOFT);
        shift_text(t, job_car); sprintf(b, "%s   $%d A SHIFT", t, job_pay()); ui_text(54, top + 28, 1, b, UI_INK);
        ui_text(54, top + 42, 1, "TODAY", UI_SOFT); ui_bar(90, top + 42, 108, 7, MIN(100, job_worked * 100 / len), UI_GREEN);
        ui_button(208, top + 38, 96, 14, "GO TO WORK", IC_CAR, on);
        } }
    ui_section(12, top + 59, 296, jobs_board || phone_tier >= 2 ? "EVERY JOB GOING NEAR YOU" : "IN TODAY'S PAPER. A SMARTPHONE SHOWS THEM ALL");
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
    ui_hints(160, 217, jobs_sel ? "DPAD PICK|X TAKE THIS JOB|O BACK" : job_none ? "DPAD PICK A JOB|O BACK" : "DPAD PICK|X GO TO WORK|O BACK", 1);
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
        { 2800, 'c', -1 }, { 2810, 'g', -1 }, { 2840, 'b', 'X' }, { 2880, 'p', 4 }, { 2910, 'b', 'X' }, { 2935, 'b', 'O' },   // a crib: PUT TO BED
        { 3060, 'y', -1 }, { 3065, 'g', -1 }, { 3080, 'k', 2 }, { 3170, 'k', 1 }, { 3250, 'k', 0 },   // (close up, from the side and from above: the baby in its crib)
        { 3300, 's', 0 },                                 // school, TUESDAY 8 AM
        { 3600, 'e', 0 },                                 // home at 3
        { 3800, 'w', 2 }, { 3830, 'b', 'S' }, { 3860, 'b', 'D' }, { 3890, 'b', 'X' }, { 3930, 'b', '<' }, { 4050, 'b', '<' }, { 4170, 'b', 'S' },   // the teen's panel: SCHOOL, PEOPLE
        { 4250, 'w', 0 }, { 4280, 'b', 'S' }, { 4310, 'b', 'D' }, { 4330, 'b', 'D' }, { 4350, 'b', 'D' }, { 4370, 'b', 'D' }, { 4390, 'b', 'D' }, { 4420, 'b', 'X' },
        { 4450, 'b', 'R' }, { 4470, 'b', 'R' }, { 4490, 'b', 'R' }, { 4510, 'b', 'R' }, { 4640, 'b', 'O' }, { 4670, 'b', 'O' },   // OPTIONS: LIFE
        { 4800, 'o', 1 },                                 // MOSS: the end of a long life (at midnight)
        { 5200, 'f', (s8)0x83 }, { 5230, 'b', 'X' },     // the gravestone: REMEMBER (its only choice: no pie)
        { 5600, 'b', 'S' }, { 5630, 'b', 'D' }, { 5650, 'b', 'D' }, { 5670, 'b', 'D' }, { 5690, 'b', 'D' }, { 5710, 'b', 'D' }, { 5730, 'b', 'D' }, { 5760, 'b', 'X' },
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
       O_AGING, O_OLDAGE, O_TOWNAGE, O_CAMH, O_CAMSPD, O_ZOOMTILT, O_MUSIC, O_CUTAWAY, O_ALONE, O_GHOSTS, O_AGGRO, O_AGWHO, O_RAGE, O_RAGEWHO, O_PEACE, O_WIDE, O_TRAF, O_GOTCHI, O_N };
static const char *const optNm[O_N] = { "PRESET", "VIEW DISTANCE", "FULL DETAIL", "FAR BUILDINGS", "FAR TREES", "HAZE", "CROWD SIZE",
    "PEOPLE IN FULL 3D", "PEOPLE SPRITES", "SHADOWS", "FLOOR SHADING", "WALL LIGHT", "FRAME RATE", "DITHERING", "CAMERA", "SHOW FPS",
    "LIFESPAN", "OLD AGE", "THE TOWN GROWS UP", "CAMERA HEIGHT", "TURN SPEED", "ZOOM TILTS", "MUSIC", "WALL VIEW", "YOUR BORE ALONE", "GHOSTS", "AGGRESSION", "WHO JOINS FIGHTS", "RAGE", "WHO LUNGES AT YOU", "PEACEMAKERS", "SCREEN SHAPE", "TRAFFIC CHEAT", "BOREAGOTCHI TIME" };
static const char *const optV[O_N][6] = {
    { "CUSTOM", "LOW", "MEDIUM", "HIGH", "ULTRA" }, { "50 M", "250 M", "500 M", "1 KM", "1.5 KM", "2 KM" },
    { "10 M", "12 M", "15 M", "17 M" }, { "TALL ONLY", "ALL", "WITH WINDOWS" }, { "OFF", "100 M", "200 M", "350 M" },
    { "OFF", "LIGHT", "THICK" }, { "16", "32", "48", "64" }, { "1", "2", "4", "6" }, { "SLOW", "NORMAL", "FAST" },
    { "OFF", "PEOPLE", "EVERYTHING" }, { "OFF", "ON" }, { "FLAT", "SOFT" }, { "AS FAST AS IT CAN", "STEADY 30", "STEADY 20" },
    { "OFF", "ON" }, { "THE SIMS", "BEHIND YOU", "FIRST PERSON" }, { "OFF", "ON" },
    { "NOBODY AGES", "SHORT", "NORMAL", "LONG" }, { "LIVE ON", "PASS ON" }, { "OFF", "ON" },
    { "LOW", "MEDIUM", "HIGH", "OVERHEAD" }, { "SLOW", "NORMAL", "FAST" }, { "OFF", "ON" }, { "OFF", "LOW", "MEDIUM", "FULL" },
    { "AUTO", "CUT AWAY", "FULL WALLS" }, { "OFF", "LOW", "HIGH" }, { "OFF", "ON", "HAUNTED" },
    { "OFF", "20%", "40%", "60%", "80%", "100%" }, { "EVERYONE", "CIVILIANS", "POLICE", "FAMILY", "CIVILIANS + POLICE", "FAMILY + POLICE" },
    { "OFF", "20%", "40%", "60%", "80%", "100%" }, { "EVERYONE", "CIVILIANS", "POLICE", "FAMILY", "CIVILIANS + POLICE", "FAMILY + POLICE" }, { "OFF", "ON" }, { "4:3 NORMAL", "16:9 WIDE" }, { "OFF", "+10 CARS", "+30 CARS", "+60 CARS", "LUDICROUS" }, { "GAME CLOCK", "LIVE IN HAND", "OFF: THEY WAIT" } };
static const u8 optN[O_N] = { 5, 6, 4, 3, 4, 3, 4, 4, 3, 3, 2, 2, 3, 2, 3, 2, 4, 2, 2, 4, 3, 2, 4, 3, 3, 3, 6, 6, 6, 6, 2, 2, 5, 3 };
static const char *const optBlurb[O_N] = {
    "LOW, MEDIUM, HIGH OR ULTRA SETS THEM ALL", "HOW FAR SAN BOREAS GOES BEFORE THE SKY", "WHERE ROOMS, WALLS AND THINGS ARE DRAWN IN FULL",
    "THE TOWNS PAST THE MAP: TALL ONES, ALL, OR WINDOWS TOO", "TREES PAST THE NEAR DETAIL, OUT TO", "THE AIR BETWEEN YOU AND THE HORIZON",
    "HOW MANY PEOPLE CAN BE DRAWN AT ONCE", "THE NEAREST, DRAWN AS REAL 3D (THE REST ARE SPRITES)", "HOW OFTEN THE CROWD'S SPRITES ARE REDRAWN",
    "SOFT SHADOWS UNDER PEOPLE AND THINGS", "FLOORS DARKEN WHERE THEY MEET WALLS", "WALLS LIGHTER UP TOP, DARKER AT THE FOOT",
    "A STEADY RATE FEELS SMOOTHER THAN A JUMPY ONE", "THE PLAYSTATION'S SMOOTHING OF COLOURS", "L2 + R2 TOGETHER SWITCH IT. FIRST PERSON: UP WALKS, L2 R2 LOOK",
    "THE FRAME RATE IN A CORNER (L3 IN PLAY)",
    "HOW FAST BORES GROW UP, FROM BABY TO ELDER", "AT THE END OF THE ELDER YEARS: GO ON, OR SAY GOODBYE",
    "EVERYONE IN SAN BOREAS GETS OLDER AND HAS BABIES TOO",
    "HOW HIGH THE CAMERA LOOKS DOWN FROM (LOW SEES THE SKYLINE)", "HOW FAST L1 AND R1 TURN THE CAMERA", "ZOOMING IN WITH R2 LOWERS THE CAMERA TOO", "HOW LOUD THE SONGS AND THE RADIO PLAY",
    "WALLS BETWEEN YOU AND THE CAMERA: AUTO, CUT LOW OR FULL",
    "PUT THE PAD DOWN AND YOUR BORE LOOKS AFTER ITSELF (LOW: AFTER 25 S, HIGH: 10 S)",
    "THE FAMILY'S DEPARTED RISE FROM THEIR GRAVES AT NIGHT (HAUNTED: DAY AND NIGHT)",
    "START A FIGHT AND OTHERS MAY PILE IN. AT 100% EVERYONE IN SIGHT DOES",
    "WHO MAY JOIN A FIGHT YOU START",
    "NO FIGHT NEEDED: PEOPLE GO FOR YOU ON SIGHT. AT 100% EVERYONE DOES",
    "WHO GOES FOR YOU ON SIGHT, WHEN RAGE IS ON",
    "BYSTANDERS TRY TO BREAK UP YOUR FIGHTS (HIT ONE AND THEY JOIN IN)",
    "16:9: DRAWN SQUEEZED SO A WIDE SCREEN OR EMULATOR (16:9 DISPLAY) SHOWS MORE ON THE SIDES, NOT A STRETCHED PICTURE",
    "ADD CARS TO THE STREETS",
    "PET TIME: WITH THE GAME CLOCK, LIVE (RUNS WHILE YOU PLAY THE TOY: YOU MAY FORGET TO EAT!) OR OFF" };
static const u8 presets[4][O_N] = {   // LOW, MEDIUM, HIGH, ULTRA (the graphics knobs; the frame rate, dithering, camera and the counter are left be)
    { 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0 }, { 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1 },
    { 3, 3, 2, 2, 2, 1, 3, 3, 1, 2, 1, 1 }, { 4, 5, 3, 2, 3, 2, 3, 3, 2, 2, 1, 1 } };
#define O_PRESET_LAST O_WALLS                 // (presets set O_VIEW .. O_WALLS)
static u8 opt[O_N] = { 3, 3, 2, 2, 2, 1, 3, 3, 1, 2, 1, 1, 0, 1, 0, 0, 2, 1, 1, 2, 1, 1, 3, 0, 1, 1, 2, 0, 0, 1, 1, 0 };   // HIGH: a kilometre of view; NORMAL lives; the Sims camera, high
static int opt_sel, opt_back, opt_tab;
static void opts_apply(void);
#ifdef AUTO_FLOAT
void float_opts(const u8 *p, int n) { for (int i = 0; i < n && i < O_N; i++) opt[i] = p[i]; opts_apply(); }
#endif
static void opts_apply(void) {
    extern int getenv_noshadow, dbg_noao, dbg_nograd, world_detail_r, cit_live_max, cit_maxdraw, cit_sprite_rate;
    far_view = opt[O_VIEW]; far_detail = opt[O_FARB]; far_trees = opt[O_TREES]; far_haze = opt[O_HAZE];
    static const u8 rr[4] = { 8, 10, 12, 14 }, cr[4] = { 16, 32, 48, 64 }, lv[4] = { 1, 2, 4, 6 };
    world_detail_r = det_opt = rr[opt[O_DETAIL] & 3]; cit_maxdraw = cr[opt[O_CROWD] & 3]; cit_live_max = lv[opt[O_LIVE] & 3]; cit_sprite_rate = opt[O_SPR];
    gfx_noshadow = opt[O_SHADOW] == 0; getenv_noshadow = opt[O_SHADOW] < 2;
    dbg_noao = !opt[O_AO]; dbg_nograd = !opt[O_WALLS];
    gfx_min_vsyncs = opt[O_FPSCAP] == 0 ? 1 : opt[O_FPSCAP] == 1 ? 2 : 3;
    gfx_set_dither(opt[O_DITHER]);
    alone_opt = opt[O_ALONE]; ghost_mode = opt[O_GHOSTS];
    { extern int aggro_pct, rage_pct, aggro_who, rage_who, peace_on; aggro_pct = opt[O_AGGRO] * 20; rage_pct = opt[O_RAGE] * 20; aggro_who = opt[O_AGWHO]; rage_who = opt[O_RAGEWHO]; peace_on = opt[O_PEACE]; }
    gfx_wide = opt[O_WIDE]; traffic_cheat = opt[O_TRAF] % 5; { extern int gotchi_time; gotchi_time = opt[O_GOTCHI] % 3; }
#ifdef WIDE_TEST
    gfx_wide = 1;
#endif
    show_fps = opt[O_SHOWFPS]; audio_music_level(opt[O_MUSIC]);
    aging = opt[O_AGING]; old_age = opt[O_OLDAGE]; pop_aging = opt[O_TOWNAGE];
    cam_view = opt[O_CAM] % 3; cam_turn_rate = opt[O_CAMSPD] == 0 ? 24 : opt[O_CAMSPD] == 1 ? 40 : 64; cam_zoom_tilt = opt[O_ZOOMTILT]; wall_view = opt[O_CUTAWAY] % 3;
}
void cam_view_set(int v) {   // (L2 + R2 in play, and the options)
    cam_view = v; opt[O_CAM] = (u8)v; fp_pitch = 0;
    char b[32]; sprintf(b, "CAMERA: %s", camViewNm[v]); sim_note(b); audio_voice(VOICE_FINISHED);
}
#ifdef AUTO_GOTCHI
static void opts_dbg(int tab, int sel, int back) { opt_back = back; opt_tab = tab; opt_sel = sel; state = ST_OPTS; }
#endif
static void opts_open(int back) { opt_back = back; opt_sel = 0; opt_tab = 0; state = ST_OPTS; }
// the knobs in four tabs (L1 R1), like The Sims 4's game options: sliders for amounts, switches for on and off, pills for choices
#define NOTAB 7
static const char *const optTabNm[NOTAB] = { "GRAPHICS", "PEOPLE", "LIGHT", "CAMERA", "PLAY", "LIFE", "CHEATS" };
static const s8 optTabIc[NOTAB] = { IC_EYE, IC_PEOPLE, IC_SUN, IC_CAMERA, IC_BOLT, IC_SPROUT, IC_STAR };
static const s8 optTab[NOTAB][7] = { { O_PRESET, O_VIEW, O_DETAIL, O_FARB, O_TREES, O_HAZE, O_WIDE }, { O_CROWD, O_LIVE, O_SPR, -1 },
                                     { O_SHADOW, O_AO, O_WALLS, O_DITHER, -1 }, { O_CAM, O_CAMH, O_CAMSPD, O_ZOOMTILT, O_CUTAWAY, -1 },
                                     { O_FPSCAP, O_SHOWFPS, O_MUSIC, -1 }, { O_AGING, O_OLDAGE, O_TOWNAGE, O_ALONE, O_GHOSTS, O_GOTCHI, -1 }, { O_AGGRO, O_AGWHO, O_RAGE, O_RAGEWHO, O_PEACE, O_TRAF, -1 } };
static int opt_kind(int i) {   // 0 a choice, 1 a slider, 2 a switch
    if (i == O_AO || i == O_DITHER || i == O_SHOWFPS || i == O_TOWNAGE || i == O_ZOOMTILT || i == O_PEACE) return 2;
    if (i == O_AGGRO || i == O_RAGE) return 1;
    if (i == O_CAMH || i == O_CAMSPD || i == O_MUSIC) return 1;
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
            opt[O_PRESET] = (u8)p; for (int k = O_VIEW; k <= O_PRESET_LAST; k++) opt[k] = presets[p - 1][k];   // (each row starts with the preset itself, like opt)
        } else {
            if (opt_kind(i) == 1) opt[i] = (u8)CLAMP(opt[i] + d, 0, n - 1);     // (a slider stops at its ends)
            else opt[i] = (u8)((opt[i] + d + n) % n);
            if (i <= O_PRESET_LAST) opt[O_PRESET] = 0;
            if (i == O_CAMH) { static const s16 ph[4] = { 330, 520, 680, 940 }; cam_pitch = ph[opt[O_CAMH] & 3]; }
            if (i == O_CAM) fp_pitch = 0;
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

// ---- the memory card: SAVE GAME in the pause menu, CONTINUE on the title ----
static const char *save_msg;                                 // why the last load failed (the title shows it)
static void save_body(void) {
    sv_u32(world_seed()); sv_u8(bm_packed);
    sv_u8(traffic_level); sv_u8(bm_land); sv_u8(bm_sea); sv_u8(bm_clim); sv_u8(bm_hills); sv_u8(bm_ncity); sv_u8(bm_ntown); sv_u8(bm_nvil); sv_u8(bm_capsz); sv_u8(pop_people); sv_u8(pop_famsize); sv_u8(isl_size); sv_u8(isl_wild); sv_u8(isl_jungle); sv_u8(isl_rocks);   // (CREATE A WORLD's dials: version 7)
    { int n = 0; for (int i = 0; i < BM_NSET_MAX; i++) if (bm_renamed(i)[0]) n++;   // (the towns renamed)
      sv_u8(n); for (int i = 0; i < BM_NSET_MAX; i++) if (bm_renamed(i)[0]) { sv_u8(i); sv_str(bm_renamed(i), 17); } }
    sv_n(opt, O_N);
    sv_u8(home_city); sv_u16(home_bx); sv_u16(home_by); sv_u32((u32)home_paid);
    sim_save(); world_save(!home_city); pop_save(); isl_save();
    story_save();
}
static void load_body(void) {
    u32 seed = ld_u32();
    if (save_ver >= 3) { bm_packed = ld_u8(); bm_gen = save_ver >= 17 ? 4 : 3; wo_name_for(seed); } else { bm_packed = 1; bm_gen = 2; bm_cap_name[0] = 0; }   // (a save from before the TOWNS option: its world as it was made)
    bm_dials_default(); isl_dials_default(); pop_people = 3; pop_famsize = 1; traffic_level = 2;
    traffic_level = 2; if (save_ver >= 19) traffic_level = ld_u8() % 5;
    if (save_ver >= 7) { bm_land = ld_u8() % BL_N; bm_sea = ld_u8() % 5; bm_clim = ld_u8() % 5; bm_hills = ld_u8() % 5;
        if (save_ver >= 8) { bm_ncity = ld_u8() % (BM_MAXCITY + 1); bm_ntown = ld_u8() % (BM_MAXTOWN + 1); bm_nvil = ld_u8() % (BM_MAXVIL + 1); bm_capsz = ld_u8() % 6; pop_people = ld_u8() % 6; pop_famsize = ld_u8() % 4; if (save_ver < 9) bm_capsz = bm_capsz == 0 ? 1 : bm_capsz == 1 ? 2 : 4; }   // (version 8: capital SMALL NORMAL HUGE)
        else { static const u8 c7[4] = { 0, 1, 2, 3 }, t7[4] = { 2, 4, 6, 8 }, v7[4] = { 4, 8, 12, 12 }; int t = ld_u8() % 4; bm_ncity = c7[t]; bm_ntown = t7[t]; bm_nvil = v7[t]; }   // (version 7: one TOWNS dial)
        isl_size = ld_u8() % 3; isl_wild = ld_u8() % 3; isl_jungle = ld_u8() % 3; isl_rocks = ld_u8() % 3; }
    bm_rename_clear();
    if (save_ver >= 4) { int n = ld_u8(); for (int k = 0; k < n; k++) { int i = ld_u8(); char nm[18]; ld_str(nm, 17); bm_rename(i, nm); } }
    { int o_n = save_ver >= 21 ? O_N : save_ver >= 19 ? O_N - 1 : O_N - 2; ld_n(opt, save_ver >= 16 ? o_n : save_ver >= 15 ? o_n - 1 : save_ver >= 14 ? o_n - 6 : save_ver >= 11 ? o_n - 7 : o_n - 8); } if (save_ver < 19) opt[O_TRAF] = 0; if (save_ver < 21) opt[O_GOTCHI] = 0; if (save_ver < 16) opt[O_WIDE] = 0; if (save_ver < 11) opt[O_ALONE] = 1; if (save_ver < 14) opt[O_GHOSTS] = 1;
    if (save_ver < 15) { opt[O_AGGRO] = 2; opt[O_AGWHO] = 0; opt[O_RAGE] = 0; opt[O_RAGEWHO] = 1; opt[O_PEACE] = 1; } opts_apply();   // (YOUR BORE ALONE came with version 11)
    loading("LOADING YOUR GAME", 30);
    bm_new(seed); world_new_seed(seed);
    { u32 sb; void *sc = crowd_scratch(&sb); pop_new(seed, sc, sb); }
    far_new_world();
    world_reset_all(); home_reset(); isl_reset();
    for (int i = 0; i < nsims; i++) look_release(&sims[i].bore);
    sim_init();
    home_city = ld_u8(); home_bx = (s16)ld_u16(); home_by = (s16)ld_u16(); home_paid = (int)ld_u32();
    sim_load(&edit); world_load(); pop_load(); isl_load();
    if (save_ver >= 5) story_load(); else story_reset();
}
static int save_game(int slot) {
    loading("SAVING YOUR GAME", 50);
    SaveInfo in; memset(&in, 0, sizeof in);
    strncpy(in.name, sims[player].bore.last, NAME_LEN - 1); in.day = (u16)day_count; in.nsims = (u8)nsims; in.zone = (u8)world_zone; in.cash = (s32)cash;
    save_set_info(&in);
    if (save_write(slot, save_body)) { char t[32]; sprintf(t, "GAME SAVED IN SLOT %d", slot + 1); sim_note(t); audio_voice(VOICE_WIN); return 1; }
    sim_note(save_error()); audio_voice(VOICE_NAH); return 0;
}
static int load_game(int slot) {   // 1: the saved life is up and running
    loading("LOADING YOUR GAME", 10);
    life_started = 0;
    if (!save_read(slot, load_body)) { save_msg = save_error(); return 0; }
    world_place();
    citizens_init();
    if (world_zone != ZONE_HOME) citizens_enter(world_zone);
    if (world_zone == ZONE_CITY) far_reset();
    life_started = 1; save_msg = 0;
    cam_yaw = 2600; cam_pitch = 640; cam_dist = 1500; frame = 0;
    sim_note("WELCOME BACK");
    return 1;
}
// the slot list: three lives on the card (SAVE GAME picks where, CONTINUE picks which)
static int slot_ask;   // 1: X again replaces the save in that slot, 2: SQUARE again deletes it
static int slot_save, slot_sel, slot_back, slot_have[SAVE_SLOTS], slot_card; static SaveInfo slot_in[SAVE_SLOTS];
static void slots_open(int saving, int back) {
    loading("LOOKING AT THE MEMORY CARD", 50);
    slot_save = saving; slot_back = back; slot_sel = 0; save_msg = 0; slot_ask = 0;
    mcard_dir_keep = 0; slot_have[0] = save_info(0, &slot_in[0]);   // (the card's directory read once for all three)
    mcard_dir_keep = 1; for (int i = 1; i < SAVE_SLOTS; i++) slot_have[i] = save_info(i, &slot_in[i]);
    mcard_dir_keep = 0;
    slot_card = mcard_present();
    if (!saving) for (int i = 0; i < SAVE_SLOTS; i++) if (slot_have[i]) { slot_sel = i; break; }
    state = ST_SLOTS;
}
static void slots_update(void) {
    if (PRESSED(PAD_UP)) { slot_sel = (slot_sel + SAVE_SLOTS - 1) % SAVE_SLOTS; slot_ask = 0; save_msg = 0; }
    if (PRESSED(PAD_DOWN)) { slot_sel = (slot_sel + 1) % SAVE_SLOTS; slot_ask = 0; save_msg = 0; }
    if (PRESSED(PAD_CIRCLE)) { if (slot_ask) { slot_ask = 0; return; } state = slot_back; return; }
    if (PRESSED(PAD_SQUARE) && slot_card && slot_have[slot_sel]) {   // delete: SQUARE, then SQUARE again
        if (slot_ask != 2) { slot_ask = 2; audio_voice(VOICE_NAH); return; }
        slot_ask = 0; loading("DELETING THE SAVE", 50);
        if (save_delete(slot_sel)) { slot_have[slot_sel] = 0; save_msg = "SAVE DELETED"; } else save_msg = save_error();
        return;
    }
    if (!PRESSED(PAD_CROSS)) return;
    if (!slot_card) { save_msg = "NO MEMORY CARD IN SLOT 1"; audio_voice(VOICE_NAH); slot_card = mcard_present(); return; }
    if (slot_save) {
        if (slot_have[slot_sel] && slot_ask != 1) { slot_ask = 1; return; }   // (a slot in use: X again to replace it)
        slot_ask = 0; save_game(slot_sel); state = ST_LIFE; return;
    }
    slot_ask = 0;
    if (!slot_have[slot_sel]) { save_msg = "NOTHING SAVED IN THAT SLOT YET"; audio_voice(VOICE_NAH); return; }
    if (load_game(slot_sel)) state = ST_LIFE;
    else { audio_voice(VOICE_NAH); state = ST_SLOTS; if (!life_started) slot_back = ST_MENU; }
}
static void slots_draw(void) {
    if (slot_back == ST_MENU) title_draw(); else { life_draw(); ui_dim(); }
    int top = ui_window(40, 40, 240, 152, slot_save ? "SAVE GAME" : "CONTINUE", IC_BOOK);
    for (int i = 0; i < SAVE_SLOTS; i++) {
        int y = top + 4 + i * 30; char t[40], u[40];
        ui_round(48, y, 224, 26, 4, i == slot_sel ? UI_HILITE : UI_PANEL, i == slot_sel ? UI_HILITE : UI_PANEL, 0);
        if (slot_have[i]) { sprintf(t, "%d. THE %s FAMILY", i + 1, slot_in[i].name); sprintf(u, "DAY %d   %d BORES   $%d", slot_in[i].day + 1, slot_in[i].nsims, (int)slot_in[i].cash); }
        else { sprintf(t, "%d. EMPTY", i + 1); strcpy(u, slot_save ? "FREE: YOUR LIFE CAN GO HERE" : "FREE: NO LIFE SAVED HERE YET"); }
        ui_text(56, y + 3, 1, t, UI_INK); ui_text(56, y + 14, 1, u, UI_SOFT);
    }
    const char *m = slot_ask == 1 ? "REPLACE THIS SAVE? X AGAIN TO REPLACE" : slot_ask == 2 ? "DELETE THIS SAVE? SQUARE AGAIN" : save_msg ? save_msg : !slot_card ? "NO MEMORY CARD IN SLOT 1" : "";
    ui_textc(160, top + 98, 1, m, slot_ask || (save_msg && strcmp(save_msg, "SAVE DELETED")) || !slot_card ? UI_BADT : UI_SOFT);
    ui_hints(160, top + 112, slot_ask ? (slot_ask == 1 ? "X REPLACE|O NO" : "SQ DELETE|O NO") : slot_have[slot_sel] ? (slot_save ? "X SAVE|SQ DELETE|O BACK" : "X LOAD|SQ DELETE|O BACK") : (slot_save ? "X SAVE|O BACK" : "X LOAD|O BACK"), 1);
}
#ifdef AUTO_STREAM
static void stream_tick(void) {   // (test builds: a streamed song from the first second of a life, for an audio capture)
    static int t0 = -1; if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); audio_jukebox_play(AUTO_STREAM); }
}
#endif
#ifdef AUTO_STORY
int dbg_story[40]; int tst_trace; int tst_on, tst_wx, tst_wz, tst_held, tst_pr;   // (test builds) SKATE LIFE by itself: a taxi to the skate plaza, then the board at a ramp from each side
static void story_test_tick(void) {
    extern int dbg_air_max, dbg_tricks, story_dbg(int k); extern int on_board;
    static int t0 = -1, k, rx = -1, ry, att;
#ifndef STORY_SKIP
    if (state == ST_SCARD) { story_card_shown(); state = ST_LIFE; }
#else
    { static int ct; if (state == ST_SCARD) { if (++ct == 150) { ct = 0; story_card_shown(); if (!story_card()) state = ST_LIFE; } } }
#endif
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); }
    int t = VSync(-1) - t0; Sim *s = &sims[player];
#ifdef AUTO_SKATE
    {   // (the skate plaza from four sides, close: how the ramps, the rails and the boxes look; then AUTO_SKATE's own runs)
        static int g, lx, ly, pbx, pby;
        if (g == 0 && t >= 60) { g = 1; story_taxi(); }
        if (g >= 2) { lx = (pbx - world_wbx) * BM_BLK + BM_LOT0; ly = (pby - world_wby) * BM_BLK + BM_LOT0; }   // (the plaza where it is now: the window moves as you ride)
        if (g == 1 && t >= 200) { g = 2; pbx = world_wbx + sim_tile_x(s) / BM_BLK; pby = world_wby + sim_tile_y(s) / BM_BLK; lx = (pbx - world_wbx) * BM_BLK + BM_LOT0; ly = (pby - world_wby) * BM_BLK + BM_LOT0;
            sim_player_cancel_sim(s); s->x = (lx + 6) * TILE + TILE / 2; s->z = -((ly + 6) * TILE + TILE / 2); }
        if (g == 2) { int v = (t - 200) / 120; if (v > 1) { g = 3; } else { cam_yaw = (v & 3) * 1024 + 300; cam_pitch = 520; cam_dist = 2600; } }
#ifdef AUTO_POSE
        if (g == 3) {   // (the poses, held still, from the side and from the front)
            extern void skate_dbg_pose(int n); extern int tst_on; tst_on = 1; { extern int tst_wx, tst_wz; tst_wx = tst_wz = 0; }
            static int ph = -1; int p_ = (t - 440) / 80;
            if (p_ >= 20) { skate_dbg_pose(-1); return; }
            if (p_ != ph) { ph = p_; if (!on_board) { has_board = 1; sim_player_cancel(); }
                s->x = (lx + 3) * TILE + TILE / 2; s->z = -((ly + 7) * TILE + TILE / 2); s->yaw = 1024; skate_on(s); skate_dbg_pose(p_ % 10); { static const char *const nm[10] = { "RIDING", "WOUND UP", "TUCK", "INDY", "NOSEGRAB", "MANUAL", "GRIND", "PUSH", "KICKFLIP", "IMPOSSIBLE" }; sim_note(nm[p_ % 10]); }
                if (p_ % 10 == 2 || p_ % 10 == 3 || p_ % 10 == 4 || p_ % 10 == 8 || p_ % 10 == 9) board_lift = 120; }
            cam_yaw = p_ < 10 ? 1024 + 300 : 3072 - 300; cam_pitch = 240; cam_dist = 700;
            return;
        }
#endif
        if (g == 3) {   // the runs: each puts you somewhere in the plaza on the board, then a script of stick and buttons
            extern int tst_on, tst_wx, tst_wz, tst_held, tst_pr, skate_dbgv[12]; static int run = -1, rt;
            static const struct { s8 x, y; s16 yaw; } at[6] = { { 0, 10, 1024 }, { 0, 5, 1024 }, { 0, 8, 1024 }, { 1, 3, 1024 }, { 1, 7, 1024 }, { 1, 10, 1024 } };
            int r_ = (t - 440) / 300; if (r_ > 5) { tst_on = 0; dbg_story[19] = t; return; }
            if (r_ != run) { run = r_; rt = 0; tst_on = 1; tst_held = 0;
                if (lx < 0 || ly < 0 || lx + 13 > MW || ly + 13 > MH) { taxi_to_block(pbx, pby, 6, 10, "TEST"); lx = (pbx - world_wbx) * BM_BLK + BM_LOT0; ly = (pby - world_wby) * BM_BLK + BM_LOT0; }
                if (!on_board) { has_board = 1; sim_player_cancel(); }
                sim_player_cancel_sim(s); s->x = (lx + at[run].x) * TILE + TILE / 2; s->z = -((ly + at[run].y) * TILE + TILE / 2); s->yaw = at[run].yaw; skate_on(s); { extern void skate_dbg_speed(int v); skate_dbg_speed(22); }
                { extern int skate_ev[], skate_nev; if (skate_nev < 128) { int *e_ = &skate_ev[skate_nev++ * 4]; e_[0] = 100 + run; e_[1] = s->x; e_[2] = s->z; e_[3] = lx; } }
                cam_yaw = (2048 - at[run].yaw + 4096 + 700) & 4095; cam_pitch = 380; cam_dist = 900; }
            rt++; tst_trace = run == 0;
            int yw = at[run].yaw; tst_wx = (isin(yw) * 4096) >> 12; tst_wz = -((icos(yw) * 4096) >> 12);
            tst_held = 0;
            switch (run) {
                case 0: if (rt > 8 && rt < 18) tst_held = SKB_X; if (rt == 21) tst_pr = SKB_SQ; break;   // an ollie and a kickflip, then up the quarter pipe with a 180
                case 1: break;                                                                                                       // over the manual pad, the kicker, onto the funbox
                case 2: break;                                                                                                       // north: the launch ramp, then the quarter pipe (vert)
                case 3: if (rt == 5) tst_pr = SKB_TRI; break;                                                                       // a grind along the rail
                case 4: if (rt == 8) tst_pr = SKB_SQ; break;                                                                        // a manual
                case 5: if (rt > 4 && rt < 14) tst_held = SKB_X; if (rt == 17) tst_pr = SKB_O; if (rt > 17 && rt < 34) tst_held = SKB_O; break;   // a grab
            }
            { int *o = &dbg_story[run * 6]; o[0] = skate_dbgv[0]; o[1] = skate_dbgv[9]; o[2] = skate_dbgv[10]; o[3] = skate_dbgv[11]; o[4] = skate_dbgv[8] & 255; o[5] = skate_dbgv[1]; }
            dbg_story[36] = skate_dbgv[7]; dbg_story[37] = skate_dbgv[2]; dbg_story[38] = (int)skate_score; dbg_story[39] = skate_dbgv[3];
        }
        return;
    }
#endif
#ifdef AUTO_GUEST
    {   // (BEST FRIENDS chapter 4: a party at home, then a taxi to a town square)
        extern void story_dbg_ch(int ch); static int g;
        if (g == 0 && t >= 100) { g = 1; story_dbg_ch(3); }
        if (g == 1 && t >= 200) { g = 2; throw_party(); dbg_story[0] = story_dbg(0); }
        if (g == 2 && t >= 400) { g = 3; dbg_story[1] = story_dbg(0); story_taxi(); }
        if (g == 3 && t >= 700) { g = 4; dbg_story[2] = story_dbg(0); }
        dbg_story[14] = story_ch; dbg_story[15] = story_dbg(0); dbg_story[19] = t; return;
    }
#endif
#ifdef AUTO_SOCIAL
    {   // (SECOND CHANCE: chats with VAL in a loop, skipping the chapter that needs a fight; the places by taxi)
        extern void story_dbg_ch(int ch); static int nx = 120, nt = 600, ns;
        if (story_ch == 1) story_dbg_ch(2);
        if (t >= nt) { nt = t + 700; int l; if (story_place(&l)) story_taxi(); }
        if (t >= nx && nsims > 1 && state == ST_LIFE) { nx = t + 90; Sim *o = &sims[1];
            sim_player_cancel_sim(s); sim_player_cancel_sim(o); o->x = s->x + TILE; o->z = s->z; o->path_n = 0; s->yaw = 1024; o->yaw = 3072;
            char opt[8][16]; int n = sim_pie_open(opt), pick = -1;
            { static const char *const want[4] = { "GO STEADY", "FLIRT", "HUG", "JOKE" }; int r = ns++ % 3;
              for (int i = 0; i < n; i++) if (!strcmp(opt[i], want[0])) pick = i;
              for (int k2 = 0; k2 < 3 && pick < 0; k2++) for (int i = 0; i < n; i++) if (!strcmp(opt[i], want[1 + (r + k2) % 3])) pick = i; }
            if (pick >= 0) sim_pie_do(pick);
            dbg_story[6] = n; dbg_story[7] = pick; dbg_story[8] = s->rel[1]; dbg_story[9] = story_dbg(5); dbg_story[10] = s->partner == o->uid; dbg_story[11]++; }
        dbg_story[14] = story_ch; dbg_story[15] = story_dbg(0); dbg_story[19] = t; return;
    }
#endif
    if (k == 0 && t >= 60) { k++; story_taxi(); dbg_story[0] = world_zone; }
    if (k == 1 && t >= 200) { k++; int px = sim_tile_x(s), py = sim_tile_y(s), bd = 999;
        for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) { char c = world_at(x, y); if (c == '9' || c == '5' || c == '1') { int d = ABS(x - px) + ABS(y - py); if (d < bd) { bd = d; rx = x; ry = y; } } }
        dbg_story[1] = rx; dbg_story[2] = ry; dbg_story[3] = bd; dbg_story[4] = story_dbg(0); }
    if (k == 2 && rx >= 0 && t >= 240 + att * 200 && att < 8) {   // a run at the ramp from one side
        static const s8 dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 }; int d = att & 3, dist = att < 4 ? 4 : 6;
        { int px = sim_tile_x(s), py = sim_tile_y(s), bd = 999; for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) { char c = world_at(x, y); if (c == '9' || c == '5' || c == '1') { int dd = ABS(x - px) + ABS(y - py); if (dd < bd) { bd = dd; rx = x; ry = y; } } } }
        int sx = rx - dx[d] * dist, sy = ry - dy[d] * dist;
        dbg_story[20 + (att & 3)] = world_walkable(sx, sy) * 1000 + world_at(sx, sy);
        if (world_walkable(sx, sy)) { sim_player_cancel_sim(s); s->x = sx * TILE + TILE / 2; s->z = -(sy * TILE + TILE / 2); s->yaw = 0; if (!on_board) sim_player_cancel(); }
        att++;
    }
    if (k == 2 && att > 0 && att <= 8) { static const s8 dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 }; int d = (att - 1) & 3; sim_player_move(dx[d] * 4096, -dy[d] * 4096, 1);
        dbg_story[5 + ((att - 1) & 7)] = dbg_air_max * 100 + dbg_tricks; }
    dbg_story[14] = story_ch; dbg_story[15] = story_dbg(0); dbg_story[16] = story_dbg(1); dbg_story[17] = story_dbg(2) * 100 + story_dbg(4); dbg_story[18] = on_board; dbg_story[19] = t;
}
#endif
#if defined(AUTO_SAVE) || defined(AUTO_CELLS)
int dbg_cells[6];
#endif
#ifdef AUTO_SAVE
int dbg_sv[16], dbg_st[8];   // (test builds) before / after a save and a load: the zone, the tent, the bench, the hut's walls, the player's action, food; then the long-life save's size
static void save_tick(void) {
    extern int sim_debug_goto(char c, int lot); extern u32 save_last_bytes;
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30) return; t0 = VSync(-1); clock_min = 22 * 60;
        { Bore *b = crowd_temp_bore();   // (how many cells a Bore needs: the most, and the most that show)
          for (int r = 0; r < 200; r++) { memset(b, 0, sizeof *b); look_default(b); b->stage = (u8)(r % 5); look_random(b); if (r & 1) for (int q = 0; q < LK_COUNT; q++) if (look_allowed(b, q, b->look[q] + 3)) b->look[q] += 3; look_build(b);
            int vis = 0; for (int i = 0; i < b->ncell; i++) if ((b->cell[i].hide & 63) != 63) vis++;
            if (b->ncell > dbg_cells[0]) dbg_cells[0] = b->ncell; if (vis > dbg_cells[1]) dbg_cells[1] = vis; dbg_cells[2] += b->ncell; dbg_cells[3] += vis; } }
        travel_go(ZONE_ISLAND); }
    int t = VSync(-1) - t0;
    if (k == 0 && t >= 60) { k++; isl_wood = isl_stone = isl_leaves = 40; isl_food = 7; isl_make(4); isl_make(0); isl_make(1); }
    if (k == 1 && t >= 120) { k++; sims[player].need[N_REST] = 10 << 10; dbg_sv[14] = sim_debug_goto('z', -1); dbg_sv[15] = sim_player_interact(); }
    if (k == 2 && t >= 300) { k++; dbg_sv[0] = world_zone; dbg_sv[1] = world_count('z'); dbg_sv[2] = world_count('%'); dbg_sv[3] = world_count('W'); dbg_sv[4] = sims[player].action; dbg_sv[5] = isl_food;
                              story_id = STY_SKATE; story_ch = 2; save_game(0); story_reset(); }   // (and a story: it must come back)
    if (k == 3 && t >= 400) { k++; if (load_game(0)) state = ST_LIFE; }
    if (k == 4 && t >= 460) { k++; dbg_sv[8] = world_zone; dbg_sv[9] = world_count('z'); dbg_sv[10] = world_count('%'); dbg_sv[11] = world_count('W'); dbg_sv[12] = sims[player].action; dbg_sv[13] = isl_food; dbg_sv[6] = (int)save_last_bytes; dbg_sv[14] = story_id * 10 + story_ch; }
    { static int played; if (k == 5 && t >= 600 && !played) { extern int audio_dbg_song(void); played = 1; audio_jukebox_play(7); dbg_cells[2] = audio_dbg_song(); } }   // (the biggest song: it must still fit)
    { static int sp; extern int audio_dbg_stream(int k); extern int dbg_st[8];   // (a streamed song: it must keep coming off the disc)
      if (k == 6 && !sp) { sp = 1; audio_jukebox_play(33); dbg_st[0] = t; }
      if (sp) { dbg_st[1] = audio_dbg_stream(0); dbg_st[2] = audio_dbg_stream(1); dbg_st[3] = audio_dbg_stream(2); dbg_st[4] = audio_dbg_stream(3); dbg_st[5] = t; dbg_st[6] = audio_jukebox_current(); } }
    if (k == 5 && t >= 700) { k++; aging = 2; pop_aging = 1; for (int d = 0; d < 400; d++) pop_midnight(); save_game(1); dbg_sv[7] = (int)save_last_bytes; }
}
#endif
static const char *const pauseNm[] = { "RESUME", "BORE PANEL", "BUILD MODE", "KINGDOM MODE", "BOREAGOTCHI", "SUPERSTAR BAND", "CREATE A DOG", "CREATE A BORE", "ADD A BORE", "OPTIONS", "HOW TO PLAY", "SAVE GAME", "NEW LIFE", "QUIT TO TITLE" };
#define NPAUSE 14  // (calls, parties, the map, travel, jobs, homes, the people, music and the goals are on the phone: SELECT)
static void homes_open(void);
#ifdef AUTO_PARTY
int ap_frames, ap_vs, ap_cpu;
#endif
void throw_party(void) {   // (also the phone's THROW A PARTY, and the DJ booth's SEND INVITES): the guest list first
    if (citizens_party_on()) { sim_note("THE PARTY IS ALREADY GOING!"); return; }
    if (cash < 100) { sim_note("YOU NEED $100 TO THROW A PARTY"); audio_voice(VOICE_NAH); return; }
#ifdef AUTO_PARTY
    { extern void party_begin_t(void); party_open = 1; party_begin_t(); return; }
#endif
    sim_want_party = 1;
}
#ifdef AUTO_PARTY
void party_begin_t(void);
#endif
static void party_begin(void) {   // the list is made (PARTY.OVL): the party starts
    long cost = party_open ? 200 : 100;
    if (cash < cost) { sim_note("NOT ENOUGH CASH FOR THAT PARTY"); audio_voice(VOICE_NAH); return; }
    int n = citizens_party();
    if (n <= 0) { sim_note(n < 0 ? "THE PARTY IS ALREADY GOING!" : "NOBODY CAN COME, OR NO ROOM TO DANCE HERE"); audio_voice(VOICE_NAH); return; }
    cash -= cost;
    char b[40]; sprintf(b, "PARTY! %d GUESTS ARE COMING", n); sim_note(b);
    audio_station_play(1);                                            // RAVE 101
    audio_voice(VOICE_YAHOO);
    for (int i = 0; i < nsims; i++) { sim_moodlet(&sims[i], 15); wants_event(&sims[i], W_PARTY, 0); }   // FUN TIME
}
#ifdef AUTO_PARTY
void party_begin_t(void) { party_begin(); }
#endif
static void pause_new_life(void) { life_started = 0; start_life(); state = ST_LIFE; }
static void pause_quit(void) { audio_jukebox_stop(); life_started = 0; state = ST_TITLE; look_release(&title_bore); look_default(&title_bore); look_random(&title_bore); }
void gotchi_pause_open(void) {
    if (!gotchi_any()) { sim_note("NO BOREAGOTCHI YET: $5000 AT ELECTRONICS SHOPS. HERE IS THE GUIDE"); sim_want_gotchi = 4; state = ST_LIFE; return; }
    sim_want_gotchi = 1; state = ST_LIFE;
}
static void pause_update(void) {
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + NPAUSE - 1) % NPAUSE;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % NPAUSE;
    if (PRESSED(PAD_START) || PRESSED(PAD_CIRCLE)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_SELECT)) { extern void phone_open(void); phone_open(); return; }
    if (PRESSED(PAD_CROSS)) {
        sub_back = ST_PAUSE;
        switch (menu_sel) {
            case 0: state = ST_LIFE; break;
            case 1: panel_tab = 0; state = ST_PANEL; break;
            case 4: { extern void gotchi_pause_open(void); gotchi_pause_open(); } break;
            case 5: sim_want_super = 1; state = ST_LIFE; break;   // (SUPERSTAR: bands and concerts, a file of its own on the disc)
            case 6: sim_want_dog = 2; state = ST_LIFE; break;   // (YOUR DOG: the dog maker, a file of its own on the disc)
            case 7: { TexRef te = edit.tex_eye, tm = edit.tex_mouth; edit = sims[player].bore; edit.tex_eye = te; edit.tex_mouth = tm; look_build(&edit); }   // (the creator draws on its own faces)
                    if (!world_backup()) { sim_note("NO ROOM FOR THE STUDIO JUST NOW. TRY AT HOME"); state = ST_LIFE; break; }
                    create_tab = 0; create_row = 0; prev_state = ST_LIFE; state = ST_CREATE; world_gen_studio(); break;
            case 8: { Bore b; look_default(&b); b.stage = (u8)(AG_CHILD + look_rand() % 3); look_random(&b);
                      sim_add(&b, sim_tile_x(&sims[player]), sim_tile_y(&sims[player]) + 1, 1); sim_note("A NEW BORE MOVED IN"); audio_voice(VOICE_MOVE_YES); state = ST_LIFE; } break;
            case 3: if ((ovl = ovl_load("KINGDOM.OVL"))) { ovl->open(0); state = ST_REALM; } else state = ST_LIFE; break;
            case 2: build_x = sim_tile_x(&sims[player]); build_y = sim_tile_y(&sims[player]); room_on = 0; build_msg_t = 0; state = ST_BUILD;
                     if (world_zone == ZONE_CITY && !world_own_lot(build_x, build_y)) { int tx, ty; if (world_own_door(&tx, &ty)) { build_x = tx; build_y = ty - 1; } }   // (in SAN BOREAS: on your own lot)
                     break;
            case 9: opts_open(ST_PAUSE); break;
            case 10: prev_state = ST_PAUSE; help_page = 0; state = ST_HELP; break;
            case 11: slots_open(1, ST_PAUSE); break;
            case 12: confirm_open("A NEW LIFE?", "THIS LIFE ENDS HERE, AND A NEW ONE BEGINS IN A NEW WORLD. ANYTHING NOT SAVED IS LOST. SAVE FIRST FROM THE PAUSE MENU IF YOU WANT TO COME BACK TO IT.", pause_new_life); break;
            case 13: confirm_open("QUIT TO THE TITLE?", "ANYTHING NOT SAVED SINCE YOUR LAST SAVE IS LOST.", pause_quit); break;
        }
    }
}
static void pause_draw(void) {   // one short column: the game itself (the rest of life is on the phone)
    static const s8 ic[NPAUSE] = { IC_PLAY, IC_ARROW, IC_HAMMER, IC_SPROUT, IC_HEART, IC_NOTE, IC_HEART, IC_STAR, IC_ADD, IC_GEAR, IC_HELP, IC_BOOK, IC_SPROUT, IC_DOOR };
    life_draw();
    ui_dim();
    int top = ui_window(70, 4, 180, 228, "PAUSED", IC_PAUSE);
    for (int i = 0; i < NPAUSE; i++) ui_button(78, top + i * 14, 164, 13, pauseNm[i], ic[i], i == menu_sel);
    ui_hints(160, top + NPAUSE * 14 + 3, "SEL PHONE|O BACK", 1);
}

// ---- THE PHONE (SELECT): The Sims' cell phone. Twelve apps on a grid; the family's list for playing as someone else ----
static const char *const phNm[PH_N] = { "CALL", "PIZZA", "PARTY", "MAP", "TRAVEL", "JOBS", "HOMES", "PEOPLE", "FAMILY", "ADOPT", "MUSIC", "GOALS", "TEXT", "PHOTO", "GAMES" };
static const char *const phBlurb[PH_N] = { "CHAT WITH A FRIEND, RIGHT WHERE YOU ARE", "$40. A HOT MEAL, NO COOKING", "$100. GUESTS COME OVER TO DANCE",
    "SAN BOREAS, AND A TAXI ANYWHERE IN IT", "THE ISLAND, THE DESERT TOWN, HOME", "FIND WORK, OR GET A RIDE TO WORK", "HOUSES AND LOTS FOR SALE NEAR YOU",
    "EVERYONE IN SAN BOREAS, AND WHERE THEY ARE", "PLAY AS SOMEONE ELSE IN THE FAMILY", "$1000. A BABY OR A CHILD, HOME TOMORROW", "EVERY SONG THERE IS, WHEREVER YOU ARE", "THE STORY SO FAR: YOUR GOALS",
    "A QUICK HELLO TO A FRIEND. QUICKER THAN A CALL", "A SELFIE. SMILE!", "A GAME ON THE HANDHELD, OR ON THE PHONE ITSELF" };
static const s8 phIc[PH_N] = { IC_CHAT, IC_FOOD, IC_PARTY, IC_PIN, IC_CAR, IC_CASE, IC_HOUSE, IC_PEOPLE, IC_ARROW, IC_BABY, IC_NOTE, IC_BOOK, IC_BUBBLES, IC_CAMERA, IC_STAR };
static const u32 phCol[PH_N] = { UI_RGB(76, 196, 92), UI_RGB(240, 140, 50), UI_RGB(236, 90, 150), UI_RGB(40, 170, 170), UI_RGB(38, 146, 222), UI_RGB(110, 100, 200),
    UI_RGB(70, 170, 110), UI_RGB(150, 90, 210), UI_RGB(84, 196, 72), UI_RGB(240, 110, 130), UI_RGB(228, 70, 70), UI_RGB(220, 160, 30),
    UI_RGB(60, 180, 230), UI_RGB(90, 96, 120), UI_RGB(200, 70, 200) };
static int ph_sel, ph_fam, ph_fsel, ph_t, ph_moveout;
static void phone_move_out_yes(void) {
    char ad[24], b[48], nm[NAME_LEN]; strcpy(nm, sims[ph_moveout].bore.first);
    if (sim_move_out(ph_moveout, ad)) { snprintf(b, sizeof b, "%s MOVED TO %s", nm, ad); sim_note(b); audio_voice(VOICE_MOVE_YES); ph_fsel = player; }
    else { sim_note("NO HOME FREE FOR THEM RIGHT NOW"); audio_voice(VOICE_MOVE_NO); }
}
// what each app needs: 1 any phone (even the old flip), 2 a smartphone, 3 the BORE PHONE MAX (GAMES: a handheld will do as well)
static const u8 phNeed[PH_N] = { 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 3, 2, 1, 2, 3 };   // (JOBS on any phone: the flip reads the paper's classifieds, a smartphone the whole job board)
static const char *const phoneNm[4] = { "NO PHONE", "FLIP PHONE", "SMARTPHONE", "BORE PHONE MAX" };
static int ph_has(int i) { return phone_tier >= phNeed[i] || (i == PH_GAMES && handheld && phone_tier >= 1); }
void phone_open(void) {
    if (!phone_tier) { sim_note("NO PHONE! ELECTRONICS SHOPS SELL THEM"); audio_voice(VOICE_NAH); return; }
    state = ST_PHONE; ph_fam = 0; ph_t = 0; if (ph_sel < 0 || ph_sel >= PH_N) ph_sel = 0; audio_voice(VOICE_FINISHED); }
static void phone_update(void) {
    ph_t++;
    if (ph_fam) {   // the family: X plays as them
        if (PRESSED(PAD_UP)) ph_fsel = (ph_fsel + nsims - 1) % nsims;
        if (PRESSED(PAD_DOWN)) ph_fsel = (ph_fsel + 1) % nsims;
        if (PRESSED(PAD_CIRCLE)) { ph_fam = 0; return; }
        if (PRESSED(PAD_START) || PRESSED(PAD_SELECT)) { state = ST_LIFE; return; }
        if (PRESSED(PAD_SQUARE)) {   // MOVE OUT: a grown-up of the family to a home of their own in town (they stay someone you know)
            const Sim *o = &sims[ph_fsel]; char b[160];
            if (ph_fsel == player) { sim_note("PLAY AS SOMEONE ELSE FIRST, THEN MOVE THIS ONE OUT"); audio_voice(VOICE_NAH); return; }
            if (o->bore.stage != AG_ADULT && o->bore.stage != AG_ELDER) { sim_note("ONLY A GROWN-UP CAN MOVE OUT"); audio_voice(VOICE_NAH); return; }
            snprintf(b, sizeof b, "%s MOVES TO A HOME OF THEIR OWN IN SAN BOREAS. THEY STAY A FRIEND: YOU CAN VISIT, OR ASK THEM TO MOVE BACK IN.", o->bore.first);
            ph_moveout = ph_fsel; confirm_open("MOVE OUT?", b, phone_move_out_yes); return;
        }
        if (PRESSED(PAD_CROSS)) {
            const Sim *o = &sims[ph_fsel]; char b[40];
            if (ph_fsel == player) { state = ST_LIFE; return; }
            if (o->bore.stage == AG_BABY) { sim_note("BABIES JUST SLEEP AND EAT. PICK SOMEONE OLDER"); audio_voice(VOICE_NAH); return; }
            if (o->away) { sprintf(b, "%s IS AT SCHOOL", o->bore.first); sim_note(b); audio_voice(VOICE_NAH); return; }
            if (sim_play_as(ph_fsel)) { sprintf(b, "NOW PLAYING AS %s", sims[player].bore.first); sim_note(b); audio_voice(VOICE_YAHOO); }
            state = ST_LIFE;
        }
        return;
    }
    if (PRESSED(PAD_LEFT)) ph_sel = (ph_sel + PH_N - 1) % PH_N;
    if (PRESSED(PAD_RIGHT)) ph_sel = (ph_sel + 1) % PH_N;
    if (PRESSED(PAD_UP)) ph_sel = (ph_sel + PH_N - 3) % PH_N;
    if (PRESSED(PAD_DOWN)) ph_sel = (ph_sel + 3) % PH_N;   // (three across, five down)
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START) || PRESSED(PAD_SELECT)) { state = ST_LIFE; return; }
    if (!PRESSED(PAD_CROSS)) return;
    if (!ph_has(ph_sel)) { sim_note(ph_sel == PH_GAMES ? "GAMES: BUY A BOREBOY, OR THE BORE PHONE MAX" : phNeed[ph_sel] >= 3 ? "THAT NEEDS THE BORE PHONE MAX" : "THAT NEEDS A SMARTPHONE"); audio_voice(VOICE_NAH); return; }
    sub_back = ST_PHONE;
    switch (ph_sel) {
        case PH_CALL: case PH_PIZZA: state = ST_LIFE; if (!sim_cell(ph_sel == PH_PIZZA)) audio_voice(VOICE_NAH); break;
        case PH_PARTY: state = ST_LIFE; throw_party(); break;
        case PH_MAP: map_t = -1; state = ST_MAP; break;   // (opened on the map's own first frame: this frame still draws the people, whose sprites share that VRAM)
        case PH_TRAVEL: travel_sel = home_city && home_is_here() ? 0 : world_zone + home_city; state = ST_TRAVEL; break;
        case PH_JOBS: jobs_board = 0; jobs_open(); state = ST_JOBS; break;
        case PH_HOMES: nhood_open(0); break;
        case PH_PEOPLE: dir_open(); state = ST_DIR; break;
        case PH_FAMILY: ph_fam = 1; ph_fsel = player; break;
        case PH_ADOPT: state = ST_LIFE; sim_cell(2); break;
        case PH_MUSIC: prev_state = ST_LIFE; state = ST_JUKEBOX; break;
        case PH_GOALS: state = ST_STORY; break;
        case PH_TEXT: state = ST_LIFE; if (!sim_cell(3)) audio_voice(VOICE_NAH); break;
        case PH_PHOTO: state = ST_LIFE; sim_cell(4); break;
        case PH_GAMES: state = ST_LIFE; if (!sim_cell(5)) audio_voice(VOICE_NAH); break;
    }
}
static void phone_draw(void) {
    life_draw();
    ui_dim();
    char b[40];
    const int PX = 164, PY = 10, PW = 148, PH_ = 218;              // the phone, standing up on the right
    int lift = ph_t < 8 ? (8 - ph_t) * 6 : 0;                        // (it slides up out of the pocket)
    int py = PY + lift;
    ui_shadow(PX, py, PW, PH_, 16);
    static const u32 bodyT[4] = { 0, UI_RGB(96, 100, 110), UI_RGB(44, 50, 64), UI_RGB(170, 140, 70) }, bodyB[4] = { 0, UI_RGB(52, 56, 64), UI_RGB(22, 25, 34), UI_RGB(96, 70, 30) };
    ui_round(PX, py, PW, PH_, 16, bodyT[phone_tier & 3], bodyB[phone_tier & 3], 0);       // the body: a grey flip, a black smartphone, a gold MAX
    ui_round(PX + 58, py + 5, 32, 4, 2, UI_RGB(12, 14, 20), UI_RGB(12, 14, 20), 0);  // the speaker
    const int SX = PX + 6, SW = PW - 12; int sy = py + 13, sh = PH_ - 30;
    static const u32 scrT[4] = { 0, UI_RGB(150, 190, 150), UI_RGB(120, 196, 246), UI_RGB(250, 170, 120) }, scrB[4] = { 0, UI_RGB(70, 110, 80), UI_RGB(58, 120, 206), UI_RGB(170, 70, 150) };
    ui_round(SX, sy, SW, sh, 9, scrT[phone_tier & 3], scrB[phone_tier & 3], 0);     // the screen's wallpaper: a green flip display, a blue sky, a sunset
    sprintf(b, "%02d:%02d", clock_min / 60, clock_min % 60); gfx_text_raw(SX + 7, sy + 5, 1, b, 255, 255, 255);
    sprintf(b, "$%ld", cash); gfx_text_raw(SX + SW - 7 - gfx_text_w(1, b), sy + 5, 1, b, 220, 255, 220);
    ui_round(PX + 54, py + PH_ - 11, 40, 4, 2, UI_RGB(200, 206, 216), UI_RGB(170, 176, 188), 0);   // the home bar
    if (ph_fam) {   // the family list
        gfx_text_raw(SX + 8, sy + 19, 2, "FAMILY", 255, 255, 255);
        for (int i = 0; i < nsims; i++) {
            const Sim *o = &sims[i]; int y = sy + 38 + i * 26, sel = i == ph_fsel;
            ui_round(SX + 4, y, SW - 8, 23, 7, sel ? UI_WHITE : UI_RGB(232, 240, 250), sel ? UI_HILITE : UI_RGB(214, 226, 242), 0);
            if (sel) gfx_rect(SX + 4, y + 4, 2, 15, 38, 146, 222, 0);
            int mm = o->mood; u32 ring = i == player ? UI_BLUE : mm > 15 ? UI_GREEN : mm > -15 ? UI_RGB(236, 176, 40) : UI_RGB(228, 84, 70);
            ui_disc(SX + 17, y + 11, 19, ring); ui_disc(SX + 17, y + 11, 15, UI_RGB(244, 248, 252)); draw_face_k(SX + 9, y + 3, mm, 1);
            ui_text(SX + 30, y + 3, 1, o->bore.first, UI_INK);
            ui_text(SX + 30, y + 13, 1, i == player ? "PLAYING NOW" : o->away ? "AT SCHOOL" : look_stage_name(o->bore.stage), i == player ? UI_BLUE2 : UI_SOFT);
        }
    } else {        // the apps: three across, four down
        for (int i = 0; i < PH_N; i++) {
            int cx = SX + 22 + (i % 3) * 46, cy = sy + 30 + (i / 3) * 34, sel = i == ph_sel, has = ph_has(i);
            if (sel) ui_round(cx - 21, cy - 13, 42, 33, 8, UI_RGB(255, 255, 255), UI_RGB(220, 236, 252), 0);
            ui_icon_disc(cx, cy, 22, has ? phIc[i] : IC_LOCK, has ? phCol[i] : UI_RGB(120, 126, 140), UI_WHITE);
            if (sel) ui_textc(cx, cy + 12, 1, phNm[i], UI_INK);
            else { gfx_text_raw(cx - gfx_text_w(1, phNm[i]) / 2 + 1, cy + 13, 1, phNm[i], 20, 50, 110); gfx_text_raw(cx - gfx_text_w(1, phNm[i]) / 2, cy + 12, 1, phNm[i], has ? 255 : 170, has ? 255 : 176, has ? 255 : 190); }
        }
    }
    {   // what the app does, and the buttons, on a card on the left
        const char *t = ph_fam ? "FAMILY" : phNm[ph_sel], *bl = ph_fam ? "X PLAYS AS THEM. SQUARE: A GROWN-UP MOVES OUT TO A HOME OF THEIR OWN." : phBlurb[ph_sel];
        if (!ph_fam && ph_sel == PH_MAP && world_zone == ZONE_ISLAND) bl = "THE ISLAND, AND THE SECRETS FOUND ON IT";
        if (!ph_fam && !ph_has(ph_sel)) bl = ph_sel == PH_GAMES ? "LOCKED. A BOREBOY OR THE BORE PHONE MAX PLAYS GAMES: ELECTRONICS SHOPS SELL BOTH"
                                       : phNeed[ph_sel] >= 3 ? "LOCKED. GET THE BORE PHONE MAX ($900) AT AN ELECTRONICS SHOP" : "LOCKED. GET A SMARTPHONE ($350) AT AN ELECTRONICS SHOP";
        int nl = ui_wrap_n(132, bl), ch = 34 + nl * 10;
        int cy = 196 - ch;
        ui_card(8, cy, 148, ch + 30);
        ui_badge_i(8, cy - 16, IC_CHAT, phoneNm[phone_tier & 3], UI_WHITE, UI_INK);   // (which phone this is)
        ui_icon_disc(20, cy + 12, 18, ph_fam ? IC_ARROW : phIc[ph_sel], ph_fam ? UI_RGB(236, 176, 40) : phCol[ph_sel], UI_WHITE);
        ui_text(34, cy + 6, 2, t, UI_INK);
        ui_wrap(14, cy + 24, 132, 4, bl, UI_SOFT);
        ui_hints(82, cy + ch + 12, ph_fam ? "X PLAY|SQ MOVE OUT" : "X OPEN|O CLOSE", 1);
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
    if (PRESSED(PAD_CROSS)) { state = ST_LIFE;
        if (pie_cit) { if (citizens_pie_do(pie_sel)) { pie_n = citizens_pie_more(pie_opt); pie_sel = 0; if (pie_n > 1) state = ST_PIE; } } else sim_pie_do(pie_sel); }
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START) || PRESSED(PAD_TRIANGLE)) state = ST_LIFE;
}
static void pie_draw(void) {
    life_draw();
    int cx = 160, cy = 100, R = pie_n > 4 ? 60 : 50;
    { const char *t = pie_cit ? citizens_pie_title() : sim_pie_title();   // the centre: who or what it is, on a round badge
      ui_shadow(cx - 9, cy - 9, 18, 18, 6); ui_icon_disc(cx, cy, 20, pie_cit ? IC_CHAT : IC_ARROW, UI_BLUE, UI_WHITE);
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
static void story_taxi(void) {   // where the story's chapter sends you, by taxi ($20; STRANGE TOWN by the travel way, home for free)
    int lot = 0, k = story_place(&lot);
    if (k == 3) { if (home_city) home_go(); else if (world_zone != ZONE_HOME) travel_go(ZONE_HOME); state = ST_LIFE; return; }
    if (k == 2) { if (world_zone != ZONE_TOWN) { travel_go(ZONE_TOWN); story_visit[2] = 1; for (int i = 0; i < nsims; i++) wants_event(&sims[i], W_TOWN, 0); } state = ST_LIFE; return; }
    if (k != 1) return;
    if (cash < 20) { sim_note("A TAXI RIDE COSTS $20"); audio_voice(VOICE_NAH); return; }
    const Sim *s = &sims[player];
    int bx0 = world_zone == ZONE_CITY ? world_wbx + sim_tile_x(s) / BM_BLK : home_city ? home_bx : bm_set(bm_capital())->x;
    int by0 = world_zone == ZONE_CITY ? world_wby + sim_tile_y(s) / BM_BLK : home_city ? home_by : bm_set(bm_capital())->y;
    int bi = -1, bd = 1 << 30;
    for (int i = 0; i < pop_nplace; i++) if (pop_place[i].lot == lot) { int d = ABS(pop_place[i].bx - bx0) + ABS(pop_place[i].by - by0); if (d < bd) { bd = d; bi = i; } }
    if (bi < 0) { sim_note("SAN BOREAS HAS NO PLACE LIKE THAT"); audio_voice(VOICE_NAH); return; }
    cash -= 20; state = ST_LIFE;
    char nm[24]; const Place *p = &pop_place[bi]; bm_place_name(p->lot, bm_hash(p->bx, p->by, 3), nm);
    taxi_to_block(p->bx, p->by, -1, -1, nm);
}
static void spick_update(void) {   // STORY MISSIONS: one story's card at a time
    sc_t++;
    if (PRESSED(PAD_LEFT) || PRESSED(PAD_L1) || PRESSED(PAD_UP)) sp_sel = (sp_sel + STY_N - 3) % (STY_N - 1) + 1;
    if (PRESSED(PAD_RIGHT) || PRESSED(PAD_R1) || PRESSED(PAD_DOWN)) sp_sel = sp_sel % (STY_N - 1) + 1;
    if (PRESSED(PAD_CIRCLE)) { state = sp_join ? ST_STORY : ST_WORLDS; return; }
    if (PRESSED(PAD_CROSS)) {
        if (sp_sel > STY_PLAYABLE) { audio_voice(VOICE_NAH); return; }
        if (sp_join) { if (story_join(sp_sel)) { goals_old = 0; state = ST_LIFE; } else audio_voice(VOICE_NAH); return; }   // (from GOALS: this life)
        life_story = sp_sel; worlds_go(0, sp_sel == STY_RAGS ? 2 : 0);   // (RAGS TO RICHES: $1,000 and no home)
    }
}
static void scard_update(void) {
    sc_t++;
    if (sc_t < 20) return;   // (a moment to read it)
    if (PRESSED(PAD_TRIANGLE)) { int l; while (story_card()) story_card_shown(); if (story_place(&l)) story_taxi(); state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { story_card_shown(); sc_t = 0; if (!story_card()) state = ST_LIFE; }
}
static void scard_draw(void) { life_draw(); ui_dim(); story_card_draw(story_card(), sc_t); }
static void story_update(void) {
    if (story_id && PRESSED(PAD_TRIANGLE)) { goals_old ^= 1; return; }
    if (story_id && !goals_old && PRESSED(PAD_CROSS)) { int l; if (story_place(&l)) { story_taxi(); return; } }
    if (!story_id && PRESSED(PAD_SQUARE)) { sp_join = 1; sc_t = 0; state = ST_SPICK; return; }   // a story for this life
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_CROSS) || PRESSED(PAD_START)) state = BACK_TO();
}
static void story_draw(void) {   // the goals of this life, ticked off as they are done
    life_draw();
    ui_dim();
    if (story_id && !goals_old) { story_journal_draw(frame); return; }   // (a story: its journal)
    char b[48], isl[40];
    sprintf(isl, "SAIL TO %s", world_zone_name(ZONE_ISLAND));
    enum { NG = 8 };
    struct { const char *txt; int done; } g[NG] = {
        { "EARN A PAYCHECK (PHONE, JOBS)", story_paid > 0 },
        { isl, story_visit[1] },
        { "GET PROMOTED TWICE", job_level >= 2 },
        { "SEE THE DESERT TOWN", story_visit[2] },
        { "MAKE FRIENDS: CHAT 5 TIMES", story_talks >= 5 },
        { "SAVE UP $20000", cash >= 20000 },
        { "BUY A HOME (PHONE, HOMES)", home_city },
        { "FIND THE ISLAND'S 8 SECRETS", isl_secret_count() >= SEC_N },
    };
    int top = ui_window(30, 10, 260, 220, "THE STORY SO FAR", IC_BOOK), done = 0;
    for (int i = 0; i < NG; i++) {
        int y = top + 2 + i * 20, d = g[i].done;
        ui_round(38, y, 244, 18, 6, d ? UI_RGB(234, 249, 234) : UI_WHITE, d ? UI_RGB(212, 238, 212) : UI_PANEL, 0);
        if (d) ui_icon_disc(49, y + 9, 15, IC_TICK, UI_GREEN, UI_WHITE);
        else { ui_disc(49, y + 9, 15, UI_RGB(186, 196, 210)); ui_disc(49, y + 9, 11, UI_WHITE); }
        ui_text(62, y + 5, 1, g[i].txt, d ? UI_GOODT : UI_INK);
        done += d;
    }
    sprintf(b, "%d OF %d DONE", done, NG);
    ui_text(40, top + 165, 1, b, UI_INK);
    ui_bar(124, top + 165, 156, 7, done * 100 / NG, UI_GREEN);
    ui_hints(160, top + 180, story_id ? "TRI THE STORY|O BACK" : "SQ PICK A STORY|O BACK", 1);
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
    int top = ui_window(4, 4, 312, 232, b, IC_ARROW);
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
        if (!any) ui_text(168, y, 1, "FEELING PRETTY ORDINARY", UI_SOFT);
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
        int raw[8] = { s->skill_cook, s->skill_body, s->skill_fun, s->skill_band[0], s->skill_band[1], s->skill_band[2], s->skill_band[3], s->skill_band[4] };
        ui_section(12, y0, 294, "SKILLS");
        for (int i = 0; i < 8; i++) { int y = y0 + 11 + i * 10, lv = wants_skill_level(raw[i]);
            ui_text(12, y, 1, i < 3 ? skNm[i] : band_skill_name[i - 3], UI_INK); ui_pips(108, y, lv, 10, i < 3 ? UI_AMBER : UI_RED); sprintf(b, "LEVEL %d", lv); ui_text(236, y, 1, b, UI_SOFT); }
        ui_section(12, y0 + 94, 294, "PERSONALITY");
        for (int i = 0; i < TR_N; i++) { int y = y0 + 105 + i * 10;
            ui_text(12, y, 1, look_trait_name(i), UI_INK); ui_pips(108, y, s->bore.tr[i], 10, UI_BLUE); }
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
        if (job_none) { ui_icon_disc(30, y0 + 13, 28, IC_CASE, UI_RGB(150, 156, 170), UI_WHITE); ui_text(52, y0 + 1, 2, "LOOKING FOR WORK", UI_INK);
                        ui_wrap(12, y0 + 34, 296, 3, "NO JOB YET. THE PHONE'S JOBS HAS TODAY'S CLASSIFIEDS, AND A SMARTPHONE OR THE COMPUTER HAS THEM ALL.", UI_SOFT); }
        else {
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
    }
    gfx_rect(10, 212, 300, 1, 200, 210, 222, 0);
    ui_hints(160, 217, "L1 R1 TAB|SEL NEXT BORE|O BACK", 1);
}

// ---- the creator ----
// ---- the name keyboard (a letter grid, like the PC games' name boxes on a pad): the Bore's name, the capital's name ----
static char *nk_buf; static int nk_cap, nk_back, nk_r, nk_c, nk_t; static const char *nk_title; static void (*nk_done)(void);
static char nk_text[24];
static const char nk_keys[4][11] = { "ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ0123", "456789 -'." };
static void confirm_update(void) {
    cf_t++;
    if (PRESSED(PAD_LEFT) || PRESSED(PAD_RIGHT) || PRESSED(PAD_UP) || PRESSED(PAD_DOWN)) cf_sel ^= 1;
    if (PRESSED(PAD_CIRCLE)) { state = cf_back; return; }
    if (PRESSED(PAD_CROSS)) { state = cf_back; if (cf_sel == 0 && cf_yes) { audio_voice(VOICE_AGREE); cf_yes(); } }
}
static void confirm_draw(void) {
    if (cf_back == ST_NHOOD) { gfx_set_clear(188, 204, 222); gfx_camera_update(); ui_backdrop(); } else { life_draw(); ui_dim(); }
    int nl = ui_wrap_n(236, cf_body), h = 58 + nl * 10;
    int top = ui_window(30, 120 - h / 2 - 10, 260, h + 20, cf_title, IC_HELP);
    ui_wrap(42, top + 4, 236, 6, cf_body, UI_INK);
    int by = top + 10 + nl * 10;
    ui_button(42, by, 112, 18, "YES", IC_TICK, cf_sel == 0);
    ui_button(166, by, 112, 18, "NO, GO BACK", IC_BX + 1, cf_sel == 1);
}
static void name_open(char *buf, int cap, const char *title, void (*done)(void)) {   // edit buf (at most cap letters); done runs when it is kept
    nk_buf = buf; nk_cap = MIN(cap, 23); nk_title = title; nk_done = done; nk_back = state; nk_r = 0; nk_c = 0; nk_t = 0;
    strncpy(nk_text, buf, nk_cap); nk_text[nk_cap] = 0;
    state = ST_NAME;
}
static void name_update(void) {
    nk_t++;
    int len = (int)strlen(nk_text);
    if (PRESSED(PAD_UP)) nk_r = (nk_r + 4) % 5;
    if (PRESSED(PAD_DOWN)) nk_r = (nk_r + 1) % 5;
    if (PRESSED(PAD_LEFT)) nk_c = (nk_c + 9) % 10;
    if (PRESSED(PAD_RIGHT)) nk_c = (nk_c + 1) % 10;
    if (PRESSED(PAD_CIRCLE)) { state = nk_back; return; }                         // (left as it was)
    if (PRESSED(PAD_SQUARE) && len) { nk_text[len - 1] = 0; return; }
    int done = PRESSED(PAD_START);
    if (PRESSED(PAD_CROSS)) {
        if (nk_r < 4) { if (len < nk_cap) { nk_text[len] = nk_keys[nk_r][nk_c]; nk_text[len + 1] = 0; } }
        else if (nk_c < 4) { if (len) nk_text[len - 1] = 0; }                     // DELETE
        else done = 1;                                                            // DONE
    }
    if (done) {
        while (nk_text[0] == ' ') memmove(nk_text, nk_text + 1, strlen(nk_text));
        len = (int)strlen(nk_text); while (len && nk_text[len - 1] == ' ') nk_text[--len] = 0;
        if (!len) { audio_voice(VOICE_NAH); return; }                           // (a name, at least a letter)
        strcpy(nk_buf, nk_text); state = nk_back; audio_voice(VOICE_AGREE);
        if (nk_done) nk_done();
    }
}
static void name_draw(void) {
    gfx_set_clear(188, 204, 222);
    gfx_camera_update();
    ui_backdrop();
    int top = ui_window(20, 26, 280, 186, nk_title, IC_BRUSH);
    ui_round(30, top + 2, 260, 20, 6, UI_WHITE, UI_PANEL, 0);
    char b[32]; snprintf(b, sizeof b, "%s%s", nk_text, (nk_t >> 4) & 1 ? "_" : " ");
    ui_text(38, top + 8, 1, b, UI_INK);
    sprintf(b, "%d/%d", (int)strlen(nk_text), nk_cap); ui_text(282 - ui_tw(1, b), top + 8, 1, b, UI_SOFT);
    for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) {
        int x = 34 + c * 25, y = top + 30 + r * 22, on = r == nk_r && c == nk_c;
        ui_round(x, y, 22, 19, 4, on ? UI_BLUE : UI_PANEL, on ? UI_BLUE2 : UI_HILITE, 0);
        char k[2] = { nk_keys[r][c], 0 }; if (k[0] == ' ') { k[0] = '_'; }
        ui_textc(x + 11, y + 6, 1, k, on ? UI_WHITE : UI_INK);
    }
    { int y = top + 30 + 4 * 22; int on = nk_r == 4;
      ui_button(34, y, 97, 18, "DELETE", IC_BX + 3, on && nk_c < 4);
      ui_button(140, y, 144, 18, "DONE", IC_TICK, on && nk_c >= 4); }
    ui_hints(160, top + 152, "X TYPE|SQ DELETE|START DONE|O CANCEL", 1);
}
static void create_named(void) {   // "FIRST LAST" back into the creator's Bore
    char t[24]; strcpy(t, nk_text);
    char *sp = strchr(t, ' ');
    if (sp) { *sp = 0; strncpy(edit.last, sp + 1, NAME_LEN - 1); edit.last[NAME_LEN - 1] = 0; }
    strncpy(edit.first, t, NAME_LEN - 1); edit.first[NAME_LEN - 1] = 0;
}
static char create_nm[24];
static void create_update(void) {
#ifdef AUTO_FACE
    {   // (test builds: every face part in turn, a third of a second each, face on)
        static int t0 = -1, last = -1, cat = AUTO_FACE, item = 0; static const u8 ids[9] = { LK_EYES, LK_MOUTH, LK_BROW, LK_NOSE, LK_CHEEK, LK_GLASS, LK_X_MAKEUP, LK_X_LIPS, LK_EARS };
        if (t0 < 0) t0 = VSync(-1);
        int k_ = (VSync(-1) - t0) / 20;
        if (k_ != last && cat < 9) { last = k_;
            if (++item >= look_count(ids[cat])) { edit.look[ids[cat]] = ids[cat] == LK_EYES ? 1 : 0; cat++; item = 1; }
            if (cat < 9) { edit.look[ids[cat]] = (u8)item; if (ids[cat] == LK_X_MAKEUP) edit.look[LK_EYES] = 1; look_build(&edit); create_tab = 1; create_row = 0;
                create_turn = ids[cat] == LK_EARS ? ((k_ & 1) ? 150 : 0) : 0; } }
        return;
    }
#endif
#ifdef AUTO_WEAR
    {   // (test builds: every outfit in turn, a little over half a second each, turning slowly)
        static int t0 = -1, last = -1;
        if (t0 < 0) t0 = VSync(-1);
        int k_ = (VSync(-1) - t0) / 40;
        if (k_ != last) { last = k_; int t = 2 + k_ % (look_count(LK_X_THEME) - 2);
            edit.look[LK_X_THEME] = (u8)t; look_theme(&edit, t); look_build(&edit); create_tab = 4; create_row = 0; create_turn = (k_ & 1) ? 2400 : 400; }
        return;
    }
#endif
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
            if (r->id == LK_X_THEME) look_theme(&edit, v);   // (an outfit: dressed all at once)
            look_build(&edit);
        }
        audio_voice(VOICE_AGREE);
    }
    if (PRESSED(PAD_TRIANGLE)) { look_random(&edit); audio_voice(VOICE_LAUGH); }
    if (PRESSED(PAD_SQUARE)) { snprintf(create_nm, sizeof create_nm, "%s %s", edit.first, edit.last); name_open(create_nm, 23, "NAME YOUR BORE: FIRST AND LAST", create_named); return; }
    if (HELD(PAD_L2)) create_turn -= 40;
    if (HELD(PAD_R2)) create_turn += 40;
    if (analog) create_turn += rs_x / 3;
    if (PRESSED(PAD_START) || PRESSED(PAD_SELECT)) {
        world_restore();
        if (prev_state == ST_LIFE) { Sim *me = &sims[player]; if (me->bore.stage != edit.stage) { me->age_days = 0; me->bday = 0; }
            TexRef te = me->bore.tex_eye, tm = me->bore.tex_mouth; me->bore = edit; me->bore.tex_eye = te; me->bore.tex_mouth = tm; look_build(&me->bore);   // (on the Bore's own faces)
            crowd_forget(CROWD_MAX + player); state = ST_LIFE; }
        else { worlds_open(); }   // (a new Bore from the title: on to PICK A WORLD)
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
    voxel_draw(&edit, &p, (2048 - create_turn) & 4095, &g);   // (the face to the camera at create_turn 0)
    gfx_shadow(&p, 70);
    // the card: the tab on a blue header, its rows, the buttons at the foot
    static const s8 createIc[NTAB] = { IC_PEOPLE, IC_SMILE, IC_BRUSH, IC_SHIRT, IC_STAR, IC_SPROUT, IC_ARROW };
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
    ui_hints(241, 205, "L1 R1 TAB|TRI DICE", 1);
    ui_hints(241, 217, prev_state == ST_LIFE ? "SQ NAME|START DONE" : "SQ NAME|START LIVE", 1);
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
static const char *const toolNm[NTOOL] = { "BUY", "ROOM", "WALL", "DOORS, WINDOWS", "FLOOR", "WALLPAPER", "ROOF" };
static const char openCh[4] = { 'D', 'E', 'f', 'e' };                       // the DOORS tool: what goes into a wall
static const char *const openNm[4] = { "DOOR", "WINDOW", "HIGH WINDOW", "TINTED WINDOW" };
static const u16 openCost[4] = { 100, 60, 50, 70 };
static const char *open_name(int i) { return openNm[i & 3]; }
static void build_say(const char *t) { strncpy(build_msg, t, 31); build_msg[31] = 0; build_msg_t = 150; build_msg_ok = 0; }
static void build_say_ok(const char *t) { build_say(t); build_msg_ok = 1; }
static void carry_return(void) {   // an item being moved goes back where it came from (or, if that is taken, sells)
    if (!build_carry) return;
    if (world_at_l(build_cl, build_cx, build_cy) == '.') { *world_cell(build_cl, build_cx, build_cy) = build_carry; if (items_rotates(build_carry)) *world_wall_cell(build_cl, build_cx, build_cy) = build_crot >= 0 ? (u8)(0x80 | build_crot) : 0; }
    else cash += items_cost(build_carry) * 3 / 4;
    build_carry = 0; world_touched();
}
static int is_wallish(char c) { return c == 'W' || c == 'w' || c == 'E' || c == 'e' || c == 'f' || c == 'D'; }
static int build_ok(int x, int y) { return world_zone != ZONE_CITY || world_own_lot(x, y); }   // in SAN BOREAS: on your own lot only
static int room_rect(int *x0, int *y0, int *x1, int *y1) {   // the ROOM tool's rectangle: from the first corner to the cursor
    *x0 = MIN(room_ax, build_x); *x1 = MAX(room_ax, build_x); *y0 = MIN(room_ay, build_y); *y1 = MAX(room_ay, build_y);
    return *x1 - *x0 >= 2 && *y1 - *y0 >= 2 && *x1 - *x0 < 16 && *y1 - *y0 < 16;
}
static int room_opens(int L, int x0, int y0, int x1, int y1, int *ox, int *oy, char *oc) {   // where the ROOM tool puts its door and windows
    int n = 0; if (!room_open) return 0;
#define ADDOPEN(X, Y, C) do { char e_ = world_at_l(L, X, Y); if (n < 16 && (e_ == '.' || e_ == ' ')) { ox[n] = (X); oy[n] = (Y); oc[n] = (C); n++; } } while (0)
    int mid = (x0 + x1) / 2, midy = (y0 + y1) / 2;
    if (!L) ADDOPEN(mid, y1, 'D');   // (the front wall; upstairs there is no outside door)
    if (room_open > 1) { for (int x = x0 + 2; x <= x1 - 2; x += 3) if (x != mid || L) ADDOPEN(x, y0, 'E'); ADDOPEN(x0, midy, 'E'); ADDOPEN(x1, midy, 'E'); }
#undef ADDOPEN
    return n;
}
static int room_cost(int L, int x0, int y0, int x1, int y1) {   // new walls $50, floor $10 a tile, and the door and windows
    int c = 0;
    { int ox[16], oy[16], n; char oc[16]; n = room_opens(L, x0, y0, x1, y1, ox, oy, oc); for (int i = 0; i < n; i++) c += oc[i] == 'D' ? openCost[0] : openCost[1]; }
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        char ch = world_at_l(L, x, y);
        if (x == x0 || x == x1 || y == y0 || y == y1) { if (!is_wallish(ch)) c += 50; } else if (!(L && ch == ' ' && world_at_l(L - 1, x, y) == '|')) c += 10;
    }
    return c;
}
static void room_build(void) {   // walls round it with the wallpaper picked, the floor picked inside
    int x0, y0, x1, y1, L = build_lvl;
    if (!room_rect(&x0, &y0, &x1, &y1)) { build_say("A ROOM IS 3 TO 16 TILES A SIDE"); audio_voice(VOICE_NAH); return; }
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        if (!build_ok(x, y)) { build_say("BUILD ON YOUR OWN LOT ONLY"); audio_voice(VOICE_NAH); return; }
        char ch = world_at_l(L, x, y), below = L ? world_at_l(L - 1, x, y) : 'W';
        int edge = x == x0 || x == x1 || y == y0 || y == y1;
        if (L && ch == ' ' && below == '|' && !edge) continue;                                  // (a stairwell stays open)
        if (L && ch == ' ' && (below == ' ' || below == '|')) { build_say("NOTHING UNDER IT TO HOLD IT UP"); audio_voice(VOICE_NAH); return; }
        if (edge && !is_wallish(ch) && ch != '.' && ch != ' ') { build_say("SOMETHING IS IN THE WAY"); audio_voice(VOICE_NAH); return; }
        if (edge && !is_wallish(ch)) for (int i = 0; i < nsims; i++) if (sims[i].lvl == L && sim_tile_x(&sims[i]) == x && sim_tile_y(&sims[i]) == y) { build_say("SOMEONE IS STANDING THERE"); audio_voice(VOICE_NAH); return; }
    }
    int cost = room_cost(L, x0, y0, x1, y1);
    if (cash < cost) { char t[32]; sprintf(t, "THAT ROOM IS $%d", cost); build_say(t); audio_voice(VOICE_NAH); return; }
    int ox[16], oy[16], no; char oc[16]; no = room_opens(L, x0, y0, x1, y1, ox, oy, oc);   // (before anything changes)
    room_undo.on = 1; room_undo.L = L; room_undo.x0 = x0; room_undo.y0 = y0; room_undo.x1 = x1; room_undo.y1 = y1; room_undo.spent = cost;
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { int k = (y - y0) * 16 + (x - x0); room_undo.m[k] = world_at_l(L, x, y); room_undo.f[k] = *world_floor_cell(L, x, y); room_undo.w[k] = *world_wall_cell(L, x, y); }
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        char *c = world_cell(L, x, y);
        if (x == x0 || x == x1 || y == y0 || y == y1) { if (!is_wallish(*c)) { *c = 'W'; *world_wall_cell(L, x, y) = (u8)build_wp; } }
        else if (*c == ' ' && !(L && world_at_l(L - 1, x, y) == '|')) *c = '.';   // (not over the stairs)
        *world_floor_cell(L, x, y) = (u8)build_fl;
    }
    for (int i = 0; i < no; i++) { *world_cell(L, ox[i], oy[i]) = oc[i]; *world_wall_cell(L, ox[i], oy[i]) = (u8)build_wp; }
    cash -= cost; room_on = 0; world_touched(); audio_voice(VOICE_FINISHED);
    wants_event(&sims[player], W_BUY, 0);
    if (no) build_say_ok("ROOM BUILT. TRIANGLE UNDOES"); else build_say("ROOM BUILT. ADD A DOOR!");
}
static void room_undo_do(void) {   // take the last room back, and its cost
    if (!room_undo.on) { build_say("NOTHING TO UNDO"); audio_voice(VOICE_NAH); return; }
    for (int y = room_undo.y0; y <= room_undo.y1; y++) for (int x = room_undo.x0; x <= room_undo.x1; x++) {
        int k = (y - room_undo.y0) * 16 + (x - room_undo.x0);
        *world_cell(room_undo.L, x, y) = room_undo.m[k]; *world_floor_cell(room_undo.L, x, y) = room_undo.f[k]; *world_wall_cell(room_undo.L, x, y) = room_undo.w[k];
    }
    cash += room_undo.spent; room_undo.on = 0; world_touched(); audio_voice(VOICE_FINISHED); build_say_ok("ROOM UNDONE");
}
static int region_fill(int L, int sx, int sy) {   // the closed room round a tile, left in bfs_q: its tile count, or -1 when it is not closed (over 600 tiles, or out to the map's edge)
    char c0 = world_at_l(L, sx, sy); if (is_wallish(c0) || c0 == ' ') return -1;
    static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    memset(fill_seen, 0, sizeof fill_seen);
    int qh = 0, qt = 0, id0 = sy * MW + sx; bfs_q[qt++] = (u16)id0; fill_seen[id0 >> 3] |= 1 << (id0 & 7);
    while (qh < qt) {
        int c = bfs_q[qh++], x = c % MW, y = c / MW;
        if (x <= 1 || y <= 1 || x >= MW - 2 || y >= MH - 2) return -1;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d], id = ny * MW + nx; char ch = world_at_l(L, nx, ny);
            if (is_wallish(ch) || ch == ' ' || (fill_seen[id >> 3] >> (id & 7) & 1)) continue;
            fill_seen[id >> 3] |= 1 << (id & 7);
            if (qt >= 600) return -1;
            bfs_q[qt++] = (u16)id;
        }
    }
    return qt;
}
static void room_fill_tool(void) {   // TRIANGLE with the FLOOR tool: lay the floor over the whole room; with WALLPAPER: paper all its walls
    int L = build_lvl, n = region_fill(L, build_x, build_y);
    if (n < 0) { build_say("NOT A CLOSED ROOM"); audio_voice(VOICE_NAH); return; }
    if (build_tool == BT_FLOOR) {
        int cost = 0; for (int i = 0; i < n; i++) { int x = bfs_q[i] % MW, y = bfs_q[i] / MW; if (build_ok(x, y) && *world_floor_cell(L, x, y) != build_fl) cost += 10; }
        if (!cost) { build_say("ALREADY LIKE THAT"); audio_voice(VOICE_NAH); return; }
        if (cash < cost) { char t[32]; sprintf(t, "THE FLOOR IS $%d", cost); build_say(t); audio_voice(VOICE_NAH); return; }
        for (int i = 0; i < n; i++) { int x = bfs_q[i] % MW, y = bfs_q[i] / MW; if (build_ok(x, y)) *world_floor_cell(L, x, y) = (u8)build_fl; }
        cash -= cost; { char t[32]; sprintf(t, "FLOORED  -$%d", cost); build_say_ok(t); }
    } else {
        static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        int k = 0;
        for (int i = 0; i < n; i++) { int x = bfs_q[i] % MW, y = bfs_q[i] / MW;
            for (int d = 0; d < 4; d++) { int nx = x + dx[d], ny = y + dy[d]; if (build_ok(nx, ny) && is_wallish(world_at_l(L, nx, ny))) { u8 *w = world_wall_cell(L, nx, ny); if (*w != (u8)build_wp) { *w = (u8)build_wp; k++; } } } }
        if (!k) { build_say("ALREADY PAPERED"); audio_voice(VOICE_NAH); return; }
        build_say_ok("ROOM PAPERED");
    }
    world_touched(); audio_voice(VOICE_FINISHED);
}
static int floor_supported(int l, int x, int y) {   // an upper floor needs something under it, or a floor beside it on its own storey
    if (l == 0) return 1;
    char below = world_at_l(l - 1, x, y);
    if (below != ' ' && below != '|') return 1;
    static const signed char dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    for (int k = 0; k < 4; k++) { char c = world_at_l(l, x + dx[k], y + dy[k]); if (c != ' ') return 1; }
    return 0;
}
// ---- the catalog (Sims buy mode): the items in tabs, a scrolling list with prices ----
#define NCAT 10
static const char *const catNm[NCAT] = { "SEATS, BEDS", "TABLES, STORAGE", "KITCHEN, BATH", "FUN AND TECH", "DECOR, PLANTS", "GARDEN, OUTDOOR", "SKATE", "BOREAGOTCHI", "PARTY", "SPECIAL" };
static const char *const catSet[NCAT - 1] = {
    "CUh;S<N%z\x81" "\220\221\222\223\250" ,
    "tdibyK?#O" "\225\226\227\230\231\232\233" ,
    "F/:cTH,->&Y`" "\234\235\237" ,
    "vARj[!$()QImn]_{" "\240\241\242\243\244\245\246\247\251" ,
    "rlpZVqo}" "\252\253\254\255\256\257\260\261" ,
    "gus*036~\"" "\224\236\262\263\264\265\266\267\270\271\272\273\274\275" ,
    "=LJXM159GB",
    "\276\277\300\301\302\303\304\305\306",
    "\307\310\311\312\313\314\315\316" };
static int item_cat(char ch) { if ((u8)ch > 0x81 && (u8)ch < 0x90) return -1; /* (the cake, the gravestone and the island's own are never bought) */ for (int c = 0; c < NCAT - 1; c++) if (strchr(catSet[c], ch)) return c; return NCAT - 1; }
static int cat_list(int cat, int *idx) {   // the tab's items, cheapest first
    int n = 0; for (int i = 0; i < items_count() && n < 96; i++) if (item_cat(items_at(i)) == cat) idx[n++] = i;
    for (int i = 1; i < n; i++) { int v = idx[i], c = items_cost(items_at(v)), j = i - 1; while (j >= 0 && items_cost(items_at(idx[j])) > c) { idx[j + 1] = idx[j]; j--; } idx[j + 1] = v; }
    return n;
}
static void catalog_step(int dir) {   // L1 R1: through the tab, and on into the next tab at the end
    int idx[96], cat = item_cat(items_at(build_item)), n = cat_list(cat, idx), p = 0;
    for (int k = 0; k < n; k++) if (idx[k] == build_item) p = k;
    p += dir;
    if (p >= 0 && p < n) { build_item = idx[p]; return; }
    int nc = cat;
    for (int g = 0; g < NCAT; g++) { nc = (nc + NCAT + dir) % NCAT; n = cat_list(nc, idx); if (n) break; }
    if (n) build_item = idx[dir > 0 ? 0 : n - 1];
}
static void catalog_next_cat(void) {   // R3: jump to the next tab
    int idx[96], cat = item_cat(items_at(build_item));
    for (int g = 1; g <= NCAT; g++) { int nc = (cat + g) % NCAT; if (cat_list(nc, idx)) { build_item = idx[0]; return; } }
}
static void draw_catalog(void) {
    int idx[96], cat = item_cat(items_at(build_item)), n = cat_list(cat, idx), p = 0;
    char b[24];
    for (int k = 0; k < n; k++) if (idx[k] == build_item) p = k;
    int top = ui_window_s(196, 4, 120, 168, catNm[cat]);
    for (int c = 0; c < NCAT; c++) ui_disc(220 + c * 11, top + 2, 6, c == cat ? UI_BLUE : UI_LINE);   // which of the tabs
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
static int build_intro, build_last_frame = -99; static long build_last_cash, build_delta; static int build_delta_t;   // the BUILD MODE banner; the running +$ / -$
static int build_tile_state(void) {   // what X would do on the tile under the cursor: 0 not allowed (red), 1 go ahead (green), 2 take away or replace (orange)
    if (!build_ok(build_x, build_y)) return 0;
    int L = build_lvl; char c = world_at_l(L, build_x, build_y);
    for (int i = 0; i < nsims; i++) if (sims[i].lvl == L && sim_tile_x(&sims[i]) == build_x && sim_tile_y(&sims[i]) == build_y && build_tool != BT_PAINT && build_tool != BT_DOOR && build_tool != BT_ROOM) return 0;
    switch (build_tool) {
    case BT_BUY:   if (c == '.') return build_carry || cash >= items_cost(items_at(build_item)) ? 1 : 0;
                   if (build_carry) return 0;
                   return (c != ' ' && c != 'P' && items_is_item(c)) ? 2 : 0;
    case BT_WALL:  if (c == '.' || (c == ' ' && floor_supported(L, build_x, build_y))) return cash >= 50 ? 1 : 0;
                   return is_wallish(c) ? 2 : 0;
    case BT_FLOOR: if (c == ' ' && !floor_supported(L, build_x, build_y)) return 0;
                   return cash >= 10 ? 1 : 0;
    case BT_PAINT: case BT_DOOR: return is_wallish(c) ? 1 : 0;
    default: return 1;
    }
}
static void build_bar(int xmin, int zmax, int w, int h, int yy, int r, int g, int b) {   // a flat strip on the floor: x from xmin, z from zmax back
    SVECTOR a, bb, c, d;
    setVector(&a, xmin, yy, zmax); setVector(&bb, xmin + w, yy, zmax); setVector(&c, xmin, yy, zmax - h); setVector(&d, xmin + w, yy, zmax - h);
    gfx_quad_f(&a, &bb, &c, &d, r, g, b, 0);
}
static void build_update(void) {
    static int rep;
    if (frame - build_last_frame > 4) { room_undo.on = 0; build_intro = 90; build_last_cash = cash; build_delta = 0; build_delta_t = 0; }   // (just came in)
    build_last_frame = frame;
    if (cash != build_last_cash) { build_delta += cash - build_last_cash; build_last_cash = cash; build_delta_t = 80; }   // what the last buy or sale came to
    if (build_delta_t > 0 && --build_delta_t == 0) build_delta = 0;
    if (build_intro > 0) build_intro--;
    int dx = 0, dy = 0;
    int l = HELD(PAD_LEFT) || ls_x < -40, r = HELD(PAD_RIGHT) || ls_x > 40, u = HELD(PAD_UP) || ls_y < -40, d = HELD(PAD_DOWN) || ls_y > 40;
    if (l || r || u || d) { if (rep == 0 || rep > 12) { if (l) dx = -1; if (r) dx = 1; if (u) dy = -1; if (d) dy = 1; if (rep > 12) rep = 9; } rep++; } else rep = 0;
    if (dx || dy) {   // move with the camera: "up" is away from it
        int c = icos(cam_yaw), sn = isin(cam_yaw);
        int vx = dx * c - (-dy) * sn, vz = (-dy) * c + dx * sn;
        if (ABS(vx) >= ABS(vz)) build_x += vx > 0 ? 1 : -1; else build_y += vz > 0 ? -1 : 1;
        build_x = CLAMP(build_x, 1, MW - 2); build_y = CLAMP(build_y, 1, MH - 2);
    }
    if (build_carry && (PRESSED(PAD_CIRCLE) || PRESSED(PAD_SQUARE))) { carry_return(); build_say_ok("PUT BACK"); audio_voice(VOICE_AGREE); return; }
    if (build_carry && (PRESSED(PAD_START) || PRESSED(PAD_SELECT))) carry_return();
    if (PRESSED(PAD_SELECT)) { build_tool = (build_tool + 1) % NTOOL; room_on = 0; room_undo.on = 0; }
    if (build_msg_t > 0) build_msg_t--;
    if (PRESSED(PAD_R2) && build_lvl < NUP) { build_lvl++; audio_voice(VOICE_AGREE); }     // the storey you build on
    if (PRESSED(PAD_L2) && build_lvl > 0) { build_lvl--; audio_voice(VOICE_AGREE); }
    if (PRESSED(PAD_L3)) build_peek ^= 1;                                                  // see the roof
    if (analog) { cam_yaw += rs_x / 3; cam_pitch += rs_y / 4; cam_pitch = CLAMP(cam_pitch, 300, 1000); }
    if (build_tool == BT_BUY) { if (PRESSED(PAD_L1)) catalog_step(-1); if (PRESSED(PAD_R1)) catalog_step(1); if (PRESSED(PAD_R3)) catalog_next_cat(); }
    if (build_tool == BT_BUY && PRESSED(PAD_TRIANGLE)) {   // TRIANGLE turns the item under the cursor, the one being moved, or the next one bought (AUTO, then the four ways)
        char under = world_at_l(build_lvl, build_x, build_y);
        if (!build_carry && under != '.' && items_is_item(under) && items_rotates(under) && build_ok(build_x, build_y)) {
            int keep = world_lvl; world_lvl = build_lvl; int f = world_item_facing(build_x, build_y); world_lvl = keep;
            *world_wall_cell(build_lvl, build_x, build_y) = (u8)(0x80 | ((f + 1) & 3)); world_touched(); audio_voice(VOICE_AGREE);
        } else if (build_carry) { build_crot = build_crot >= 3 ? -1 : build_crot + 1; audio_voice(VOICE_AGREE); }
        else { build_rot = build_rot >= 3 ? -1 : build_rot + 1; build_say_ok(build_rot < 0 ? "TURN: AUTOMATIC" : build_rot == 0 ? "TURN: SOUTH" : build_rot == 1 ? "TURN: EAST" : build_rot == 2 ? "TURN: NORTH" : "TURN: WEST"); audio_voice(VOICE_AGREE); }
        return;
    }
    if (build_tool == BT_FLOOR) { if (PRESSED(PAD_L1)) build_fl = (build_fl + NFLOORS - 1) % NFLOORS; if (PRESSED(PAD_R1)) build_fl = (build_fl + 1) % NFLOORS; }
    if (build_tool == BT_PAINT || build_tool == BT_WALL || build_tool == BT_ROOM) { if (PRESSED(PAD_L1)) build_wp = (build_wp + NWALLS - 1) % NWALLS; if (PRESSED(PAD_R1)) build_wp = (build_wp + 1) % NWALLS; }
    if (build_tool == BT_ROOM && PRESSED(PAD_R3)) build_fl = (build_fl + 1) % NFLOORS;   // (the room's floor)
    if (build_tool == BT_DOOR) { if (PRESSED(PAD_L1)) build_open = (build_open + 3) & 3; if (PRESSED(PAD_R1)) build_open = (build_open + 1) & 3; }
    if (build_tool == BT_ROOF) {   // the roof: L1 R1 colour, X style, SQUARE pitch
        if (PRESSED(PAD_L1)) { world_roof_col = (world_roof_col + NROOF_COL - 1) % NROOF_COL; world_touched(); }
        if (PRESSED(PAD_R1)) { world_roof_col = (world_roof_col + 1) % NROOF_COL; world_touched(); }
        if (PRESSED(PAD_CROSS)) { world_roof_style = (world_roof_style + 1) % NROOF_STYLE; world_touched(); audio_voice(VOICE_FINISHED); }
        if (PRESSED(PAD_SQUARE)) { world_roof_pitch = (world_roof_pitch + 1) % NROOF_PITCH; world_touched(); audio_voice(VOICE_FINISHED); }
        if (PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; state = ST_LIFE; }
        return;
    }
    int L = build_lvl;
    if ((build_tool == BT_FLOOR || build_tool == BT_PAINT) && PRESSED(PAD_TRIANGLE)) { if (build_ok(build_x, build_y)) room_fill_tool(); else { build_say(home_city ? "NOT YOUR LOT" : "BUY A HOME TO BUILD HERE"); audio_voice(VOICE_NAH); } return; }
    if (build_tool == BT_ROOM) {   // the ROOM tool: X a corner, X the other corner; O lets go of the first
        if (PRESSED(PAD_CROSS)) { if (!room_on) { if (build_ok(build_x, build_y)) { room_on = 1; room_ax = build_x; room_ay = build_y; audio_voice(VOICE_AGREE); } else { build_say(home_city ? "BUILD ON YOUR OWN LOT ONLY" : "BUY A HOME TO BUILD HERE"); audio_voice(VOICE_NAH); } }
                                  else room_build(); }
        if (PRESSED(PAD_CIRCLE) && room_on) { room_on = 0; return; }
        if (PRESSED(PAD_SQUARE)) { room_open = (room_open + 1) % 3; build_say_ok(roomOpenNm[room_open]); audio_voice(VOICE_AGREE); }
        if (PRESSED(PAD_TRIANGLE)) room_undo_do();
        if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; room_on = 0; state = ST_LIFE; }
        return;
    }
    if ((PRESSED(PAD_CROSS) || PRESSED(PAD_SQUARE)) && !build_ok(build_x, build_y)) {   // in SAN BOREAS: not someone else's
        build_say(home_city ? "BUILD ON YOUR OWN LOT ONLY" : "BUY A HOME TO BUILD HERE"); audio_voice(VOICE_NAH);
        if (PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; state = ST_LIFE; }
        return;
    }
    if (HELD(PAD_CROSS) && !build_ok(build_x, build_y)) return;
    char *cell = world_cell(L, build_x, build_y);
    int occupied = 0; for (int i = 0; i < nsims; i++) if (sims[i].lvl == L && sim_tile_x(&sims[i]) == build_x && sim_tile_y(&sims[i]) == build_y) occupied = 1;
    if (build_tool == BT_DOOR && (PRESSED(PAD_CROSS) || PRESSED(PAD_SQUARE))) {   // into a wall, and out of it again
        int ok = 0;
        if (PRESSED(PAD_CROSS)) {
            char want = openCh[build_open & 3]; int cost = openCost[build_open & 3];
            if (!is_wallish(*cell)) build_say("PUT IT IN A WALL");
            else if (*cell == want) ok = 1;
            else if (cash < cost) build_say("NOT ENOUGH MONEY");
            else { *cell = want; cash -= cost; ok = 1; }
        } else if (*cell == 'D' || *cell == 'E' || *cell == 'e' || *cell == 'f') { *cell = 'W'; cash += 30; ok = 1; }   // (a wall again)
        audio_voice(ok ? VOICE_FINISHED : VOICE_NAH); world_touched();
        return;
    }
    if (PRESSED(PAD_CROSS) && !occupied) {
        int ok = 0;
        if (build_tool == BT_BUY) { char ch = build_carry ? build_carry : items_at(build_item); int cost = build_carry ? 0 : items_cost(ch), rr = build_carry ? build_crot : build_rot;
            if (*cell == '.') {
                if (cash < cost) build_say("NOT ENOUGH MONEY");
                else if (ch == '|') {   // stairs: there must be a storey above, and its tile over the stairs becomes the stairwell
                    if (L >= NUP) { build_say("NO STOREY ABOVE THE TOP ONE"); }
                    else { char *up = world_cell(L + 1, build_x, build_y); if (*up == '.' || *up == ' ') { *up = ' '; *cell = ch; cash -= cost; ok = 1; } }
                } else {
                    *cell = ch; if (items_rotates(ch)) *world_wall_cell(L, build_x, build_y) = rr >= 0 ? (u8)(0x80 | rr) : 0;
                    cash -= cost; ok = 1;
                    if (build_carry) { build_carry = 0; build_say_ok("MOVED"); }
                    else { char t_[32]; wants_event(&sims[player], W_BUY, 0); sprintf(t_, "BOUGHT  -$%d", cost); build_say_ok(t_); }
                }
            } else if (!build_carry && items_is_item(*cell) && items_cost(*cell) > 0 && *cell != '|' && *cell != 'P') {   // X on an item picks it up to move it
                char pc = *cell; int keep = world_lvl; world_lvl = L; int f = world_item_facing(build_x, build_y); world_lvl = keep;
                build_carry = pc; build_crot = items_rotates(pc) ? f : -1; build_cx = build_x; build_cy = build_y; build_cl = L;
                *cell = '.'; if (items_rotates(pc)) *world_wall_cell(L, build_x, build_y) = 0;
                ok = 1; build_say_ok("X PLACES IT, O PUTS IT BACK");
            } }
        if (build_tool == BT_WALL) { if ((*cell == '.' || (*cell == ' ' && floor_supported(L, build_x, build_y))) && cash >= 50) { *cell = 'W'; *world_wall_cell(L, build_x, build_y) = (u8)build_wp; cash -= 50; ok = 1; }
                                     else if (*cell == ' ') build_say("NOTHING UNDER IT TO HOLD IT UP"); }
        if (build_tool == BT_FLOOR) { if (*cell == ' ' && !floor_supported(L, build_x, build_y)) build_say("NOTHING UNDER IT TO HOLD IT UP");
                               else if (cash >= 10) { if (*cell == ' ') *cell = '.'; *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; ok = 1; } }
        if (build_tool == BT_PAINT) { if (is_wallish(*cell)) { *world_wall_cell(L, build_x, build_y) = (u8)build_wp; ok = 1; } }
        audio_voice(ok ? VOICE_FINISHED : VOICE_NAH);
    }
    if (HELD(PAD_CROSS) && build_tool == BT_FLOOR && cash >= 10 && *cell != ' ' && *world_floor_cell(L, build_x, build_y) != build_fl) { *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; }
    if (HELD(PAD_CROSS) && build_tool == BT_FLOOR && L > 0 && *cell == ' ' && cash >= 10 && floor_supported(L, build_x, build_y)) { *cell = '.'; *world_floor_cell(L, build_x, build_y) = (u8)build_fl; cash -= 10; }
    if (!occupied && HELD(PAD_CROSS) && !PRESSED(PAD_CROSS)) {   // hold X and walk the cursor: a run of walls, a run of wallpaper (as in The Sims)
        if (build_tool == BT_WALL && (*cell == '.' || (*cell == ' ' && floor_supported(L, build_x, build_y))) && cash >= 50) { *cell = 'W'; *world_wall_cell(L, build_x, build_y) = (u8)build_wp; cash -= 50; }
        if (build_tool == BT_PAINT && is_wallish(*cell)) *world_wall_cell(L, build_x, build_y) = (u8)build_wp;
        if (build_tool == BT_BUY && !build_carry && *cell == '.') {   // hold X and walk: a row of hedge, fence or other cheap things
            char dc = items_at(build_item); int dcost = items_cost(dc);
            if (dc != '|' && (items_along(dc) || dcost <= 120) && cash >= dcost) { *cell = dc; if (items_rotates(dc)) *world_wall_cell(L, build_x, build_y) = build_rot >= 0 ? (u8)(0x80 | build_rot) : 0; cash -= dcost; }
        }
    }
    if (!occupied && build_tool == BT_WALL && HELD(PAD_SQUARE) && !PRESSED(PAD_SQUARE) && *cell == 'W') { *cell = '.'; cash += 25; }   // hold SQUARE: a run of walls away
    if (PRESSED(PAD_SQUARE) && !occupied) {
        if (*cell != '.' && *cell != 'P' && *cell != ' ') { int back = items_is_item(*cell) ? items_cost(*cell) * 3 / 4 : (*cell == 'W' ? 25 : 0);
            if (*cell == '|' && L < NUP) { char *up = world_cell(L + 1, build_x, build_y); if (*up == ' ' && floor_supported(L + 1, build_x, build_y)) *up = '.'; }   // close the stairwell again
            if (items_rotates(*cell)) *world_wall_cell(L, build_x, build_y) = 0; *cell = '.'; cash += back;
            if (back) { char t_[32]; sprintf(t_, "SOLD  +$%d", back); build_say_ok(t_); } }
        else if (*cell == '.' && L > 0) { *cell = ' '; cash += 5; }                       // take the floor itself away (upper storeys)
    }
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_SQUARE) || HELD(PAD_CROSS) || (build_tool == BT_WALL && HELD(PAD_SQUARE))) world_touched();
    if ((build_tool != BT_BUY && build_tool != BT_FLOOR && build_tool != BT_PAINT && PRESSED(PAD_TRIANGLE)) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { build_lvl = 0; state = ST_LIFE; }
}
static void build_draw(void) {
    {   // looking at the roof: lift the camera above the ridge and pull back, so the whole roof is in view
        int roofv = build_tool == BT_ROOF || build_peek, keep = cam_dist;
        if (roofv) cam_dist = MAX(cam_dist, 2600);
        camera_apply(build_x * TILE + TILE / 2, -100 - build_lvl * WALL_FULL - (roofv ? WALL_FULL + 300 : 0), -(build_y * TILE + TILE / 2));
        cam_dist = keep;
    }
    gfx_set_clear(20 * 8, 26 * 8, 31 * 8);
    gfx_set_ambient(100, 84);
    world_view_lvl = build_lvl; world_roof_show = build_tool == BT_ROOF || build_peek;
    world_draw(build_x, build_y, frame, build_tool != BT_ROOF && !build_peek);
    draw_sims();
    // the cursor: a frame around the tile, and a ghost of the item
    gfx_set_identity();
    { int x0 = build_x * TILE, z1 = -build_y * TILE, yy = -4 - build_lvl * WALL_FULL; SVECTOR a, b, c, d;
      setVector(&a, x0, yy, z1); setVector(&b, x0 + TILE, yy, z1); setVector(&c, x0, yy, z1 - TILE); setVector(&d, x0 + TILE, yy, z1 - TILE);
      int pulse = 150 + (isin((frame * 120) & 4095) >> 6), st = build_tile_state();   // green: go ahead. orange: it would take away what is there. red: not allowed (not your lot, no money, in the way)
      if (build_tool == BT_ROOF) st = 1;
      int cr = st == 0 ? 230 : st == 2 ? pulse + 30 : 50, cg = st == 0 ? 70 : st == 2 ? 150 : pulse, cb = st == 0 ? 60 : st == 2 ? 40 : 80;
      gfx_quad_f(&a, &b, &c, &d, cr / 2 + 20, cg / 2 + 20, cb / 2 + 20, 0);   // (the tile, tinted)
      { int L_ = TILE / 4, T_ = 10, X0 = x0, X1 = x0 + TILE, Zh = z1, Zl = z1 - TILE, y2 = yy - 3;   // four corner brackets round it, breathing a little
        int e = (isin((frame * 120) & 4095) >> 9);   // (in and out: -8..8)
        build_bar(X0 - e, Zh + e, L_, T_, y2, cr, cg, cb);          build_bar(X0 - e, Zh + e, T_, L_, y2, cr, cg, cb);
        build_bar(X1 - L_ + e, Zh + e, L_, T_, y2, cr, cg, cb);     build_bar(X1 - T_ + e, Zh + e, T_, L_, y2, cr, cg, cb);
        build_bar(X0 - e, Zl + T_ - e, L_, T_, y2, cr, cg, cb);     build_bar(X0 - e, Zl + L_ - e, T_, L_, y2, cr, cg, cb);
        build_bar(X1 - L_ + e, Zl + T_ - e, L_, T_, y2, cr, cg, cb); build_bar(X1 - T_ + e, Zl + L_ - e, T_, L_, y2, cr, cg, cb);
        } }
    if (build_tool == BT_ROOM && room_on) {   // the room to be: its outline
        int x0, y0, x1, y1, good = room_rect(&x0, &y0, &x1, &y1), yy = -6 - build_lvl * WALL_FULL;
        for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
            if (x != x0 && x != x1 && y != y0 && y != y1) continue;
            SVECTOR a, b, c, d; int X = x * TILE, Z = -y * TILE;
            setVector(&a, X + 24, yy, Z - 24); setVector(&b, X + TILE - 24, yy, Z - 24); setVector(&c, X + 24, yy, Z - TILE + 24); setVector(&d, X + TILE - 24, yy, Z - TILE + 24);
            gfx_quad_f(&a, &b, &c, &d, good ? 90 : 230, good ? 200 : 80, good ? 255 : 70, 0);
        }
    }
    if (build_tool == BT_BUY && world_at_l(build_lvl, build_x, build_y) == '.') {
        char ch = build_carry ? build_carry : items_at(build_item); int rr_ = build_carry ? build_crot : build_rot;
        world_lvl = build_lvl; world_cam_push(build_lvl);
        items_draw(ch, build_x, build_y, rr_ >= 0 ? rr_ : world_item_facing(build_x, build_y), items_along(ch) ? world_along(build_x, build_y, ch) : 0, frame);
        world_cam_pop(); world_lvl = 0;
    }
    {   // the tools as round buttons, the status card, the catalog on the right, the buttons along the foot
        static const s8 toolIc[NTOOL] = { IC_SOFA, IC_ROOM, IC_BRICK, IC_DOOR, IC_TILES, IC_BRUSH, IC_HOUSE };
        char b[64];
        ui_card(4, 4, 190, 52);
        for (int i = 0; i < NTOOL; i++) { int on = i == build_tool; ui_icon_disc(15 + i * 20, 15, 18, toolIc[i], on ? UI_BLUE : UI_RGB(222, 230, 240), on ? UI_WHITE : UI_BLUE2); }
        sprintf(b, "$%ld", cash); ui_text(188 - ui_tw(1, b), 10, 1, b, UI_GOODT);
        if (build_delta_t > 0 && build_delta) { char d_[20]; sprintf(d_, build_delta < 0 ? "-$%ld" : "+$%ld", build_delta < 0 ? -build_delta : build_delta);   // the last buy or sale, beside the money
            ui_text(188 - ui_tw(1, b) - 6 - ui_tw(1, d_), 10, 1, d_, build_delta < 0 ? UI_BADT : UI_GOODT); }
        ui_text(10, 27, 1, toolNm[build_tool], UI_BLUE2);
        sprintf(b, "STOREY %d OF %d", build_lvl + 1, NLEVELS); ui_text(188 - ui_tw(1, b), 27, 1, b, UI_SOFT);
        if (build_msg_t > 0) strcpy(b, build_msg);
        else if (!build_ok(build_x, build_y)) strcpy(b, home_city ? "NOT YOUR LOT" : "BUY A HOME TO BUILD HERE");
        else if (build_carry) sprintf(b, "MOVING %s (TRI TURNS)", items_name(build_carry));
        else if (build_tool == BT_BUY && build_tile_state() == 2) { char hc = world_at_l(build_lvl, build_x, build_y); sprintf(b, "%s: X MOVES, SQ SELLS $%d", items_name(hc), items_cost(hc) * 3 / 4); }   // (an item under the cursor)
        else if (build_tool == BT_BUY) sprintf(b, "%s  $%d", items_name(items_at(build_item)), items_cost(items_at(build_item)));
        else if (build_tool == BT_ROOM) { int x0, y0, x1, y1;
            if (room_on && room_rect(&x0, &y0, &x1, &y1)) sprintf(b, "%d X %d ROOM: $%d", x1 - x0 + 1, y1 - y0 + 1, room_cost(build_lvl, x0, y0, x1, y1));
            else sprintf(b, room_on ? "OTHER CORNER (%s)" : "X ON A CORNER (%s)", roomOpenNm[room_open]); }
        else if (build_tool == BT_WALL) sprintf(b, "WALL $50  (%s)", world_wall_name(build_wp));
        else if (build_tool == BT_DOOR) sprintf(b, "%s $%d: X ON A WALL", openNm[build_open & 3], openCost[build_open & 3]);
        else if (build_tool == BT_FLOOR) sprintf(b, "%s  $10 A TILE", world_floor_name(build_fl));
        else if (build_tool == BT_PAINT) sprintf(b, "%s", world_wall_name(build_wp));
        else sprintf(b, "%s ROOF, %s PITCH", world_roof_style_name(world_roof_style), world_roof_pitch_name(world_roof_pitch));
        ui_text(10, 40, 1, b, build_msg_t > 0 && build_msg_ok ? UI_GOODT : build_msg_t > 0 || !build_ok(build_x, build_y) ? UI_BADT : UI_INK);
        if (build_tool == BT_BUY) { draw_catalog();
            char ic_ = items_at(build_item), t_[40]; int cst = items_cost(ic_);   // the pick on its own card: its price, what it sells back for, and whether you can afford it
            ui_card(196, 174, 120, 28);
            ui_text(203, 178, 1, items_name(ic_), UI_INK);
            sprintf(t_, "$%d", cst); ui_text(203, 190, 1, t_, cst > cash ? UI_BADT : UI_GOODT);
            if (cst > cash) ui_text(311 - ui_tw(1, "CAN'T AFFORD"), 190, 1, "CAN'T AFFORD", UI_BADT);
            else { sprintf(t_, "SELLS $%d", cst * 3 / 4); ui_text(311 - ui_tw(1, t_), 190, 1, t_, UI_SOFT); } }
        else if (build_tool == BT_ROOM) { draw_pick_list("ROOM WALLS", NWALLS, build_wp, world_wall_name);
            int top = ui_window_s(196, 126, 120, 30, "ROOM FLOOR (R3)"); ui_text(204, top + 2, 1, world_floor_name(build_fl), UI_INK); }
        else if (build_tool == BT_WALL) draw_pick_list("WALLS  $50", NWALLS, build_wp, world_wall_name);
        else if (build_tool == BT_DOOR) draw_pick_list("DOORS, WINDOWS", 4, build_open & 3, open_name);
        else if (build_tool == BT_FLOOR) draw_pick_list("FLOORS  $10", NFLOORS, build_fl, world_floor_name);
        else if (build_tool == BT_PAINT) draw_pick_list("WALLPAPER", NWALLS, build_wp, world_wall_name);
        else { int top = ui_window_s(196, 4, 120, 62, "THE ROOF");
               ui_text(204, top + 2, 1, world_roof_style_name(world_roof_style), UI_INK);
               ui_text(204, top + 14, 1, world_roof_pitch_name(world_roof_pitch), UI_SOFT);
               ui_text(204, top + 26, 1, world_roof_col_name(world_roof_col), UI_SOFT); }
        ui_card(4, 206, 312, 30);
        if (build_tool == BT_ROOF) { ui_hints(160, 204, "X STYLE|SQ PITCH|L1 R1 COLOUR|SEL TOOL", 1); ui_hints(160, 216, "L2 R2 STOREY|O DONE", 1); }
        else if (build_tool == BT_ROOM) { ui_hints(160, 204, room_on ? "X BUILD IT|O LET GO|L1 R1 WALLS|R3 FLOOR" : "X FIRST CORNER|L1 R1 WALLS|R3 FLOOR", 1);
               ui_hints(160, 216, "SQ DOOR|TRI UNDO|SEL TOOL|L2 R2 STOREY|O DONE", 1); }
        else { ui_hints(160, 204, build_tool == BT_BUY ? "L1 R1 PICK|R3 TAB|X BUY OR MOVE|SQ SELL" : build_tool == BT_DOOR ? "L1 R1 PICK|X PUT IN|SQ TAKE OUT" : build_tool == BT_WALL ? "L1 R1 PICK|X PLACE, HOLD TO DRAW|SQ ERASE" : build_tool == BT_PAINT ? "L1 R1 PICK|X PAINT|TRI PAPER ROOM" : build_tool == BT_FLOOR ? "L1 R1 PICK|X PLACE|TRI FILL ROOM|SQ ERASE" : "L1 R1 PICK|X PLACE|SQ ERASE", 1);
               ui_hints(160, 216, build_tool == BT_BUY ? "TRI TURN|SEL TOOL|L2 R2 STOREY|O DONE" : "SEL TOOL|L2 R2 STOREY|L3 ROOF|O DONE", 1); }
        if (build_intro > 0) { int sl = build_intro > 80 ? (build_intro - 80) * 14 : build_intro < 12 ? (12 - build_intro) * 14 : 0;   // BUILD MODE: slides in, stays, slides out
            ui_shadow(30, 92 - sl, 136, 30, 8); ui_round(30, 92 - sl, 136, 30, 8, UI_RGB(70, 176, 238), UI_BLUE2, 0);
            ui_textc(98, 97 - sl, 2, "BUILD MODE", UI_WHITE); ui_textc(98, 112 - sl, 1, "O WHEN YOU ARE DONE", UI_WHITE); }
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
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = prev_state == ST_LIFE ? BACK_TO() : ST_MENU;
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
#define NHELP 6
static const struct { const char *title; HelpTip it[5]; } help_pg[NHELP] = {
    { "HOW TO PLAY", { { IC_PLAY, "MOVE WITH THE STICK OR THE D-PAD. SQUARE RUNS." },
        { IC_FOOD, "X USES THE THING IN FRONT OF YOU: THE FRIDGE, THE BED, THE SHOWER, THE SOFA, THE TV." },
        { IC_CHAT, "X ON A PERSON TO TALK. O STOPS WHAT YOU ARE DOING." },
        { IC_FAST, "TRIANGLE CHANGES HOW FAST TIME GOES." },
        { IC_PEOPLE, "SELECT OPENS YOUR PHONE: CALL, PIZZA, PARTY, MAP AND MORE." } } },
    { "THE CAMERA", { { IC_CAMERA, "L1 R1 TURN THE CAMERA. L2 R2 ZOOM. L1 + R1 TOGETHER: SWING ROUND BEHIND YOU." },
        { IC_EYE, "L2 + R2 TOGETHER: SIMS CAMERA, BEHIND YOU, OR FIRST PERSON." },
        { IC_PLAY, "FIRST PERSON: UP WALKS, LEFT AND RIGHT TURN, L2 R2 LOOK. L1 + R1 TURN ROUND. THE DOT SHOWS WHAT X DOES." },
        { IC_CASE, "WORK: PHONE, JOBS. BILLS ARE $800, EVERY MIDNIGHT." },
        { IC_ARROW, "START: THE BORE PANEL, BUILD MODE, OPTIONS AND SAVE GAME (3 SLOTS ON THE MEMORY CARD)." } } },
    { "BUILD AND CREATE", { { IC_HAMMER, "BUILD MODE: ITEMS, WALLS, FLOORS AND 103 WALLPAPERS." },
        { IC_BRUSH, "SELECT PICKS THE TOOL. L1 R1 PICK THE THING. X PLACES IT, SQUARE TAKES IT AWAY." },
        { IC_HOUSE, "L2 R2 CHANGE THE STOREY. L3 SHOWS THE ROOF." },
        { IC_STAR, "CREATE A BORE: EVERY BODY, FACE, HAIR, CLOTHES AND PART, IN 3D." },
        { IC_DICE, "TRIANGLE ROLLS THE DICE FOR A RANDOM BORE." } } },
    { "GROWING UP", { { IC_SPROUT, "BORES GROW UP: BABY, CHILD, TEEN, ADULT, ELDER. OPTIONS SETS HOW FAST." },
        { IC_CAKE, "A BIRTHDAY? BAKE A CAKE AT THE FRIDGE OR STOVE, THEN BLOW CANDLES." },
        { IC_HEART, "TWO GROWN-UPS WHO GET ON: X ON ONE, START A FAMILY. OR ADOPT BY PHONE." },
        { IC_BABY, "A BABY CRIES WHEN IT NEEDS YOU: FEED, CHANGE, PLAY, CUDDLE. BUY A CRIB." },
        { IC_BOOK, "SCHOOL IS 8 AM TO 3 PM, MON - FRI. HOMEWORK AT A DESK OR SHELF." } } },
    { "A HOME OF YOUR OWN", { { IC_HOUSE, "PHONE, HOMES: HOUSES AND EMPTY LOTS FOR SALE NEAR YOU. X BUYS ONE." },
        { IC_CAR, "THE FAMILY MOVES IN BY TAXI. A HOUSE COMES FURNISHED FOR THEM." },
        { IC_ROOM, "BUILD MODE, ROOM: X ON ONE CORNER, X ON THE OTHER: WALLS AND A FLOOR." },
        { IC_DOOR, "DOORS, WINDOWS: X ON A WALL. L2 R2 CHANGE THE STOREY TO BUILD UP." },
        { IC_STAR, "A FOR SALE SIGN: X ON IT TO BUY THAT HOME. TRAVEL TAKES YOU HOME." } } },
    { "CASTAWAY ISLAND", { { IC_WOOD, "X ON TREES, BUSHES AND ROCKS: WOOD, LEAVES, BERRIES AND STONE." },
        { IC_FOOD, "FISH OFF THE JETTY. COOK AND EAT AT THE CAMPFIRE. NO FOOD? FORAGE." },
        { IC_HAMMER, "X ON THE CAMPFIRE, CRAFT: A TENT, A BENCH, A SPEAR, A SHELTER, A RAFT." },
        { IC_STAR, "8 SECRETS ARE HIDDEN ON THE ISLAND. THE MAP KEEPS COUNT AND GIVES HINTS." },
        { IC_CAR, "A RAFT SAILS OUT TO THE ISLET. WHAT IS OUT THERE?" } } } };
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
        int nl = ui_wrap_n(236, h->t), hh = MAX(27, nl * 10 + 6);
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
#ifdef AUTO_CELLS
    if (title_t == 30) {   // (test builds: how many cells a Bore needs, over thousands of Bores: rolled, every shape with every hair, dressed up)
        extern int dbg_cells[6]; Bore *b = crowd_temp_bore();
        for (int r = 0; r < 6000; r++) { memset(b, 0, sizeof *b); look_default(b); b->stage = (u8)(1 + r % 4); look_random(b);
            if (r & 1) { b->look[LK_SHAPE] = (u8)((r >> 1) % look_count(LK_SHAPE)); b->look[LK_HSTYLE] = (u8)((r >> 1) / look_count(LK_SHAPE) % look_count(LK_HSTYLE)); }
            if ((r & 3) == 3) for (int q = 0; q < LK_N; q++) if (look_is_slider(q)) b->look[q] = (u8)(r & 4 ? 8 : 0);
            look_build(b);
            if (b->ncell > dbg_cells[0]) { dbg_cells[0] = b->ncell; dbg_cells[4] = r; dbg_cells[5] = b->look[LK_SHAPE] * 100 + b->look[LK_HSTYLE]; }
            dbg_cells[2] += b->ncell; dbg_cells[3]++; }
    }
#endif
#ifdef AUTO_HELP
    if (title_t == 30) { prev_state = ST_MENU; help_page = AUTO_HELP; state = ST_HELP; return; }
#endif
#ifdef AUTO_STORY
    if (title_t == 30) { life_story = AUTO_STORY; worlds_go(0x5EED1234u, 0); return; }
#endif
#if defined(AUTO_WEAR) || defined(AUTO_FACE)
    if (title_t == 30) { world_backup(); prev_state = ST_MENU; create_tab = 4; create_row = 0; state = ST_CREATE; world_gen_studio(); return; }
#endif
#if defined(AUTO_PARTY) || defined(AUTO_CITY) || defined(AUTO_STREAM) || defined(AUTO_GOTCHI) || defined(AUTO_DJ) || defined(AUTO_SUPER)
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
// ---- THE WORLDS (STORY MODE): which SAN BOREAS to live in. Fresh dice, one of the presets, one you roll yourself on CREATE A
// WORLD (the map shows each as you go), or a mode: CASTAWAY ISLAND, or the missions and expansions ----
enum { WO_RANDOM, WO_PRE1, WO_PRE2, WO_PRE3, WO_CREATE, WO_CASTAWAY, WO_MORE, WO_N };
static const u32 wo_preset[3] = { 0x0B0E2001u, 0x51A7E5EDu, 0x7A11C0DEu };
static const char *const wo_cap[3] = { "MAPLE SHORES", "SUNNY HOLLOW", "PINE BAY" };   // the set worlds' names
static int gotchi_shelf;   // the BOREAGOTCHI! guide opened from the expansion shelf (not in a life)
static int wo_sel, wo_t, wo_more, wo_msel, wo_packed; static char wo_name[18];   // (wo_name: the capital named by the player, "" as made)
static u32 wo_seed, wo_painted, wo_custom = 0x00C0FFEEu;
static void paint_world(u32 seed) {                 // SAN BOREAS from above for that seed (the land, the highways, the towns), into the map's picture
    if (seed == wo_painted) return;
    wo_name_for(seed); bm_rename_clear(); if (wo_name[0]) bm_rename(0, wo_name); bm_new(seed);
    u16 *px = atlas_scratch();
    static const u16 tcol[6] = { RGB(4, 9, 19), RGB(25, 23, 15), RGB(10, 18, 7), RGB(5, 12, 5), RGB(24, 20, 12), RGB(15, 15, 15) };
    for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) px[y * 128 + x] = tcol[bm_terrain(x * 8 + 4, y * 8 + 4)] | 0x8000;
    for (int i = 0; i < bm_nseg(); i++) { int c, a, b, v; bm_seg(i, &c, &a, &b, &v);
        for (int k = a; k <= b; k += 4) { int x = v ? c / 8 : k / 8, y = v ? k / 8 : c / 8; px[y * 128 + x] = RGB(9, 9, 10) | 0x8000; } }
    for (int i = 0; i < bm_nset(); i++) { const Settle *st = bm_set(i); int r = MAX(1, st->r / 8);
        u16 col = st->kind == ST_CITY ? RGB(30, 22, 12) : st->kind == ST_TOWN ? RGB(28, 25, 16) : RGB(24, 22, 18);
        for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x * x + y * y <= r * r + 1) {
            int X = st->x / 8 + x, Y = st->y / 8 + y; if (X >= 0 && Y >= 0 && X < 128 && Y < 128) px[Y * 128 + X] = col | 0x8000; } }
    atlas_vram_load(0, 256, 128, 128, px);
    crowd_flush();
    wo_painted = seed;
}
static void wo_name_for(u32 seed) {   // a set world's capital carries the world's name (any other: BORE CITY)
    bm_cap_name[0] = 0;
    for (int k = 0; k < 3; k++) if (seed && seed == wo_preset[k]) strcpy(bm_cap_name, wo_cap[k]);
}
static u32 wo_seed_of(int k) { return k >= WO_PRE1 && k <= WO_PRE3 ? wo_preset[k - WO_PRE1] : k == WO_CREATE ? wo_custom : 0; }
static void worlds_named(void) { wo_painted = 0; }   // (the map is drawn again, the new name on it)
static void worlds_open(void) {
    wo_painted = 0; wo_sel = 0; wo_t = 0; wo_more = 0; wo_msel = 0; state = ST_WORLDS;
#ifdef AUTO_PACKS
    wo_more = 1; wo_msel = AUTO_PACKS;
#endif
}
static void worlds_go(u32 seed, int mode) {
    life_seed = seed; life_mode = mode; life_started = 0; bm_gen = 4; bm_packed = wo_packed; wo_name_for(seed); strcpy(life_cap, wo_name); start_life();
    cam_yaw = 2600; cam_pitch = 640; cam_dist = 1500; state = ST_LIFE; frame = 0;
}
// ---- CREATE A WORLD: make your own. What kind of life, then dials for the land (or the castaway's island), the map as you turn them ----
enum { WM_START, WM_LAND, WM_SEA, WM_CLIM, WM_HILLS, WM_CAPSZ, WM_CITIES, WM_TOWNS, WM_VILS, WM_PEOPLE, WM_FAMS, WM_TRAFFIC, WM_STYLE, WM_ISIZE, WM_ICOAST, WM_IJUNGLE, WM_IROCKS, WM_NAME, WM_SEED, WM_PLAY, WM_N };
static int wm_sel, wm_t, wm_start, wm_used, wm_isle_view;   // wm_start: 0 SAN BOREAS, 1 CASTAWAY ISLAND, 2 RAGS TO RICHES
static int wm_land, wm_sea = 2, wm_clim = 2, wm_hills = 2, wm_capsz = 2, wm_cities = 2, wm_towns = 6, wm_vils = 12, wm_people = 3, wm_fams = 1, wm_traffic = 2, wm_isize = 1, wm_icoast = 1, wm_ijungle = 1, wm_irocks = 1;
static const char *const wmLbl[WM_N] = { "LIFE", "LAND", "SEA", "CLIMATE", "HILLS", "CAPITAL SIZE", "CITIES", "TOWNS", "VILLAGES", "PEOPLE", "HOUSEHOLDS", "TRAFFIC", "TOWN STYLE", "ISLAND SIZE", "COAST", "JUNGLE", "ISLAND HILLS", "CAPITAL", "MAP NO.", "PLAY THIS WORLD" };
static const char *const wmStartNm[3] = { "SAN BOREAS", "CASTAWAY", "RAGS TO RICHES" };
static const char *const wmLandNm[BL_N] = { "MAINLAND", "ISLANDS", "ONE BIG ISLAND", "CONTINENT", "LAKES" };
static const char *const wmSeaNm[5] = { "VERY LOW", "LOW", "NORMAL", "HIGH", "VERY HIGH" };
static const char *const wmClimNm[5] = { "DESERT", "DRY", "MIXED", "GREEN", "JUNGLE" };
static const char *const wmHillNm[5] = { "FLAT", "GENTLE", "NORMAL", "HILLY", "MOUNTAINS" };
static const char *const wmCapNm[6] = { "TINY", "SMALL", "NORMAL", "BIG", "HUGE", "GIANT" };
static const char *const wmPeopleNm[6] = { "FEW", "SOME", "MANY", "NORMAL", "CROWDED", "PACKED" };
static const char *const wmTrafNm[5] = { "NONE", "LOW", "NORMAL", "BUSY", "JAMMED" };
static const char *const wmFamNm[4] = { "SMALL", "MIXED", "BIG", "HUGE" };
static const char *const wmI3[3][3] = { { "SMALL", "MEDIUM", "BIG" }, { "SMOOTH", "WAVY", "WILD" }, { "LESS", "SOME", "MORE" } };
static const char *const wmBlurb[WM_N] = { "A HOME, STRANDED, OR BROKE", "THE SHAPE OF SAN BOREAS", "HOW MUCH OF IT THE SEA COVERS",
    "SAND AND SCRUB TO THICK FOREST", "HOW MUCH ROCKY HIGH GROUND", "HOW BIG BORE CITY IS", "BIG CITIES BESIDES THE CAPITAL", "MIDDLE-SIZED TOWNS", "LITTLE VILLAGES AND HAMLETS",
    "HOW MANY HOMES HAVE PEOPLE IN THEM", "SMALL: SINGLES, COUPLES. BIG: FAMILIES", "CARS ON THE STREETS. MORE IN OPTIONS: CHEATS", "ROOMY: GREEN GAPS. PACKED: BUILT UP",
    "THE CASTAWAY'S ISLAND", "A ROUND SHORE, OR BAYS AND HEADLANDS", "HOW THICK THE TREES GROW", "ROCKY GROUND ON THE ISLAND",
    "X TO NAME THE CAPITAL CITY", "THE DICE: L R STEP, X NEW DICE", "START A LIFE HERE" };
static int wm_shown(int r) { return wm_start == 1 || (r < WM_ISIZE || r > WM_IROCKS); }   // (the island's dials: for a castaway)
static int *wm_dial(int r, int *n) {
    switch (r) {
        case WM_START: *n = 3; return &wm_start; case WM_LAND: *n = BL_N; return &wm_land; case WM_SEA: *n = 5; return &wm_sea;
        case WM_CLIM: *n = 5; return &wm_clim; case WM_HILLS: *n = 5; return &wm_hills; case WM_CAPSZ: *n = 6; return &wm_capsz; case WM_CITIES: *n = BM_MAXCITY + 1; return &wm_cities;
        case WM_TOWNS: *n = BM_MAXTOWN + 1; return &wm_towns; case WM_VILS: *n = BM_MAXVIL + 1; return &wm_vils;
        case WM_PEOPLE: *n = 6; return &wm_people; case WM_FAMS: *n = 4; return &wm_fams; case WM_TRAFFIC: *n = 5; return &wm_traffic;
        case WM_STYLE: *n = 2; return &wo_packed; case WM_ISIZE: *n = 3; return &wm_isize; case WM_ICOAST: *n = 3; return &wm_icoast;
        case WM_IJUNGLE: *n = 3; return &wm_ijungle; case WM_IROCKS: *n = 3; return &wm_irocks;
        default: *n = 0; return 0;
    }
}
static void wm_apply(void) {   // the dials into the world makers
    bm_land = wm_land; bm_sea = wm_sea; bm_clim = wm_clim; bm_hills = wm_hills; bm_capsz = wm_capsz; bm_ncity = wm_cities; bm_ntown = wm_towns; bm_nvil = wm_vils; pop_people = wm_people; pop_famsize = wm_fams; traffic_level = wm_traffic; bm_packed = wo_packed;
    isl_size = wm_isize; isl_wild = wm_icoast; isl_jungle = wm_ijungle; isl_rocks = wm_irocks;
}
static void wm_open(void) { wm_sel = WM_LAND; wm_t = 0; wm_used = 1; wm_apply(); wo_painted = 0; state = ST_WMAKE; audio_voice(VOICE_FINISHED); }
static u32 wm_painted_isle;
static void wm_paint(void) {
    wm_apply(); wm_check_dials();
    int isle = wm_start == 1 && wm_sel >= WM_ISIZE && wm_sel <= WM_IROCKS;
    if (isle) {
        u32 key = wo_custom ^ (u32)(wm_isize | wm_icoast << 2 | wm_ijungle << 4 | wm_irocks << 6) * 0x9E3779B1u;
        if (!wm_isle_view || wm_painted_isle != key) { u16 *px = atlas_scratch(); world_isl_preview(wo_custom, px); atlas_vram_load(0, 256, 128, 128, px); wm_painted_isle = key; wo_painted = 0; }
        wm_isle_view = 1;
    } else { if (wm_isle_view) { wm_isle_view = 0; wo_painted = 0; } paint_world(wo_custom); wm_painted_isle = 0; }
}
static void wm_surprise(void) {   // SQUARE: every dial at random, and new dice
    u32 r = look_rand() * 2654435761u ^ ((u32)VSync(-1) * 40503u) ^ (u32)wm_t;
    wm_land = (int)(r % BL_N); wm_sea = 1 + (int)((r >> 4) % 3); wm_clim = (int)((r >> 7) % 5); wm_hills = (int)((r >> 10) % 5); wm_cities = (int)((r >> 13) % (BM_MAXCITY + 1)); wm_towns = (int)((r >> 24) % (BM_MAXTOWN + 1)); wm_vils = (int)((r >> 27) % (BM_MAXVIL + 1)); wm_capsz = (int)((r >> 5) % 6); wm_people = 1 + (int)((r >> 25) % 5); wm_fams = (int)((r >> 29) % 4);
    wo_packed = (int)((r >> 15) & 1); wm_isize = (int)((r >> 16) % 3); wm_icoast = (int)((r >> 18) % 3); wm_ijungle = (int)((r >> 20) % 3); wm_irocks = (int)((r >> 22) % 3);
    wo_custom = r * 40503u + 1; wo_painted = 0; wm_painted_isle = 0; audio_voice(VOICE_FINISHED);
}
static void wm_named(void) { wo_painted = 0; }
static void wm_check_dials(void) {   // a dial turned: the map is drawn again
    static u32 k0; u32 k = (u32)(bm_land | bm_sea << 3 | bm_clim << 6 | bm_hills << 9 | bm_packed << 12 | bm_capsz << 13 | bm_ncity << 16 | bm_ntown << 20 | bm_nvil << 25 | (u32)bm_capsz << 13);
    if (k != k0) { k0 = k; wo_painted = 0; }
}
static void wm_update(void) {
    wm_t++;
    int d = 0;
    if (PRESSED(PAD_UP)) do wm_sel = (wm_sel + WM_N - 1) % WM_N; while (!wm_shown(wm_sel));
    if (PRESSED(PAD_DOWN)) do wm_sel = (wm_sel + 1) % WM_N; while (!wm_shown(wm_sel));
    if (PRESSED(PAD_LEFT)) d = -1;
    if (PRESSED(PAD_RIGHT)) d = 1;
    if (PRESSED(PAD_CIRCLE)) { wo_painted = 0; wm_isle_view = 0; state = ST_WORLDS; return; }
    if (PRESSED(PAD_SQUARE)) wm_surprise();
    if (PRESSED(PAD_L1)) { wo_custom--; wo_painted = 0; }
    if (PRESSED(PAD_R1)) { wo_custom++; wo_painted = 0; }
    int n, *v = wm_dial(wm_sel, &n);
    if (v && d) { *v = (*v + n + d) % n; wo_painted = 0; audio_voice(VOICE_AGREE); }
    if (v && PRESSED(PAD_CROSS)) { *v = (*v + 1) % n; wo_painted = 0; audio_voice(VOICE_AGREE); }
    if (wm_sel == WM_SEED && d) { wo_custom += (u32)d; wo_painted = 0; }
    if (wm_sel == WM_SEED && PRESSED(PAD_CROSS)) { wo_custom = look_rand() * 2654435761u ^ ((u32)VSync(-1) * 40503u); if (!wo_custom) wo_custom = 1; wo_painted = 0; audio_voice(VOICE_FINISHED); }
    if (PRESSED(PAD_SELECT) || (wm_sel == WM_NAME && PRESSED(PAD_CROSS))) { name_open(wo_name, 17, "NAME YOUR CAPITAL CITY", wm_named); return; }
    if (wm_start != 1 && wm_sel >= WM_ISIZE && wm_sel <= WM_IROCKS) wm_sel = WM_NAME;
    wm_paint();
    if (PRESSED(PAD_START) || (wm_sel == WM_PLAY && PRESSED(PAD_CROSS))) { wm_apply(); life_story = 0; worlds_go(wo_custom, wm_start); }
}
static void wm_draw(void) {
    title_draw();
    ui_dim();
    char b[48];
    int top = ui_window(6, 6, 308, 228, "CREATE A WORLD", IC_BRUSH);
    int y = top + 1;
    int list[WM_N], nl = 0, at = 0; for (int r = 0; r < WM_N; r++) if (wm_shown(r)) { if (r == wm_sel) at = nl; list[nl++] = r; }
    const int VIS = 13; int first = CLAMP(at - VIS / 2, 0, MAX(0, nl - VIS));   // (a long list scrolls with the cursor)
    for (int li = first; li < MIN(nl, first + VIS); li++) {
        int r = list[li];
        int on = r == wm_sel, n;
        if (r == WM_PLAY) { ui_button(10, y + 1, 138, 15, wmLbl[r], IC_PLAY, on); y += 17; continue; }
        if (on) ui_round(10, y, 138, 12, 4, UI_BLUE, UI_BLUE2, 0);
        else if (r == WM_LAND || r == WM_ISIZE || r == WM_NAME) ui_round(10, y - 1, 138, 1, 0, UI_LINE, UI_LINE, 0);   // (a rule between the groups)
        ui_text(14, y + 2, 1, wmLbl[r], on ? UI_WHITE : UI_SOFT);
        const char *val = "";
        int *v = wm_dial(r, &n);
        if (r == WM_START) val = wmStartNm[wm_start]; else if (r == WM_LAND) val = wmLandNm[wm_land]; else if (r == WM_SEA) val = wmSeaNm[wm_sea];
        else if (r == WM_CLIM) val = wmClimNm[wm_clim]; else if (r == WM_HILLS) val = wmHillNm[wm_hills]; else if (r == WM_CAPSZ) val = wmCapNm[wm_capsz];
        else if (r == WM_CITIES || r == WM_TOWNS || r == WM_VILS) { sprintf(b, "%d", *v); val = b; } else if (r == WM_PEOPLE) val = wmPeopleNm[wm_people]; else if (r == WM_FAMS) val = wmFamNm[wm_fams]; else if (r == WM_TRAFFIC) val = wmTrafNm[wm_traffic];
        else if (r == WM_STYLE) val = wo_packed ? "PACKED" : "ROOMY"; else if (r >= WM_ISIZE && r <= WM_IROCKS) val = wmI3[r == WM_ISIZE ? 0 : r == WM_ICOAST ? 1 : 2][*v];
        else if (r == WM_NAME) val = wo_name[0] ? wo_name : "BORE CITY";
        else if (r == WM_SEED) { sprintf(b, "%08X", (unsigned)wo_custom); val = b; }
        int vx = 144 - ui_tw(1, val);
        ui_text(vx, y + 2, 1, val, on ? UI_WHITE : UI_BLUE2);
        if (on && (v || r == WM_SEED)) { ui_text(vx - 8, y + 2, 1, "<", UI_WHITE); }
        y += 13;
    }
    // the map: SAN BOREAS, or the castaway's island while its dials are picked
    const int MX = 162, MY = top + 2, MS = 138;
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, 128, 128, 128, 128, 128);
    if (!wm_isle_view && wo_painted) {
        for (int i = 0; i < bm_nset(); i++) { const Settle *st = bm_set(i); int x = MX + st->x * MS / BM_N, yy = MY + st->y * MS / BM_N, dd = st->kind == ST_CITY ? 6 : st->kind == ST_TOWN ? 4 : 3;
            ui_disc(x, yy, dd + 2, UI_RGB(40, 46, 60)); ui_disc(x, yy, dd, i == bm_capital() ? UI_RGB(255, 204, 48) : UI_WHITE); }
        const Settle *c = bm_set(bm_capital()); { int nf = 0; while (nf < NFAM && pop_fam[nf].n) nf++; (void)c; static const u8 lv[6] = { 35, 60, 80, 100, 150, 200 }; sprintf(b, "%d PLACES, %d HOMES", bm_nset(), NFAM_NORMAL * lv[wm_people % 6] / 100); }
    } else strcpy(b, "THEY BEGIN STRANDED HERE");
    ui_badge(MX + 4, MY + 4, wm_isle_view ? "THE ISLAND" : "SAN BOREAS", UI_WHITE, UI_BLUE2);
    ui_text(MX, MY + MS + 5, 1, b, UI_INK);
    ui_wrap(MX, MY + MS + 16, MS + 6, 2, wmBlurb[wm_sel], UI_SOFT);
    ui_hints(160, 214, "LR CHANGE|SQ SURPRISE ME|START PLAY|O BACK", 1);
}

#define WO_NMORE 21
// the base game's missions, the six older packs and THE LAND BOREFORE TIME (the GBA game's seven), then thirteen packs still to come
static const char *const moreNm[WO_NMORE] = { "STORY MISSIONS", "BORE-OUT MAN!", "BOREZIES!", "WORLD WAR BORE", "BORTUA FIGHTER", "BOREAGOTCHI!", "BOREPILOGUE", "THE LAND BOREFORE TIME",
    "SEASONS", "ISLAND PARADISE", "SUPERSTAR", "CRIME AND LAW", "FARM LIFE", "SKY HIGH", "SPEED CITY", "BIG SCREEN", "SEASIDE", "MAGIC AND MYTH", "FAMILY FUN", "WILD WEST", "SPACE AGE" };
static const char *const moreCard[WO_NMORE] = { "STORY|MISSIONS", "BORE-OUT|MAN!", "BOREZIES!", "WORLD|WAR BORE", "BORTUA|FIGHTER", "BORE-|AGOTCHI!", "BORE-|PILOGUE", "THE LAND|BOREFORE|TIME",
    "SEASONS", "ISLAND|PARADISE", "SUPER-|STAR", "CRIME|AND LAW", "FARM|LIFE", "SKY|HIGH", "SPEED|CITY", "BIG|SCREEN", "SEA-|SIDE", "MAGIC AND|MYTH", "FAMILY|FUN", "WILD|WEST", "SPACE|AGE" };
static const char *const moreBlurb[WO_NMORE] = { "TEN LIVES WITH A STORY TO TELL. PICK ONE!", "A DESK JOCKEY BECOMES AN ASTRONAUT. NOT PORTED YET!", "THE TOWN GOES UNDEAD. NOT PORTED YET!", "THE EASTERN ARMY CUT DOWN YOUR HEDGE. NOT PORTED YET!",
    "PLAY IT AT THE ARCADE CABINET IN SAN BOREAS!", "RAISE EIGHT PIXEL PETS PER TOY. X: THE GUIDE!", "GROWN-UPS ONLY: RETIRE AND LEAVE A LEGACY. NOT PORTED YET!", "WAKE A GIANT: PAUSE, STORY, KINGDOM MODE IN A LIFE!",
    "THE YEAR TURNS. PLAY ON AND SEE.", "PLACEHOLDER PACK. NOT MADE YET!", "FORM A BAND AND PLAY GIGS! PAUSE, SUPERSTAR BAND", "BREAK THE LAW AND FIND OUT.", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!",
    "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!", "PLACEHOLDER PACK. NOT MADE YET!" };
static void worlds_update(void) {
    wo_t++;
    if (wo_more) {                                   // MISSIONS AND EXPANSIONS
        // (two rows of tall cards, a column to a pair: item i sits in column i/2, row i%2)
        if (PRESSED(PAD_UP) || PRESSED(PAD_DOWN)) { int n = wo_msel ^ 1; if (n < WO_NMORE) wo_msel = n; }
        if (PRESSED(PAD_RIGHT)) { int n = wo_msel + 2; wo_msel = n < WO_NMORE ? n : (wo_msel & 1); }
        if (PRESSED(PAD_LEFT)) { int n = wo_msel - 2; wo_msel = n >= 0 ? n : ((WO_NMORE - 1) & ~1) | (wo_msel & 1); if (wo_msel >= WO_NMORE) wo_msel = WO_NMORE - 1; }
        if (PRESSED(PAD_CIRCLE)) { wo_more = 0; return; }
        if (PRESSED(PAD_CROSS)) { if (wo_msel == 0) { sc_t = 0; sp_join = 0; state = ST_SPICK; } else if (wo_msel == 5) { if ((ovl = ovl_load("GOTCHI.OVL"))) { ovl->open(3); gotchi_shelf = 1; state = ST_GOTCHI; } else audio_voice(VOICE_NAH); } else if (wo_msel == 10) audio_voice(VOICE_FINISHED); else audio_voice(VOICE_NAH); }
        return;
    }
    if (PRESSED(PAD_UP)) wo_sel = (wo_sel + WO_N - 1) % WO_N;
    if (PRESSED(PAD_DOWN)) wo_sel = (wo_sel + 1) % WO_N;
    if (PRESSED(PAD_CIRCLE)) { state = ST_MENU; return; }
    if (PRESSED(PAD_TRIANGLE)) { wo_packed ^= 1; wo_painted = 0; audio_voice(VOICE_FINISHED); }   // TOWNS: ROOMY or PACKED
    if (PRESSED(PAD_SELECT) && wo_sel != WO_MORE) { name_open(wo_name, 17, "NAME YOUR CAPITAL CITY", worlds_named); return; }
    bm_gen = 4; bm_packed = wo_packed;
    if (wo_sel == WO_CREATE) {                       // CREATE A WORLD: roll it, or step through, until you like the look of it
        if (PRESSED(PAD_SQUARE)) { wo_custom = look_rand() * 2654435761u ^ ((u32)VSync(-1) * 40503u) ^ (u32)wo_t; if (!wo_custom) wo_custom = 1; audio_voice(VOICE_FINISHED); }
        if (PRESSED(PAD_L1)) wo_custom--;
        if (PRESSED(PAD_R1)) wo_custom++;
    }
    if (wo_sel == WO_CREATE && wm_used) wm_apply(); else { bm_dials_default(); isl_dials_default(); pop_people = 3; pop_famsize = 1; traffic_level = 2; }  traffic_load(0);   // (the made world's map with its dials; a set world with its own)
    wm_check_dials();
    u32 sd = wo_seed_of(wo_sel); if (sd) paint_world(sd);
    if (PRESSED(PAD_CROSS)) {
        if (wo_sel == WO_MORE) { wo_more = 1; return; }
        if (wo_sel == WO_CREATE) { wm_open(); return; }
        bm_dials_default(); isl_dials_default(); pop_people = 3; pop_famsize = 1; traffic_level = 2; traffic_load(0);
        life_story = 0; worlds_go(wo_seed_of(wo_sel), wo_sel == WO_CASTAWAY ? 1 : 0);
    }
}
static void worlds_draw(void) {
    title_draw();
    ui_dim();
    char b[48];
    int top = ui_window(6, 6, 308, 228, wo_more ? "MISSIONS AND EXPANSIONS" : "PICK A WORLD", wo_more ? IC_STAR : IC_PIN);
    if (wo_more) {
        static const s8 mic[WO_NMORE] = { IC_BOOK, IC_STAR, IC_PEOPLE, IC_HAMMER, IC_BX, IC_SMILE, IC_HEART, IC_TREE, IC_SUN, IC_TREE, IC_NOTE, IC_EYE, IC_SPROUT, IC_BOLT, IC_CAR, IC_CAMERA, IC_DROP, IC_STAR, IC_PARTY, IC_HAMMER, IC_BOLT };
        int c0 = CLAMP(wo_msel / 2 - 2, 0, (WO_NMORE + 1) / 2 - 5);   // (five columns show, and slide along)
        for (int i = 0; i < WO_NMORE; i++) {
            int col = i / 2 - c0; if (col < 0 || col > 4) continue;
            int x = 14 + col * 59, y = top + 2 + (i & 1) * 80, on = i == wo_msel;
            ui_round(x, y, 56, 76, 5, on ? UI_RGB(255, 236, 150) : UI_WHITE, on ? UI_RGB(255, 204, 70) : UI_RGB(214, 222, 236), 0);
            ui_icon_disc(x + 28, y + 24, 30, mic[i], (i == 0 || i == 4 || i == 7) ? UI_BLUE2 : UI_SOFT, UI_WHITE);
            { char t[24]; strncpy(t, moreCard[i], 23); t[23] = 0; char *ln[3]; int nl = 0; ln[nl++] = t; for (char *q = t; *q && nl < 3; q++) if (*q == '|') { *q = 0; ln[nl++] = q + 1; }
              int ty = y + 46 - (nl == 1 ? 0 : 0); if (nl == 1) ty += 6; for (int k = 0; k < nl; k++) ui_textc(x + 28, ty + k * 10, 1, ln[k], UI_INK); }
        }
        if (c0 > 0) ui_text(6, top + 76, 1, "<", UI_INK);
        if (c0 + 5 < (WO_NMORE + 1) / 2) ui_text(308 - 6, top + 76, 1, ">", UI_INK);
        ui_textc(160, top + 160, 1, moreNm[wo_msel], UI_BLUE2);
        ui_textc(160, top + 172, 1, moreBlurb[wo_msel], (wo_msel == 0 || wo_msel == 4 || wo_msel == 5 || wo_msel == 7) ? UI_INK : UI_SOFT);
        ui_hints(160, 214, wo_msel == 0 ? "X PICK A STORY|ARROWS MOVE|O BACK" : wo_msel == 5 ? "X THE GUIDE|ARROWS MOVE|O BACK" : "ARROWS MOVE|O BACK", 1);
        return;
    }
    static const s8 ic[WO_N] = { IC_DICE, IC_PIN, IC_PIN, IC_PIN, IC_BRUSH, IC_SUN, IC_STAR };
    for (int i = 0; i < WO_N; i++) {
        const char *nm = i == WO_RANDOM ? "RANDOM WORLD" : i == WO_CREATE ? "CREATE A WORLD" : i == WO_CASTAWAY ? "CASTAWAY ISLAND" : i == WO_MORE ? "MISSIONS, PACKS" : wo_cap[i - WO_PRE1];
        ui_button(12, top + 2 + i * 21, 124, 18, nm, ic[i], i == wo_sel);
    }
    const int MX = 152, MY = top + 2, MS = 150;
    u32 sd = wo_seed_of(wo_sel);
    ui_round(MX - 2, MY - 2, MS + 4, MS + 4, 4, UI_LINE, UI_LINE, 0);
    if (sd && wo_painted == sd) {
        gfx_sprite_c(MX, MY, MS, MS, getTPage(2, 0, 0, 256), 0, 0, 128, 128, 128, 128, 128);
        for (int i = 0; i < bm_nset(); i++) { const Settle *st = bm_set(i); int x = MX + st->x * MS / BM_N, y = MY + st->y * MS / BM_N, d = st->kind == ST_CITY ? 6 : st->kind == ST_TOWN ? 4 : 3;
            ui_disc(x, y, d + 2, UI_RGB(40, 46, 60)); ui_disc(x, y, d, i == bm_capital() ? UI_RGB(255, 204, 48) : UI_WHITE); }
    } else {
        ui_round(MX, MY, MS, MS, 4, UI_RGB(70, 120, 180), UI_RGB(40, 80, 140), 0);
        ui_icon_disc(MX + MS / 2, MY + MS / 2 - 8, 40, wo_sel == WO_CASTAWAY ? IC_SUN : wo_sel == WO_MORE ? IC_STAR : IC_DICE, UI_WHITE, UI_BLUE2);
        ui_textc(MX + MS / 2, MY + MS / 2 + 20, 1, wo_sel == WO_RANDOM ? "A NEW MAP EVERY TIME" : wo_sel == WO_CASTAWAY ? "A NEW ISLAND EVERY TIME" : "", UI_WHITE);
    }
    const char *bl = wo_sel == WO_RANDOM ? "FRESH DICE: TOWNS, ROADS AND 874 FAMILIES" : wo_sel == WO_CREATE ? "X MAKES YOUR OWN: LAND, SEA, CLIMATE, ISLANDS..." :
                     wo_sel == WO_CASTAWAY ? "BEGIN STRANDED ON THE ISLAND" : wo_sel == WO_MORE ? "A MISSION, AND PACKS ON THE WAY" : "A SET WORLD: THE SAME MAP EVERY TIME";
    ui_text(12, top + 158, 1, bl, UI_SOFT);
    if (sd) { const Settle *c = wo_painted == sd ? bm_set(bm_capital()) : 0; sprintf(b, "%d TOWNS, CAPITAL %s", bm_nset(), c ? c->name : "?"); ui_text(12, top + 170, 1, b, UI_INK); }
    else if (wo_name[0] && wo_sel != WO_MORE) { snprintf(b, sizeof b, "CAPITAL: %s", wo_name); ui_text(12, top + 170, 1, b, UI_INK); }
    if (wo_sel == WO_CREATE) { sprintf(b, "MAP NO. %08X", (unsigned)wo_custom); ui_text(310 - ui_tw(1, b), top + 170, 1, b, UI_BLUE2); }
    ui_badge(306 - ui_tw(1, wo_packed ? "TOWNS: PACKED" : "TOWNS: ROOMY") - 10, 8, wo_packed ? "TOWNS: PACKED" : "TOWNS: ROOMY", UI_WHITE, UI_BLUE2);
    ui_hints(160, 214, wo_sel == WO_CREATE ? "X MAKE IT|SQ ROLL|L1 R1|SEL NAME|O BACK" : "X PLAY|TRI TOWNS|SEL NAME CITY|O BACK", 1);
}

static const char *const menuNm[] = { "STORY MODE", "CONTINUE", "CREATE A BORE", "JUKEBOX", "OPTIONS", "HOW TO PLAY" };
static const char *const menuBlurb[] = { "START A BRAND NEW LIFE IN SAN BOREAS", "", "DREAM UP SOMEBODY NEW, HEAD TO TOE", "PUT ON A SONG. ALL 52 OF THEM", "SOUND, CAMERA, PICTURE AND SPEED", "HOW EVERYTHING WORKS, WITH PICTURES" };
#define NMENU 6
static void menu_update(void) {
    title_t++;
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + NMENU - 1) % NMENU;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % NMENU;
    if (PRESSED(PAD_CIRCLE)) state = ST_TITLE;
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_START)) {
        switch (menu_sel) {
            case 0:
#if defined(AUTO_CITY) || defined(AUTO_PARTY) || defined(AUTO_PLACES) || defined(AUTO_TOUR) || defined(AUTO_SCENE) || defined(AUTO_PHONE) || defined(AUTO_ISLE) || defined(AUTO_HOME) || defined(AUTO_GROW) || defined(AUTO_SAVE) || defined(AUTO_TOWER) || defined(AUTO_FLOAT) || defined(AUTO_STREAM)
                worlds_go(0, 0); break;   // (test builds: straight into a life, as before)
#endif
                worlds_open(); break;
            case 1: slots_open(0, ST_MENU); break;
            case 2: if (!world_backup()) { audio_voice(VOICE_NAH); break; } prev_state = ST_MENU; create_tab = 0; create_row = 0; state = ST_CREATE; world_gen_studio(); break;
            case 3: prev_state = ST_MENU; state = ST_JUKEBOX; break;
            case 4: opts_open(ST_MENU); break;
            case 5: prev_state = ST_MENU; help_page = 0; state = ST_HELP; break;
        }
    }
}
static void menu_draw(void) {
    title_draw();
    static const s8 ic[NMENU] = { IC_PLAY, IC_BOOK, IC_STAR, IC_NOTE, IC_GEAR, IC_HELP };
    ui_card(92, 56, 136, 154);
    for (int i = 0; i < NMENU; i++) ui_button(98, 62 + i * 20, 124, 17, menuNm[i], ic[i], i == menu_sel);
    const char *t = menuBlurb[menu_sel];
    if (menu_sel == 1) t = "PICK UP RIGHT WHERE YOU LEFT OFF";
    ui_wrapc(96, 184, 128, 2, t, UI_SOFT);   // (on the card, two lines at most)
}

// ---- the loading screen: a bar and what is being made, shown before any long job (start-up, a new life, travel) ----
static void loading_draw(const char *what, int pct) {   // The Sims 4's way: a big gold arrow, the name, a white card with the bar, and a tip
    static const char *const tips[] = { "BILLS ARRIVE AT MIDNIGHT. KEEP $800 HANDY!", "NEED A RIDE? PHONE, THEN MAP: ANY TAXI $20.",
        "HOLD L2 TO ZOOM OUT. THE CITY GOES ON AND ON.", "TRY A STEADY 30 IN OPTIONS: IT FEELS SMOOTHER.", "MOSS DOES NOT WANT A PLUMBOB. DO NOT ASK.",
        "CHAT WITH SOMEONE 5 TIMES AND YOU ARE FRIENDS.", "BUILD MODE HAS 103 WALLPAPERS. GO WILD.", "A SHOWER A DAY KEEPS THE NEIGHBOURS FRIENDLY.",
        "TIRED? A NAP ON THE SOFA HELPS A LITTLE.", "THE ISLAND HIDES 8 SECRETS. HAVE YOU FOUND THEM?", "SKATE TIP: HOLD TRIANGLE TO GRIND A LEDGE.",
        "FRIENDS COME TO PARTIES. SO DO THEIR FRIENDS.", "THE RADIO HAS FOUR STATIONS. FIND YOUR FAVOURITE.", "CREATE A WORLD: TRY ISLANDS WITH LOTS OF VILLAGES." };
    pct = CLAMP(pct, 0, 100);
    gfx_set_clear(60, 120, 190);
    gfx_grad(0, 0, SCREEN_W, SCREEN_H, UI_RGB(130, 196, 244), UI_RGB(130, 196, 244), UI_RGB(34, 96, 170), UI_RGB(34, 96, 170), 0);
    ui_icon_s(144, 22, IC_ARROW, 36, UI_RGB(120, 84, 10));   // (a darker copy under it, for depth)
    ui_icon_s(142, 20, IC_ARROW, 36, UI_RGB(255, 204, 48));
    { int w = gfx_text_w(3, "BORE 2"); gfx_text_raw(162 - w / 2, 66, 3, "BORE 2", 40, 60, 110); gfx_text_raw(160 - w / 2, 64, 3, "BORE 2", 255, 255, 255); }
    ui_card(60, 124, 200, 40);
    ui_text(70, 131, 1, what, UI_INK);
    { char b[8]; sprintf(b, "%d%%", pct); ui_text(250 - ui_tw(1, b), 131, 1, b, UI_SOFT); }
    ui_bar(70, 146, 180, 9, pct, UI_BLUE);
    { static int tip = -1, tip_t; int now = VSync(-1); if (tip < 0 || now - tip_t > 300) { tip = (tip + 1 + (now & 3)) % (int)(sizeof tips / sizeof *tips); tip_t = now; }
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
    int free_k = (int)((0x801FFFF0u - 0x7000u - (u32)_end) / 1024);   // (24 KB under the stack top for the stack, which reaches 15 KB in the tests (stack_paint), and the SDK's 4 KB heap past _end)
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

__attribute__((noinline)) static void stack_paint(void) {   // the free RAM under the stack, marked: how deep the stack ever reaches can be read off later
    volatile u32 here; u32 *p = (u32 *)(((u32)_end + 15) & ~15u), *q = (u32 *)(((u32)&here - 512) & ~15u);
    while (p < q) *p++ = 0x5AC0FFEEu;
}
int main(void) {
    stack_paint();
    mem8_init();                                     // (8 MB of RAM? one disc for both)
    gfx_init();
    atlas_init();
    font_init();
    ui_init();          boot("GFX ATLAS FONT OK");   // (the boot text needs the font, so the first line comes after it)
    // each stage is announced BEFORE it runs ("> NAME"), so the last line on screen names the stage that is running when the game stops
    boot("> LOOK");        look_init();
    boot("> VOXEL");       voxel_init();
    boot("> ITEMS");       items_init();
    boot("> WORLD TEX");   world_init(); ghost_init();
    boot("> AUDIO+DISC");  audio_init();
    pad_init(); pad_poll_on(1);                      // (the pad is read directly in read_pad: see pad.c; and polled in the waits between frames)
    boot("> CROWD");       crowd_init();             // the voice clips are in the sound chip now: their RAM holds the crowd's models
    opts_apply();                                    // (the options' starting choices: HIGH)
    look_default(&edit);
    look_default(&title_bore); look_random(&title_bore);
    boot("> HOME LOT");    world_gen();
    strcpy(edit.first, "DIXIE"); strcpy(edit.last, "DIPPINN");
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
        audio_update(); pad_poll();
#ifdef AUTO_TOUR
        tour_tick();
#endif
#ifdef AUTO_GROW
        grow_tick();
#endif
#ifdef AUTO_HOME
        home_tick();
#endif
#ifdef AUTO_ISLE
        isle_tick();
#endif
#ifdef AUTO_SCENE
        scene_tick();
#endif
#ifdef AUTO_PHONE
        phone_tick();
#endif
#ifdef AUTO_SAVE
        save_tick();
#endif
#ifdef AUTO_STREAM
        stream_tick();
#endif
#ifdef AUTO_STORY
        story_test_tick();
#endif
        ui_light = 0;
        {   // each screen its own colour across the top
            static const u8 acc[] = { [ST_TITLE] = 0, [ST_MENU] = 0, [ST_CREATE] = 5, [ST_LIFE] = 0, [ST_PAUSE] = 7, [ST_BUILD] = 3, [ST_JUKEBOX] = 4, [ST_HELP] = 1,
                [ST_TRAVEL] = 1, [ST_STORY] = 6, [ST_WANTS] = 6, [ST_PIE] = 0, [ST_PANEL] = 2, [ST_MAP] = 1, [ST_DIR] = 2, [ST_OPTS] = 7, [ST_JOBS] = 3,
                [ST_OVER] = 7, [ST_HOMES] = 3, [ST_CRAFT] = 3, [ST_PHONE] = 0, [ST_SLOTS] = 7, [ST_NHOOD] = 1, [ST_WORLDS] = 1, [ST_NAME] = 5, [ST_SPICK] = 6,
                [ST_SCARD] = 6, [ST_WMAKE] = 2 };
            ui_accent = state < (int)sizeof acc ? acc[state] : 0;
        }
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
            case ST_STORY: story_update(); if (state == ST_STORY) story_draw(); else if (state == ST_SPICK) { life_draw(); } else life_draw(); break;
            case ST_PIE: pie_update(); pie_draw(); break;
            case ST_PANEL: panel_update(); panel_draw(); break;
            case ST_MAP: map_update(); map_draw(); break;
            case ST_DIR: dir_update(); dir_draw(); break;
            case ST_OPTS: opts_update(); opts_draw(); break;
            case ST_JOBS: jobs_update(); jobs_draw(); break;
            case ST_OVER: over_update(); over_draw(); break;
            case ST_HOMES: homes_update(); homes_draw(); break;
            case ST_CRAFT: craft_update(); craft_draw(); break;
            case ST_CONFIRM: confirm_update(); if (state == ST_CONFIRM) confirm_draw(); else life_draw(); break;
            case ST_NAME: name_update(); if (state == ST_NAME) name_draw(); else if (state == ST_CREATE) create_draw(); else if (state == ST_WORLDS) worlds_draw(); else if (state == ST_MAP) map_draw(); else life_draw(); break;
            case ST_WORLDS: worlds_update(); if (state == ST_WORLDS) worlds_draw(); else if (state == ST_MENU) menu_draw(); else life_draw(); break;
            case ST_NHOOD: nhood_update(); if (state == ST_NHOOD) nhood_draw(); else if (state == ST_HOMES) homes_draw(); else life_draw(); break;
            case ST_SLOTS: slots_update(); if (state == ST_SLOTS) slots_draw(); else if (state == ST_LIFE) life_draw(); else if (state == ST_MENU) menu_draw(); else pause_draw(); break;
            case ST_PHONE: phone_update(); if (state == ST_PHONE) phone_draw(); else life_draw(); break;
            case ST_SPICK: spick_update(); if (state == ST_SPICK) { if (sp_join) life_draw(); else title_draw(); ui_dim(); story_pick_draw(sp_sel, sc_t, sp_join); } else if (state == ST_WORLDS) worlds_draw(); else life_draw(); break;
            case ST_SCARD: scard_update(); if (state == ST_SCARD) scard_draw(); else life_draw(); break;
            case ST_SUPER: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; audio_jukebox_stop(); ovl_leave(); state = ST_LIFE; life_draw(); } break; }
            case ST_DOG: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; ovl_leave(); state = ST_LIFE; life_draw(); } break; }
            case ST_PRISON: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; ovl_leave(); state = ST_LIFE; if (sim_want_end) { sim_want_end = 0; pause_new_life(); } life_draw(); } break; }
            case ST_DJ: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; ovl_leave(); state = ST_LIFE; life_draw(); } break; }
            case ST_PARTY: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; ovl_leave(); state = ST_LIFE; life_draw(); if (party_go) { party_go = 0; party_begin(); } } break; }
            case ST_GOTCHI: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; if (gotchi_shelf) { gotchi_shelf = 0; crowd_reset(); wo_painted = 0; state = ST_WORLDS; } else { ovl_leave(); state = ST_LIFE; life_draw(); } } break; }
            case ST_REALM: case ST_ARCADE: { int hd_ = ~btn & 0xffff, pr_ = hd_ & btn_prev; if (ovl && ovl->update(hd_, pr_, steps)) ovl->draw(); else { ovl = 0; ovl_leave(); state = ST_LIFE; life_draw(); } break; }
            case ST_WMAKE: wm_update(); if (state == ST_WMAKE) wm_draw(); else if (state == ST_WORLDS) worlds_draw(); else if (state == ST_NAME) name_draw(); else life_draw(); break;
        }
        t_cpu = hb_now() - t_frame0;
        if (state == ST_LIFE) {   // a frame that ran past two screen refreshes: the full-detail range draws in a tile (back out once things are quiet)
            extern int world_detail_r; static int slow_n, fast_n;
            if (t_cpu > 500) { fast_n = 0; if (++slow_n >= 1 && world_detail_r > det_opt - 4) { world_detail_r--; slow_n = 0; } } else slow_n = 0;
            if (t_cpu < 420) { if (++fast_n >= 120 && world_detail_r < det_opt) { world_detail_r++; fast_n = 0; } } else if (t_cpu > 470) fast_n = 0;
            if (world_detail_r > det_opt) world_detail_r = det_opt;
        }
#ifdef AUTO_CITY
        { extern int dbg_slow[16][8], dbg_slow_n; if (state == ST_LIFE && t_cpu > 525) { int *d = dbg_slow[dbg_slow_n++ & 15];   // (test builds: what the slow frames spent their time on)
            d[0] = t_cpu; d[1] = t_upd; d[2] = t_logic; d[3] = t_world; d[4] = t_sims; d[5] = t_hud + t_bub; d[6] = gfx_prof[3]; d[7] = frame; } }
#endif
        if (state == ST_LIFE) { avg_cpu16 += t_cpu - (avg_cpu16 >> 4); avg_world16 += t_world - (avg_world16 >> 4);   // (the test readouts)
            avg_hh16 += gfx_prof[4] - (avg_hh16 >> 4); hist_cpu[MIN(4, t_cpu / 262)]++;
            for (int k = 0; k < 8; k++) avg_prof16[k] += gfx_prof[k] - (avg_prof16[k] >> 4); }
        gfx_flip();
    }
    return 0;
}
