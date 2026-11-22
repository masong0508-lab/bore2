// Safe expansion shelf card status: base cards have a sentinel pack ID of 255.
// Keep indexing into have[] and PPF filenames exclusive to actual 0..11 packs.
#ifndef BORE2_PACK_CARD_STATUS_H
#define BORE2_PACK_CARD_STATUS_H

enum { K_BASE, K_PACK, K_SOON };
enum { CARD_BASE, CARD_INSTALLED, CARD_COMING, CARD_CHECKING, CARD_MISSING };
static inline int pack_card_status(unsigned kind, unsigned pack,
                                   const int have[12]) {
    if (kind == K_BASE) return CARD_BASE;
    if (kind == K_SOON || kind != K_PACK || pack >= 12u) return CARD_COMING;
    if (have[pack] < 0) return CARD_CHECKING;
    return have[pack] ? CARD_INSTALLED : CARD_MISSING;
}
#endif
