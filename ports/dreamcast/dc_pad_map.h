// Pure Dreamcast-to-PS1 digital button mapping; no KOS dependencies.
// dc_plat.c supplies semantic Dreamcast flags from the Maple button words.
// Keeping this in one tested helper avoids inventing stick-click hardware
// (the original Dreamcast pad has no L3/R3 or SELECT buttons).
#pragma once
#include <stdint.h>

enum {
    DCI_START = 1u << 0, DCI_UP = 1u << 1,
    DCI_RIGHT = 1u << 2, DCI_DOWN = 1u << 3,
    DCI_LEFT = 1u << 4, DCI_A = 1u << 5,
    DCI_B = 1u << 6, DCI_X = 1u << 7,
    DCI_Y = 1u << 8, DCI_C = 1u << 9,
    DCI_Z = 1u << 10, DCI_D = 1u << 11
};

// Returned bits are ACTIVE HIGH, identical to ~pad_read's active-low pair.
// Both triggers with no d-pad direction retain L1+R1 (camera recenter).
// Hold both triggers with a d-pad direction for one of four unavailable
// PS1 buttons: UP=L2, DOWN=R2, LEFT=L3, RIGHT=R3.
// Both triggers + Y is L2+R2 (cycle camera); START+both is SELECT.
// C/Z/D extra face buttons still work.
static inline uint16_t dc_pad_translate(uint32_t f, unsigned lt, unsigned rt) {
    uint16_t d = 0;
    const int both = lt > 100 && rt > 100;
    const int chord = both && !(f & DCI_START);
    if (f & DCI_START) d |= both ? 1u : (1u << 3);
    const uint32_t dirs = f & (DCI_UP | DCI_DOWN | DCI_LEFT | DCI_RIGHT);
    const int view_chord = chord && !dirs && (f & DCI_Y);
    if (chord) {
        if (view_chord) d |= (1u << 8) | (1u << 9); // hold both triggers + Y: cycle camera
        else {
            if (dirs & DCI_UP) d |= 1u << 8;
            if (dirs & DCI_DOWN) d |= 1u << 9;
            if (dirs & DCI_LEFT) d |= 1u << 1;
            if (dirs & DCI_RIGHT) d |= 1u << 2;
            if (!dirs) d |= (1u << 10) | (1u << 11);
        }
    } else {
        // START+both triggers is SELECT alone: do not also turn the camera.
        if (!(both && (f & DCI_START))) {
            if (lt > 100) d |= 1u << 10;
            if (rt > 100) d |= 1u << 11;
        }
        if (f & DCI_UP) d |= 1u << 4;
        if (f & DCI_RIGHT) d |= 1u << 5;
        if (f & DCI_DOWN) d |= 1u << 6;
        if (f & DCI_LEFT) d |= 1u << 7;
    }
    if ((f & DCI_Y) && !view_chord) d |= 1u << 12; // triangle unless cycling camera
    if (f & DCI_B) d |= 1u << 13;  // circle
    if (f & DCI_A) d |= 1u << 14;  // cross
    if (f & DCI_X) d |= 1u << 15;  // square
    if (f & DCI_Z) d |= 1u << 8;   // optional controller's extra buttons
    if (f & DCI_C) d |= 1u << 9;
    if (f & DCI_D) d |= 1u;
    return d;
}
