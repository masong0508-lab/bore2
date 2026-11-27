#include <assert.h>
#include <stdio.h>
#include "../app/src/main/cpp/window_math.h"
int main(void){
    assert(bore_window_side(8)==17);
    assert(bore_window_side(16)==33);
    assert(bore_window_side(0)==0&&bore_window_side(17)==0);
    assert(bore_window_center(-20,8,1024)==8);
    assert(bore_window_center(2000,8,1024)==1015);
    assert(bore_window_center(512,8,1024)==512);
    assert(bore_window_center(2,8,10)==-1);
    assert(bore_window_valid(8,289)&&!bore_window_valid(8,288));
    unsigned p=bore_window_pack(3,21,0xa5,4,255);
    assert((p&7)==3&&((p>>3)&63)==21&&((p>>9)&255)==0xa5);
    assert(((p>>17)&7)==4&&((p>>20)&255)==255);
    puts("Android map bounds, capacities and tile protocol passed");
    return 0;
}
