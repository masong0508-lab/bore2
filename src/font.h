// font: BORE's smooth font on the PS1. size 0 = s (cap 5), 1 = m (cap 10), 2 = l (cap 24, the title's letters only)
#pragma once
#include "common.h"
void font_init(void);
int  font_line(int size);
int  font_w(int size, const char *s);
void font_text(int x, int y, int size, const char *s, int r, int g, int b);
void font_textc(int cx, int y, int size, const char *s, int r, int g, int b);
void font_debug(void);
int  font_w_zoom(int zoom, const char *s);
void font_text_zoom(int x, int y, int zoom, const char *s, int r, int g, int b);   // the medium type, zoom times as big
