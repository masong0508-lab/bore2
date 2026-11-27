#pragma once
#include <stdint.h>
#include <string.h>
enum { GROUND_PATCH_SIDE=18, GROUND_PATCH_MAX=324, GROUND_PATCH_CACHE=80 };
static inline uint32_t ground_patch_pack(int x,int y,int w,int h,int mat){
    return (uint32_t)x|((uint32_t)y<<5)|((uint32_t)w<<10)|((uint32_t)h<<15)|((uint32_t)mat<<20);
}
static inline int ground_patch_x(uint32_t p){return p&31;}
static inline int ground_patch_y(uint32_t p){return (p>>5)&31;}
static inline int ground_patch_w(uint32_t p){return (p>>10)&31;}
static inline int ground_patch_h(uint32_t p){return (p>>15)&31;}
static inline int ground_patch_mat(uint32_t p){return (p>>20)&63;}
// Disjoint material rectangles. Never replace a whole area by a sample
// of one tile. Six-tile bound limits plane error on slopes.
static inline int ground_patch_build(const uint8_t *mat,uint32_t *out,int cap){
    /* One bit per occupied cell. Preserve the exact scan order,
     * 6x6 rectangle bound and material coverage of the old builder. */
    uint32_t used[GROUND_PATCH_SIDE]={0};
    int n=0;
    for(int y=0;y<18;y++)for(int x=0;x<18;){
        unsigned bit=1u<<x;
        if(used[y]&bit){x++;continue;}
        int i=y*18+x,k=mat[i],w=1,h=1;
        while(w<6&&x+w<18&&!(used[y]&(bit<<w))&&mat[i+w]==k)w++;
        unsigned mask=((1u<<w)-1u)<<x;
        while(h<6&&y+h<18&&!(used[y+h]&mask)){
            int a=0;while(a<w&&mat[i+h*18+a]==k)a++;
            if(a<w)break;
            h++;
        }
        for(int b=0;b<h;b++)used[y+b]|=mask;
        if(n<cap)out[n]=ground_patch_pack(x,y,w,h,k);
        n++;x+=w;
    }
    return n;
}

/* Island corner colours and city surface rectangles are mutually exclusive
 * render paths. Keep their full cache sizes while sharing resident RAM.
 * Claim returns true on a switch: callers invalidate the new owner's tags. */
enum { TERRAIN_CACHE_NONE, TERRAIN_CACHE_CITY, TERRAIN_CACHE_ISLAND };
typedef union {
    uint32_t city[4][4][GROUND_PATCH_CACHE];
    struct {uint16_t col[32][32],tag[32][32];} island;
} TerrainRenderCache;
static inline int terrain_cache_claim(int *owner,int next){
    if(*owner==next)return 0;
    *owner=next;return 1;
}

/* The 4x4 city window has exactly one block for each pair of low
 * coordinate bits. Retained blocks keep their cache slot across a rebase.
 * Key 0 is invalid; real blocks at the origin are not confused with it. */
static inline int ground_cache_slot(int bx,int by){return ((by&3)<<2)|(bx&3);}
static inline int ground_cache_key(int bx,int by){return 1+bx+by*1024;}
static inline void ground_cache_drop(int *keys,int bx,int by){
    int i=ground_cache_slot(bx,by);if(keys[i]==ground_cache_key(bx,by))keys[i]=0;
}
