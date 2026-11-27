// Portable map-window protocol. Keep work bounded to 33x33 blocks.
#pragma once
static inline int bore_window_side(int r){return r>=1&&r<=16?2*r+1:0;}
static inline int bore_window_center(int x,int r,int n){
    int side=bore_window_side(r);if(!side||n<side)return -1;
    return x<r?r:x>=n-r?n-r-1:x;
}
static inline int bore_window_valid(int r,int n){int s=bore_window_side(r);return s&&n>=s*s;}
static inline unsigned bore_window_pack(unsigned terrain,unsigned lot,unsigned road,unsigned storeys,unsigned settlement){
    return (terrain&7u)|((lot&63u)<<3)|((road&255u)<<9)|((storeys&7u)<<17)|((settlement&255u)<<20);
}
