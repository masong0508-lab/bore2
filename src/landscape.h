/* Shared integer terrain: no heap, no floating-point work, identical seams. */
#pragma once
#include <stdint.h>
enum { BORE_LAND_PAD=0x8000, BORE_LAND_MAX=6144, BORE_LAND_N=4 };
typedef struct { uint16_t h[4][4]; } BoreLandPatch;
static const int bore_land_knots[4]={0,5,9,18};
static inline int bore_land_h(uint16_t s){return s&32767;}
static inline int bore_land_lerp(int a,int b,int u,int n){return (a*(n-u)+b*u+n/2)/n;}
static inline int bore_land_corner(uint16_t nw,uint16_t ne,uint16_t sw,uint16_t se){
    return (nw&BORE_LAND_PAD)?bore_land_h(nw):(bore_land_h(nw)+bore_land_h(ne)+bore_land_h(sw)+bore_land_h(se)+2)/4;
}
/* The west/north pad owns its far boundary. Every patch evaluates the same
 * shared edge from the same four centres, including mixed town/wild edges. */
static inline int bore_land_edge(uint16_t a,uint16_t b,int lo,int hi,int u){
    int h=bore_land_h(a);
    if(a&BORE_LAND_PAD)return u>=5?h:bore_land_lerp(lo,h,u,5);
    int mid=(h+bore_land_h(b)+1)/2;
    return u<=9?bore_land_lerp(lo,mid,u,9):bore_land_lerp(mid,hi,u-9,9);
}
static inline void bore_land_build(BoreLandPatch *p,const uint16_t s[9]){
    int c00=bore_land_corner(s[0],s[1],s[3],s[4]),c10=bore_land_corner(s[1],s[2],s[4],s[5]);
    int c01=bore_land_corner(s[3],s[4],s[6],s[7]),c11=bore_land_corner(s[4],s[5],s[7],s[8]);
    int h=bore_land_h(s[4]);
    for(int k=0;k<4;k++){
        int u=bore_land_knots[k];
        p->h[0][k]=(uint16_t)bore_land_edge(s[1],s[4],c00,c10,u);
        p->h[3][k]=(uint16_t)bore_land_edge(s[4],s[7],c01,c11,u);
        p->h[k][0]=(uint16_t)bore_land_edge(s[3],s[4],c00,c01,u);
        p->h[k][3]=(uint16_t)bore_land_edge(s[4],s[5],c10,c11,u);
    }
    for(int y=1;y<=2;y++)for(int x=1;x<=2;x++){
        int u=bore_land_knots[x]+9,v=bore_land_knots[y]+9;
        int top=bore_land_lerp(bore_land_h(s[0]),bore_land_h(s[1]),u,18);
        int bot=bore_land_lerp(bore_land_h(s[3]),h,u,18);
        p->h[y][x]=(uint16_t)((s[4]&BORE_LAND_PAD)?h:bore_land_lerp(top,bot,v,18));
    }
}
/* Half-tile input also lets handheld walkers sample the terrain underfoot.
 * Triangular interpolation matches the rendered diagonal on each patch. */
static inline int bore_land_sample2(const BoreLandPatch *p,int x,int y){
    x=x<0?0:x>36?36:x;y=y<0?0:y>36?36:y;
    int i=x<10?0:x<18?1:2,j=y<10?0:y<18?1:2;
    int w=2*(bore_land_knots[i+1]-bore_land_knots[i]),d=2*(bore_land_knots[j+1]-bore_land_knots[j]);
    int u=x-2*bore_land_knots[i],v=y-2*bore_land_knots[j],n=w*d;
    int a=p->h[j][i],b=p->h[j][i+1],c=p->h[j+1][i],e=p->h[j+1][i+1];
    int U=u*d,V=v*w;
    return U>=V?(a*(n-U)+b*(U-V)+e*V+n/2)/n:(a*(n-V)+c*(V-U)+e*U+n/2)/n;
}
static inline int bore_land_sample(const BoreLandPatch *p,int x,int y){return bore_land_sample2(p,x*2,y*2);}
/* Continuous summit shaping. Material changes never create height steps. */
static inline int bore_land_relief(int elevation,int mode){
    static const int scale[5]={0,6,11,16,22},ridge[5]={0,0,5,14,30};
    if(mode<0||mode>4||elevation<=9)return 0;
    int base=elevation-9;if(base>230)base=230;
    int high=base>32?base-32:0;
    int h=base*scale[mode]+high*high*ridge[mode]/128;
    return h>BORE_LAND_MAX?BORE_LAND_MAX:h;
}
static inline int bore_land_brush_delta(int dx,int dy,int radius){
    int d=(dx<0?-dx:dx)+(dy<0?-dy:dy);
    if(!d)return 2;
    return radius>0&&d<=radius+1?1:0;
}
