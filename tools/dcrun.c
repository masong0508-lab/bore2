// dcrun: a headless libretro frontend for OpenGL cores (flycast), for testing the Dreamcast port without a window or a display.
// It makes a surfaceless EGL context (Mesa llvmpipe, software GL), runs the core for N frames with scripted pad input, and
// saves PPM screenshots read back from the core's framebuffer. No Sega BIOS: flycast's HLE BIOS (reios) boots an .elf / .bin directly.
//   dcrun <flycast_libretro.so> <game.elf|.cdi|.gdi> <frames> <outprefix> [script]
// script lines: "frame button" (held 8 frames: up down left right a b x y l r start select) or "frame shot".
// A screenshot is also saved at the last frame. Env: DCRUN_SYS = system/save dir (default /tmp/dcsys).
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include "libretro.h"

// ---- the little EGL / GL we need, loaded by hand (no development headers on this machine) ----
typedef void *EGLDisplay; typedef void *EGLConfig; typedef void *EGLContext; typedef int EGLint; typedef unsigned EGLBoolean;
#define EGL_NONE 0x3038
#define EGL_OPENGL_API 0x30A2
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_BIT 0x0008
#define EGL_SURFACE_TYPE 0x3033
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x1
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x2
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
static void *egl;
static void *(*p_eglGetProcAddress)(const char *);
static EGLDisplay (*p_eglGetPlatformDisplayEXT)(unsigned, void *, const EGLint *);
static EGLBoolean (*p_eglInitialize)(EGLDisplay, EGLint *, EGLint *);
static EGLBoolean (*p_eglBindAPI)(unsigned);
static EGLBoolean (*p_eglChooseConfig)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *);
static EGLContext (*p_eglCreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint *);
static EGLBoolean (*p_eglMakeCurrent)(EGLDisplay, void *, void *, EGLContext);
static EGLDisplay dpy; static EGLContext ctx;
typedef unsigned GLuint; typedef int GLint; typedef int GLsizei; typedef unsigned GLenum;
static void (*glGenFramebuffers)(GLsizei, GLuint *); static void (*glBindFramebuffer)(GLenum, GLuint);
static void (*glGenTextures)(GLsizei, GLuint *); static void (*glBindTexture)(GLenum, GLuint);
static void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
static void (*glTexParameteri)(GLenum, GLenum, GLint);
static void (*glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
static void (*glGenRenderbuffers)(GLsizei, GLuint *); static void (*glBindRenderbuffer)(GLenum, GLuint);
static void (*glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);
static void (*glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint);
static void (*glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
static void (*glFinish)(void); static GLenum (*glCheckFramebufferStatus)(GLenum);
static const unsigned char *(*glGetString)(GLenum);

static void *get_proc(const char *n) { return p_eglGetProcAddress(n); }

static struct retro_hw_render_callback hw;
static GLuint fbo, fbo_tex, fbo_rb; static unsigned fbo_w, fbo_h;
static int ctx_ready;

static void make_fbo(unsigned w, unsigned h) {
    if (!fbo) glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    if (!fbo_tex) glGenTextures(1, &fbo_tex);
    glBindTexture(0x0DE1, fbo_tex);
    glTexImage2D(0x0DE1, 0, 0x8058 /*RGBA8*/, w, h, 0, 0x1908, 0x1401, 0);
    glTexParameteri(0x0DE1, 0x2801, 0x2600); glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fbo_tex, 0);
    if (!fbo_rb) glGenRenderbuffers(1, &fbo_rb);
    glBindRenderbuffer(0x8D41, fbo_rb);
    glRenderbufferStorage(0x8D41, 0x88F0 /*DEPTH24_STENCIL8*/, w, h);
    glFramebufferRenderbuffer(0x8D40, 0x821A /*DEPTH_STENCIL_ATTACHMENT*/, 0x8D41, fbo_rb);
    fbo_w = w; fbo_h = h;
}
static uintptr_t get_fb(void) { return fbo; }

static int init_egl(void) {
    egl = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL); if (!egl) { fprintf(stderr, "no libEGL\n"); return 0; }
#define E(n) *(void **)(&p_##n) = dlsym(egl, #n)
    E(eglGetProcAddress); E(eglInitialize); E(eglBindAPI); E(eglChooseConfig); E(eglCreateContext); E(eglMakeCurrent);
    p_eglGetPlatformDisplayEXT = p_eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!p_eglGetPlatformDisplayEXT) { fprintf(stderr, "no eglGetPlatformDisplayEXT\n"); return 0; }
    dpy = p_eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, 0, 0);
    EGLint maj, min; if (!dpy || !p_eglInitialize(dpy, &maj, &min)) { fprintf(stderr, "eglInitialize failed\n"); return 0; }
    return 1;
}
static int make_ctx(void) {
    int es = hw.context_type == RETRO_HW_CONTEXT_OPENGLES2 || hw.context_type == RETRO_HW_CONTEXT_OPENGLES3;
    p_eglBindAPI(es ? EGL_OPENGL_ES_API : EGL_OPENGL_API);
    const EGLint ca[] = { EGL_RENDERABLE_TYPE, es ? 0x0040 /*ES3*/ : EGL_OPENGL_BIT, EGL_SURFACE_TYPE, 0, EGL_NONE };
    EGLConfig cfg; EGLint n = 0;
    if (!p_eglChooseConfig(dpy, ca, &cfg, 1, &n) || !n) { const EGLint cb[] = { EGL_NONE }; if (!p_eglChooseConfig(dpy, cb, &cfg, 1, &n) || !n) { fprintf(stderr, "no EGL config\n"); return 0; } }
    EGLint at[16]; int k = 0;
    int mj = hw.version_major, mn = hw.version_minor;
    if (hw.context_type == RETRO_HW_CONTEXT_OPENGL_CORE || es || mj >= 3) {
        at[k++] = EGL_CONTEXT_MAJOR_VERSION; at[k++] = mj ? mj : 3; at[k++] = EGL_CONTEXT_MINOR_VERSION; at[k++] = mn;
        if (!es) { at[k++] = EGL_CONTEXT_OPENGL_PROFILE_MASK; at[k++] = hw.context_type == RETRO_HW_CONTEXT_OPENGL_CORE ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT; }
    }
    at[k++] = EGL_NONE;
    ctx = p_eglCreateContext(dpy, cfg, 0, at);
    if (!ctx) { fprintf(stderr, "eglCreateContext failed (type %d %d.%d)\n", hw.context_type, mj, mn); return 0; }
    if (!p_eglMakeCurrent(dpy, 0, 0, ctx)) { fprintf(stderr, "eglMakeCurrent failed\n"); return 0; }
#define G(n) *(void **)(&n) = get_proc(#n)
    G(glGenFramebuffers); G(glBindFramebuffer); G(glGenTextures); G(glBindTexture); G(glTexImage2D); G(glTexParameteri); G(glFramebufferTexture2D);
    G(glGenRenderbuffers); G(glBindRenderbuffer); G(glRenderbufferStorage); G(glFramebufferRenderbuffer); G(glReadPixels); G(glFinish); G(glCheckFramebufferStatus); G(glGetString);
    fprintf(stderr, "GL: %s | %s\n", glGetString(0x1F01), glGetString(0x1F02));
    return 1;
}

