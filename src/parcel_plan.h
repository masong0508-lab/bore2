#pragma once
#include "bigmap.h"
/* The visible front garden boundary, not a change to ownership. The
 * south-normalised boundary bends inwards near one or both ends, using
 * the plan's own geometry and palette. Keep at least two clear rows
 * past the entrance and never encroach upon a gate or drive. */
static inline int bore_parcel_row(const LotPlan *p,int x){
    if(p->dy>13)return 16;
    int u=p->mir?22-x:x;
    int style=(p->pal+p->roof+p->wall)&3;
    int inset=style==0?(u<=8||u>=15):style==1?(u<=9):style==2?(u>=14):0;
    if(p->dy<=12&&style==0&&(u==6||u==17))inset=2;
    int r=16-inset;
    return r<p->dy+2?p->dy+2:r;
}
static inline int bore_parcel_gate(const LotPlan *p,int x){
    int d=x-p->dx;if(d>=-1&&d<=1)return 1;
    if(p->d2x){d=x-p->d2x;if(d>=-1&&d<=1)return 1;}
    return p->drv&&x>=p->drv&&x<=p->drv+1;
}
