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
static int real_roads(const BInfo *b){
    int rd=b->road&3;
    if(b->lot!=LOT_RIVER&&(b->road&BR_MEGA)){int p=BR_PART(b->road);if(p&1)rd&=~2;if(p&2)rd&=~1;}
    return rd;
}
int main(void){
    const u32 seed[]={1,0x5eed1234u,0xc0ffeeu};
    unsigned roads=0,farms=0,buildings=0,north=0,south=0,mega=0;
    for(int k=0;k<3;k++){
        bm_ovr_clear();bm_dials_default();bm_gen=BM_GEN_CURRENT;bm_new(seed[k]);
        for(int s=0;s<bm_nseg();s++){
            int c,a,b,v;bm_seg(s,&c,&a,&b,&v);
            for(int z=a;z<=b;z++){
                int x=v?c:z,y=v?z:c;BInfo q;bm_block(x,y,&q);
                assert(real_roads(&q)&(v?2:1));roads++;
                for(int d=-1;d<=1;d++){
                    int yy=y+d;if(yy<0||yy>=BM_N)continue;
                    BInfo f;bm_block(x,yy,&f);
                    if(f.set!=255||f.lot!=LOT_FARM)continue;
                    LotPlan p;u8 ft[5];bm_lot_plan(x,yy,&f,&p);
                    BInfo next;bm_block(x,yy+1,&next);
                    assert((p.face==LF_NORTH&&(real_roads(&f)&1))||(p.face==LF_SOUTH&&(real_roads(&next)&1)));
                    assert(bm_lot_foot(x,yy,&f,ft,0));
                    assert(ft[0]==BM_LOT0+8);
                    if(p.face==LF_NORTH){north++;assert(ft[1]==BM_LOT0+6&&ft[3]==BM_LOT0+11);}
                    else {south++;assert(ft[1]==BM_LOT0+1&&ft[3]==BM_LOT0+6);}
                    int pad=0;bm_hill_t(x,yy,&pad);assert(pad);
                    farms++;
                }
            }
        }
        for(int i=0;i<bm_nset();i++){
            const Settle *s=bm_set(i);
            for(int y=s->y-s->r;y<=s->y+s->r;y++)for(int x=s->x-s->r;x<=s->x+s->r;x++){
                BInfo b,q;bm_block(x,y,&b);if(b.set!=i)continue;
                bm_block(x+3,y-2,&q);bm_block(x-2,y+3,&q);
                bm_block(x,y,&q);assert(!memcmp(&b,&q,sizeof b));
                if(b.terrain!=BT_SEA&&!(b.road&BR_MEGA)&&!(b.road&3)){
                    BInfo w,n;bm_block(x-1,y,&w);bm_block(x,y-1,&n);
                    assert(!!(b.road&4)==!!((real_roads(&w)&1)||(real_roads(&n)&2)));
                }
                if(b.lot==LOT_RIVER||b.terrain==BT_SEA)continue;
                if(b.road&BR_MEGA){mega++;continue;}
                u8 f[5];if(!bm_lot_foot(x,y,&b,f,0))continue;
                if(b.lot==LOT_FARM)continue;
                int face=bm_lot_face(x,y);bm_block(x,y+1,&q);
                assert((face==LF_NORTH&&(real_roads(&b)&1))||(face==LF_SOUTH&&(real_roads(&q)&1)));
                buildings++;
            }
        }
    }
    assert(roads>100&&farms>0&&north>0&&south>0&&buildings>100);
    printf("World v9: %u highway edges, %u rural farm checks (%u north / %u south), %u accessible building frontages, %u plaza blocks\n",roads,farms,north,south,buildings,mega);
    return 0;
}
