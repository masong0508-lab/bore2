#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bigmap.h"
#include "outings.h"
#include "../app/src/main/cpp/world_bridge.h"
#include "../app/src/main/cpp/window_math.h"
#include "../app/src/main/cpp/mesh_footprint.h"
#include "../app/src/main/cpp/venue_footprint.h"
#include "../app/src/main/cpp/scene_surface.h"

int main(void){
    uint32_t center=bore_android_init(1234u),a[17*17],b[17*17];
    int x=(int)(center&65535),y=(int)(center>>16);
    x=bore_window_center(x,8,BM_N);y=bore_window_center(y,8,BM_N);
    assert(bore_android_window(x,y,8,a,289)==289);
    assert(bore_android_window(x,y,8,b,288)==0);
    assert(!bore_android_window(x,y,17,b,289));
    for(int iy=0;iy<17;iy++)for(int ix=0;ix<17;ix++){
        BInfo q;bm_block(x-8+ix,y-8+iy,&q);
        unsigned p=bore_window_pack(q.terrain,q.lot,q.road,bm_storeys(&q),q.set);
        assert(a[iy*17+ix]==p);
    }
    assert(bore_android_init(1234u)==center);
    assert(bore_android_window(x,y,8,b,289)==289&&!memcmp(a,b,sizeof a));
    int found=0;
    for(int iy=y-16;iy<=y+16&&!found;iy++)for(int ix=x-16;ix<=x+16&&!found;ix++){
        if(ix<0||iy<0||ix>=BM_N||iy>=BM_N)continue;
        BInfo q;bm_block(ix,iy,&q);int g=outing_group(q.lot,0);
        if(g<0)continue;
        char weekday[256],weekend[256];
        assert(bore_android_describe(ix,iy,0,weekday,sizeof weekday)>0);
        assert(bore_android_describe(ix,iy,5,weekend,sizeof weekend)>0);
        assert(strstr(weekday,outing_get(g*2)->title));
        assert(strstr(weekend,outing_get(24+g*2)->title));
        assert(strcmp(weekday,weekend));
        int v[BORE_ANDROID_ACTIVITY_FIELDS];
        const Outing *o=outing_get(g*2);
        assert(bore_android_can_visit(ix,iy));
        assert(bore_android_activity(ix,iy,0,0,o->from*60,100,100,v));
        assert(v[0]==OUT_OK&&v[1]==g*2&&v[2]==o->fee&&v[3]==o->energy&&v[9]==o->gain&&v[10]>0);
        assert(!strcmp(bore_android_activity_text(v[1],0),o->title));
        assert(bore_android_activity(ix,iy,0,0,o->to*60,100,100,v)&&v[0]==OUT_CLOSED);
        assert(bore_android_activity(ix,iy,0,0,o->from*60,100,0,v)&&v[0]==OUT_TIRED);
        if(o->fee)assert(bore_android_activity(ix,iy,0,0,o->from*60,o->fee-1,100,v)&&v[0]==OUT_MONEY);
        assert(bore_android_activity(ix,iy,5,0,600,100,100,v)&&v[1]==24+g*2);
        assert(!bore_android_activity(ix,iy,0,2,600,100,100,v));found=1;
    }
    assert(found);
    uint32_t route[BORE_ANDROID_ROUTE_MAX];
    assert(bore_android_route(x,y,x,y,route,BORE_ANDROID_ROUTE_MAX)==0);
    int routes=0,shifts=0;
    for(int iy=y-6;iy<=y+6;iy++)for(int ix=x-6;ix<=x+6;ix++){
        if(!bore_android_can_visit(ix,iy))continue;
        int len=bore_android_route(x,y,ix,iy,route,BORE_ANDROID_ROUTE_MAX);
        if(len>0){
            int px=x,py=y;
            for(int k=0;k<len;k++){
                int nx=route[k]&65535,ny=route[k]>>16;
                int dx=nx-px,dy=ny-py;assert(abs(dx)+abs(dy)==1);
                BInfo q;bm_block(nx,ny,&q);
                if(q.terrain==BT_SEA)assert(dx?(q.road&1):(q.road&2));
                if(q.lot==LOT_RIVER)assert((q.road&BR_MEGA)?(dx&&(q.road&1)):(dy&&(q.road&2)));
                px=nx;py=ny;
            }
            assert(px==ix&&py==iy);routes++;
        }
        int v[BORE_ANDROID_SHIFT_FIELDS];
        if(bore_android_shift(ix,iy,0,600,100,100,v)){
            assert(v[0]==SHIFT_OK&&v[1]>0&&v[2]>0);
            assert(bore_android_shift(ix,iy,0,600,0,100,v)&&v[0]==SHIFT_TIRED);
            assert(bore_android_shift(ix,iy,0,600,100,0,v)&&v[0]==SHIFT_HUNGRY);
            assert(bore_android_shift(ix,iy,0,0,100,100,v)&&v[0]==SHIFT_CLOSED);shifts++;
        }
    }
    assert(routes>0&&shifts>0);
    assert(bore_android_route(x,y,-1,y,route,BORE_ANDROID_ROUTE_MAX)<0);
    int invalid[BORE_ANDROID_ACTIVITY_FIELDS];
    assert(!bore_android_can_visit(-1,0)&&!bore_android_activity(-1,0,0,0,600,100,100,invalid));
    assert(bore_android_window(-100,9999,8,a,289)==289);
    assert(!bore_android_describe(-1,0,0,(char*)b,sizeof b));
    int homes=0,not_homes=0,room[BORE_ANDROID_INTERIOR_FIELDS];
    for(int seed=0;seed<3;seed++){
        uint32_t c=bore_android_init((uint32_t)(1234+seed*313));
        int cx=(int)(c&65535),cy=(int)(c>>16);
        for(int y=cy-8;y<=cy+8;y++)for(int x=cx-8;x<=cx+8;x++){
            if(x<0||y<0||x>=BM_N||y>=BM_N)continue;
            BInfo b;bm_block(x,y,&b);
            int yes=bore_android_interior(x,y,room,BORE_ANDROID_INTERIOR_FIELDS);
            if(b.lot<LOT_HOUSE||b.lot>LOT_APART||b.terrain==BT_SEA){assert(!yes);not_homes++;continue;}
            u8 f[5];LotPlan p;memset(&p,0,sizeof p);
            if(!bm_lot_foot(x,y,&b,f,&p)){assert(!yes);continue;}
            assert(yes&&room[0]==b.lot);
            BoreFoot foot={f[0],f[1],f[2],f[3],0,0,0,0};
            if(f[4])foot=(BoreFoot){p.x0,p.y0,p.x1,p.y1,p.wx0,p.wy0,p.wx1,p.wy1};
            assert(room[1]>=0&&room[1]<18&&room[2]>=0&&room[2]<18);
            assert(bore_foot_has(&foot,room[1],room[2]));
            for(int y=0;y<18;y++)for(int x=0;x<18;x++)assert(room[3+y*18+x]==bore_foot_has(&foot,x,y));
            homes++;
        }
    }
    assert(homes>20&&not_homes>20);
    int venues=0;
    for(int seed=0;seed<3;seed++){
        uint32_t c=bore_android_init((uint32_t)(1234+seed*313));
        int cx=(int)(c&65535),cy=(int)(c>>16);
        for(int y=cy-8;y<=cy+8;y++)for(int x=cx-8;x<=cx+8;x++){
            if(x<0||y<0||x>=BM_N||y>=BM_N)continue;
            BInfo b;bm_block(x,y,&b);u8 f[5];LotPlan p={0};
            int has=b.terrain!=BT_SEA&&bore_venue_supported(b.lot)&&bm_lot_foot(x,y,&b,f,&p);
            int yes=bore_android_place_interior(x,y,room,BORE_ANDROID_INTERIOR_FIELDS);
            assert(yes==has);if(!yes)continue;
            BoreFoot foot={f[0],f[1],f[2],f[3],0,0,0,0};
            int ex=-1,ey=-1,front=bore_venue_entry(&foot,p.face,&ex,&ey);
            assert(front&&room[0]==b.lot&&room[1]==ex&&room[2]==ey);
            assert(!bore_foot_has(&foot,ex,ey+front));
            for(int j=0;j<18;j++)for(int i=0;i<18;i++)assert(room[3+j*18+i]==bore_foot_has(&foot,i,j));
            venues++;
        }
    }
    assert(venues>20);assert(!bore_android_place_interior(-1,0,room,BORE_ANDROID_INTERIOR_FIELDS));
    int tall=0;
    for(int seed=0;seed<3;seed++){
        uint32_t c=bore_android_init((uint32_t)(1234+seed*313));
        int cx=(int)(c&65535),cy=(int)(c>>16);
        for(int y=cy-8;y<=cy+8;y++)for(int x=cx-8;x<=cx+8;x++){
            if(x<0||y<0||x>=BM_N||y>=BM_N)continue;
            BInfo b;LotPlan p;u8 f[5];bm_block(x,y,&b);
            int expected=0;
            if(b.terrain!=BT_SEA&&((b.lot>=LOT_HOUSE&&b.lot<=LOT_APART)||bore_venue_supported(b.lot))&&bm_lot_foot(x,y,&b,f,&p))
                expected=CLAMP(bm_storeys(&b),1,6);
            assert(bore_android_storeys(x,y)==expected);
            if(expected>1)tall++;
        }
    }
    assert(tall>0);assert(bore_android_storeys(-1,0)==0);printf("Android native storey count: %d multi-storey lots PASS\n",tall);
    printf("Android commercial footprint/door alignment: %d venues PASS\\n",venues);
    int mask[BORE_ANDROID_WALK_FIELDS];
    uint32_t cap=bore_android_init(1234u);
    int wx=(int)(cap&65535),wy=(int)(cap>>16);
    wx=MAX(1,MIN(BM_N-2,wx));wy=MAX(1,MIN(BM_N-2,wy));
    assert(bore_android_walk_mask(wx,wy,mask,BORE_ANDROID_WALK_FIELDS));
    int walk_open=0,walk_blocked=0;
    for(int by=0;by<3;by++)for(int bx=0;bx<3;bx++){
        BInfo b;LotPlan p={0};u8 f[5];bm_block(wx+bx-1,wy+by-1,&b);
        int built=b.terrain!=BT_SEA&&b.lot!=LOT_RIVER&&bm_lot_foot(wx+bx-1,wy+by-1,&b,f,&p);
        BoreFoot foot={0};
        if(built){foot=(BoreFoot){f[0],f[1],f[2],f[3],0,0,0,0};if(f[4])foot=(BoreFoot){p.x0,p.y0,p.x1,p.y1,p.wx0,p.wy0,p.wx1,p.wy1};}
        for(int y=0;y<18;y++)for(int x=0;x<18;x++){
            int open=mask[(by*18+y)*54+bx*18+x];assert(open==0||open==1);
            if(built&&bore_foot_has(&foot,x,y))assert(!open);
            if(bore_surface(&b,x,y)==BS_WATER)assert(!open);
            if(open)walk_open++;else walk_blocked++;
        }
    }
    assert(walk_open>0&&walk_blocked>0);
    /* Keep mask ABI unchanged; heights are a separate native tile grid. */
    assert(bore_android_walk_heights(wx,wy,mask,BORE_ANDROID_WALK_FIELDS));
    for(int j=0;j<54;j++)for(int i=0;i<54;i++){
        int ax=(wx-1)*BM_BLK+i,ay=(wy-1)*BM_BLK+j;
        int a=bm_ground_corner(ax,ay),b=bm_ground_corner(ax+1,ay+1);
        assert(mask[j*54+i]==(a+b+1)/2);
    }
    assert(!bore_android_walk_heights(0,0,mask,BORE_ANDROID_WALK_FIELDS));
    assert(!bore_android_walk_heights(wx,wy,mask,BORE_ANDROID_WALK_FIELDS-1));
    assert(!bore_android_walk_mask(0,0,mask,BORE_ANDROID_WALK_FIELDS));
    assert(!bore_android_walk_mask(wx,wy,mask,BORE_ANDROID_WALK_FIELDS-1));
    puts("Android outdoor collision mask: native buildings, water and bounded window PASS");
    assert(!bore_android_interior(-1,0,room,BORE_ANDROID_INTERIOR_FIELDS));
    assert(!bore_android_interior(x,y,room,BORE_ANDROID_INTERIOR_FIELDS-1));
    printf("Android bridge: %d generated residential footprints and %d nonhomes checked\\n",homes,not_homes);
    puts("Android bridge: actual v8 map, stable seed and native weekday/weekend venues");
    /* Exercise real generated terrain as well as the portable synthetic test. */
    int pads=0,hillx=-1,hilly=-1;
    for(int j=-8;j<=8;j++)for(int i=-8;i<=8;i++){
        int bx=x+i,by=y+j,flag=0;
        int h=bm_hill_t(bx,by,&flag);
        if(flag){
            pads++;
            for(int v=BM_LOT0;v<BM_BLK;v++)for(int u=BM_LOT0;u<BM_BLK;u++)
                assert(bm_ground_corner(bx*BM_BLK+u,by*BM_BLK+v)==h);
        }
        BoreLandPatch p,q,r;
        bm_land_patch(bx,by,&p);bm_land_patch(bx+1,by,&q);bm_land_patch(bx,by+1,&r);
        for(int v=0;v<=BM_BLK*2;v++){
            assert(bore_land_sample2(&p,BM_BLK*2,v)==bore_land_sample2(&q,0,v));
            assert(bore_land_sample2(&p,v,BM_BLK*2)==bore_land_sample2(&r,v,0));
        }
        if(hillx<0&&h>64&&h<3000){hillx=bx;hilly=by;}
    }
    assert(pads>0&&hillx>=0);
    bm_ovr_clear();
    int old=bm_hill(hillx,hilly);
    assert(bm_ovr_sculpt(hillx,hilly,1,1));
    assert(bm_ovr_delta(hillx,hilly)==2&&bm_ovr_delta(hillx+1,hilly)==1);
    assert(bm_hill(hillx,hilly)>old);
    assert(bm_ovr_sculpt(hillx,hilly,1,-1));
    assert(bm_hill(hillx,hilly)==old&&bm_ovr_delta(hillx,hilly)==0);
    bm_ovr_clear();
    puts("Generated terrain foundations, shared seams and reversible sculpt: PASS");
    return 0;
}
