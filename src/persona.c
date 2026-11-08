// persona.c: see persona.h
#include "persona.h"
const u8 pers_sign_tr[12][5] = { {5,8,6,3,3},{5,5,3,8,4},{4,7,8,3,3},{6,3,6,4,6},{4,10,4,4,3},{9,2,6,3,5},
                                 {2,8,2,6,7},{6,5,8,3,3},{2,3,9,7,4},{7,4,8,2,4},{4,4,4,7,6},{5,3,4,4,9} };   // each adds up to 25
int persona_sign(const u8 bar[5]) {
    int best = 0, bd = 999;
    for (int s = 0; s < 12; s++) { int d = 0; for (int i = 0; i < 5; i++) { int e = bar[i] - pers_sign_tr[s][i]; d += e < 0 ? -e : e; } if (d < bd) { bd = d; best = s; } }
    return best;
}
static u32 hmix(u32 a, u32 b) { u32 h = (a ^ 0x5bd1e995u) * 2654435761u ^ (b + 0x9e3779b9u) * 40503u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16; return h; }
void persona_of(u32 seed, int stage, Persona *p) {
    int sg = (int)(hmix(seed, 1) % 12u);
    for (int i = 0; i < 5; i++) p->bar[i] = pers_sign_tr[sg][i];
    for (int k = 0; k < 3; k++) {   // up to three one-point moves between bars, so two people of one sign differ (a move that would change the sign is not made)
        u32 h = hmix(seed, 10u + (u32)k); int a = (int)(h % 5u), b = (int)((h >> 8) % 5u);
        if (a == b || p->bar[a] == 0 || p->bar[b] >= 10) continue;
        p->bar[a]--; p->bar[b]++;
        if (persona_sign(p->bar) != sg) { p->bar[a]++; p->bar[b]--; }
    }
    p->sign = (u8)sg;
    p->asp = (u8)(stage < 2 ? 5 : hmix(seed, 2) % 5u);
    p->ltw = (u8)(hmix(seed, 3) % 4u);
}
