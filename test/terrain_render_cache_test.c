#include <assert.h>
#include <stdio.h>
#include "ground_patch.h"
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
    puts("Terrain render cache: full capacity, exclusive owner transitions; 4096 bytes reclaimed");
    return 0;
}
