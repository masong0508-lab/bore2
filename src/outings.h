// Local Activities: common rules and content for both engines. No save
// format change; entry fees are paid on start, benefits only on completion.
#pragma once
#include "scene_music.h"
enum { OUT_FUN, OUT_LEARN, OUT_COMMUNITY };
enum { OUT_SK_FUN, OUT_SK_LOGIC, OUT_SK_BODY, OUT_SK_CREAT, OUT_SK_COOK, OUT_SK_FARM, OUT_SK_CHAR };
enum { OUT_OK, OUT_NO_VENUE, OUT_AGE, OUT_CLOSED, OUT_MONEY, OUT_TIRED, OUT_BUSY };
#define OUT_N (SC_VENUE_COUNT*2)
#define OUT_DS_ACTION 81
typedef struct {
    const char *title,*desc,*done;
    unsigned short duration;
    unsigned char fee,age,from,to,energy,fun,social,food,rest,kind,skill,gain;
} Outing;
extern const Outing outing_table[OUT_N];
extern const char *const outing_venue_names[SC_VENUE_COUNT];
extern const char *const outing_reason[];
#ifdef OUTINGS_IMPLEMENTATION
const char *const outing_venue_names[SC_VENUE_COUNT]={
    "Cafe","Library","Gym","Pool","Cinema","Farm","Park","Arcade","Gallery","Hotel and spa","School","Science center"
};
const char *const outing_reason[]={"Ready","Visit a local venue first","Too young for this activity","Not open at this hour","Not enough money","Rest first","Finish your current action"};
// Steps at nominal 60 Hz; age is normalized: 0 child, 1 teen, 2 adult.
const Outing outing_table[OUT_N]={
    {"Coffee tasting","Try a flight of coffees and compare the flavors.","Found a favorite blend!",360,8,1,6,20,8,16,8,8,10,OUT_FUN,OUT_SK_COOK,24},
    {"Baking workshop","Mix a batch of pastries with the cafe baker.","Fresh pastries, new skills!",540,18,1,8,18,15,12,14,18,0,OUT_LEARN,OUT_SK_COOK,80},
    {"Reading circle","Read a short story and join the discussion.","What a good chapter!",480,0,0,9,19,8,18,15,0,0,OUT_LEARN,OUT_SK_LOGIC,60},
    {"Library volunteer","Sort returned books and help visitors find titles.","The shelves look great!",600,0,1,9,18,20,8,20,0,0,OUT_COMMUNITY,OUT_SK_CHAR,55},
    {"Circuit class","Follow a supervised strength circuit.","A proper workout!",540,10,1,6,21,25,18,10,0,0,OUT_LEARN,OUT_SK_BODY,85},
    {"Stretch session","Work through a gentle mobility routine.","Feeling flexible!",300,4,0,6,21,10,14,5,0,5,OUT_FUN,OUT_SK_BODY,35},
    {"Swim lesson","Practice strokes with the pool instructor.","A better swimmer!",540,12,0,9,19,22,20,8,0,0,OUT_LEARN,OUT_SK_BODY,75},
    {"Poolside social","Relax and meet the regulars by the pool.","A lovely afternoon!",360,6,0,9,19,8,20,18,0,4,OUT_FUN,OUT_SK_CHAR,20},
    {"Matinee screening","Enjoy a feature and the afternoon crowd.","That was a great film!",720,12,0,12,21,10,32,6,0,4,OUT_FUN,OUT_SK_FUN,35},
    {"Film discussion","Discuss a scene and how the story was made.","Seeing films differently!",420,5,1,14,21,12,16,18,0,0,OUT_LEARN,OUT_SK_CREAT,65},
    {"Garden workshop","Learn how to plant and care for seedlings.","Ready to grow!",480,5,0,7,18,18,16,10,0,0,OUT_LEARN,OUT_SK_FARM,70},
    {"Farm helper","Help prepare crates for the market.","The growers thank you!",600,0,1,7,17,24,10,24,0,0,OUT_COMMUNITY,OUT_SK_FARM,60},
    {"Nature observation","Look for birds and identify local plants.","Spotted something new!",360,0,0,6,20,10,20,5,0,0,OUT_LEARN,OUT_SK_LOGIC,40},
    {"Park clean up","Collect litter and tidy the common space.","A cleaner neighborhood!",480,0,0,7,19,18,10,24,0,0,OUT_COMMUNITY,OUT_SK_BODY,30},
    {"Arcade tournament","Join the local high-score competition.","What a close match!",540,10,0,11,22,15,32,14,0,0,OUT_FUN,OUT_SK_FUN,60},
    {"Game design club","Trade ideas for levels and clever puzzles.","New ideas for games!",480,4,1,13,21,12,22,15,0,0,OUT_LEARN,OUT_SK_CREAT,70},
    {"Sketching class","Learn to compose and sketch a still life.","A finished sketch!",540,12,0,10,18,12,20,8,0,0,OUT_LEARN,OUT_SK_CREAT,85},
    {"Gallery guide","Help introduce an exhibition to visitors.","The visitors enjoyed it!",480,0,1,10,17,14,10,25,0,0,OUT_COMMUNITY,OUT_SK_CHAR,65},
    {"Relaxation session","Take time out in the wellness lounge.","Much more relaxed!",480,15,1,9,21,5,18,4,0,25,OUT_FUN,OUT_SK_FUN,20},
    {"Cooking demonstration","Watch the kitchen team prepare a meal.","Dinner inspiration!",480,15,1,11,20,10,14,10,20,4,OUT_LEARN,OUT_SK_COOK,70},
    {"Study workshop","Work through a lesson with other learners.","Learned something new!",540,0,0,8,17,12,12,10,0,0,OUT_LEARN,OUT_SK_LOGIC,80},
    {"School community fair","Help set up displays and greet families.","Everyone pitched in!",480,0,0,9,17,16,20,24,0,0,OUT_COMMUNITY,OUT_SK_CHAR,45},
    {"Observatory tour","Explore the exhibits and study the night sky.","The sky feels closer!",540,8,0,10,21,10,24,8,0,0,OUT_LEARN,OUT_SK_LOGIC,85},
    {"Rocket workshop","Build a model and learn about flight.","Successful model launch!",600,12,1,10,19,16,24,12,0,0,OUT_LEARN,OUT_SK_LOGIC,95}
};
#endif
static inline int outing_group(int lot,int street){int cue=scene_music_venue(lot);return street||cue<SC_VENUE_FIRST?-1:cue-SC_VENUE_FIRST;}
static inline const Outing *outing_get(int id){return id>=0&&id<OUT_N?&outing_table[id]:0;}
static inline int outing_check(int id,int group,int age,int minute,long money,int rest,int busy){
    const Outing *o=outing_get(id);
    if(!o||group<0||group>=SC_VENUE_COUNT||id/2!=group)return OUT_NO_VENUE;
    if(age<o->age)return OUT_AGE;
    if(minute<o->from*60||minute>=o->to*60)return OUT_CLOSED;
    if(money<o->fee)return OUT_MONEY;
    if(rest<(int)o->energy+8)return OUT_TIRED;
    return busy?OUT_BUSY:OUT_OK;
}
static inline int outing_complete(int id,int group,int remaining){return outing_get(id)&&group==id/2&&remaining==0;}
