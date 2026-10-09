// BORE 2 - the sequel to BORE (GBA), on the PlayStation. True 3D: the voxel people, the rooms and the items are real geometry.
#include "common.h"
#include "gfx.h"
#include "atlas.h"
#include "look.h"
#include "voxel.h"
#include "items.h"
#include "world.h"
#include "sim.h"
#include "audio.h"
#include "font.h"

// ---- input ----
static u8 pad_buf[2][34];
static u16 btn, btn_prev;
static int ls_x, ls_y, rs_x, rs_y, analog;
#define HELD(b) (!(btn & (b)))
#define PRESSED(b) (!(btn & (b)) && (btn_prev & (b)))
static void read_pad(void) {
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
enum { ST_TITLE, ST_MENU, ST_CREATE, ST_LIFE, ST_PAUSE, ST_BUILD, ST_JUKEBOX, ST_HELP, ST_TRAVEL };
static int state = ST_TITLE, prev_state = ST_TITLE;
static int menu_sel;
static int create_tab, create_row, create_from, create_turn;
static int build_tool, build_item, build_x = 8, build_y = 8, build_fl = 1, build_wp = 1;
static int juke_sel, juke_scroll;
static int life_started;
static int help_page;
static int show_fps = 0, fps_val, fps_frames, fps_vb, prims_last, t_world, t_sims, t_logic;
static int travel_sel;

static void draw_panel(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, 10, 12, 18, 1);
    gfx_rect(x, y, w, 1, 255, 200, 40, 0);
    gfx_rect(x, y + h - 1, w, 1, 60, 60, 70, 0);
}
static void draw_bar(int x, int y, int w, int v, int r, int g, int b) {
    gfx_rect(x, y, w, 6, 30, 30, 36, 0);
    int fw = (w - 2) * v / 100;
    gfx_rect(x + 1, y + 1, fw, 4, r, g, b, 0);
}

// ---- the life ----
static void start_life(void) {
    if (life_started) return;
    world_reset_all();
    sim_init();
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 5; py = 12; }
    sim_add(&edit, px, py, 0);
    // two housemates from the dice
    Bore b; look_default(&b); b.stage = AG_ADULT; look_random(&b); sim_add(&b, px + 2, py + 1, 1);
    look_default(&b); b.stage = AG_CHILD; look_random(&b); sim_add(&b, px - 1, py + 2, 1);
    life_started = 1;
    sim_note("WELCOME HOME. X USES THINGS.");
}

static void draw_plumbob(const Sim *s) {   // the green crystal over the Bore you play: it spins, and it colours with the mood
    VECTOR p; setVector(&p, s->x, 0, s->z);
    int top = -H * VOX - 30 - (isin((frame * 60) & 4095) >> 9);
    gfx_set_object(&p, (frame * 50) & 4095);
    int m = s->mood;
    int r = m > 15 ? 40 : m > -15 ? 230 : 255, g = m > 15 ? 230 : m > -15 ? 200 : 60, b = m > 15 ? 60 : m > -15 ? 40 : 60;
    SVECTOR apex, base[4], bot;
    setVector(&apex, 0, top - 58, 0); setVector(&bot, 0, top + 30, 0);
    setVector(&base[0], -22, top, -22); setVector(&base[1], 22, top, -22); setVector(&base[2], 22, top, 22); setVector(&base[3], -22, top, 22);
    gfx_set_light_yaw((frame * 50) & 4095);
    for (int k = 0; k < 4; k++) {
        const SVECTOR *b0 = &base[k], *b1 = &base[(k + 1) & 3];
        int nx = (b0->vx + b1->vx), nz = (b0->vz + b1->vz);
        int L = gfx_light(nx * 90, -2400, nz * 90);
        gfx_tri_f(&apex, b1, b0, (r * L) >> 7, (g * L) >> 7, (b * L) >> 7, 0);
        L = gfx_light(nx * 90, 2400, nz * 90);
        gfx_tri_f(b0, b1, &bot, (r * L) >> 8, (g * L) >> 8, (b * L) >> 8, 0);
    }
}
static void draw_sims(void) {
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        VECTOR p; setVector(&p, s->x, 0, s->z);
        if (s->action == ACT_SLEEP) p.vy = -items_height('S') + 40;
        if (s->action == ACT_SIT || s->action == ACT_WC) p.vy = -items_height(world_at(s->act_x, s->act_y)) / 2 + 10;
        voxel_draw(&s->bore, &p, s->yaw, &s->gait);
        if (s->action != ACT_SLEEP) { VECTOR sp; setVector(&sp, s->x, 0, s->z); gfx_shadow(&sp, 70); }
    }
    if (nsims) draw_plumbob(&sims[player]);
}

