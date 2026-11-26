#pragma once
#include <stdint.h>
// One owner per floor cell. Mixed hidden/exposed 2x2 groups must use
// individual cells, never suppress the visible portion of the street.
static inline int render_floor_group(int hidden_mask){
    return hidden_mask==0?1:hidden_mask==15?2:0;
}
static inline int render_group_fits(int x,int y,int x1,int y1,int w,int h){
    return x>=0&&y>=0&&x+1<w&&y+1<h&&x+1<=x1&&y+1<=y1;
}
// The detail pass owns any building whose footprint touches the detail
// range or surrounds the eye. The skyline must never discard its owner.
static inline int render_eye_close(int ex,int ey,int x0,int y0,int x1,int y1){
    return ex>=x0-3&&ex<=x1+3&&ey>=y0-3&&ey<=y1+3;
}
// Cached render pieces are reusable only when their exact key matches.
static inline int render_piece_dirty(uint32_t old,uint32_t key,int words,int over){
    return old!=key||words<=0||over;
}
// Pick empty/unpinned LRU storage; a page already referenced by the
// current submission is never a replacement candidate.
static inline int render_cache_prefer(int pinned,int allocated,uint32_t age,
                                    int have_best,int best_alloc,uint32_t best_age){
    return !pinned&&(!have_best||(!allocated&&best_alloc)||
                     (allocated==best_alloc&&age<best_age));
}
