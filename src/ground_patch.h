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
    uint8_t used[GROUND_PATCH_MAX];memset(used,0,sizeof used);
    int n=0;
    for(int y=0;y<18;y++)for(int x=0;x<18;x++){
        int i=y*18+x;if(used[i])continue;
        int k=mat[i],w=1,h=1;
        while(w<6&&x+w<18&&!used[i+w]&&mat[i+w]==k)w++;
        while(h<6&&y+h<18){
            int ok=1;for(int a=0;a<w;a++)if(used[i+h*18+a]||mat[i+h*18+a]!=k){ok=0;break;}
            if(!ok)break;
            h++;
        }
        for(int b=0;b<h;b++)memset(used+i+b*18,1,w);
        if(n<cap)out[n]=ground_patch_pack(x,y,w,h,k);
        n++;
    }
    return n;
}