static void draw_bubbles(void) {   // thoughts and notes over heads
    for (int i = 0; i < nsims; i++) {
        Sim *s = &sims[i];
        if (s->think < 0 && s->note_t <= 0) continue;
        VECTOR p; setVector(&p, s->x, 0, s->z);
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
}

static void draw_mood_face(int x, int y, int m) {   // a little round face: the mood, like a Sims portrait's plumbob colour
    int r = m > 15 ? 120 : m > -15 ? 230 : 255, g = m > 15 ? 230 : m > -15 ? 210 : 100, bb = m > 15 ? 90 : m > -15 ? 70 : 90;
    gfx_rect(x + 2, y, 12, 16, r, g, bb, 0); gfx_rect(x, y + 2, 16, 12, r, g, bb, 0);
    gfx_rect(x + 4, y + 5, 2, 3, INK, 0); gfx_rect(x + 10, y + 5, 2, 3, INK, 0);
    if (m > 15) { gfx_rect(x + 4, y + 10, 8, 1, INK, 0); gfx_rect(x + 3, y + 9, 1, 1, INK, 0); gfx_rect(x + 12, y + 9, 1, 1, INK, 0); }
    else if (m > -15) gfx_rect(x + 4, y + 10, 8, 1, INK, 0);
    else { gfx_rect(x + 4, y + 11, 8, 1, INK, 0); gfx_rect(x + 3, y + 12, 1, 1, INK, 0); gfx_rect(x + 12, y + 12, 1, 1, INK, 0); }
}
static void hud_life(void) {
    Sim *s = &sims[player];
    char b[48];
    // the bottom bar: needs in two columns, the Bore in the middle, the hint on the right
    gfx_rect(0, 188, 320, 52, 8, 10, 22, 1);
    gfx_rect(0, 188, 320, 1, 255, 200, 40, 0);
    static const int col[NEED_N][3] = { {255,160,40}, {80,160,255}, {160,100,255}, {60,220,220}, {255,120,200}, {255,230,60}, {120,255,120}, {200,200,200} };
    for (int n = 0; n < NEED_N; n++) {
        int v = sim_need_val(s, n), cx = (n < 4) ? 6 : 112, cy = 194 + (n & 3) * 11;
        gfx_text(cx, cy, 1, sim_need_name(n), v < 20 ? 255 : 190, v < 20 ? 70 : 190, v < 20 ? 70 : 200);
        draw_bar(cx + 42, cy + 1, 54, v, col[n][0], col[n][1], col[n][2]);
    }
    // the Bore: name, mood
    draw_mood_face(218, 194, s->mood);
    gfx_text(238, 192, 2, s->bore.first, WHITE);
    gfx_text(238 + gfx_text_w(2, s->bore.first) + 4, 197, 1, s->bore.last, GREY);
    int m = s->mood;
    const char *mood = m > 50 ? "STOKED" : m > 15 ? "HAPPY" : m > -15 ? "FINE" : m > -50 ? "SAD" : "MISERABLE";
    sprintf(b, "%s  LVL %d %s", mood, job_level, (clock_day < 5 && clock_min >= 540 && clock_min < 1020) ? "SHIFT" : "");
    gfx_text(238, 206, 1, b, m > 15 ? 140 : m > -15 ? 220 : 255, m > 15 ? 255 : m > -15 ? 220 : 110, 130);
    const char *hint = sim_hint();
    if (hint[0]) { sprintf(b, "X: %s", hint); gfx_text(238, 218, 1, b, GOLD); }
    else gfx_text(238, 218, 1, "START: MENU", DIM);
    // the clock and the money, top right
    gfx_rect(226, 4, 90, 26, 8, 10, 22, 1); gfx_rect(226, 4, 90, 1, 255, 200, 40, 0);
    sprintf(b, "%s %02d:%02d", sim_day_name(clock_day), clock_min / 60, clock_min % 60);
    gfx_text(232, 7, 2, b, 255, 230, 120);
    sprintf(b, "$%ld", cash);
    gfx_text(232, 20, 1, b, 120, 255, 120);
    if (sim_speed > 1) { sprintf(b, "X%d", sim_speed == 2 ? 3 : 15); gfx_text(296, 20, 1, b, 255, 120, 60); }
    if (last_note_t > 0) { int w = gfx_text_w(2, last_note) + 16; gfx_rect(160 - w / 2, 40, w, 18, 8, 10, 22, 1); gfx_textc(160, 43, 2, last_note, 255, 240, 160); }
    if (frame < 900) gfx_textc(160, 180, 1, "STICK MOVES   L1 R1 ORBIT   L2 R2 ZOOM   X USES   SQUARE RUNS", GREY);
    if (show_fps) { extern int vox_t_hair, vox_t_block, vox_t_face, vox_n_hair, vox_n_block;
        sprintf(b, "%d FPS %d PRIMS W%d S%d L%d", fps_val, prims_last, t_world, t_sims, t_logic); gfx_text(4, 4, 1, b, 120, 255, 120);
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
    if (PRESSED(PAD_CROSS)) { if (!sim_player_interact()) audio_voice(VOICE_NAH); }
    if (PRESSED(PAD_CIRCLE)) sim_player_cancel();
    if (PRESSED(PAD_TRIANGLE)) { sim_speed = sim_speed % 3 + 1; }
    if (PRESSED(PAD_L3)) show_fps ^= 1;
    { extern int getenv_noshadow, dbg_noao, dbg_nograd; if (PRESSED(PAD_R3)) { static int mode; mode = (mode + 1) & 3; getenv_noshadow = mode == 1; dbg_noao = mode == 2; dbg_nograd = mode == 3; } }
    if (PRESSED(PAD_SELECT)) { sim_switch_player(); sim_note("NOW PLAYING AS THE NEXT BORE"); }
    { int t0 = VSync(1); for (int k = 0; k < steps; k++) sim_step(); t_logic = VSync(1) - t0; }
    camera_input();
    if (PRESSED(PAD_START)) { state = ST_PAUSE; menu_sel = 0; }
    if (travel_asked) { travel_asked = 0; travel_sel = world_zone; state = ST_TRAVEL; }
}

static void life_draw(void) {
    Sim *s = &sims[player];
    camera_apply(s->x, -200, s->z);
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
    { int t0 = VSync(1); world_draw(sim_tile_x(s), sim_tile_y(s), frame, 1); t_world = VSync(1) - t0;
      t0 = VSync(1); gfx_prim_count = 0; draw_sims(); t_sims = VSync(1) - t0; }
    draw_bubbles();
    prims_last = gfx_prim_count;
    hud_life();
}

// ---- travel between the zones ----
static void travel_go(int z) {
    world_switch(z);
    int px, py; world_find('P', &px, &py); if (px < 0) { px = 20; py = 20; }
    for (int i = 0; i < nsims; i++) {
        Sim *sm = &sims[i];
        sim_player_cancel_sim(sm);
        sm->x = (px + (i % 3)) * TILE + TILE / 2; sm->z = -((py + (i / 3)) * TILE + TILE / 2);
        sm->path_n = 0;
    }
    char b[40]; sprintf(b, "WELCOME TO %s", world_zone_name(z)); sim_note(b);
    audio_voice(VOICE_YAHOO);
}
static void travel_update(void) {
    if (PRESSED(PAD_UP)) travel_sel = (travel_sel + ZONE_N - 1) % ZONE_N;
    if (PRESSED(PAD_DOWN)) travel_sel = (travel_sel + 1) % ZONE_N;
    if (PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) { if (travel_sel != world_zone) travel_go(travel_sel); state = ST_LIFE; }
}
static void travel_draw(void) {
    life_draw();
    draw_panel(70, 50, 180, 90);
    gfx_textc(160, 54, 2, "WHERE TO?", GOLD);
    static const char *const blurb[ZONE_N] = { "HOME: THE HOUSE, THE FACTORY, THE RAIL PARK", "A SAND ISLAND: A CAMP, PALMS, THE SEA", "A DESERT MAIN STREET: DINER, TRAILER, THE LAB" };
    for (int i = 0; i < ZONE_N; i++) {
        int sel = i == travel_sel, here = i == world_zone;
        gfx_text(86, 74 + i * 20, 1, world_zone_name(i), sel ? 255 : here ? 120 : 190, sel ? 255 : here ? 120 : 190, sel ? 255 : 130);
        gfx_text(86, 83 + i * 20, 1, here ? "YOU ARE HERE" : blurb[i], DIM);
        if (sel) gfx_text(76, 74 + i * 20, 1, ">", GOLD);
    }
}

// ---- the pause menu ----
static const char *const pauseNm[] = { "RESUME", "TRAVEL", "CREATE A BORE", "ADD A BORE", "BUILD MODE", "JUKEBOX", "HOW TO PLAY", "NEW LIFE", "QUIT TO TITLE" };
#define NPAUSE 9
static void pause_update(void) {
    if (PRESSED(PAD_UP)) menu_sel = (menu_sel + NPAUSE - 1) % NPAUSE;
    if (PRESSED(PAD_DOWN)) menu_sel = (menu_sel + 1) % NPAUSE;
    if (PRESSED(PAD_START) || PRESSED(PAD_CIRCLE)) { state = ST_LIFE; return; }
    if (PRESSED(PAD_CROSS)) {
        switch (menu_sel) {
            case 0: state = ST_LIFE; break;
            case 1: travel_sel = world_zone; state = ST_TRAVEL; break;
            case 2: edit = sims[player].bore; create_tab = 0; create_row = 0; prev_state = ST_LIFE; state = ST_CREATE; world_backup(); world_gen_studio(); break;
            case 3: { Bore b; look_default(&b); b.stage = (u8)(AG_CHILD + look_rand() % 3); look_random(&b);
                      sim_add(&b, sim_tile_x(&sims[player]), sim_tile_y(&sims[player]) + 1, 1); sim_note("A NEW BORE MOVED IN"); state = ST_LIFE; } break;
            case 4: build_x = sim_tile_x(&sims[player]); build_y = sim_tile_y(&sims[player]); state = ST_BUILD; break;
            case 5: prev_state = ST_LIFE; state = ST_JUKEBOX; break;
            case 6: prev_state = ST_PAUSE; help_page = 0; state = ST_HELP; break;
            case 7: life_started = 0; start_life(); state = ST_LIFE; break;
            case 8: audio_jukebox_stop(); life_started = 0; state = ST_TITLE; break;
        }
    }
}
static void pause_draw(void) {
    life_draw();
    draw_panel(90, 36, 140, 132);
    gfx_textc(160, 40, 2, "PAUSED", GOLD);
    for (int i = 0; i < NPAUSE; i++) gfx_text(104, 62 + i * 11, 1, pauseNm[i], i == menu_sel ? 255 : 170, i == menu_sel ? 255 : 170, i == menu_sel ? 255 : 180);
    gfx_text(96, 62 + menu_sel * 11, 1, ">", GOLD);
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
    world_draw(4, 5, frame, 1);
    VECTOR p; setVector(&p, bx, 0, bz);
    Gait g = { 0 }; g.phase = (frame * 90) & 4095; g.amp = 0;
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
            for (int k = 0; k < cnt && k < 12; k++) { u16 c = look_option_colour(r->id, k); gfx_rect_c(172 + k * 11, y + 9, 9, 8, c, 0); if (k == edit.look[r->id]) gfx_rect(172 + k * 11 - 1, y + 8, 11, 1, GOLD, 0); }
        }
        else { const char *nm = look_option_name(r->id, edit.look[r->id]); if (nm) strncpy(val, nm, 23); }
        if (val[0]) gfx_text(172, y + 9, 1, val, WHITE);
        if (sel) gfx_text(164, y, 1, ">", GOLD);
    }
    if (create_tab == 5) {
        char b[32]; sprintf(b, "SIGN: %s", look_sign_name(look_sign_of(&edit))); gfx_text(172, 206, 1, b, GREY);
    }
    gfx_textc(241, 216, 1, "L1 R1 TAB  TRIANGLE DICE", GREY);
    gfx_textc(241, 226, 1, prev_state == ST_LIFE ? "START DONE  O BACK" : "START GO LIVE LIFE", GOLD);
    char nm[32]; sprintf(nm, "%s %s", edit.first, edit.last); gfx_text(8, 8, 1, nm, GOLD);
    gfx_text(8, 18, 1, look_stage_name(edit.stage), GREY);
    gfx_text(8, 226, 1, "CREATE A BORE", WHITE);
}

