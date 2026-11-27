#pragma once
#include <stdint.h>
enum { TRAIL_NONE, TRAIL_WALK, TRAIL_HEAD, TRAIL_SUMMIT };
enum { TRAIL_N, TRAIL_E, TRAIL_S, TRAIL_W, TRAIL_SIDE=18, TRAIL_HIGH=384 };
typedef struct { uint8_t links, kind, cx, cy, edge[4]; } BoreTrail;
/* A boundary has one decision in either direction. Avoid dramatic height jumps. */
static inline int bore_trail_join(int a,int b,uint32_t hash){
    int d=a>b?a-b:b-a;
    return (a>=TRAIL_HIGH||b>=TRAIL_HIGH)&&d<=2048&&(hash&3u)!=0;
}
static inline void bore_trail_build(BoreTrail *p,unsigned links,int kind,uint32_t hash,const uint32_t edge[4]){
    p->links=(uint8_t)(links&15u);p->kind=(uint8_t)(p->links?kind:TRAIL_NONE);
    p->cx=(uint8_t)(7+hash%5u);p->cy=(uint8_t)(7+(hash>>8)%5u);
    for(int d=0;d<4;d++)p->edge[d]=(uint8_t)(6+(edge[d]>>8)%7u);
}
/* Rasterize a connected winding center-to-edge route without stored tiles.
 * Each row includes the previous row's offset, so diagonal steps never leave gaps. */
static inline int bore_trail_cell(const BoreTrail *p,int x,int y){
    if(x<0||y<0||x>=TRAIL_SIDE||y>=TRAIL_SIDE)return 0;
    for(int d=0;d<4;d++)if(p->links&(1u<<d)){
        int along=d==TRAIL_N?y:d==TRAIL_E?17-x:d==TRAIL_S?17-y:x;
        int run=d==TRAIL_N?p->cy:d==TRAIL_E?17-p->cx:d==TRAIL_S?17-p->cy:p->cx;
        int across=(d&1)?y:x,center=(d&1)?p->cy:p->cx,edge=p->edge[d];
        if(along>run)continue;
        int a=(edge*(run-along)+center*along+run/2)/run;
        int prev=along?along-1:0,b=(edge*(run-prev)+center*prev+run/2)/run;
        if((across>=a&&across<=b)||(across>=b&&across<=a))return 1;
    }
    return 0;
}
