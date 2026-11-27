#pragma once
#include "lot_footprints.h"
/* DS public buildings have walls at cell edges. Inclusive PS1 wall tiles
 * become half-open DS floor rectangles, with a central service aisle. */
typedef struct { int x0,y0,x1,y1,door; } DsVenuePlan;
static inline int ds_venue_plan(int lot,DsVenuePlan *p){
    if(lot<LOT_SHOP||lot==LOT_FARM||!bore_lot_has_footprint(lot))return 0;
    const u8 *f=bore_lot_footprints[lot];
    *p=(DsVenuePlan){f[0],f[1],f[2]+1,f[3]+1,(f[0]+f[2])/2};
    return 1;
}
static inline int ds_venue_item_fits(const DsVenuePlan *p,int x,int y,int w,int h){
    return w>0&&h>0&&x>=p->x0&&y>=p->y0&&x+w<=p->x1&&y+h<=p->y1-2&&
           (x+w<=p->door-1||x>=p->door+2);
}
/* Each venue has actual catalogue items appropriate to its use. First
 * pair: rear wall; second pair: side services; third: reception. */
static inline const char *ds_venue_catalogue(int lot){
    switch(lot){
    case LOT_SHOP:case LOT_CONV:return "F77yyI";
    case LOT_GAS:return "F7yyI7";
    case LOT_DINER:case LOT_CAFE:case LOT_RESTAURANT:return "F/thyI";
    case LOT_BAR:return "Fcyh$j";
    case LOT_LIBRARY:return "bb[hty";
    case LOT_OFFICE:case LOT_TOWER:return "[dbhty";
    case LOT_SCHOOL:return "db[hty";
    case LOT_CLINIC:return "S-dd;y";
    case LOT_SPA:return "H>S;hy";
    case LOT_SALON:return "`-hi;y";
    case LOT_CINEMA:return "vA;;y:";
    case LOT_ARCADE:return "!!!!jy";
    case LOT_GYM:return "m_]m;y";
    case LOT_INN:case LOT_HOTEL:return "SSl;Iy";
    case LOT_GALLERY:return "))}b;y";
    case LOT_SPACE:case LOT_OBS:return "[db;Iy";
    default:return "byth;y";
    }
}
