// gotchi_act: the names, the foods and the hands-on actions of the BOREAGOTCHI! pack (the overlay GOTCHI.OVL; the rules that run all the time are gotchi.c)
#pragma once
#include "gotchi.h"

#define NFOOD 12
#define NACC 8
#define NGAME 4
#define NACH 16
const char *gx_species(int sp);
const char *gx_given(int nm);
const char *gx_pers(int p);
const char *gx_pers_note(int p);
const char *gx_form(int f);
const char *gx_stage(int s);
const char *gx_colour(int c);
const char *gx_acc(int a);
int         gx_acc_cost(int a);
const char *gx_food(int f);
int         gx_food_cost(int f);
const char *gx_level(int lv);
const char *gx_ach(int i);
const char *gx_ach_how(int i);
const char *gx_game(int g);
const char *gx_room(int r);
void  gx_name(const Blob *b, char *out);                 // "SUPER FROG"
const char *gx_want(const Blob *b);                      // what it is asking for
void  gx_diary(const Diary *d, char *out);               // one line of the diary
// actions: each returns what to say
const char *gx_feed(Toy *t, int slot, int food);
const char *gx_lunch(Toy *t, int slot);
const char *gx_light(Toy *t, int slot);
const char *gx_medicine(Toy *t, int slot);
const char *gx_clean(Toy *t, int slot);
const char *gx_scold(Toy *t, int slot);
const char *gx_pet(Toy *t, int slot);                    // cuddle: calms a tantrum, a nuzzle cheers the owner
const char *gx_game_end(Toy *t, int slot, int won, int game);
const char *gx_meet(Toy *t, int a, Toy *u, int c, int battle);   // a playdate or a battle (u may be t; may be a housemate's toy)
const char *gx_dress(Toy *t, int slot, int acc);
const char *gx_quick_feed(Toy *t, int slot);   // one press: its favourite (or a safe free food); a snack when full but sad
int   gx_urgent(const Blob *b);                // what it needs most: 0 nothing 1 meds 2 clean 3 cuddle 4 scold 5 food 6 lights out 7 a game
// families (the rules that are not needed while you are in the city: they catch up on the hours when the toy is opened)
int   gx_kin(int a, int c);                  // are these two line ids close family (parent, child, sibling, grandparent)? then no eggs
int   gx_children(int id, int *out, int max);   // line ids of its children
int   gx_egg_of(Toy *t, int slot, int mom, int dad);   // an egg whose parents have those line ids (it inherits their traits)
int   gx_score(void);                        // the care score: every toy, every pet and every generation counted
void  gx_award(int bit);                     // an achievement (once): a reward for the human
int   gx_check(void);                        // catch the families up on the hours gone by, and look for new achievements; the awards won
