// DS generated-lot exterior. Uses the actual playable DS plan and
// half-open build-cell boundaries, not the console skyline's full box.
#pragma once
#include "bigmap.h"
typedef struct { int x0,z0,x1,z1; } DsShellRect;
typedef struct { DsShellRect body,wing,loft; int home,north; } DsLotShell;
static inline int ds_shell_valid(DsShellRect r){return r.x1>r.x0&&r.z1>r.z0;}
static inline int ds_shell_has(DsShellRect r,int x,int z){
    return x>=r.x0&&x<r.x1&&z>=r.z0&&z<r.z1;
}
static inline int ds_shell_contains(const DsLotShell *s,int x,int z){
    return ds_shell_has(s->body,x,z)||ds_shell_has(s->wing,x,z);
}
static inline DsShellRect ds_shell_flip(DsShellRect r){
    return (DsShellRect){r.x0,13-r.z1,r.x1,13-r.z0};
}
static inline int ds_lot_shell(const BInfo *b,const LotPlan *p,int bx,int by,DsLotShell *out){
    *out=(DsLotShell){0};
    if(!p||b->terrain==BT_SEA)return 0;
    out->north=p->face==LF_NORTH;
    if(b->lot>=LOT_HOUSE&&b->lot<=LOT_APART){
        out->home=1;
        if(p->type){
            out->body=(DsShellRect){p->x0-BM_LOT0,p->y0-BM_LOT0,p->x1-BM_LOT0+1,p->y1-BM_LOT0+1};
            if(p->wx1){
                DsShellRect w={p->wx0-BM_LOT0,p->wy0-BM_LOT0,p->wx1-BM_LOT0+1,p->wy1-BM_LOT0+1};
                if(w.x0<out->body.x0)w.x1=out->body.x0;
                else w.x0=out->body.x1;
                if(ds_shell_valid(w))out->wing=w;
            }
            // kitD builds a loft over the back three/four rows, not a
            // second floor across the entire lower house and its garage.
            if(p->type==HT_TWO||p->type==HT_MANSION){
                DsShellRect a=out->body;int d=a.z1-a.z0,back=d>=8?4:3;
                if(out->north)a.z0=a.z1-back;else a.z1=a.z0+back;
                out->loft=a;
            }
        }else{
            // Apartments currently enter gen_house, not gen_public.
            // Match gen_house's coordinate seed, and its unflipped plot.
            unsigned h=((unsigned)bx*73856093u^(unsigned)by*19349663u)*2654435761u+12345u;
            int x=(h>>7)&1,w=11+((h>>5)&1);
            out->body=(DsShellRect){x,1,x+w,10};out->north=0;
            if(b->lot==LOT_TWO)out->loft=(DsShellRect){x,1,x+w,5};
        }
        return ds_shell_valid(out->body);
    }
    switch(b->lot){
    case LOT_WILD:case LOT_PARK:case LOT_SQUARE:case LOT_SKATE:case LOT_EMPTY:case LOT_RIVER:return 0;
    case LOT_FARM:
        if(p->face==LF_NONE)return 0;
        out->body=(DsShellRect){8,1,13,7};break;
    case LOT_ARCADE:out->body=(DsShellRect){1,2,11,9};break;
    default:
        if(b->lot>=LOT_N)return 0;
        // The current playable DS public venues, including the pool's
        // indoor facilities, are gen_public's common single-storey hall.
        out->body=(DsShellRect){2,1,11,9};break;
    }
    if(out->north)out->body=ds_shell_flip(out->body);
    return 1;
}
