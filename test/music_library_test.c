#include <assert.h>
#include <stdio.h>
#include "music_library.h"
int main(void){
    assert(music_library_size(0,0)==0&&music_library_size(0,1)==0);
    assert(music_library_size(40,0)==40&&music_library_size(40,1)==0);
    assert(music_library_size(52,0)==52&&music_library_size(52,1)==0);
    assert(music_library_size(126,0)==52&&music_library_size(126,1)==74);
    for(int i=0;i<126;i++){
        assert(music_library_index(126,i>=52,i>=52?i-52:i)==i);
        assert(music_library_index(126,i<52,i>=52?i:i+74)==-1);
    }
    assert(music_library_index(126,1,-1)==-1);
    assert(music_library_skip(126,1,125,1)==52);
    assert(music_library_skip(126,1,52,-1)==125);
    assert(music_library_skip(126,0,51,1)==0);
    assert(music_library_skip(126,0,85,-1)==51);
    assert(music_library_skip(52,1,0,1)==-1);
    assert(music_classic_next(126,0,0,0)==1);
    assert(music_classic_next(126,51,0,0)==1);
    assert(music_classic_next(126,88,0,0)==1);
    assert(music_classic_next(1,0,1,9)==0);
    assert(music_classic_next(0,0,1,9)==-1);
    assert(!music_classic_playing(126,0)&&!music_classic_playing(126,52));
    assert(music_classic_playing(1,0)&&music_classic_playing(126,1));
    for(int count=2;count<=126;count++)for(int current=-1;current<=126;current++)for(unsigned seed=0;seed<100;seed++){
        int next=music_classic_next(count,current,1,seed);
        assert(next>=1&&next<music_library_size(count,0));
        if(music_library_size(count,0)>2)assert(next!=current);
    }
    puts("Music library: classic autoplay, archive access, old banks and shuffle bounds PASS");
}
