// Shared 13x13 landscape plan, expressed in playable lot cells.
// Existing console park templates are authoritative; previews consume the
// same deterministic footprint rather than independently scattering trees.
#pragma once
enum { PARK_GRASS, PARK_PATH, PARK_SOIL, PARK_WATER, PARK_PATIO };
static inline int park_variant(unsigned h){return (int)((h>>2)%5u);}
static inline int park_between(int x,int a,int b){return x>=a&&x<=b;}
static inline int park_surface(unsigned h,int square,int north,int x,int y){
    if((unsigned)x>=13u||(unsigned)y>=13u)return PARK_GRASS;
    if(north)y=12-y;
    if(square)return PARK_PATH;
    if(h&1u)x=12-x;
    int v=park_variant(h);
    if(v==0&&((x==1||x==2||x==3||x==9||x==10||x==11)&&
              (y==1||y==2||y==3||y==9||y==10||y==11)))return PARK_SOIL;
    if(v==1){
        if(park_between(x,2,4)&&park_between(y,2,4))return PARK_WATER;
        if((park_between(x,1,5)&&park_between(y,1,5))||
           (park_between(x,8,10)&&park_between(y,8,10)))return PARK_PATIO;
    }
    if(v==3&&(y==2||y==5||y==8)&&((x>=1&&x<=4)||(x>=8&&x<=11)))return PARK_SOIL;
    if(x==6)return PARK_PATH;
    if((v==0||v==1||v==4)&&y==6)return PARK_PATH;
    if(v==0&&park_between(x,4,8)&&park_between(y,4,8))return PARK_PATH;
    if(v==2&&((park_between(x,1,5)&&park_between(y,2,9))||
              (park_between(x,7,11)&&y==6)))return PARK_PATH;
    return PARK_GRASS;
}
static inline int park_tree_at(unsigned h,int square,int north,int i,int *x,int *y){
    static const unsigned char counts[]={4,11,8,4,8,4};
    static const unsigned char coords[6][11]={
        {1+5*16,11+5*16,1+7*16,11+7*16},
        {8+1*16,10+1*16,9+2*16,11+3*16,8+4*16,0+1*16,0+3*16,11+11*16,1+11*16,2+9*16,1+8*16},
        {11+1*16,11+3*16,11+5*16,11+7*16,11+9*16,11+11*16,8+1*16,9+1*16},
        {0,12,12*16,12+12*16},
        {1+1*16,11+1*16,3+2*16,9+2*16,1+11*16,11+11*16,2+9*16,10+9*16},
        {3+3*16,9+3*16,3+9*16,9+9*16}
    };
    int v=square?5:park_variant(h);
    if(i<0||i>=counts[v]||!x||!y)return 0;
    int p=coords[v][i];*x=p&15;*y=p>>4;
    if(!square&&(h&1u))*x=12-*x;
    if(north)*y=12-*y;
    return 1;
}
static inline const char *park_design_name(unsigned h,int square){
    static const char *const names[]={"Fountain garden","Pond and picnic grove","Recreation court","Flower garden","Open lawn"};
    return square?"Town square":names[park_variant(h)];
}
