// seqdec: a packed song's notes, unpacked as they are played (tools/pack_songs.py packs them: LZSS, a 4 KB window).
// The notes are laid out in play order: the intro (the order list up to the loop point), then the part that loops. The sequencer
// asks for one row at a time; the unpacker keeps the last 4 KB it made, which is all a match ever reaches back into.
#pragma once
typedef struct {
    const unsigned char *src;   // the packed stream
    unsigned left;              // unpacked bytes still to come in this part
    unsigned short rp, mpos;    // where the next byte goes in the window; where a match copies from
    unsigned char mrem, flags, fbits, pend;   // bytes left of the match; the token flags and how many are left; empty rows still to play
    unsigned char *ring;        // the 4 KB window (audio.c keeps it in the sequence buffer's spare end: no RAM of its own)
} SeqDec;
static inline void seqdec_start(SeqDec *d, const unsigned char *src, unsigned len) { d->src = src; d->left = len; d->rp = 0; d->mrem = 0; d->fbits = 0; d->pend = 0; }
static inline int seqdec_byte(SeqDec *d) {
    int c;
    if (!d->left) return 0x81;                                  // (past the end: an empty row)
    d->left--;
    if (d->mrem) { c = d->ring[d->mpos++ & 4095]; d->mrem--; }
    else {
        if (!d->fbits) { d->flags = *d->src++; d->fbits = 8; }
        d->fbits--; int lit = d->flags & 1; d->flags >>= 1;
        if (lit) c = *d->src++;
        else { int a = *d->src++, b = *d->src++; d->mpos = (unsigned short)((d->rp - ((a | (b & 15) << 8) + 1)) & 4095); d->mrem = (unsigned char)((b >> 4) + 2); c = d->ring[d->mpos++ & 4095]; }
    }
    d->ring[d->rp++ & 4095] = (unsigned char)c;
    return c;
}
// the next row: how many notes it has (their 3 bytes each follow, through seqdec_byte)
static inline int seqdec_row(SeqDec *d) {
    if (d->pend) { d->pend--; return 0; }
    for (;;) {
        int h = seqdec_byte(d);
        if (h & 0x80) { h &= 0x7f; if (!h) continue; d->pend = (unsigned char)(h - 1); return 0; }
        return h;
    }
}
