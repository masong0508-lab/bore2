// ghost.c - GHOSTS (borrowed from BORE on the GBA, fx.h): a Bore of the family who has passed on rises from their gravestone at
// night and drifts about near it, through the walls, see-through. Wander close and they say BOO: your Bore screams (one of the
// ghost-scare takes, a male or a female voice), the mood drops, and the ghost leaves you alone a while. The household screams too.
// OPTIONS > LIFE > GHOSTS: OFF, ON (at night), HAUNTED (day and night). Nothing new is saved: the graves are the ghosts.
#include "ghost.h"
#include "sim.h"
#include "world.h"
#include "gfx.h"
#include "atlas.h"
#include "audio.h"
static const char ghArt[2][32][17]={
 {
    "                ",
    "                ",
    "                ",
    "     ######     ",
    "   #ooooooos#   ",
    "  #ooooooooos#  ",
    " #ooooooooooos# ",
    " #ooooooooooos# ",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#ooooooooooooos#",
    "#ooooommmmoooos#",
    "#oooooommooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#oooooooooooooo#",
    "#ooo#oo##oo#ooo#",
    "#oo# #o##o# #oo#",
    " ##   ##  ##   #",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "
 },
 {
    "                ",
    "                ",
    "                ",
    "     ######     ",
    "   #ooooooos#   ",
    "  #ooooooooos#  ",
    " #ooooooooooos# ",
    " #ooooooooooos# ",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#oooeeooooeeoos#",
    "#ooooooooooooos#",
    "#ooooommmmoooos#",
    "#oooooommooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooos#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#ooooooooooooss#",
    "#oooooooooooooo#",
    "#oo#ooo##ooo#oo#",
    "#o# #oo##oo# #o#",
    " #   ##  ##   ##",
    "                ",
    "                ",
    "                ",
    "                ",
    "                "
 }
};
int ghost_mode = 1;
static TexRef gtex[2]; static int gtex_ok;
#define GH_MAX 6
static u16 gcool[GH_MAX];
void ghost_init(void) { extern void ghost_tex_(void); ghost_tex_(); }
void ghost_tex_(void) {   // the two frames, see-through (the body blends with what is behind; the eyes and mouth do not)
    u16 *px = atlas_scratch_hi();   // (16 x 32)
    for (int f = 0; f < 2; f++) {
        for (int y = 0; y < 32; y++) for (int x = 0; x < 16; x++) { char c = ghArt[f][y][x]; u16 v = 0;
            if (c == '#') v = RGB(12, 14, 22) | 0x8000; else if (c == 'o') v = RGB(27, 29, 31) | 0x8000; else if (c == 's') v = RGB(17, 21, 28) | 0x8000;
            else if (c == 'e') v = RGB(2, 2, 6); else if (c == 'm') v = RGB(6, 4, 10);
            px[y * 16 + x] = v; }
        gtex[f] = atlas_try(16, 32); if (gtex[f].tpage) atlas_upload(&gtex[f], px);
    }
    gtex_ok = 1;
}
static int is_night_(void) { return clock_min >= 21 * 60 || clock_min < 5 * 60; }
static int ghost_out(void) { return ghost_mode == 2 || (ghost_mode == 1 && is_night_()); }
static void ghost_at(int i, int gx, int gy, int t, int *wx, int *wz) {   // where ghost i drifts: round its grave, slowly, a different path each
    int a = (t * (3 + i) + i * 1300) & 4095, b = (t * (2 + i) / 2 + i * 700) & 4095;
    *wx = gx * TILE + TILE / 2 + ((isin(a) * 3 * TILE) >> 12);
    *wz = -(gy * TILE + TILE / 2) - ((icos(b) * 3 * TILE) >> 12);
}
void ghost_tick(int steps, int t) {
    if (!ghost_mode || !ghost_out()) return;
    int n = MIN(sim_grave_count(), GH_MAX);
    for (int i = 0; i < n; i++) {
        int gx, gy; if (!sim_grave_spot(i, &gx, &gy)) continue;
        if (gcool[i] > steps) { gcool[i] -= (u16)steps; continue; } gcool[i] = 0;
        int wx, wz; ghost_at(i, gx, gy, t, &wx, &wz);
        int scared = 0, voiced = 0;
        for (int k = 0; k < nsims; k++) { Sim *s = &sims[k]; if (s->away || s->bore.stage == AG_BABY) continue;
            int d = (ABS(s->x - wx) + ABS(s->z - wz)) / TILE; if (d > (k == player ? 4 : 3)) continue;
            scared = 1; s->mood = MAX(-100, s->mood - 15); sim_say_to(s, "AAAH!");
            if (!voiced) { voiced = 1; static const u8 take[4] = { VOICE_SCARE_M1, VOICE_SCARE_M2, VOICE_SCARE_F1, VOICE_SCARE_F2 };
                           audio_voice(take[((s->uid & 1) << 1) | (t & 1)]); } }
        if (scared) { char b[40], nm[NAME_LEN]; sim_grave_named(i, nm); snprintf(b, sizeof b, "BOO! THE GHOST OF %s", nm); sim_note(b); }
        gcool[i] = (u16)(scared ? 240 : 30);
    }
}
void ghost_draw(int cam_yaw, int t) {
    if (!ghost_mode || !ghost_out()) return;
    if (!gtex_ok || !gtex[0].tpage) return;   // (no room in the atlas for them: no ghosts drawn, the scares still happen)
    int n = MIN(sim_grave_count(), GH_MAX);
    for (int i = 0; i < n; i++) {
        int gx, gy; if (!sim_grave_spot(i, &gx, &gy)) continue;
        int wx, wz; ghost_at(i, gx, gy, t, &wx, &wz);
        VECTOR p; setVector(&p, wx, -60 - ((isin((t * 40 + i * 900) & 4095) * 30) >> 12), wz);   // (floating, bobbing)
        gfx_set_object(&p, cam_yaw & 4095);   // (facing the camera)
        const TexRef *tx = &gtex[(t >> 4) & 1];
        SVECTOR a, b, c, d; setVector(&a, -110, -420, 0); setVector(&b, 110, -420, 0); setVector(&c, -110, 0, 0); setVector(&d, 110, 0, 0);
        int u0 = tx->u, v0 = tx->v, u1 = tx->u + 15, v1 = tx->v + 31;
        gfx_quad_ft(&a, &b, &c, &d, tx->tpage, 0, u0, v0, u1, v0, u0, v1, u1, v1, 128, 0, 1);
    }
    gfx_set_identity();
}
