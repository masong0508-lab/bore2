// Bounded, deterministic leisure preferences for town residents.
// Traits retain pop.c's stable numbers. Avoid choices based only on block dice.
#pragma once
#include "bigmap.h"
static inline int neighborhood_preference(int trait,int lot){
    switch(trait){
    case 3:return lot==LOT_GYM||lot==LOT_POOL||lot==LOT_SKATE?5:0;
    case 4:return lot==LOT_GALLERY||lot==LOT_LIBRARY||lot==LOT_CINEMA?5:0;
    case 5:return lot==LOT_ARCADE||lot==LOT_OBS||lot==LOT_SPACE?5:0;
    case 6:return lot==LOT_PARK||lot==LOT_RESTAURANT||lot==LOT_CAFE?4:0;
    case 7:return lot==LOT_DINER||lot==LOT_CAFE||lot==LOT_RESTAURANT?5:0;
    case 8:return lot==LOT_LIBRARY||lot==LOT_PARK||lot==LOT_FARM?5:0;
    case 9:return lot==LOT_SQUARE||lot==LOT_BAR||lot==LOT_CAFE?5:0;
    case 13:return lot==LOT_LIBRARY||lot==LOT_GALLERY?6:0;
    case 14:return lot==LOT_BAR||lot==LOT_CAFE||lot==LOT_CINEMA?4:0;
    case 15:return lot==LOT_PARK||lot==LOT_POOL||lot==LOT_SQUARE?5:0;
    default:return lot==LOT_PARK||lot==LOT_CAFE?2:0;
    }
}
static inline int neighborhood_open(int lot,int minute){
    switch(lot){
    case LOT_LIBRARY:case LOT_GALLERY:case LOT_FARM:return minute>=480&&minute<1140;
    case LOT_POOL:return minute>=540&&minute<1140;
    case LOT_SCHOOL:case LOT_OFFICE:case LOT_TOWER:case LOT_CLINIC:return 0;
    case LOT_BAR:return minute>=960&&minute<1440;
    case LOT_ARCADE:case LOT_CINEMA:return minute>=660&&minute<1380;
    case LOT_GYM:return minute>=360&&minute<1320;
    case LOT_CAFE:case LOT_SHOP:case LOT_SPA:return minute>=420&&minute<1260;
    default:return 1;
    }
}
static inline int neighborhood_score(int trait,int lot,int distance,unsigned tie){
    return 5*neighborhood_preference(trait,lot)-2*distance+(int)(tie&7u);
}
