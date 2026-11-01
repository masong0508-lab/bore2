// BORE 2 on the Dreamcast: the platform layer's program. Stages: BOOT (hello), TITLE (the logo), LIFE (SAN BOREAS in 3D).
// The game's shared code (bigmap.c, look.c, pop.c ...) is compiled in from ../../src unchanged; this file only talks to KOS.
#include <kos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "dc_gfx.h"
#include "dc_life.h"
#include "logo_rects.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

enum { ST_BOOT, ST_TITLE, ST_LIFE };
static int state = ST_BOOT, t_state;

static uint32_t rgb15(unsigned short c) {
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

static void boot_draw(int t) {
    dc_frame_begin(0x081028, 0x182860);
    dc_textc(160, 90, 2.0f, "BORE 2", 0xffffff);
    dc_textc(160, 120, 1.0f, "HELLO FROM THE DREAMCAST", 0xe0e0ff);
    dc_textc(160, 136, 1.0f, "KALLISTIOS + POWERVR  640X480", 0x9fb0e0);
    char b[48]; sprintf(b, "FRAME %d", t);
    dc_textc(160, 160, 1.0f, b, 0x80ff80);
    dc_frame_end();
}

static void title_draw(int t) {
    dc_frame_begin(0x203a8a, 0x60a0e0);
    // a few clouds of ground below the logo: the town's lounge is the PS1's backdrop; here the plain evening sky and the logo
    dc_rect_bg(0, 150, 320, 90, 0x3b6a3a);
    dc_rect_bg(0, 150, 320, 3, 0x5a9a50);
    life_title_scene(t);
    for (int i = 0; i < LOGO_N; i++) {   // the BORE 2 logo (tools/make_logo.py), drawn at 6/5 size exactly as the PS1 does
        int x = 70 + LOGO_R[i].x * 6 / 5, y = 4 + LOGO_R[i].y * 6 / 5;
        int w = 70 + (LOGO_R[i].x + LOGO_R[i].w) * 6 / 5 - x, h = 4 + (LOGO_R[i].y + LOGO_R[i].h) * 6 / 5 - y;
        dc_rect(x, y, w < 1 ? 1 : w, h < 1 ? 1 : h, rgb15(LOGO_P[LOGO_R[i].c]));
    }
    dc_textc(160, 112, 1.0f, "DIPPINN PRODUCTIONS", 0xe6e6ec);
    dc_rect(66, 196, 188, 36, 0xf0f4ff); dc_rect(67, 197, 186, 34, 0xffffff);
    dc_textc(160, 201, 0.75f, "THE VOXEL LIFE, NOW IN FULL 3D", 0x1a70be);
    if ((t / 20) & 1) dc_textc(160, 216, 1.0f, "START PLAY", 0x305078);
    { char pb[32]; sprintf(pb, "PAD %04X", dc_pad()); dc_text(4, 230, 0.75f, pb, 0x90a0c0); }
    dc_frame_end();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    dc_init();
    life_init();
    int frame = 0, prev_pad = 0;
    for (;;) {
        int pad = dc_pad(), pressed = pad & ~prev_pad; prev_pad = pad;
        t_state++; frame++;
        if (state == ST_BOOT) {
            boot_draw(t_state);
            if (t_state > 60 || (pressed & CONT_START)) { state = ST_TITLE; t_state = 0; }
        } else if (state == ST_TITLE) {
            title_draw(t_state);
            if (pressed & (CONT_START | CONT_A)) { state = ST_LIFE; t_state = 0; life_enter(); }
        } else {
            life_frame(t_state, pad, pressed);
            if (pressed & CONT_B) { state = ST_TITLE; t_state = 0; }
        }
    }
    return 0;
}
