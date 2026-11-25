#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../source/ds_city_budget.h"
static uint32_t seed=701;
static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static int between(int a,int b){return a+(int)(rnd()%(unsigned)(b-a+1));}
int main(void) {
    CityGrid g;int x[24],y[24];unsigned long old_checks=0,new_checks=0;
    for(int batch=0;batch<200;batch++){
        city_grid_clear(&g);uint32_t live=0;
        for(int k=0;k<24;k++){
            x[k]=between(-300,600);y[k]=between(-300,600);
            if(rnd()%5){live|=UINT32_C(1)<<k;city_grid_put(&g,(unsigned)k,x[k],y[k]);}
        }
        for(int j=0;j<100;j++){
            int px=between(-400,700),py=between(-400,700),r=between(1,44);
            uint32_t m=city_grid_query(&g,px,py,r),expected=0,actual=0;
            for(int k=0;k<24;k++){
                uint32_t b=UINT32_C(1)<<k;
                if(live&b){old_checks++;if(city_within(x[k],y[k],px,py,r))expected|=b;}
                if(m&b){new_checks++;assert(live&b);if(city_within(x[k],y[k],px,py,r))actual|=b;}
            }
            assert(actual==expected);
        }
    }
    assert(new_checks*4<old_checks);
    printf("Exact collision parity: %lu full scans vs %lu candidates\n",old_checks,new_checks);
    CityDraw q[3];int n=0;
    city_draw_insert(q,&n,3,1,100);city_draw_insert(q,&n,3,2,200);city_draw_insert(q,&n,3,3,300);
    city_draw_insert(q,&n,3,4,50);city_draw_insert(q,&n,3,5,200);
    assert(n==3&&q[0].id==4&&q[1].id==1&&q[2].id==2);
    // Independently project random points within an actor's bounding sphere.
    // Whenever a point projects inside the real viewport, its whole actor
    // must survive the conservative fixed-point cull.
    int visible_points=0,culled=0;
    for(int j=0;j<30000;j++){
        double yaw=between(0,32767)*6.283185307179586/32768.0;
        double pitch=between(1800,7600)*6.283185307179586/32768.0;
        CityView v={between(-7000,7000),between(0,2000),between(-7000,7000),between(3500,18000),
            (int)round(cos(yaw)*4096),(int)round(sin(yaw)*4096),
            (int)round(cos(pitch)*4096),(int)round(sin(pitch)*4096)};
        int ax=between(-18000,18000),ay=between(0,1000),az=between(-18000,18000),r=between(100,640);
        int vis=city_visible(&v,ax,ay,az,r);if(!vis)culled++;
        for(int k=0;k<12;k++){
            int dx=between(-r,r),dy=between(-r,r),dz=between(-r,r);
            if(dx*dx+dy*dy+dz*dz>r*r)continue;
            double X=ax+dx-v.x,Y=ay+dy-v.y,Z=az+dz-v.z;
            double side=X*cos(yaw)+Z*sin(yaw),forward=-X*sin(yaw)+Z*cos(yaw);
            double up=Y*cos(pitch)-forward*sin(pitch),depth=v.dist-Y*sin(pitch)-forward*cos(pitch);
            if(depth>0&&fabs(side)<=depth*.4853&&fabs(up)<=depth*.3640){visible_points++;assert(vis);}
        }
    }
    assert(visible_points>1000&&culled>1000);
    printf("Conservative cull: %d visible sample points retained; %d offscreen actors rejected\n",visible_points,culled);
    puts("City budget, stable priority, spatial collision and culling passed");return 0;
}
