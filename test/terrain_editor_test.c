#include "bigmap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#undef sprintf
#undef snprintf
int bsprintf(char *o,const char *f,...){va_list a;va_start(a,f);int n=vsprintf(o,f,a);va_end(a);return n;}
int bsnprintf(char *o,unsigned cap,const char *f,...){va_list a;va_start(a,f);int n=vsnprintf(o,cap,f,a);va_end(a);return n;}
void boot(const char *s){(void)s;}
void loading(const char *s,int p){(void)s;(void)p;}
int ram8,dbg_miss[4];
void *xalloc(size_t n){return calloc(1,n);}
void world_gh_reset(void){}
int main(void){
    bm_ovr_clear();
    /* The final coordinate must be as editable and serializable as (0,0). */
    assert(bm_ovr_set(0,0,BO_ELEV,3));
    assert(bm_ovr_sculpt(BM_N-1,BM_N-1,0,1));
    assert(bm_ovr_delta(BM_N-1,BM_N-1)==2);
    assert(bm_ovr_delta(0,0)==3);
    int it=0,x,y,f,seen=0;u8 v[5];
    while(bm_ovr_next(&it,&x,&y,&f,v))if(x==BM_N-1&&y==BM_N-1){assert(f&BO_ELEV);assert(v[4]==2);seen++;}
    assert(seen==1);
    /* Repeated painting and undo must recover capacity, including clusters of colliding keys. */
    bm_ovr_clear();
    for(int i=0;i<1400;i++){
        int x=i%BM_N,y=i/BM_N;
        assert(bm_ovr_set(x,y,BO_ELEV,5));
        assert(bm_ovr_set(x,y,BO_NONE,0));
        assert(bm_ovr_n()==0);
    }
    for(int i=0;i<537;i++)assert(bm_ovr_set(i,20,BO_ELEV,3));
    assert(!bm_ovr_sculpt(800,800,2,1));
    assert(bm_ovr_delta(800,800)==0&&bm_ovr_n()==537);
    for(int i=0;i<537;i+=2)bm_ovr_unset(i,20,BO_ELEV);
    for(int i=1;i<537;i+=2)assert(bm_ovr_delta(i,20)==3);
    assert(bm_ovr_sculpt(800,800,2,1));
    assert(bm_ovr_delta(800,800)==2&&bm_ovr_delta(802,801)==1&&bm_ovr_delta(802,802)==0);
    assert(bm_ovr_sculpt(800,800,2,-1));
    assert(bm_ovr_n()==268);
    assert(bm_ovr_set(800,800,BO_ROAD,3));
    assert(bm_ovr_sculpt(800,800,0,1));
    assert(bm_ovr_sculpt(800,800,0,-1));
    it=0;seen=0;while(bm_ovr_next(&it,&x,&y,&f,v))if(x==800&&y==800){assert(f==BO_ROAD&&v[2]==3);seen++;}
    assert(seen==1);
    bm_ovr_clear();bm_gen=BM_GEN_CURRENT;bm_new(0x12345678u);
    const Settle *s=bm_set(0);assert(s);
    int cx=s->x,cy=s->y;
    assert(bm_render_territory(cx,cy)==0);
    assert(bm_ovr_set(cx,cy,BO_TERR,255));
    assert(bm_render_territory(cx,cy)==255);
    assert(bm_ovr_set(cx,cy,BO_NONE,0));
    assert(bm_render_territory(cx,cy)==0);
    int found=0;
    for(int yy=0;yy<BM_N&&!found;yy+=7)for(int xx=0;xx<BM_N&&!found;xx+=7){
        if(bm_terrain(xx,yy)==BT_SEA||bm_set_at(xx,yy)!=255)continue;
        assert(bm_ovr_set(xx,yy,BO_TERR,0));assert(bm_render_territory(xx,yy)==0);
        BInfo b;bm_block(xx,yy,&b);assert(b.set==0);
        assert(bm_ovr_set(xx,yy,BO_NONE,0));assert(bm_render_territory(xx,yy)==255);found=1;
    }
    assert(found&&bm_render_territory(-1,0)==255);
    puts("terrain editor: corners, undo reuse, collision chains, atomic capacity, reversible brush and visible territory PASS");
    return 0;
}