static void (*p_retro_init)(void), (*p_retro_deinit)(void), (*p_retro_run)(void);
static void (*p_retro_set_environment)(retro_environment_t), (*p_retro_set_video_refresh)(retro_video_refresh_t);
static void (*p_retro_set_audio_sample)(retro_audio_sample_t), (*p_retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void (*p_retro_set_input_poll)(retro_input_poll_t), (*p_retro_set_input_state)(retro_input_state_t);
static bool (*p_retro_load_game)(const struct retro_game_info *);
static void (*p_retro_get_system_av_info)(struct retro_system_av_info *);

static int frame_no, want_shot; static char outprefix[512]; static int held_until[16];
static int shots_at[1024], nshots; static const char *sysdir = "/tmp/dcsys";
static int ctx_reset_pending;

static void log_cb(enum retro_log_level level, const char *fmt, ...) {
    if (level < RETRO_LOG_WARN && !getenv("DCRUN_VERBOSE")) return;
    va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
}
static bool env_cb(unsigned cmd, void *data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: ((struct retro_log_callback *)data)->log = log_cb; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char **)data = sysdir; return true;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool *)data = true; return true;
        case RETRO_ENVIRONMENT_SET_HW_RENDER: {
            struct retro_hw_render_callback *h = data;
            if (h->context_type == RETRO_HW_CONTEXT_VULKAN || h->context_type == RETRO_HW_CONTEXT_DIRECT3D) return false;
            hw = *h;
            if (!make_ctx()) return false;
            make_fbo(640, 480);
            h->get_current_framebuffer = get_fb; h->get_proc_address = (retro_hw_get_proc_address_t)get_proc;
            hw.get_current_framebuffer = get_fb; hw.get_proc_address = (retro_hw_get_proc_address_t)get_proc;
            ctx_reset_pending = 1; ctx_ready = 1; return true;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            struct retro_variable *v = data; v->value = NULL;
            if (!strcmp(v->key, "reicast_threaded_rendering")) v->value = "disabled";
            if (!strcmp(v->key, "reicast_enable_dsp")) v->value = "disabled";
            if (!strcmp(v->key, "reicast_internal_resolution")) v->value = "640x480";
            if (!strcmp(v->key, "reicast_hle_bios")) v->value = "enabled";
            if (!strcmp(v->key, "reicast_boot_to_bios")) v->value = "disabled";
            if (!strcmp(v->key, "reicast_region")) v->value = "USA";
            if (!strcmp(v->key, "reicast_broadcast")) v->value = "NTSC";
            if (!strcmp(v->key, "reicast_cable_type")) v->value = "VGA";   // a 640x480 progressive picture
            if (!strcmp(v->key, "reicast_per_content_vmus")) v->value = "disabled";
            return v->value != NULL;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool *)data = false; return true;
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return false;
        default: return false;
    }
}
static void save_ppm_rgba(const char *path, const uint8_t *rgba, unsigned w, unsigned h, int flip) {
    FILE *f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned y = 0; y < h; y++) {
        const uint8_t *row = rgba + (size_t)(flip ? h - 1 - y : y) * w * 4;
        for (unsigned x = 0; x < w; x++) fwrite(row + x * 4, 1, 3, f);
    }
    fclose(f);
}
static int frames_seen;
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) return;
    frames_seen++;
    if (!want_shot) return;
    want_shot = 0;
    char p[600]; snprintf(p, sizeof p, "%s_%05d.ppm", outprefix, frame_no);
    if (data == RETRO_HW_FRAME_BUFFER_VALID) {
        uint8_t *buf = malloc((size_t)w * h * 4);
        glBindFramebuffer(0x8D40, fbo); glFinish();
        glReadPixels(0, 0, w, h, 0x1908, 0x1401, buf);
        save_ppm_rgba(p, buf, w, h, hw.bottom_left_origin);
        free(buf);
    } else {   // a software frame (XRGB8888)
        uint8_t *buf = malloc((size_t)w * h * 4);
        for (unsigned y = 0; y < h; y++) for (unsigned x = 0; x < w; x++) { uint32_t px = ((const uint32_t *)((const uint8_t *)data + y * pitch))[x]; uint8_t *o = buf + ((size_t)y * w + x) * 4; o[0] = px >> 16; o[1] = px >> 8; o[2] = px; o[3] = 255; }
        save_ppm_rgba(p, buf, w, h, 0); free(buf);
    }
    fprintf(stderr, "shot %s (%ux%u)\n", p, w, h);
}
static FILE *audio_out;
static void audio_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch_cb(const int16_t *d, size_t frames) { if (audio_out) fwrite(d, 4, frames, audio_out); return frames; }
static void input_poll_cb(void) {}
static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index; if (port != 0 || device != RETRO_DEVICE_JOYPAD) return 0;
    return id < 16 && frame_no < held_until[id];
}
static int button_id(const char *s) {
    static const char *names[] = { "b", "y", "select", "start", "up", "down", "left", "right", "a", "x", "l", "r", "l2", "r2", "l3", "r3" };
    for (int i = 0; i < 16; i++) if (!strcmp(s, names[i])) return i;
    return -1;
}
int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: dcrun core.so game frames outprefix [script]\n"); return 1; }
    if (getenv("DCRUN_SYS")) sysdir = getenv("DCRUN_SYS");
    mkdir(sysdir, 0755);
    if (!init_egl()) return 1;
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
#define L(n) *(void **)(&p_##n) = dlsym(h, #n)
    L(retro_init); L(retro_deinit); L(retro_set_environment); L(retro_set_video_refresh); L(retro_set_audio_sample);
    L(retro_set_audio_sample_batch); L(retro_set_input_poll); L(retro_set_input_state); L(retro_load_game); L(retro_run); L(retro_get_system_av_info);
    int frames = atoi(argv[3]); strncpy(outprefix, argv[4], sizeof outprefix - 1);
    typedef struct { int frame, btn; } Ev; static Ev evs[4096]; int nev = 0;
    if (argc > 5) {
        FILE *f = fopen(argv[5], "r"); char line[128];
        while (f && fgets(line, sizeof line, f)) {
            int fr; char b[32];
            if (sscanf(line, "%d %31s", &fr, b) == 2) {
                if (!strcmp(b, "shot")) { if (nshots < 1024) shots_at[nshots++] = fr; }
                else { int id = button_id(b); if (id >= 0 && nev < 4096) { evs[nev].frame = fr; evs[nev].btn = id; nev++; } }
            }
        }
        if (f) fclose(f);
    }
    { char ap[600]; snprintf(ap, sizeof ap, "%s_audio.raw", outprefix); audio_out = fopen(ap, "wb"); }
    p_retro_set_environment(env_cb); p_retro_set_video_refresh(video_cb); p_retro_set_audio_sample(audio_cb);
    p_retro_set_audio_sample_batch(audio_batch_cb); p_retro_set_input_poll(input_poll_cb); p_retro_set_input_state(input_state_cb);
    p_retro_init();
    { void (*scpd)(unsigned, unsigned) = dlsym(h, "retro_set_controller_port_device"); if (scpd) scpd(0, RETRO_DEVICE_JOYPAD); }
    struct retro_game_info gi = { 0 }; gi.path = argv[2];
    if (!p_retro_load_game(&gi)) { fprintf(stderr, "load_game failed\n"); return 1; }
    struct retro_system_av_info av; p_retro_get_system_av_info(&av);
    fprintf(stderr, "av: %ux%u (max %ux%u) %.2f fps, hw ctx %d\n", av.geometry.base_width, av.geometry.base_height, av.geometry.max_width, av.geometry.max_height, av.timing.fps, ctx_ready ? hw.context_type : -1);
    if (ctx_ready) {
        unsigned mw = av.geometry.max_width ? av.geometry.max_width : 640, mh = av.geometry.max_height ? av.geometry.max_height : 480;
        make_fbo(mw, mh);
        glBindFramebuffer(0x8D40, fbo); fprintf(stderr, "fbo %ux%u status %x\n", mw, mh, glCheckFramebufferStatus(0x8D40));
        if (hw.context_reset) hw.context_reset();
    }
    for (frame_no = 0; frame_no < frames; frame_no++) {
        for (int i = 0; i < nev; i++) if (evs[i].frame == frame_no) held_until[evs[i].btn] = frame_no + 8;
        for (int i = 0; i < nshots; i++) if (shots_at[i] == frame_no) want_shot = 1;
        if (frame_no == frames - 1) want_shot = 1;
        if (getenv("DCRUN_VERBOSE")) fprintf(stderr, "frame %d\n", frame_no);
        p_retro_run();
    }
    for (int k = 0; k < 8 && want_shot; k++) { frame_no++; p_retro_run(); }
    fprintf(stderr, "frames drawn: %d\n", frames_seen);
    if (audio_out) fclose(audio_out);
    _exit(0);   // (the core's threads and the GL context are not worth a clean shutdown here)
}
