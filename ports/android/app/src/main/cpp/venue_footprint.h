#pragma once
#include "bigmap.h"
#include "mesh_footprint.h"
static inline int bore_venue_supported(int lot){
    switch(lot){
    case LOT_SHOP:case LOT_DINER:case LOT_GYM:case LOT_OFFICE:case LOT_INN:
    case LOT_SCHOOL:case LOT_ARCADE:case LOT_TOWER:case LOT_HOTEL:case LOT_CAFE:
    case LOT_CINEMA:case LOT_RESTAURANT:case LOT_LIBRARY:case LOT_SPA:
    case LOT_GALLERY:case LOT_GAS:case LOT_CONV:case LOT_SPACE:case LOT_OBS:return 1;
    default:return 0;
    }
}
// Shared entry edge cell for the 3D facade and the interior cutaway.
static inline int bore_venue_entry(const BoreFoot *f,int face,int *x,int *y){
    int front=face==LF_NORTH?-1:1;
    *x=(f->x0+f->x1)/2;*y=front<0?f->z0:f->z1;
    if(!bore_foot_has(f,*x,*y)||bore_foot_has(f,*x,*y+front))return 0;
    return front;
}
