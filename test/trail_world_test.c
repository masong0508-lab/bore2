#include "bigmap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#undef sprintf
#undef snprintf
int bsprintf(char *o,const char *f,...){va_list a;va_start(a,f);int n=vsprintf(o,f,a);va_end(a);return n;}
int bsnprintf(char *o,unsigned cap,const char *f,...){va_list a;va_start(a,f);int n=vsnprintf(o,cap,f,a);va_end(a);return n;}
void boot(const char *s){(void)s;}
void loading(const char *s,int p){(void)s;(void)p;}
int ram8,dbg_miss[4];
void *xalloc(size_t n){return calloc(1,n);}
void world_gh_reset(void){}
static int eligible(const BInfo *b){return b->set==255&&b->lot==LOT_WILD&&(b->terrain==BT_GRASS||b->terrain==BT_FOREST||b->terrain==BT_ROCKY);}
int main(void){
    const u32 seeds[]={1,0x5eed1234u,0xc0ffeeu};
    int trails=0,joins=0,summits=0;
    assert(BM_BLK==TRAIL_SIDE);
    for(int k=0;k<3;k++){
        bm_ovr_clear();bm_dials_default();bm_gen=8;bm_hills=4;bm_new(seeds[k]);
        for(int y=24;y<BM_N-1;y+=11)for(int x=24;x<BM_N-1;x+=11){
            BInfo b;BoreTrail p,again;bm_block(x,y,&b);bm_trail_plan(x,y,&b,&p);bm_trail_plan(x,y,&b,&again);
            assert(!memcmp(&p,&again,sizeof p));
            if(!eligible(&b)){assert(!p.links);continue;}
            if(p.links)trails++;
            if(p.kind==TRAIL_SUMMIT)summits++;
            for(int d=0;d<2;d++){
                int nx=x+(d==0),ny=y+(d==1),dir=d==0?TRAIL_E:TRAIL_S;
                BInfo q;BoreTrail r;bm_block(nx,ny,&q);if(!eligible(&q))continue;
                bm_trail_plan(nx,ny,&q,&r);
                assert(!!(p.links&(1u<<dir))==!!(r.links&(1u<<((dir+2)&3))));
                for(int i=0;i<BM_BLK;i++)
                    assert(bore_trail_cell(&p,d==0?17:i,d==0?i:17)==bore_trail_cell(&r,d==0?0:i,d==0?i:0));
                if(p.links&(1u<<dir))joins++;
            }
        }
    }
    assert(trails>0&&joins>0);
    printf("generated trails: %d trail blocks, %d matching edges, %d sampled summit lookouts\n",trails,joins,summits);
}
