// ui: the look of the menus and the HUD, after The Sims 4: white rounded cards with soft shadows, a blue header, dark ink,
// round icon buttons, toggle switches, sliders and pill-shaped bars. Everything is 2D, drawn over the scene in call order.
#pragma once
#include "common.h"

#define UI_RGB(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16))
#define UI_INK    UI_RGB(22, 28, 40)       // text on the white cards
#define UI_SOFT   UI_RGB(80, 92, 112)      // quieter text
#define UI_BLUE   UI_RGB(38, 146, 222)     // the accent
#define UI_BLUE2  UI_RGB(26, 112, 190)
#define UI_GREEN  UI_RGB(84, 196, 72)      // the plumbob
#define UI_WHITE  UI_RGB(255, 255, 255)
#define UI_CARD   UI_RGB(240, 244, 249)
#define UI_CARD2  UI_RGB(214, 223, 235)
#define UI_LINE   UI_RGB(196, 206, 220)
#define UI_HILITE UI_RGB(214, 236, 252)
#define UI_PANEL  UI_RGB(234, 240, 248)    // a box inside a card
#define UI_RED    UI_RGB(228, 84, 70)
#define UI_AMBER  UI_RGB(236, 176, 40)
#define UI_GOODT  UI_RGB(30, 132, 48)      // green, deep enough to read on white
#define UI_BADT   UI_RGB(204, 58, 48)      // red, the same
#define UI_GOLDT  UI_RGB(170, 116, 8)      // gold, the same

enum { IC_PLAY, IC_PLUMBOB, IC_BOOK, IC_CAR, IC_PIN, IC_PEOPLE, IC_STAR, IC_ADD, IC_PARTY, IC_HAMMER, IC_NOTE, IC_GEAR, IC_HELP,
       IC_SPROUT, IC_DOOR, IC_EYE, IC_TREE, IC_SUN, IC_BOLT, IC_CAMERA, IC_FOOD, IC_DROP, IC_BED, IC_BUBBLES, IC_SOFA, IC_SMILE,
       IC_CHAT, IC_HOUSE, IC_PAUSE, IC_FAST, IC_MONEY, IC_TICK, IC_LOCK, IC_HEART, IC_CASE, IC_BRICK, IC_TILES, IC_BRUSH, IC_DICE,
       IC_SHIRT, IC_BX, IC_BO, IC_BT, IC_BS, IC_N };   // (IC_BX .. IC_BS: the pad's cross, circle, triangle and square)

void ui_init(void);                                            // the icons into the texture atlas (once)
int  ui_tw(int size, const char *s);                           // its width
void ui_text(int x, int y, int size, const char *s, u32 c);    // size 1 the interface's type, 2 big
void ui_textc(int cx, int y, int size, const char *s, u32 c);
void ui_round(int x, int y, int w, int h, int r, u32 top, u32 bot, int semi);   // a rounded box, shaded top to bottom
void ui_shadow(int x, int y, int w, int h, int r);             // its soft shadow (call first)
void ui_card(int x, int y, int w, int h);                      // a white card with its shadow
int  ui_window(int x, int y, int w, int h, const char *title, int icon);   // a card with a blue header; returns the content's top y
void ui_icon(int x, int y, int icon, u32 c);                   // a 12x12 icon in a colour
void ui_disc(int cx, int cy, int d, u32 c);                    // a filled circle, d across
void ui_icon_disc(int cx, int cy, int d, int icon, u32 disc, u32 ink);   // an icon on a round button
void ui_row(int x, int y, int w, int h, const char *label, int icon, int sel);   // a menu line (the chosen one lit blue)
void ui_button(int x, int y, int w, int h, const char *label, int icon, int sel);   // a rounded button with an icon disc
void ui_toggle(int x, int y, int on, int sel);                 // a switch, 22 x 11
void ui_slider(int x, int y, int w, int v, int n, int sel);    // step v of n
void ui_bar(int x, int y, int w, int h, int pct, u32 c);       // a pill-shaped bar, filled to pct
void ui_badge(int x, int y, const char *s, u32 bg, u32 ink);   // a little pill of text
int  ui_wrap(int x, int y, int w, int lines, const char *s, u32 c);   // words broken onto lines that fit
int  ui_wrap_n(int w, const char *s);                          // how many lines that would take
void ui_dim(void);                                             // darken the scene behind a menu
void ui_backdrop(void);                                        // the soft blue behind a full-screen window
void ui_icon_s(int x, int y, int icon, int size, u32 c);       // an icon drawn size x size (24, 36: big and chunky)
int  ui_badge_i(int x, int y, int icon, const char *s, u32 bg, u32 ink);   // a pill with an icon in front; returns its width
int  ui_key(int x, int y, const char *k);                      // a pad button, 12 high: X O TRI SQ L1 R1 L2 R2 L3 R3 START SEL DPAD RS; its width
int  ui_hints(int x, int y, const char *spec, int align);     // "X PLACE|O BACK": the buttons, then what they do. align 0 left, 1 centre, 2 right
void ui_hintbar(int y, const char *spec);                      // the same on a white pill across the middle of the screen
int  ui_window_s(int x, int y, int w, int h, const char *title);   // a card with a slim blue header; returns the content's top y
void ui_pips(int x, int y, int v, int n, u32 c);               // n little blocks, v of them filled (12 apart)
void ui_meter(int x, int y, int w, int m);                     // -100..100 out from the middle: green to the right, red to the left
void ui_section(int x, int y, int w, const char *title);       // a section's name with a thin rule after it
void ui_bubble(int cx, int y, const char *s, u32 c);           // a speech bubble; y is its bottom edge (the tail hangs below)
