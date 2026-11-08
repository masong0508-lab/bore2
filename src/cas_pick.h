// cas_pick.h: the hook between CREATE.OVL's PERSONALITY step (create_ui.c) and the trait and wish pickers (traits_ui.c). Both are in the same overlay.
#pragma once
typedef struct {
    void (*open)(int inlife);                                              // the creator opens (inlife: it edits a living Bore)
    void (*update)(int tab, int held, int pressed);                        // every frame on the PERSONALITY step: tab 0 TRAITS, 1 WISH
    void (*draw)(int tab, int x, int y, int w, int h);                     // inside the panel's content box
    void (*commit)(int inlife);                                            // the Bore is kept: the picks go to it (or are staged for the Bore main.c adds)
    const char *(*sum)(char *buf);                                         // the summary row: "3 TRAITS"
    const char *(*hint)(int tab);                                          // the buttons for the bottom line
} CasPick;
extern const CasPick tr_cas;                                               // traits_ui.c
void tr_page_open(void);                                                // the Life panel's TRAITS page (CREATE.OVL open(2))
int tr_page_update(int held, int pressed);                                  // 0: leave
void tr_page_draw(void);
void tr_names_fill(void);                                                  // CREATE.OVL open(3): the names of the household members' traits (the assignment itself is resident: traits_glue.inc tr_assign_run)
