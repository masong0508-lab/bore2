// Spatial broad phase for traffic and conservative camera visibility.
// World movement uses 1/16 of a cell; the 3x3 neighborhood is 864 units.
#pragma once
#include <stdint.h>
#include <string.h>
#define CITY_GRID_N 9
#define CITY_GRID_MIN (-288)
#define CITY_GRID_STEP 96
#define CITY_GRID_END (CITY_GRID_MIN+CITY_GRID_N*CITY_GRID_STEP)
typedef struct { uint32_t bin[CITY_GRID_N*CITY_GRID_N],outside; } CityGrid;
static inline void city_grid_clear(CityGrid *g) { memset(g,0,sizeof *g); }
static inline int city_grid_index(int x) {
    if(x<CITY_GRID_MIN || x>=CITY_GRID_END)return -1;
    return (x-CITY_GRID_MIN)/CITY_GRID_STEP;
}
static inline void city_grid_put(CityGrid *g,unsigned slot,int x,int y) {
    if(slot>=32)return;
    uint32_t bit=UINT32_C(1)<<slot;int a=city_grid_index(x),b=city_grid_index(y);
    if(a<0||b<0)g->outside|=bit;else g->bin[b*CITY_GRID_N+a]|=bit;
}
static inline uint32_t city_grid_query(const CityGrid *g,int x,int y,int r) {
    uint32_t m=g->outside;
    if(r<=0||x+r<CITY_GRID_MIN||x-r>=CITY_GRID_END||y+r<CITY_GRID_MIN||y-r>=CITY_GRID_END)return m;
    int x0=city_grid_index(x-r),y0=city_grid_index(y-r);
    int x1=city_grid_index(x+r),y1=city_grid_index(y+r);
    if(x0<0)x0=0;if(y0<0)y0=0;
    if(x1<0)x1=CITY_GRID_N-1;if(y1<0)y1=CITY_GRID_N-1;
    for(int j=y0;j<=y1;j++)for(int i=x0;i<=x1;i++)m|=g->bin[j*CITY_GRID_N+i];
    return m;
}
static inline int city_within(int a,int b,int x,int y,int r) {
    int dx=a-x,dy=b-y;
    if(dx<=-r||dx>=r||dy<=-r||dy>=r)return 0;
    return dx*dx+dy*dy<r*r;
}
// Match gx_cam / bub_proj. Deliberately expand the frustum (tan half-FOV
// 0.4853 horizontal, 0.3640 vertical) and object bounding sphere.
// Fixed-point rounding has an additional 16-unit guard.
typedef struct { int x,y,z,dist,cs,sn,cp,sp; } CityView;
static inline int city_visible(const CityView *v,int x,int y,int z,int radius) {
    int dx=x-v->x,dy=y-v->y,dz=z-v->z;
    int side=(dx*v->cs+dz*v->sn)>>12;
    int forward=(-dx*v->sn+dz*v->cs)>>12;
    int up=(dy*v->cp-forward*v->sp)>>12;
    int depth=v->dist-((dy*v->sp+forward*v->cp)>>12);
    int pad=2*radius+16;
    if(depth < -radius-16)return 0;
    if(depth<0)depth=0;
    int h=depth/2+pad,ver=depth*3/8+pad;
    return side>=-h&&side<=h&&up>=-ver&&up<=ver;
}
typedef struct { int id,dist; } CityDraw;
static inline void city_draw_insert(CityDraw *list,int *n,int cap,int id,int dist) {
    if(cap<=0)return;
    int pos=*n;
    if(pos>=cap){if(dist>=list[cap-1].dist)return;pos=cap-1;}else ++*n;
    while(pos>0 && list[pos-1].dist>dist){list[pos]=list[pos-1];--pos;}
    list[pos]=(CityDraw){id,dist};
}
