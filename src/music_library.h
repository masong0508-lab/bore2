#pragma once
#define MUSIC_CLASSIC_COUNT 52
static inline int music_library_size(int bank,int archive){
    if(bank<=0)return 0;
    return archive?(bank>MUSIC_CLASSIC_COUNT?bank-MUSIC_CLASSIC_COUNT:0):(bank<MUSIC_CLASSIC_COUNT?bank:MUSIC_CLASSIC_COUNT);
}
static inline int music_library_index(int bank,int archive,int index){
    return index>=0&&index<music_library_size(bank,archive)?index+(archive?MUSIC_CLASSIC_COUNT:0):-1;
}
static inline int music_library_skip(int bank,int archive,int current,int direction){
    int n=music_library_size(bank,archive),base=archive?MUSIC_CLASSIC_COUNT:0;
    if(!n)return -1;
    int i=current-base;
    if(i<0||i>=n)return base+(direction<0?n-1:0);
    return base+(i+n+(direction<0?-1:1))%n;
}
static inline int music_classic_playing(int bank,int current){
    int n=music_library_size(bank,0);
    return n==1?current==0:current>0&&current<n;
}
static inline int music_classic_next(int bank,int current,int shuffle,unsigned entropy){
    int n=music_library_size(bank,0);
    if(n<=1)return n?0:-1;
    if(!shuffle)return current>=1&&current<n-1?current+1:1;
    int next=1+(int)(entropy%(unsigned)(n-1));
    if(next==current&&n>2)next=next<n-1?next+1:1;
    return next;
}
