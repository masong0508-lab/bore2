// Local Activities: common rules and content for both engines. No save
// format change; entry fees are paid on start, benefits only on completion.
#pragma once
#include "scene_music.h"
#include "living_calendar.h"
enum { OUT_FUN, OUT_LEARN, OUT_COMMUNITY };
enum { OUT_SK_FUN, OUT_SK_LOGIC, OUT_SK_BODY, OUT_SK_CREAT, OUT_SK_COOK, OUT_SK_FARM, OUT_SK_CHAR };
enum { OUT_OK, OUT_NO_VENUE, OUT_AGE, OUT_CLOSED, OUT_MONEY, OUT_TIRED, OUT_BUSY };
#define OUT_N LIVING_OUTING_COUNT
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
    {"Rocket workshop","Build a model and learn about flight.","Successful model launch!",600,12,1,10,19,16,24,12,0,0,OUT_LEARN,OUT_SK_LOGIC,95},
    {"Brunch club","Share a weekend breakfast with the cafe regulars.","A lovely brunch!",480,12,0,8,14,8,21,22,25,8,OUT_FUN,OUT_SK_COOK,40},
    {"Bread for neighbors","Bake rolls for the neighborhood table.","Fresh bread to share!",600,8,1,9,17,20,14,23,8,0,OUT_COMMUNITY,OUT_SK_COOK,90},
    {"Local history club","Trade stories about San Boreas with local readers.","Found some history!",540,0,0,10,18,10,17,20,0,0,OUT_LEARN,OUT_SK_LOGIC,70},
    {"Book exchange","Help sort books for the community exchange.","Books found new homes!",480,0,0,10,17,15,15,25,0,0,OUT_COMMUNITY,OUT_SK_CHAR,45},
    {"Fitness meet","Practice a friendly group endurance circuit.","A new personal best!",660,12,1,8,20,30,25,18,0,0,OUT_LEARN,OUT_SK_BODY,100},
    {"Family exercise","Join a gentle group movement session.","Everyone got moving!",360,0,0,9,18,12,20,20,0,5,OUT_FUN,OUT_SK_BODY,40},
    {"Family swim","Practice together in the shallow lanes.","A great weekend swim!",600,10,0,10,19,24,30,18,0,0,OUT_LEARN,OUT_SK_BODY,80},
    {"Water safety club","Help visitors learn basic pool safety.","A safer pool today!",420,0,1,10,17,12,14,25,0,0,OUT_COMMUNITY,OUT_SK_CHAR,55},
    {"Festival matinee","Watch a weekend feature with the locals.","The crowd loved it!",720,14,0,11,20,10,36,17,0,5,OUT_FUN,OUT_SK_FUN,40},
    {"Movie makers club","Discuss scripts and practice a short scene.","That scene worked!",540,7,1,12,20,16,24,20,0,0,OUT_LEARN,OUT_SK_CREAT,85},
    {"Seed exchange","Swap growing tips and sort local seed varieties.","Ready for next season!",540,4,0,8,17,18,20,20,0,0,OUT_LEARN,OUT_SK_FARM,100},
    {"Produce fair help","Help neighbors prepare their produce displays.","The market is ready!",660,0,1,7,18,25,18,26,0,0,OUT_COMMUNITY,OUT_SK_FARM,85},
    {"Community picnic","Share an afternoon with local park visitors.","A friendly afternoon!",480,5,0,10,19,8,27,28,18,8,OUT_FUN,OUT_SK_CHAR,40},
    {"Trail volunteers","Clear a walking path for neighborhood visitors.","The path looks great!",600,0,1,8,18,24,15,26,0,0,OUT_COMMUNITY,OUT_SK_BODY,55},
    {"Arcade league","Practice in the weekend score league.","A close competition!",660,15,0,10,22,20,35,21,0,0,OUT_FUN,OUT_SK_FUN,85},
    {"Puzzle makers","Build a puzzle concept with the local club.","A clever new puzzle!",540,5,1,12,20,14,26,20,0,0,OUT_LEARN,OUT_SK_CREAT,90},
    {"Open studio","Work on a painting with local artists.","A lovely new painting!",600,15,0,10,18,15,25,20,0,0,OUT_LEARN,OUT_SK_CREAT,100},
    {"Art fair helpers","Welcome visitors to the weekend exhibition.","The gallery is buzzing!",540,0,1,10,18,18,20,27,0,0,OUT_COMMUNITY,OUT_SK_CHAR,75},
    {"Weekend retreat","Join a calm group wellness session.","What a restful break!",600,18,1,9,20,6,24,12,0,32,OUT_FUN,OUT_SK_FUN,30},
    {"Hotel cooking club","Learn a new weekend lunch recipe.","The meal was delicious!",540,16,1,11,19,14,19,17,25,0,OUT_LEARN,OUT_SK_COOK,95},
    {"Homework club","Study with classmates before the new week.","Ready for Monday!",600,0,0,9,17,15,15,18,0,0,OUT_LEARN,OUT_SK_LOGIC,90},
    {"School open day","Welcome families and help with displays.","The school feels alive!",540,0,0,10,17,16,23,27,0,0,OUT_COMMUNITY,OUT_SK_CHAR,55},
    {"Science open house","Explore demonstrations with local visitors.","A fascinating day!",600,10,0,10,20,14,29,20,0,0,OUT_LEARN,OUT_SK_LOGIC,95},
    {"Astronomy club","Discuss the sky and build a star chart.","A new view of the sky!",540,6,1,12,21,12,25,20,0,0,OUT_LEARN,OUT_SK_LOGIC,90}
};
#endif
static inline int outing_group(int lot,int street){int cue=scene_music_venue(lot);return street||cue<SC_VENUE_FIRST?-1:cue-SC_VENUE_FIRST;}
static inline const Outing *outing_get(int id){return id>=0&&id<OUT_N?&outing_table[id]:0;}
static inline int outing_check(int id,int group,int age,int minute,long money,int rest,int busy){
    const Outing *o=outing_get(id);
    if(!o||group<0||group>=SC_VENUE_COUNT||living_outing_group(id)!=group)return OUT_NO_VENUE;
    if(age<o->age)return OUT_AGE;
    if(minute<o->from*60||minute>=o->to*60)return OUT_CLOSED;
    if(money<o->fee)return OUT_MONEY;
    if(rest<(int)o->energy+8)return OUT_TIRED;
    return busy?OUT_BUSY:OUT_OK;
}
static inline int outing_complete(int id,int group,int remaining){return outing_get(id)&&group==living_outing_group(id)&&remaining==0;}
