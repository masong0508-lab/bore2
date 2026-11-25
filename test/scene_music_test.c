#include <assert.h>
#include <stdio.h>
#include "../src/scene_music.h"
int main(void) {
    assert(scene_music_role(SC_MENU_BUILD,1,1,0,LOT_SKATE,BT_BEACH,1)==SC_BUILD);
    assert(scene_music_role(SC_MENU_TRAVEL,0,0,0,LOT_HOUSE,BT_GRASS,0)==SC_TRAVEL);
    assert(scene_music_role(0,1,0,0,LOT_WILD,BT_BEACH,0)==SC_ISLAND);
    assert(scene_music_role(0,0,1,0,LOT_HOUSE,BT_GRASS,0)==SC_SKATE);
    assert(scene_music_role(0,0,0,1,LOT_HOUSE,BT_GRASS,1)==SC_TRAVEL);
    assert(scene_music_role(0,0,0,0,LOT_RIVER,BT_GRASS,1)==SC_WATER);
    assert(scene_music_role(0,0,0,0,LOT_CAFE,BT_GRASS,0)==SC_CAFE);
    assert(scene_music_role(0,0,0,0,LOT_HOUSE,BT_GRASS,1)==SC_STREET);
    assert(scene_music_role(0,0,0,0,LOT_HOUSE,BT_GRASS,0)==SC_NEIGHBORHOOD);
    assert(SC_N==20&&SC_FIRST+SC_N==72);
    assert(scene_music_role(0,0,0,0,LOT_LIBRARY,BT_GRASS,0)==SC_LIBRARY);
    assert(scene_music_role(0,0,0,0,LOT_POOL,BT_GRASS,0)==SC_POOL);
    assert(scene_music_role(0,0,0,0,LOT_GYM,BT_GRASS,1)==SC_STREET);
    SceneMusicGate g=SC_MUSIC_GATE_INIT;
    assert(scene_music_settle(&g,SC_NEIGHBORHOOD)==-1);
    assert(scene_music_settle(&g,SC_NEIGHBORHOOD)==SC_NEIGHBORHOOD);
    assert(scene_music_settle(&g,SC_STREET)==SC_NEIGHBORHOOD);
    assert(scene_music_settle(&g,SC_NEIGHBORHOOD)==SC_NEIGHBORHOOD);
    for(int i=0;i<3;i++)assert(scene_music_settle(&g,SC_MARKET)==SC_NEIGHBORHOOD);
    assert(scene_music_settle(&g,SC_MARKET)==SC_MARKET);
    assert(scene_music_settle(&g,SC_STREET)==SC_MARKET);
    assert(scene_music_settle(&g,-1)==SC_MARKET);
    for(int i=0;i<3;i++)assert(scene_music_settle(&g,SC_STREET)==SC_MARKET);
    assert(scene_music_settle(&g,SC_STREET)==SC_STREET);
    assert(scene_music_settle(&g,SC_SKATE)==SC_STREET);
    assert(scene_music_settle(&g,SC_SKATE)==SC_SKATE);
    assert(scene_music_settle(&g,-1)==SC_SKATE);
    scene_music_reset(&g);assert(g.settled==-1);
    puts("Area music roles, precedence and debounce passed");return 0;
}
