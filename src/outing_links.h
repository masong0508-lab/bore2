// Persistent links from venue sessions to the ordinary life simulation.
#pragma once
static inline int outing_link_social_gain(int community,int social) {
    return community ? 6 : social >= 14 ? 3 : 0;
}
// Coordinates are tiles within the same loaded map; never credit a resident
// from a neighboring block, even if they are physically close to its edge.
static inline int outing_link_distance(int ax,int ay,int bx,int by,int block) {
    if(block<=0||ax<0||ay<0||bx<0||by<0||ax/block!=bx/block||ay/block!=by/block)return -1;
    int dx=ax-bx,dy=ay-by;
    if(dx < -4||dx > 4||dy < -4||dy > 4)return -1;
    int d=dx*dx+dy*dy;
    return d<=16?d:-1;
}
static inline int outing_link_grade(int grade,int study,int student) {
    if(!study||!student)return grade;
    return grade>=96?100:grade+4;
}
