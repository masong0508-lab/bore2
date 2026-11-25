// Stable bank and activity IDs shared by the native ports.
#pragma once
#define LIVING_SCORE_FIRST 52
#define LIVING_SCORE_ROLES 20
#define LIVING_SCORE_COUNT 74
#define LIVING_BANK_COUNT (LIVING_SCORE_FIRST+LIVING_SCORE_COUNT)
#define LIVING_OUTING_GROUPS 12
#define LIVING_OUTING_COUNT 48
static inline int living_score_index(int cue,int minute,int weekday,int count){
    if(cue<0||cue>=LIVING_SCORE_ROLES)return -1;
    int base=LIVING_SCORE_FIRST+cue;
    if(base>=count)return -1;
    if(count<LIVING_BANK_COUNT||minute<0||minute>=1440)return base;
    if(minute<360||minute>=1080)return 92+cue;
    if(minute<720)return 72+cue;
    if(weekday==5||weekday==6){
        if(cue==0)return 112;
        if(cue==2)return 113;
        if(cue>=8)return 114+cue-8;
    }
    return base;
}
static inline int living_outing_index(int group,int slot,int day){
    if(group<0||group>=LIVING_OUTING_GROUPS||slot<0||slot>1)return -1;
    return group*2+slot+((day==5||day==6)?24:0);
}
static inline int living_outing_group(int id){
    return id<0||id>=LIVING_OUTING_COUNT?-1:(id%24)/2;
}
