#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "landscape.h"
static unsigned mix(unsigned x){x^=x>>16;x*=0x7feb352du;x^=x>>15;return x;}
static uint16_t heights[5][5];
static BoreLandPatch patch(int x,int y){
    uint16_t h[9];for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++)h[(j+1)*3+i+1]=heights[y+j][x+i];
    BoreLandPatch p;bore_land_build(&p,h);return p;
}
int main(void){
    int seams=0,pads=0,points=0;
    for(unsigned seed=1;seed<=300;seed++){
        for(int y=0;y<5;y++)for(int x=0;x<5;x++){unsigned h=mix(seed*9817u+x*97u+y*3011u);heights[y][x]=(uint16_t)((h%6000u)|((h&32u)?BORE_LAND_PAD:0));}
        for(int y=1;y<=3;y++)for(int x=1;x<=3;x++){
            BoreLandPatch p=patch(x,y);int h=heights[y][x]&32767;
            assert(bore_land_sample(&p,9,9)==h);
            for(int v=0;v<=36;v++)for(int u=0;u<=36;u++){
                int h2=bore_land_sample2(&p,u,v);assert(h2>=0&&h2<6000);points++;
                if((heights[y][x]&BORE_LAND_PAD)&&u>=10&&v>=10){assert(h2==h);pads++;}
            }
            if(x<3){BoreLandPatch q=patch(x+1,y);for(int v=0;v<=36;v++){assert(bore_land_sample2(&p,36,v)==bore_land_sample2(&q,0,v));seams++;}}
            if(y<3){BoreLandPatch q=patch(x,y+1);for(int u=0;u<=36;u++){assert(bore_land_sample2(&p,u,36)==bore_land_sample2(&q,u,0));seams++;}}
        }
    }
    for(int mode=0;mode<=4;mode++){int prev=0;for(int e=-100;e<=230;e++){
        int h=bore_land_relief(e,mode);assert(h>=prev&&h<=BORE_LAND_MAX);assert(h-prev<=190);if(mode==0)assert(!h);prev=h;
    }}
    assert(bore_land_relief(120,4)>bore_land_relief(120,2));
    assert(bore_land_relief(150,4)>4800);
    assert(bore_land_brush_delta(0,0,2)==2&&bore_land_brush_delta(1,1,2)==1&&bore_land_brush_delta(2,2,2)==0);
    assert(bore_land_brush_delta(2,0,2)==1&&bore_land_brush_delta(0,0,0)==2);
    printf("landscape: %d shared edge samples, %d foundation samples, %d bounded surface points; relief and brush PASS\n",seams,pads,points);
}
