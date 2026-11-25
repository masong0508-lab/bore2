#include <assert.h>
#include <stdio.h>
#include "../src/living_calendar.h"
int main(void) {
    assert(living_score_index(0, 359, 0, 126) == 92);
    assert(living_score_index(0, 360, 0, 126) == 72);
    assert(living_score_index(19, 719, 0, 126) == 91);
    assert(living_score_index(19, 720, 0, 126) == 71);
    assert(living_score_index(19, 720, 5, 126) == 125);
    assert(living_score_index(2, 720, 6, 126) == 113);
    assert(living_score_index(1, 720, 6, 126) == 53);
    assert(living_score_index(8, 1080, 6, 126) == 100);
    assert(living_score_index(8, 360, 0, 72) == 60);
    assert(living_score_index(8, 360, 0, 60) == -1);
    assert(living_score_index(-1, 360, 0, 126) == -1);
    assert(living_outing_index(0,0,0) == 0);
    assert(living_outing_index(11,1,6) == 47);
    assert(living_outing_index(1,0,5) == 26);
    assert(living_outing_index(12,0,5) == -1);
    assert(living_outing_index(0,2,5) == -1);
    assert(living_outing_group(47) == 11);
    assert(living_outing_group(48) == -1);
    for(int day=0;day<7;day++)for(int minute=0;minute<1440;minute+=15)for(int cue=0;cue<20;cue++){
        int id=living_score_index(cue,minute,day,126);assert(id>=52&&id<126);
    }
    puts("Living calendar: scene schedule, legacy bank fallback and all venue IDs passed");
    return 0;
}
