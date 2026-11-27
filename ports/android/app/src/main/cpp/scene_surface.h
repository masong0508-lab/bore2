#pragma once
#include "world_surface.h"

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
