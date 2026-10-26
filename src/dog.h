// dog: the CREATE-A-DOG pack. A dog is a DogLook: 29 look rows (sliders, choices, colours) like a Bore's, five traits and a name.
// The dog's maker is the overlay DOG.OVL (dog_ui.c); the model is drawn by items_draw_dog (items.c); breeds, colours and the dice are in dog.c.
#pragma once
#include "common.h"

enum { DL_BREED, DL_SIZE, DL_LEGS, DL_BODY, DL_HEAD, DL_SNOUT, DL_EARS, DL_EARSZ, DL_TAIL, DL_TAILSZ, DL_FUR,      // the body
       DL_C1, DL_C2, DL_PAT, DL_PATC, DL_NOSEC,                                                                       // the coat
       DL_EYEC, DL_EYESZ, DL_EYEST, DL_BROWS, DL_MOUTH, DL_NOSESZ,                                                    // the face
       DL_COLLAR, DL_COLLARC, DL_HAT, DL_HATC, DL_GLASS, DL_OUTFIT, DL_OUTC,                                          // the wear
       DL_N };
#define DOG_TR 5   // traits: PLAYFUL, LAZY, LOYAL, HUNGRY, BRAVE (0..10 each)
#define DOG_NAME 13
typedef struct { u8 look[DL_N]; u8 tr[DOG_TR]; char name[DOG_NAME]; } DogLook;
extern DogLook pet_dog;
extern DogLook dog_edit; extern int dog_want_name, dog_resume, dog_tab, dog_row;   // the maker hands the dog to the typing screen and comes back for it

#define DOG_NBREED 12
#define DOG_NPAL 24
#define DOG_NPAT 9
extern const char *const dog_breed_name[DOG_NBREED];
extern const char *const dog_pal_name[DOG_NPAL];
void dog_pal(int i, int *r, int *g, int *b);
void dog_default(DogLook *d);                       // a plain brown dog
void dog_breed(DogLook *d, int breed);              // a breed's shape and coat (the wear and the face are left)
void dog_random(DogLook *d, u32 seed);              // a whole new dog (a store's puppy)
