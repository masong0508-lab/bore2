#pragma once
#include <stdint.h>
/* Deterministic nearest free arrival, shared by the two console
 * coordinate systems. Pack only real cells; -1 is never a coordinate. */
static inline int bore_arrival_cell(int x,int y,int width,int height,
                                   const uint16_t *used,int n,int (*clear)(int,int)){
    if(x<0)x=0;
    if(y<0)y=0;
    if(x>=width)x=width-1;
    if(y>=height)y=height-1;
    for(int d=0;d<width+height;d++){
        for(int dy=-d;dy<=d;dy++){
            int cy=y+dy,dx=d-(dy<0?-dy:dy);
            if(cy<0||cy>=height)continue;
            for(int sign=0;sign<(dx?2:1);sign++){
                int cx=x+(sign?-dx:dx);
                if(cx<0||cx>=width)continue;
                int id=cy*width+cx,k=0;
                while(k<n&&used[k]!=id)k++;
                if(k==n&&clear(cx,cy))return id;
            }
        }
    }
    return -1;
}
