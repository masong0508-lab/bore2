// Local service anchors. Shared by console and handheld; no save changes.
#pragma once
#include "outings.h"
enum { SITE_NONE, SITE_ROAD, SITE_WATER, SITE_AIR };
#define WORLD_OUT_0 97
#define WORLD_OUT_1 98
#define WORLD_TRIP 99
#define WORLD_HEIST 88
#define WORLD_DS_PENDING 82
static inline int world_hotel(int lot){return lot==LOT_HOTEL||lot==LOT_INN;}
static inline int world_heist_site(int lot,int terrain){
    if(lot==LOT_GAS)return SITE_ROAD;
    if(lot==LOT_SPACE)return SITE_AIR;
    if(lot==LOT_PARK&&terrain==BT_BEACH)return SITE_WATER;
    return SITE_NONE;
}
static inline int world_marker_needed(int lot,int terrain){
    return scene_music_venue(lot)>=SC_VENUE_FIRST||world_heist_site(lot,terrain)!=SITE_NONE||lot==LOT_SQUARE;
}
static inline int world_outing_anchor(int lot,unsigned char c){
    if(outing_group(lot,0)<0)return 0;
    if(c=='a')return 1;
    switch(lot){
    case LOT_CAFE:return c=='y'||c=='c';
    case LOT_LIBRARY:return c=='b'||c=='d';
    case LOT_GYM:return c==']'||c=='m'||c=='_';
    case LOT_POOL:return c=='~'||c=='>'||c==0xE7;
    case LOT_CINEMA:return c==';'||c=='y';
    case LOT_FARM:return c==0xDE;
    case LOT_PARK:return c=='N'||c==0xE4;
    case LOT_ARCADE:return c=='!'||c=='y';
    case LOT_GALLERY:return c==')'||c=='y';
    case LOT_HOTEL:case LOT_INN:return c=='y'||c=='I';
    case LOT_SPA:return c=='S'||c=='>';
    case LOT_SCHOOL:return c=='d'||c=='b';
    case LOT_SPACE:case LOT_OBS:return c>=0xD3&&c<=0xD9;
    default:return 0;
    }
}
static inline int world_trip_anchor(int lot,unsigned char c){return c=='+'||c=='@'||(world_hotel(lot)&&(c=='y'||c=='I'||c=='a'))||(lot==LOT_SQUARE&&c=='a');}
static inline int world_heist_anchor(int lot,int terrain,unsigned char c){return c=='a'?world_heist_site(lot,terrain):SITE_NONE;}
static inline int world_heist_first(int site){return site==SITE_WATER?5:site==SITE_AIR?7:0;}
static inline int world_heist_count(int site){return site==SITE_ROAD?4:site==SITE_WATER?2:site==SITE_AIR?3:10;}
static inline int world_heist_valid(int site,int target){int first=world_heist_first(site);return target>=first&&target<first+world_heist_count(site);}
static inline int world_heist_cycle(int site,int target,int dir){int n=world_heist_count(site),first=world_heist_first(site);return !world_heist_valid(site,target)?first:first+(target-first+n+(dir<0?-1:1))%n;}
static inline int world_trip_fare(int island,int desert){return island?35:desert?20:0;}
