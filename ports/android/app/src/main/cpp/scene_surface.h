#pragma once
#include "bigmap.h"
enum { BS_LAND,BS_ROAD,BS_PAVING,BS_WATER,BS_SOIL,BS_PATIO,BS_CROP,BS_POOL };
// Match the console's five-tile street, sixth-tile pavement, mega
// courtyards, and river's actual two banks. Each cell has one surface.
static inline int bore_surface(const BInfo *b,int x,int z){
    int rd=b->road&7,part=BR_PART(b->road);
    if(b->terrain==BT_SEA)return ((rd&1)&&z<5)||((rd&2)&&x<5)?BS_ROAD:BS_WATER;
    if(b->lot==LOT_RIVER){
        int vert=(b->road&BR_MEGA)!=0;
        int across=vert?x:z,along=vert?z:x,bridge=vert?(rd&1):(rd&2);
        if(across>=5&&across<=15&&(along>=5||!bridge)){
            if(across>=7&&across<=13)return bridge&&along==5?BS_PAVING:BS_WATER;
            return BS_PAVING;
        }
        if(bridge&&along<5&&across>=7&&across<=13)return BS_ROAD;
    }else if(b->road&BR_MEGA){
        if(part&1)rd&=~2;
        if(part&2)rd&=~1;
        int strip=((part&1)&&x<5&&(z>=5||(part&2)))||
                  ((part&2)&&z<5&&(x>=5||(part&1)));
        if(strip)return BR_KIND(b->road)!=MK_PARK||((part&1)&&x==2)||((part&2)&&z==2)?BS_PAVING:BS_LAND;
    }
    if(((rd&1)&&z<5)||((rd&2)&&x<5)||((rd&4)&&x<5&&z<5))return BS_ROAD;
    if(b->set!=255&&(((rd&1)&&z==5&&x>=5)||((rd&2)&&x==5&&z>=5)))return BS_PAVING;
    return BS_LAND;
}

// One material per terrain cell. A second plane 0.02 units above land
// cannot be reliably depth-sorted on 16-bit mobile depth buffers.
static inline int bore_lot_surface(const BInfo*b,const LotPlan*p,int x,int z){
    int m=bore_surface(b,x,z);
    if(m!=BS_LAND||x<BM_LOT0||z<BM_LOT0)return m;
    int lx=x-BM_LOT0,lz=z-BM_LOT0;
    if(p&&p->face==LF_NORTH)lz=12-lz;
    if(b->lot==LOT_POOL){
        int w=6+(b->h&1),h=4+((b->h>>1)&1);
        return lx>=3&&lx<3+w&&lz>=3&&lz<3+h?BS_POOL:BS_PATIO;
    }
    if(b->lot==LOT_SKATE)return BS_PAVING;
    if(b->lot==LOT_FARM&&lx>=1&&lx<=5&&(lx&1)&&lz>=7&&lz<=11)
        return lx==3?BS_CROP:BS_SOIL;
    if(p&&p->type&&p->face!=LF_NONE&&(x==p->dx||(p->d2x&&x==p->d2x))){
        if((p->face==LF_NORTH&&z<p->dy)||(p->face==LF_SOUTH&&z>p->dy))return BS_PATIO;
    }
    return m;
}