// ---- build mode ----
static const char *const toolNm[4] = { "ITEM", "WALL", "FLOOR", "WALLPAPER" };
static void build_update(void) {
    static int rep;
    int dx = 0, dy = 0;
    int l = HELD(PAD_LEFT) || ls_x < -40, r = HELD(PAD_RIGHT) || ls_x > 40, u = HELD(PAD_UP) || ls_y < -40, d = HELD(PAD_DOWN) || ls_y > 40;
    if (l || r || u || d) { if (rep == 0 || rep > 12) { if (l) dx = -1; if (r) dx = 1; if (u) dy = -1; if (d) dy = 1; if (rep > 12) rep = 9; } rep++; } else rep = 0;
    if (dx || dy) {   // move with the camera: "up" is away from it
        int c = icos(cam_yaw), sn = isin(cam_yaw);
        int wx = (dx * c * 4096 - dy * sn * 4096) >> 24, wz = (-dy * c * 4096 - dx * sn * 4096) >> 24;   // sign: map y grows south = world -z
        (void)wx; (void)wz;
        // pick the dominant world direction
        int vx = dx * c - (-dy) * sn, vz = (-dy) * c + dx * sn;
        if (ABS(vx) >= ABS(vz)) build_x += vx > 0 ? 1 : -1; else build_y += vz > 0 ? -1 : 1;
        build_x = CLAMP(build_x, 1, MW - 2); build_y = CLAMP(build_y, 1, MH - 2);
    }
    if (PRESSED(PAD_SELECT)) build_tool = (build_tool + 1) & 3;
    if (build_tool == 0) { if (PRESSED(PAD_L1)) build_item = (build_item + items_count() - 1) % items_count(); if (PRESSED(PAD_R1)) build_item = (build_item + 1) % items_count(); }
    if (build_tool == 2) { if (PRESSED(PAD_L1)) build_fl = (build_fl + NFLOORS - 1) % NFLOORS; if (PRESSED(PAD_R1)) build_fl = (build_fl + 1) % NFLOORS; }
    if (build_tool == 3) { if (PRESSED(PAD_L1)) build_wp = (build_wp + NWALLS - 1) % NWALLS; if (PRESSED(PAD_R1)) build_wp = (build_wp + 1) % NWALLS; }
    if (build_tool == 1 || build_tool == 0) { if (HELD(PAD_L2)) cam_yaw -= 40; if (HELD(PAD_R2)) cam_yaw += 40; }
    char *cell = &lifeMap[build_y][build_x];
    int occupied = 0; for (int i = 0; i < nsims; i++) if (sim_tile_x(&sims[i]) == build_x && sim_tile_y(&sims[i]) == build_y) occupied = 1;
    if (PRESSED(PAD_CROSS) && !occupied) {
        if (build_tool == 0) { char ch = items_at(build_item); int cost = items_cost(ch);
            if (*cell == '.' && cash >= cost) { *cell = ch; cash -= cost; audio_voice(VOICE_FINISHED); } else audio_voice(VOICE_NAH); }
        if (build_tool == 1) { if (*cell == '.' && cash >= 50) { *cell = 'W'; wallMap[build_y][build_x] = (u8)build_wp; cash -= 50; } }
        if (build_tool == 2) { if (cash >= 10) { floorMap[build_y][build_x] = (u8)build_fl; cash -= 10; } }
        if (build_tool == 3) { if (*cell == 'W' || *cell == 'w') wallMap[build_y][build_x] = (u8)build_wp; }
    }
    if (HELD(PAD_CROSS) && build_tool == 2 && cash >= 10 && floorMap[build_y][build_x] != build_fl) { floorMap[build_y][build_x] = (u8)build_fl; cash -= 10; }
    if (PRESSED(PAD_SQUARE) && !occupied) {
        if (*cell != '.' && *cell != 'P') { int back = items_is_item(*cell) ? items_cost(*cell) * 3 / 4 : (*cell == 'W' ? 25 : 0); *cell = '.'; cash += back; }
    }
    if (PRESSED(PAD_CROSS) || PRESSED(PAD_SQUARE)) world_touched();
    if (PRESSED(PAD_TRIANGLE) || PRESSED(PAD_CIRCLE) || PRESSED(PAD_START)) state = ST_LIFE;
}
static void build_draw(void) {
    camera_apply(build_x * TILE + TILE / 2, -100, -(build_y * TILE + TILE / 2));
    gfx_set_clear(20 * 8, 26 * 8, 31 * 8);
    gfx_set_ambient(100, 84);
    world_draw(build_x, build_y, frame, 1);
    draw_sims();
    // the cursor: a frame around the tile, and a ghost of the item
    gfx_set_identity();
    { int x0 = build_x * TILE, z1 = -build_y * TILE; SVECTOR a, b, c, d;
      setVector(&a, x0, -4, z1); setVector(&b, x0 + TILE, -4, z1); setVector(&c, x0, -4, z1 - TILE); setVector(&d, x0 + TILE, -4, z1 - TILE);
      int pulse = 160 + (isin((frame * 120) & 4095) >> 6);
      gfx_quad_f(&a, &b, &c, &d, pulse, pulse, 40, 0); }
    if (build_tool == 0 && lifeMap[build_y][build_x] == '.') { char ch = items_at(build_item); items_draw(ch, build_x, build_y, world_item_facing(build_x, build_y), items_along(ch) ? world_along(build_x, build_y, ch) : 0, frame); }
    draw_panel(4, 4, 200, 34);
    char b[48];
    sprintf(b, "BUILD: %s   $%ld", toolNm[build_tool], cash); gfx_text(8, 8, 1, b, GOLD);
    if (build_tool == 0) sprintf(b, "%s  $%d", items_name(items_at(build_item)), items_cost(items_at(build_item)));
    else if (build_tool == 1) sprintf(b, "WALL $50  (%s)", world_wall_name(build_wp));
    else if (build_tool == 2) sprintf(b, "%s $10", world_floor_name(build_fl));
    else sprintf(b, "%s", world_wall_name(build_wp));
    gfx_text(8, 18, 1, b, WHITE);
    gfx_text(8, 28, 1, "L1 R1 PICK  X PLACE  SQ ERASE  SEL TOOL  O DONE", GREY);
}

