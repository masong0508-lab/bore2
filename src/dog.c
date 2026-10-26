// dog: the breeds, the colours and the dice for the CREATE-A-DOG pack (see dog.h)
#include "dog.h"
#include <string.h>

DogLook pet_dog, dog_edit; int dog_want_name, dog_resume, dog_tab, dog_row;
const char *const dog_breed_name[DOG_NBREED] = { "LABRADOR", "CORGI", "GREYHOUND", "BULLDOG", "HUSKY", "DACHSHUND", "POODLE", "TERRIER", "SHEPHERD", "PUG", "SPANIEL", "MUTT" };
const char *const dog_pal_name[DOG_NPAL] = { "BLACK", "DARK BROWN", "BROWN", "TAN", "CREAM", "WHITE", "GREY", "CHARCOAL", "SILVER", "GINGER", "RED", "GOLD", "YELLOW", "GREEN", "TEAL", "BLUE", "NAVY", "PURPLE", "PINK", "HOT PINK", "LIME", "SKY", "MAROON", "MINT" };
static const u8 pal[DOG_NPAL][3] = { { 30, 28, 32 }, { 80, 50, 30 }, { 150, 100, 60 }, { 206, 172, 120 }, { 238, 225, 190 }, { 242, 242, 238 }, { 140, 140, 148 }, { 80, 80, 88 }, { 190, 196, 208 }, { 226, 140, 50 }, { 190, 50, 40 }, { 230, 190, 50 },
                                     { 240, 225, 90 }, { 70, 160, 70 }, { 60, 170, 170 }, { 60, 110, 200 }, { 40, 50, 110 }, { 130, 70, 170 }, { 240, 140, 180 }, { 230, 80, 150 }, { 150, 220, 80 }, { 130, 190, 240 }, { 110, 30, 40 }, { 150, 230, 190 } };
void dog_pal(int i, int *r, int *g, int *b) { const u8 *p = pal[i % DOG_NPAL]; *r = p[0]; *g = p[1]; *b = p[2]; }
// per breed: size legs body head snout ears earsz tail tailsz fur  c1 c2 pat patc
static const u8 BR[DOG_NBREED][14] = {
    { 5, 4, 4, 4, 4, 0, 4, 0, 4, 0,   3, 4, 0, 2 },    // LABRADOR: floppy ears, a thick tail, a short coat
    { 3, 1, 4, 4, 3, 1, 5, 3, 1, 1,   9, 5, 5, 5 },    // CORGI: short legs, big pointed ears, no tail, a white belly
    { 6, 8, 5, 2, 6, 3, 3, 4, 7, 0,   8, 5, 0, 6 },    // GREYHOUND: long legs, a thin head, a whip tail
    { 3, 2, 3, 6, 1, 2, 3, 3, 1, 0,   4, 5, 7, 2 },    // BULLDOG: broad head, a flat face, a stub tail
    { 5, 4, 4, 4, 4, 1, 4, 1, 5, 2,   6, 5, 8, 7 },    // HUSKY: pointed ears, a curled tail, a mask
    { 2, 0, 8, 3, 5, 0, 5, 4, 3, 0,   2, 1, 0, 1 },    // DACHSHUND: very long, very low
    { 4, 5, 3, 4, 5, 0, 5, 5, 3, 3,   5, 4, 0, 4 },    // POODLE: fluffy coat, a pom tail
    { 3, 4, 3, 3, 4, 4, 3, 6, 3, 3,   4, 2, 2, 2 },    // TERRIER: folded ears, a tail up, a shaggy coat
    { 6, 5, 5, 5, 5, 1, 5, 4, 5, 1,   2, 0, 3, 0 },    // SHEPHERD: big pointed ears, a saddle
    { 3, 3, 3, 5, 0, 2, 3, 1, 2, 0,   3, 0, 8, 0 },    // PUG: a flat face, a curled tail, a dark mask
    { 4, 3, 4, 4, 4, 3, 7, 4, 4, 2,   2, 5, 5, 5 },    // SPANIEL: long ears, a feathery coat
    { 4, 4, 4, 4, 4, 4, 4, 5, 4, 1,   3, 5, 1, 1 } }; // MUTT: a bit of everything
void dog_breed(DogLook *d, int br) {
    const u8 *p = BR[br % DOG_NBREED]; u8 *l = d->look; l[DL_BREED] = (u8)(br % DOG_NBREED);
    l[DL_SIZE] = p[0]; l[DL_LEGS] = p[1]; l[DL_BODY] = p[2]; l[DL_HEAD] = p[3]; l[DL_SNOUT] = p[4]; l[DL_EARS] = p[5]; l[DL_EARSZ] = p[6]; l[DL_TAIL] = p[7]; l[DL_TAILSZ] = p[8]; l[DL_FUR] = p[9];
    l[DL_C1] = p[10]; l[DL_C2] = p[11]; l[DL_PAT] = p[12]; l[DL_PATC] = p[13];
}
void dog_default(DogLook *d) {
    memset(d, 0, sizeof *d); dog_breed(d, 0); d->look[DL_NOSEC] = 0; d->look[DL_EYEC] = 1; d->look[DL_EYESZ] = 4; d->look[DL_NOSESZ] = 4; d->look[DL_COLLARC] = 10; d->look[DL_HATC] = 10; d->look[DL_OUTC] = 15;
    for (int i = 0; i < DOG_TR; i++) d->tr[i] = 5;
    strcpy(d->name, "BUDDY");
}
void dog_random(DogLook *d, u32 seed) {
    static const char *const nm[16] = { "BUDDY", "MAX", "BELLA", "ROCKY", "LUNA", "COOKIE", "DUKE", "PEPPER", "MOCHI", "BEAR", "NOODLE", "SCOUT", "WAFFLES", "TANK", "ZIGGY", "BISCUIT" };
    u32 s = seed * 2654435761u + 12345u; dog_default(d);
    #define R(n) (s ^= s << 13, s ^= s >> 17, s ^= s << 5, (int)((s >> 4) % (u32)(n)))
    dog_breed(d, R(DOG_NBREED));
    d->look[DL_SIZE] = (u8)CLAMP(d->look[DL_SIZE] + R(3) - 1, 0, 8); d->look[DL_C1] = (u8)R(10); d->look[DL_C2] = (u8)R(10);
    d->look[DL_EYEC] = (u8)R(3); d->look[DL_EYEST] = (u8)R(4);
    for (int i = 0; i < DOG_TR; i++) d->tr[i] = (u8)(2 + R(7));
    strcpy(d->name, nm[R(16)]);
    #undef R
}
