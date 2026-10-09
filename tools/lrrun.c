// lrrun: a tiny headless libretro frontend. Runs a PS1 disc image in pcsx_rearmed (HLE BIOS, no BIOS file needed)
// for N frames, with scripted pad input, and writes PPM screenshots.
//   lrrun <core.so> <game.cue> <frames> <outprefix> [script]
// script: lines "frame button" where button is one of up down left right a b x y l r l2 r2 start select, held for 8 frames,
// or "frame shot" to save a screenshot at that frame. Screenshots also at the end.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "libretro.h"
#include <sys/stat.h>

static void (*p_retro_init)(void);
static void (*p_retro_deinit)(void);
static void (*p_retro_set_environment)(retro_environment_t);
static void (*p_retro_set_video_refresh)(retro_video_refresh_t);
static void (*p_retro_set_audio_sample)(retro_audio_sample_t);
static void (*p_retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void (*p_retro_set_input_poll)(retro_input_poll_t);
static void (*p_retro_set_input_state)(retro_input_state_t);
static bool (*p_retro_load_game)(const struct retro_game_info *);
static void (*p_retro_run)(void);
static void (*p_retro_get_system_av_info)(struct retro_system_av_info *);

static enum retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static int frame_no, want_shot;
static char outprefix[512];
static uint16_t held[16]; static int held_until[16];
static int shots_at[256], nshots;

static void log_cb(enum retro_log_level level, const char *fmt, ...) {
    (void)level; va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
}
static bool env_cb(unsigned cmd, void *data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: ((struct retro_log_callback *)data)->log = log_cb; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: pixfmt = *(enum retro_pixel_format *)data; return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: *(const char **)data = "/tmp/lrsys"; return true;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char **)data = "/tmp/lrsys"; return true;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool *)data = true; return true;
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            struct retro_variable *v = (struct retro_variable *)data;
            v->value = NULL;
            if (!strcmp(v->key, "pcsx_rearmed_bios")) v->value = "hle";
            if (!strcmp(v->key, "pcsx_rearmed_drc")) v->value = "disabled";
            if (!strcmp(v->key, "pcsx_rearmed_show_bios_bootlogo")) v->value = "disabled";
            if (!strcmp(v->key, "pcsx_rearmed_dithering")) v->value = "enabled";
            if (!strcmp(v->key, "pcsx_rearmed_pad1type")) v->value = "analog";
            return v->value != NULL;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool *)data = false; return true;
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return false;
        default: return false;
    }
}
static void save_ppm(const char *path, const void *data, unsigned w, unsigned h, size_t pitch) {
    FILE *f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)data + y * pitch;
        for (unsigned x = 0; x < w; x++) {
            uint8_t rgb[3];
            if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) { uint32_t p = ((const uint32_t *)row)[x]; rgb[0] = p >> 16; rgb[1] = p >> 8; rgb[2] = p; }
            else if (pixfmt == RETRO_PIXEL_FORMAT_RGB565) { uint16_t p = ((const uint16_t *)row)[x]; rgb[0] = (p >> 11) << 3; rgb[1] = ((p >> 5) & 63) << 2; rgb[2] = (p & 31) << 3; }
            else { uint16_t p = ((const uint16_t *)row)[x]; rgb[0] = ((p >> 10) & 31) << 3; rgb[1] = ((p >> 5) & 31) << 3; rgb[2] = (p & 31) << 3; }
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}
static int frames_seen, frames_null;
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) { frames_null++; return; }
    frames_seen++;
    int shot = want_shot;
    if (shot) { char p[600]; snprintf(p, sizeof p, "%s_%05d.ppm", outprefix, frame_no); save_ppm(p, data, w, h, pitch); want_shot = 0; }
}
static FILE *audio_out;
static void audio_cb(int16_t l, int16_t r) { if (audio_out) { fwrite(&l, 2, 1, audio_out); fwrite(&r, 2, 1, audio_out); } }
static long audio_calls, audio_frames;
static size_t audio_batch_cb(const int16_t *d, size_t frames) { audio_calls++; audio_frames += frames; if (audio_out) fwrite(d, 4, frames, audio_out); return frames; }
static void input_poll_cb(void) {}
static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;
    if (port != 0) return 0;
    if (device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG) {
        if (device == RETRO_DEVICE_ANALOG) return 0;
        if (id < 16 && frame_no < held_until[id]) return 1;
    }
    return 0;
}
static int button_id(const char *s) {
    static const char *names[] = { "b", "y", "select", "start", "up", "down", "left", "right", "a", "x", "l", "r", "l2", "r2", "l3", "r3" };
    for (int i = 0; i < 16; i++) if (!strcmp(s, names[i])) return i;
    return -1;
}
int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: lrrun core.so game.cue frames outprefix [script]\n"); return 1; }
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
#define L(n) *(void **)(&p_##n) = dlsym(h, #n)
    L(retro_init); L(retro_deinit); L(retro_set_environment); L(retro_set_video_refresh); L(retro_set_audio_sample);
    L(retro_set_audio_sample_batch); L(retro_set_input_poll); L(retro_set_input_state); L(retro_load_game); L(retro_run); L(retro_get_system_av_info);
    int frames = atoi(argv[3]);
    strncpy(outprefix, argv[4], sizeof outprefix - 1);
    // the script: "frame button" presses (held 8 frames) and "frame shot"
    typedef struct { int frame; int btn; } Ev; static Ev evs[4096]; int nev = 0;
    if (argc > 5) {
        FILE *f = fopen(argv[5], "r"); char line[128];
        while (f && fgets(line, sizeof line, f)) {
            int fr; char b[32];
            if (sscanf(line, "%d %31s", &fr, b) == 2) {
                if (!strcmp(b, "shot")) { if (nshots < 256) shots_at[nshots++] = fr; }
                else { int id = button_id(b); if (id >= 0 && nev < 4096) { evs[nev].frame = fr; evs[nev].btn = id; nev++; } }
            }
        }
        if (f) fclose(f);
    }
    mkdir("/tmp/lrsys", 0755);
    { char ap[600]; snprintf(ap, sizeof ap, "%s_audio.raw", outprefix); audio_out = fopen(ap, "wb"); }
    p_retro_set_environment(env_cb);
    p_retro_set_video_refresh(video_cb);
    p_retro_set_audio_sample(audio_cb);
    p_retro_set_audio_sample_batch(audio_batch_cb);
    p_retro_set_input_poll(input_poll_cb);
    p_retro_set_input_state(input_state_cb);
    p_retro_init();
    struct retro_game_info gi = { 0 };
    gi.path = argv[2];
    if (!p_retro_load_game(&gi)) { fprintf(stderr, "load_game failed\n"); return 1; }
    for (frame_no = 0; frame_no < frames; frame_no++) {
        for (int i = 0; i < nev; i++) if (evs[i].frame == frame_no) held_until[evs[i].btn] = frame_no + 8;
        for (int i = 0; i < nshots; i++) if (shots_at[i] == frame_no) want_shot = 1;
        if (frame_no == frames - 1) want_shot = 1;
        p_retro_run();
    }
    for (int k = 0; k < 8 && want_shot; k++) { frame_no++; p_retro_run(); }   // a dupe at the end: run on until a real frame comes
    fprintf(stderr, "frames: %d drawn, %d dupes; audio %ld calls %ld frames\n", frames_seen, frames_null, audio_calls, audio_frames); if (audio_out) fclose(audio_out);
    p_retro_deinit();
    return 0;
}
