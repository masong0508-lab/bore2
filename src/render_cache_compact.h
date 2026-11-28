#ifndef BORE_RENDER_CACHE_COMPACT_H
#define BORE_RENDER_CACHE_COMPACT_H
#include <stdint.h>
#include <string.h>

/* One bit per exact cached shade. Clear at person boundaries, and clear
 * a slot on material replacement; no generation alias or wraparound. */
typedef struct { uint32_t bits[32][2]; } BoreShadeValid;
static inline void bore_shade_reset(BoreShadeValid *v){memset(v,0,sizeof *v);}
static inline void bore_shade_drop(BoreShadeValid *v,int slot){v->bits[slot][0]=v->bits[slot][1]=0;}
static inline int bore_shade_has(const BoreShadeValid *v,int slot,int shade){return (v->bits[slot][shade>>5]&(UINT32_C(1)<<(shade&31)))!=0;}
static inline void bore_shade_mark(BoreShadeValid *v,int slot,int shade){v->bits[slot][shade>>5]|=UINT32_C(1)<<(shade&31);}

/* A four-by-four block cell contains 1..16 distinct building records.
 * The existing maximum storey count is 0..7. Neither loses precision. */
static inline uint8_t bore_far_cell_pack(int count,int storeys){return (uint8_t)((count-1)|(storeys<<4));}
static inline int bore_far_cell_count(uint8_t info){return (info&15)+1;}
static inline int bore_far_cell_storeys(uint8_t info){return (info>>4)&7;}
#endif
