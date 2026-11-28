#include <assert.h>
#include <stdio.h>
#include "render_cache_compact.h"
int main(void){
    BoreShadeValid v;
    uint8_t reference[32][64];
    uint32_t rng=0x12345678u;
    assert(sizeof v==256);
    for(int person=0;person<520;person++){
        bore_shade_reset(&v);memset(reference,0,sizeof reference);
        for(int j=0;j<2048;j++){
            rng=rng*1664525u+1013904223u;
            int slot=(rng>>24)&31, shade=(rng>>16)&63;
            if((j&31)==0){bore_shade_drop(&v,slot);memset(reference[slot],0,64);}
            assert(bore_shade_has(&v,slot,shade)==reference[slot][shade]);
            bore_shade_mark(&v,slot,shade);reference[slot][shade]=1;
        }
        for(int s=0;s<32;s++)for(int i=0;i<64;i++)assert(bore_shade_has(&v,s,i)==reference[s][i]);
    }
    for(int count=1;count<=16;count++)for(int sto=0;sto<=7;sto++){
        uint8_t info=bore_far_cell_pack(count,sto);
        assert(bore_far_cell_count(info)==count&&bore_far_cell_storeys(info)==sto);
        assert(2399+bore_far_cell_count(info)==2399+count);
    }
    puts("Compact render cache: exact shade validity and all 128 far-cell encodings PASS");
    return 0;
}
