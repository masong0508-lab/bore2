#include <assert.h>
#include <stdio.h>
#include "ground_patch.h"
/* Independent reference, unchanged original byte-mask builder. */
static int reference(const uint8_t *mat,uint32_t *out){
    uint8_t used[324]={0};int n=0;
    for(int y=0;y<18;y++)for(int x=0;x<18;x++){
        int i=y*18+x;if(used[i])continue;
        int k=mat[i],w=1,h=1;
        while(w<6&&x+w<18&&!used[i+w]&&mat[i+w]==k)w++;
        while(h<6&&y+h<18){
            int ok=1;for(int a=0;a<w;a++)if(used[i+h*18+a]||mat[i+h*18+a]!=k){ok=0;break;}
            if(!ok)break;
            h++;
        }
        for(int b=0;b<h;b++)memset(used+i+b*18,1,w);
        out[n++]=ground_patch_pack(x,y,w,h,k);
    }
    return n;
}
int main(void){
    TerrainRenderCache cache;
    int owner=TERRAIN_CACHE_NONE;
    assert(sizeof cache==4u*4u*GROUND_PATCH_CACHE*sizeof(uint32_t));
    assert(sizeof cache.island==4096);
    assert(terrain_cache_claim(&owner,TERRAIN_CACHE_CITY));
    memset(cache.city,0,sizeof cache.city);
    cache.city[3][3][GROUND_PATCH_CACHE-1]=ground_patch_pack(12,13,6,5,32);
    assert(!terrain_cache_claim(&owner,TERRAIN_CACHE_CITY));
    assert(ground_patch_mat(cache.city[3][3][GROUND_PATCH_CACHE-1])==32);
    assert(terrain_cache_claim(&owner,TERRAIN_CACHE_ISLAND));
    memset(cache.island.tag,0,sizeof cache.island.tag);
    cache.island.tag[31][31]=321;
    assert(!terrain_cache_claim(&owner,TERRAIN_CACHE_ISLAND));
    assert(cache.island.tag[31][31]==321);
    assert(terrain_cache_claim(&owner,TERRAIN_CACHE_CITY));
    memset(cache.city,0,sizeof cache.city);
    assert(cache.city[0][0][0]==0);
    unsigned state=0xc0ffee;
    uint8_t mat[324];uint32_t a[324],b[324];
    for(int t=0;t<4096;t++){
        for(int i=0;i<324;i++){
            state=state*1664525u+1013904223u;
            mat[i]=(uint8_t)((t&3)==0?0:(state>>24)%((t&3)==1?2:(t&3)==2?8:33));
        }
        int n=reference(mat,a),m=ground_patch_build(mat,b,324);
        assert(n==m&&!memcmp(a,b,(unsigned)n*sizeof *a));
        memset(b,0,sizeof b);assert(ground_patch_build(mat,b,80)==n);
        assert(!memcmp(a,b,(unsigned)(n<80?n:80)*sizeof *a));
    }
    for(int bx=0;bx<1021;bx+=7)for(int by=0;by<1021;by+=19){
        int keys[16]={0},n=0;
        for(int y=0;y<4;y++)for(int x=0;x<4;x++){
            int s=ground_cache_slot(bx+x,by+y);assert(!keys[s]);
            keys[s]=ground_cache_key(bx+x,by+y);
        }
        for(int y=0;y<4;y++)for(int x=0;x<4;x++){
            int s=ground_cache_slot(bx+x+1,by+y+1);
            if(keys[s]==ground_cache_key(bx+x+1,by+y+1))n++;
        }
        assert(n==9);ground_cache_drop(keys,bx+2,by+2);
        assert(!keys[ground_cache_slot(bx+2,by+2)]);
        assert(keys[ground_cache_slot(bx+1,by+1)]==ground_cache_key(bx+1,by+1));
    }
    puts("Terrain cache: 4096 exact rectangle comparisons and retained-block rebase checks PASS");
    return 0;
}
