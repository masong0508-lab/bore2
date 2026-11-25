#include <assert.h>
#include <stdio.h>
#include "../src/social_activity.h"
int main(void) {
    assert(social_activity_check(SA_SHARE,4,100)==SA_NO_MONEY);
    assert(social_activity_check(SA_SHARE,5,0)==SA_OK);
    assert(social_activity_check(SA_HELP,100,16)==SA_TIRED);
    assert(social_activity_check(SA_HELP,0,17)==SA_OK);
    assert(social_activity_check(SA_STORY,0,0)==SA_OK);
    assert(SA_SNACK_FOOD==15 && SA_HELP_REST==12 && SA_STORY_FUN==12);
    puts("Shared social activity cost and eligibility rules passed");
    return 0;
}
