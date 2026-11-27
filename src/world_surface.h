#pragma once
#include "bigmap.h"
/* Coarse but exact ownership of roads, water, banks and public courts,
 * before buildings or detailed plot furnishing. Console stream proxies,
 * Android and DS collision use these same absolute block tile coordinates. */
enum { BS_LAND,BS_ROAD,BS_PAVING,BS_WATER,BS_SOIL,BS_PATIO,BS_CROP,BS_POOL };
static inline int bore_surface(const BInfo *b,int x,int y){
    int rd=b->road&7,p=BR_PART(b->road);
    if(b->terrain==BT_SEA)return ((rd&1)&&y<5)||((rd&2)&&x<5)?BS_ROAD:BS_WATER;
    if(b->lot==LOT_RIVER){
        int vert=(b->road&BR_MEGA)!=0,d=vert?x:y,e=vert?y:x,br=vert?(rd&1):(rd&2);
        if(d>=5&&(e>=5||!br)){
            if(d>=7&&d<=13)return br&&e==5?BS_PAVING:BS_WATER;
            return d<=15?BS_PAVING:BS_LAND;
        }
        if(br&&e<5&&d>=7&&d<=13)return BS_ROAD;
    }else if(b->road&BR_MEGA){
        if(p&1)rd&=~2;
        if(p&2)rd&=~1;
        if(p==3&&x<5&&y<5){
            if(BR_KIND(b->road)==MK_PARK)return BS_WATER;
            return x>=1&&x<=3&&y>=1&&y<=3?BS_WATER:BS_PATIO;
        }
        int strip=((p&1)&&x<5&&(y>=5||(p&2)))||((p&2)&&y<5&&(x>=5||(p&1)));
        if(strip)return BR_KIND(b->road)!=MK_PARK||((p&1)&&x==2)||((p&2)&&y==2)?BS_PAVING:BS_LAND;
    }
    if(((rd&1)&&y<5)||((rd&2)&&x<5)||((rd&4)&&x<5&&y<5))return BS_ROAD;
    if(b->set!=255&&(((rd&1)&&y==5&&x>=5)||((rd&2)&&x==5&&y>=5)))return BS_PAVING;
    return BS_LAND;
}
/* DS plots start five cells after the canonical north-west block corner. */
static inline int bore_surface_slot(int x,int *local){
    int a=x+BM_LOT0,q=a>=0?a/BM_BLK:-((-a+BM_BLK-1)/BM_BLK);
    *local=a-q*BM_BLK;return q;
}
