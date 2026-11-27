#pragma once
static inline int bore_vehicle_abs(int n){return n<0?-n:n;}
static inline int bore_vehicle_half_width(int type){return type<2?80:210;}
static inline int bore_vehicle_half_length(int type){return type<2?200:type==3||type==5?560:450;}
/* SAT for a rotated vehicle and a square tile, in 1/256 tile units.
 * sin/cos are Q12; touching an edge alone is not overlap. */
static inline int bore_vehicle_cell_overlap(int x,int y,int sn,int cs,int hw,int hl,int cx,int cy,int tilehalf){
    int dx=cx-x,dy=cy-y,a=bore_vehicle_abs(sn),b=bore_vehicle_abs(cs);
    if(bore_vehicle_abs(dx)*4096>=hw*b+hl*a+tilehalf*4096)return 0;
    if(bore_vehicle_abs(dy)*4096>=hw*a+hl*b+tilehalf*4096)return 0;
    int ext=tilehalf*(a+b);
    return bore_vehicle_abs(dx*cs-dy*sn)<hw*4096+ext&&bore_vehicle_abs(dx*sn+dy*cs)<hl*4096+ext;
}
/* Rebase positions without wrapping a departing car to a different road. */
static inline int bore_vehicle_rebase(int *x,int *y,int dx,int dy,int span){
    *x-=dx*span;*y-=dy*span;
    return *x>=-span&&*x<2*span&&*y>=-span&&*y<2*span;
}
static inline int bore_vehicle_junction(int x,int y){return x<-1||x>1||y<-1||y>1?0:(y+1)*3+x+2;}
static inline int bore_vehicle_claim_ok(int count,int axis,int turn,int wantaxis,int wantturn){return !count||(axis==wantaxis&&!turn&&!wantturn);}

/* Fixed-point separating axes for two moving or parked model footprints. */
static inline int bore_vehicle_axis_overlap(int dx,int dy,int ax,int ay,int ar,int bs,int bc,int bw,int bl){
    int br=(bore_vehicle_abs(ax*bc-ay*bs)+4095)/4096;
    int bf=(bore_vehicle_abs(ax*bs+ay*bc)+4095)/4096;
    return bore_vehicle_abs(dx*ax+dy*ay)<ar*4096+bw*br+bl*bf;
}
static inline int bore_vehicle_box_overlap(int x,int y,int sn,int cs,int hw,int hl,int bx,int by,int bs,int bc,int bw,int bl){
    int dx=bx-x,dy=by-y,bound=hw+hl+bw+bl;
    if(bore_vehicle_abs(dx)>=bound||bore_vehicle_abs(dy)>=bound)return 0;
    return bore_vehicle_axis_overlap(dx,dy,cs,-sn,hw,bs,bc,bw,bl)&&
        bore_vehicle_axis_overlap(dx,dy,sn,cs,hl,bs,bc,bw,bl)&&
        bore_vehicle_axis_overlap(dx,dy,bc,-bs,bw,sn,cs,hw,hl)&&
        bore_vehicle_axis_overlap(dx,dy,bs,bc,bl,sn,cs,hw,hl);
}
