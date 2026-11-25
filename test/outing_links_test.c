#include <assert.h>
#include <stdio.h>
#include "../src/outing_links.h"
int main(void) {
    assert(outing_link_social_gain(0, 13) == 0);
    assert(outing_link_social_gain(0, 14) == 3);
    assert(outing_link_social_gain(1, 0) == 6);
    assert(outing_link_distance(6, 6, 9, 8, 18) == 13);
    assert(outing_link_distance(6, 6, 10, 6, 18) == 16);
    assert(outing_link_distance(6, 6, 10, 7, 18) == -1);
    assert(outing_link_distance(17, 6, 18, 6, 18) == -1);
    assert(outing_link_distance(0, 0, -1, 0, 18) == -1);
    assert(outing_link_distance(0, 0, 0, 0, 0) == -1);
    assert(outing_link_grade(60, 1, 1) == 64);
    assert(outing_link_grade(98, 1, 1) == 100);
    assert(outing_link_grade(60, 0, 1) == 60);
    assert(outing_link_grade(60, 1, 0) == 60);
    puts("Outing links: local contact range, social gains and school progress passed");
    return 0;
}
