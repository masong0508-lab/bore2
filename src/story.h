// story: STORY MISSIONS, the GBA game's story mode brought over. Ten stories of five chapters (TV SHOW & TELL, the eleventh,
// is held back for its own voiced, filmed version). Each chapter is a scene: a title, two lines of what is going on, and up
// to three tasks that tick off in any order: places to go in SAN BOREAS, things to do, people to win round, money to save.
// A finished chapter pays and the next begins, with a card for each; the phone's GOALS is the story journal.
#pragma once
#include "common.h"

enum { STY_NONE, STY_ROOM, STY_WED, STY_PARENT, STY_SKATE, STY_HOUSE, STY_FRIEND, STY_RAGS, STY_CLIMB, STY_TOWN, STY_SECOND, STY_TVSHOW, STY_N };
#define STY_PLAYABLE 10           // the first ten: TV SHOW & TELL is not out yet

// what happened (story_event): the wants' own W_ kinds (wants.h), and these
enum { E_PIZZA = 64, E_TV, E_RADIO, E_JOKE, E_HUG, E_FLIRT, E_TALK, E_TRICK, E_BIGAIR, E_FIGHT, E_WANT, E_CARE, E_CITCHAT, E_STEADY };

extern int story_id, story_ch;    // the story being lived (0: none) and its chapter (0..4; 5: THE END)

void story_reset(void);                         // no story (a new life, an old save)
void story_household(int id, int px, int py);   // a new life in a story: its lead (who you play) and its people move in round (px, py)
void story_begin(void);                         // ... once the household is in: the story starts (the first chapter's card)
void story_event(int ev, int arg);              // something the one you play did (arg: a sim's uid, a combo's length...)
void story_tick(int steps);                     // the life loop
int  story_card(void);                          // a card waiting to be shown: 1 a chapter begins, 2 a chapter is done (0 none)
void story_card_shown(void);
int  story_place(int *lot);                     // where this chapter still sends you: 1 a kind of place (*lot), 2 STRANGE TOWN, 3 home (0: nowhere)
int  story_town_open(void);                     // STRANGE TOWN is on this chapter's way (it opens for it)
int  story_is_partner(int uid);                 // the story's own person (the one to fall for, or win back)
int  story_loved(void);                         // ... and you are in love with them
const char *story_hud(char *out);               // this chapter's next task, for the HUD (0: nothing to show)
void story_save(void);
void story_load(void);

// the screens (drawn over the life by main.c)
void story_card_draw(int kind, int t);          // the chapter card
void story_journal_draw(int t);                 // the phone's GOALS with a story on
void story_pick_draw(int sel, int t);           // PICK A WORLD > MISSIONS > STORY MISSIONS: one story's card
const char *story_name(int id);
