// arc_i: what the arcade's files share (the games are in ag_*.c, the floor and the till in arcade.c)
#ifdef BORE_DS
#include "ds_arc_i.h"   // (the DS builds the same games against its own drawing calls: ports/dsi/source/ds_agames.c)
#else
#pragma GCC optimize ("Os")
#pragma once
#include "arcade.h"
#include "fight.h"
#include "gfx.h"
#include "ui.h"
#include "sim.h"
#include "audio.h"
#include "wants.h"
#include "pad.h"
#include "gotchi.h"
#include "dog.h"
#include "look.h"

// a game's field: 240 x 160, drawn 5/4 as big inside the bezel (300 x 200)
#define FX(x) (10 + (x) * 5 / 4)
#define FY(y) (20 + (y) * 5 / 4)

void RC(int x, int y, int w, int h, u32 c);                      // ... in a UI_RGB colour
void RA(int x, int y, int w, int h, u32 c);                      // ... half see-through
void DSC(int cx, int cy, int r, u32 c);                          // a disc (r: field radius)
void TX(int x, int y, const char *s, u32 c);                     // small text in field coordinates
void TXC(int cx, int y, const char *s, u32 c);
void TXB(int cx, int y, const char *s, u32 c);                   // big text
void NUM(int x, int y, const char *label, int v, u32 c);         // "LABEL 123"
int  ar8(void);                                                  // 0..255
enum { CUE_GOOD, CUE_BAD, CUE_GREAT, CUE_WIN, CUE_LOSE };
void cue(int k);                                                 // a sound cue (the voices)
void part(int x, int y, int vx, int vy, int life, u32 c);        // a spark (field px, speeds in 1/16 px a tick)
void burst(int x, int y, int n, u32 c);                          // n sparks every which way
void pop(int x, int y, const char *s, u32 c);                    // a floating "+10"
extern int ag_pts, ag_t, ag_lives, ag_combo, ag_best, ag_perf, ag_np, ag_foe, ag_shake, ag_level;   // the running game's score, ticks, lives, combo (best), flawless?, players, a challenger's score to beat, shake, level
#define SOLO() (ag_np < 2)
// a game: start, one 30 Hz tick (hd/pr: the pad's held/pressed; h2/p2: the second pad, or the CPU's stand-in is the game's business), draw
typedef struct { void (*start)(void); int (*tick)(int hd, int pr, int h2, int p2); void (*draw)(void); } AGFn;
#define GAME(n) void n##_start(void); int n##_tick(int, int, int, int); void n##_draw(void);
GAME(brk) GAME(cat) GAME(wak) GAME(str) GAME(blk) GAME(shm) GAME(rac) GAME(mze) GAME(hoo) GAME(ski) GAME(hoc) GAME(rhy) GAME(claw)
#define PAD_ANY (PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE | PAD_SQUARE)
void K(int x, int y, int w, int h, u32 c);                       // a plain rectangle in screen coordinates (one colour: calls stay small)
void KA(int x, int y, int w, int h, u32 c);                      // ... half see-through
#undef R
#define R(x, y, w, h, r, g, b) RC(x, y, w, h, UI_RGB(r, g, b))
// what each game is (arc_core.c)
typedef struct { const char *nm, *tag, *h1, *h2, *song; u8 tok; u16 par; u8 two, r, g, b; } GInfo;
extern const GInfo gi[AG_NGAME]; extern const u8 logic_game[AG_NGAME];
extern const char *const medal_nm[20];
#define SC(e) ((int)((e) & 0x1FFFF))
void ini_str(u32 e, char *o); int hs_rank(int g, int sc); void hs_put(int g, int k, int sc, const char *ini);
void medal(int b);
void need_add(Sim *s, int n, int v);
extern const AGFn ag_fn[AG_NGAME];                                 // the games this overlay holds (the others: 0)
// the cabinet's flow (arc_core.c): the rules page that is also the attract screen, play, game over, the initials. 0 from core_update: back to the floor
void core_open(int g);
int  core_update(int hd, int pr, int steps);
void core_draw(void);
void core_song(int g);                                           // the music of a game (-1: the floor's)
void core_fx_tick(void); void core_fx_draw(void);
extern int core_game, core_ft;
void hearts(int n, int max);
void bezel(int g); void scan(void);
#endif
