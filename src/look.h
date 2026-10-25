// look: a BORE person. The look (shape, hair, clothes, parts, sliders) is turned into the 6x4x8 voxel model by the
// GBA game's own buildLook() (look_port.inc), then rendered in 3D by voxel.c.
#pragma once
#include "common.h"
#include "atlas.h"
#include "look_enum.inc"
#define NEYE_PUB 9   // eye styles (the dec codes below this are eyes, then mouths)
#define AS_PICK 5   // the aspirations the creator offers (GROW UP comes with being young)
#define LTN 4

#define LK_COUNT 128
// BORE 2's own wardrobe (after the GBA looks): the WEAR tab's accessories, their two colours, and clothes patterns
enum { LK_X_HEAD = LK_N, LK_X_FACE, LK_X_NECK, LK_X_BACK, LK_X_HAND, LK_X_WAIST, LK_X_COLA, LK_X_COLB, LK_X_TOPPAT, LK_X_BOTPAT, LK_X_PATTONE,
       LK_X_EARS, LK_X_WRIST, LK_X_SHOULDER, LK_X_FEET, LK_X_THEME,
       LK_X_MAKEUP, LK_X_LIPS, LK_X_MARKS, LK_X_END };   // (the bigger FACE tab: eye make-up, lip colour, a second set of cheek marks)   // (the last four came with the big wardrobe: appended, so saves keep their places)
#define NX_COL 32
#define NAME_LEN 12

#define NCELL_MAX 72   // solid blocks a Bore keeps (the build box holds 192, but 6,000 Bores - rolled, every shape with every hair, every slider at its ends - never passed 60)
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
    u16  facepal[17];           // face art colours: k w r s i b g h l y p c n e t m a (FACE_KEYS)
    u8   sty_eye, sty_mouth;    // the eye / mouth style the dec codes refer to
    u8   legs, torso, headsz;   // bodyPlan: rows of legs, rows of torso, head scale
    u8   bx0, bxw, bxh, bxd;    // the build box this stage uses
    TexRef tex_eye, tex_mouth;  // the baked face art (allocated once per person, re-uploaded on every rebuild)
    u8   baked;
    // the drawing cache (voxel_prepare): the solid cells, grouped by body part, with their hidden faces worked out once
    u8   ncell;
    struct { u8 x, y, z, v, hide, grp, flags, pad; SVECTOR c[8]; } cell[NCELL_MAX];   // c: the 8 corners (top 4, bottom 4), rounded hull and wedges folded in; flags: 1 hair, 2 limb
    u8   ndec;
    // where the WEAR tab's accessories attach (voxel_prepare), body-space units
    s16  w_headx, w_heady, w_headz, w_headh;    // the top centre of the head (on the hair), the head's half width
    s16  w_facey, w_facez;                       // the eye line and the face's front
    s16  w_tx, w_shy, w_hipy, w_tfz, w_tbz;      // torso centre x, shoulder and hip lines, torso front and back
    s16  w_hx, w_hy, w_hz;                       // the right hand's bottom centre (in the right arm's space)
    // the head's sculpt (voxel.c head_xf): scaled about its bottom centre, the jaw tapered, lifted onto a neck
    s16  h_cx, h_cz, h_cy0, h_top, h_k, h_jaw, h_lift, h_neckw;
    s16  w_lax, w_rax, w_lgx, w_rgx;             // the shoulder and hip joints' x (arm and leg centres), for the A-pose and the stance
    u8   decc[8];               // the cells that carry a face sprite's top-left
    s16  j_el[2][3], j_kn[2][3];  // the elbows and knees (left, right): the joints the forearms and shins hang from (voxel.c split_limbs)
    u8   j_ok;                  // bits: 1 2 the elbows, 4 8 the knees are there to bend
} __attribute__((aligned(8))) Bore;   // (aligned: the GTE loads the corners as words; an array of Bores must keep every one on a word boundary)

void look_init(void);
void look_default(Bore *b);                 // the starter creature (look 0 everywhere)
void look_random(Bore *b);                  // the Create-A-Bore dice
void look_build(Bore *b);                   // look -> vox / dec / colours / face textures
int  look_allowed(const Bore *b, int id, int v);
void look_fix(Bore *b);
int  look_theme_named(const char *nm);    // an outfit by its name (0: none)
void look_theme(Bore *b, int t);            // dress them in a ready-made outfit (the WEAR tab's OUTFIT: 0 takes everything off)                     // after a birthday: anything the new age may not wear changed to something it may
void look_release(Bore *b);                 // a Bore that has gone: its face textures are kept for the next new face
void look_own_face(Bore *b);                // a copy of a Bore gets face textures of its own (and is built again)
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
u32  look_seed(u32 s);
const char *look_last_name(int i);          // one of the 16 surnames                      // reseed the dice, returns the old state

// the creator's rows: tab, label, look id (or LK_AGE for the stage row), kind
enum { ROW_PICK, ROW_COLOUR, ROW_SLIDER, ROW_AGE, ROW_PERSONA };
#define LK_AGE 250
#define LK_ASP 251
#define LK_LTW 252
#define LK_TRAIT0 240   // 240..244 the five traits
typedef struct { const char *label; u8 id; u8 kind; } LookRow;
#define NTAB 7
extern const char *const look_tab_name[NTAB];
int  look_tab_rows(int tab, const LookRow **rows);
