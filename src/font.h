// font: BORE 2's text styles. The glyph sheets are the disc file FONTS.BIN (tools/make_fonts.py, docs/FONTS.md): 4-bit grey sheets
// in one block of VRAM, drawn as textured sprites that add (light text) or take away (dark ink on a white card) the grey.
//   FS_BODY  clean small body type, caps 7 px   FS_READ  the readable setting: wider and heavier, caps 7 px
//   FS_HEAD  rounded bold for titles, caps 9    FS_NUM   tabular digits for money and the clock (other letters: BODY)
//   FS_FUN   chunky playful title type, caps 12 FS_BIG   big letters and digits, caps 20
// Every y is the TOP OF THE CAPITALS (the cap line); the baseline is font_cap(style) rows below it.
#pragma once
#include "common.h"
enum { FS_BODY, FS_READ, FS_HEAD, FS_NUM, FS_FUN, FS_BIG, FS_N };
enum { LOOK_SIMS, LOOK_READABLE, LOOK_CAPS };   // the TEXT option: Title and sentence case in the friendly fonts; the readable fonts (heavier, wider); the text exactly as written
extern int font_look;                           // (set by OPTIONS; 0 until then)
void font_init(void);
int  font_ready(void);                          // 1 when the sheets are in VRAM
int  font_line(int style);                      // height of a line cell
int  font_cap(int style);                       // height of the capitals
int  font_w(int style, const char *s);          // advance width of s (what the next text would add to x)
int  font_ink_w(int style, const char *s);      // the width of the visible ink (no trailing space): what to centre
void font_text(int style, int x, int y, const char *s, int r, int g, int b);
void font_textc(int style, int cx, int y, const char *s, int r, int g, int b);
void font_debug(void);
int  font_w_zoom(int zoom, const char *s);
void font_text_zoom(int x, int y, int zoom, const char *s, int r, int g, int b);   // the FUN type, zoom times as big
int  font_style_of(int scale);                  // the old gfx_text scale: 1 = BODY (READ in the readable look), 2 = HEAD, 3 and up = BIG
int  font_case(const char *s, char *out, int n);   // the text in the friendly case (Title Case for a few words, Sentence case for more), into out; 1 if it changed
const char *font_shaped(const char *s, char *buf, int n);   // font_case, or s itself when nothing changes (or the look says capitals): call it on a whole text before wrapping it
extern int font_raw;   // set to 1 around text that must stay as written
