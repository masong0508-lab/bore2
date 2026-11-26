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

// Packet words are 32-bit. Align first, then check available room.
static inline int render_packet_bytes(int bytes,int room){
    if(bytes<=0||bytes>0x7ffffffc||room<0)return 0;
    int aligned=(bytes+3)&~3;
    return aligned<=room?aligned:0;
}
// GTE input for proxy vertices is 1/16 scale. The normal GTE's AVSZ4
// uses 0x100 and AVSZ3 uses 0x155; recover exactly their local OT units.
static inline int render_proxy_slot(int sum,int vertices,int shift,int limit){
    int z=sum>0?(vertices==3?(sum*341)>>8:sum):0;
    int slot=z>>shift;
    return slot<1?1:slot>limit?limit:slot;
}

// The near renderer orders GTE OTZ (roughly a quarter of world depth).
// Continue that same scale into the far buckets; never reuse the three
// reserved floor/shadow indices for geometry.
static inline int render_world_depth_slot(int quarter,int shift,int near,int last){
    int split=near<<shift;
    if(quarter<split){
        int i=quarter>0?quarter>>shift:1;
        return i<1?1:i>near-4?near-4:i;
    }
    int i=near+((quarter-split)>>8);
    return i>last?last:i;
}
static inline int render_proxy_depth(int sum,int vertices){
    return sum>0?(vertices==3?(sum*341)>>8:sum):0;
}
// With a lifted GTE matrix the translation is rounded per axis. Use the
// identical translation for the software clipper (not an unlifted point).
static inline int render_lift_translation(int t,int lift,int y_column){
    return t-((lift*y_column)>>12);
}
