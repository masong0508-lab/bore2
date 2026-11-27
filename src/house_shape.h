#pragma once
/* All rectangles are half-open wall-tile bounds. The planned L-home's
 * garage is 1..3 tiles wide beyond the main house, with a two-tile
 * recess at the rear. Encode in the four unused footprint-cache bits. */
typedef struct {int x0,z0,x1,z1;} HouseShapeRect;
static inline int house_shape_equal(HouseShapeRect a,HouseShapeRect b){return a.x0==b.x0&&a.z0==b.z0&&a.x1==b.x1&&a.z1==b.z1;}
static inline int house_shape_contains(HouseShapeRect a,int x,int z){return x>=a.x0&&x<a.x1&&z>=a.z0&&z<a.z1;}
static inline int house_shape_decode(HouseShapeRect a,int code,HouseShapeRect*b,HouseShapeRect*g){
    if(code<1||code>12)return 0;
    int v=code-1,w=1+(v>>2),east=v&1,front=v&2;
    if(a.x1-a.x0<=w||a.z1-a.z0<=2)return 0;
    *b=a;*g=a;
    if(east){b->x1-=w;g->x0=b->x1;}else{b->x0+=w;g->x1=b->x0;}
    if(front)g->z1-=2;else g->z0+=2;
    return 1;
}
static inline int house_shape_encode(HouseShapeRect a,HouseShapeRect b,HouseShapeRect g){
    for(int code=1;code<=12;code++){
        HouseShapeRect bb,gg;
        if(house_shape_decode(a,code,&bb,&gg)&&house_shape_equal(b,bb)&&house_shape_equal(g,gg))return code;
    }
    return 0;
}
