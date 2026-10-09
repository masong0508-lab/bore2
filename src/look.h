// look: a BORE person. The look (shape, hair, clothes, parts, sliders) is turned into the 6x4x8 voxel model by the
// GBA game's own buildLook() (look_port.inc), then rendered in 3D by voxel.c.
#pragma once
#include "common.h"
#include "atlas.h"
#include "look_enum.inc"
#define NEYE_PUB 9   // eye styles (the dec codes below this are eyes, then mouths)
#define AS_PICK 5   // the aspirations the creator offers (GROW UP comes with being young)
#define LTN 4

#define LK_COUNT 112
#define NAME_LEN 12

typedef struct {
    // what the creator chose
    u8   look[LK_COUNT];
    u8   stage;                 // AG_*
    u8   asp, ltw, tr[TR_N];    // persona (ASPIRE tab)
    char first[NAME_LEN], last[NAME_LEN];
    // what buildLook made of it
    u8   vox[H][D][W];          // low nibble: colour slot 1..8, high nibble: shape (0 cube, 1 arm, 2 hand, 3 leg, 4..11 wedge)
    u16  dec[H][D][W];          // face sprite codes on the +Z face
    u16  col[9];                // colour slots: 1 skin 2 white 3 black 4 red 5 hair 6 top 7 bottom 8 gold
    u16  facepal[9];            // face art colours: k w r s i b g h l
    u8   sty_eye, sty_mouth;    // the eye / mouth style the dec codes refer to
    u8   legs, torso, headsz;   // bodyPlan: rows of legs, rows of torso, head scale
    u8   bx0, bxw, bxh, bxd;    // the build box this stage uses
    TexRef tex_eye, tex_mouth;  // the baked face art (allocated once per person, re-uploaded on every rebuild)
    u8   baked;
    // the drawing cache (voxel_prepare): the solid cells, grouped by body part, with their hidden faces worked out once
    u8   ncell;
    struct { u8 x, y, z, v, hide, grp, flags, pad; SVECTOR c[8]; } cell[W * D * H];   // c: the 8 corners (top 4, bottom 4), rounded hull and wedges folded in; flags: 1 hair, 2 limb
    u8   ndec;
    u8   decc[8];               // the cells that carry a face sprite's top-left
} Bore;

void look_init(void);
void look_default(Bore *b);                 // the starter creature (look 0 everywhere)
void look_random(Bore *b);                  // the Create-A-Bore dice
void look_build(Bore *b);                   // look -> vox / dec / colours / face textures
int  look_allowed(const Bore *b, int id, int v);
int  look_count(int id);                    // options in row id (sliders: 9)
int  look_is_slider(int id);
const char *look_option_name(int id, int v); // 0 when the row is a colour or a slider
u16  look_option_colour(int id, int v);     // the swatch, for colour rows
const char *look_stage_name(int stage);
const char *look_shape_name(int v);
const char *look_asp_name(int a);
const char *look_trait_name(int t);
const char *look_sign_name(int s);
int  look_sign_of(const Bore *b);
void look_set_sign(Bore *b, int s);
u32  look_rand(void);

// the creator's rows: tab, label, look id (or LK_AGE for the stage row), kind
enum { ROW_PICK, ROW_COLOUR, ROW_SLIDER, ROW_AGE, ROW_PERSONA };
#define LK_AGE 250
#define LK_ASP 251
#define LK_LTW 252
#define LK_TRAIT0 240   // 240..244 the five traits
typedef struct { const char *label; u8 id; u8 kind; } LookRow;
#define NTAB 6
extern const char *const look_tab_name[NTAB];
int  look_tab_rows(int tab, const LookRow **rows);