// ---- the jukebox ----
static void juke_update(void) {
    int n = audio_song_count();
    if (n) {
        if (PRESSED(PAD_UP)) juke_sel = (juke_sel + n - 1) % n;
        if (PRESSED(PAD_DOWN)) juke_sel = (juke_sel + 1) % n;
        if (PRESSED(PAD_CROSS)) { if (audio_jukebox_current() == juke_sel) audio_jukebox_stop(); else audio_jukebox_play(juke_sel); }
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
        sprintf(b, "NOW PLAYING: %s", audio_song_name(cur)); gfx_text(16, 22, 1, b, 120, 255, 160);
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
    { "THE CAMERA", "", "FULL 3D: L1 AND R1 ORBIT,", "L2 AND R2 ZOOM, THE RIGHT STICK", "ORBITS AND TILTS.", "WALLS BETWEEN YOU AND THE CAMERA", "DROP LOW, LIKE A SIMS GAME.", "", "THE JOB: STAND ON THE GOLD WORK", "MARKER IN THE FACTORY 9 TO 5.", "BILLS: 800 EVERY MIDNIGHT.", "R1: NEXT PAGE" },
    { "BUILD AND CREATE", "", "BUILD MODE (PAUSE MENU): PLACE", "BORE'S ITEMS, WALLS, FLOORS AND", "103 WALLPAPERS. SELECT PICKS THE", "TOOL, L1 R1 PICK, X PLACES.", "", "CREATE A BORE: EVERY BODY, FACE,", "HAIR, CLOTHES AND SPORE PART OF", "THE GBA GAME, NOW IN 3D.", "", "O: BACK" } };
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
    world_draw(tx, ty, title_t, 1);
    VECTOR p; setVector(&p, fx, 0, fz);
    Gait g = { 0 }; g.phase = (title_t * 120) & 4095; g.amp = 4096;
    voxel_draw(&title_bore, &p, (title_t * 9) & 4095, &g);
    gfx_shadow(&p, 70);
    gfx_textc(160, 14, 3, "BORE 2", GOLD);
    gfx_textc(160, 52, 1, "DIPPINN PRODUCTIONS", GREY);
    gfx_textc(160, 200, 1, "THE VOXEL LIFE, NOW IN FULL 3D", 180, 200, 255);
    if ((title_t / 20) & 1) gfx_textc(160, 222, 1, "PRESS START", WHITE);
}
static const char *const menuNm[] = { "PLAY", "CREATE A BORE", "JUKEBOX", "HOW TO PLAY" };
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
    draw_panel(100, 70, 120, 70);
    for (int i = 0; i < 4; i++) gfx_text(114, 78 + i * 14, 1, menuNm[i], i == menu_sel ? 255 : 170, i == menu_sel ? 255 : 170, i == menu_sel ? 255 : 180);
    gfx_text(106, 78 + menu_sel * 14, 1, ">", GOLD);
}

int main(void) {
    gfx_init();
    atlas_init();
    font_init();
    look_init();
    voxel_init();
    items_init();
    world_init();
    audio_init();
    InitPAD(pad_buf[0], 34, pad_buf[1], 34);
    StartPAD();
    ChangeClearPAD(0);
    look_default(&edit);
    look_default(&title_bore); look_random(&title_bore);
    world_gen();
    strcpy(edit.first, "DIPPA"); strcpy(edit.last, "DIPPINN");
    audio_jukebox_play(0);   // THE DIPPER MAN, the title song (as on the GBA)
    t_vb = VSync(-1);
    while (1) {
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
            case ST_LIFE: life_update(steps); life_draw(); break;
            case ST_PAUSE: pause_update(); pause_draw(); break;
            case ST_BUILD: build_update(); build_draw(); break;
            case ST_JUKEBOX: juke_update(); juke_draw(); break;
            case ST_HELP: help_update(); help_draw(); break;
            case ST_TRAVEL: travel_update(); travel_draw(); break;
        }
        gfx_flip();
    }
    return 0;
}
