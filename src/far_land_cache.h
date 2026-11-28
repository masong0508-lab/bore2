#pragma once
#include <stdint.h>
/* A visible unknown cell is resolved with its actual world coordinates,
 * once, instead of painting it grass until the background fill catches
 * up. Town tree candidate bits are not part of its land material. */
static inline int bore_far_sample(uint8_t *cell,int *unknown,int x,int y,int (*classify)(int,int)){
    if(*cell==255){*cell=(uint8_t)classify(x,y);--*unknown;}
    return (*cell&8)?8:*cell&7;
}
static inline int bore_far_majority(int a,int b,int c,int d){
    return (a==b||a==c||a==d)?a:(b==c||b==d)?b:c==d?c:a;
}
/* 0 = culled, 1 = visible fine group, 2 = already owned by coarse LOD. */
static inline int bore_far_group_needed(int state){return state==1;}
