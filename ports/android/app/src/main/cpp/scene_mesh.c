#include "scene_mesh.h"
#include "mesh_footprint.h"
#include "window_math.h"
#include "bigmap.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
extern void bm_hill_center(int bx,int by);

typedef struct {float x,y,z;} Point;
typedef struct {float r,g,b;} Color;
typedef struct {BoreMesh *m; int ox,oz;} Scene;
static Point P(float x,float y,float z){return (Point){x,y,z};}
static Color C(unsigned c){return (Color){((c>>16)&255)/255.f,((c>>8)&255)/255.f,(c&255)/255.f};}
static Color lit(Color c,float s){return (Color){fminf(1,c.r*s),fminf(1,c.g*s),fminf(1,c.b*s)};}
static int reserve(BoreMesh *m,int n){
    if(m->failed||n>BORE_SCENE_MAX_VERTICES-m->count){m->failed=1;return 0;}
    if(m->count+n<=m->capacity)return 1;
    int cap=m->capacity?m->capacity:4096;
    while(cap<m->count+n)cap=cap>BORE_SCENE_MAX_VERTICES/2?BORE_SCENE_MAX_VERTICES:cap*2;
    float *p=realloc(m->v,(size_t)cap*6*sizeof(float));
    if(!p){m->failed=1;return 0;}m->v=p;m->capacity=cap;return 1;
}
static void tri(Scene *s,Point a,Point b,Point c,Color k){
    if(!reserve(s->m,3))return;
    Point q[3]={a,b,c};float *p=s->m->v+s->m->count*6;
    for(int i=0;i<3;i++){
        *p++=q[i].x-s->ox;*p++=q[i].y;*p++=q[i].z-s->oz;
        *p++=k.r;*p++=k.g;*p++=k.b;
    }
    s->m->count+=3;
}
static void quad(Scene *s,Point a,Point b,Point c,Point d,Color k){tri(s,a,b,c,k);tri(s,a,c,d,k);}
static float shade(Point a,Point b,Point c){
    float x=b.x-a.x,y=b.y-a.y,z=b.z-a.z,X=c.x-a.x,Y=c.y-a.y,Z=c.z-a.z;
    float nx=y*Z-z*Y,ny=z*X-x*Z,nz=x*Y-y*X;
    if(ny<0){nx=-nx;ny=-ny;nz=-nz;}
    float len=sqrtf(nx*nx+ny*ny+nz*nz);
    return len>0?.72f+.28f*fmaxf(0,(-.42f*nx+.83f*ny-.36f*nz)/len):1;
}
static void slope(Scene*s,Point a,Point b,Point c,Point d,Color k){
    tri(s,a,b,c,lit(k,shade(a,b,c)));tri(s,a,c,d,lit(k,shade(a,c,d)));
}
// Single world-coordinate height function: all adjacent blocks use the very
// same corner values. Town lots are level pads, street strips blend them.
// This follows world.c gh_raw0 rather than inventing a second elevation map.
static float height(int ax,int az){
    int bx=ax/BM_BLK,bz=az/BM_BLK,lx=ax%BM_BLK,lz=az%BM_BLK,town=0;
    int h=bm_hill_t(bx,bz,&town);
    if(town){
        int u=MIN(lx,BM_LOT0),v=MIN(lz,BM_LOT0),n=BM_LOT0;
        int top=bm_hill(bx-1,bz-1)*(n-u)+bm_hill(bx,bz-1)*u;
        int bot=bm_hill(bx-1,bz)*(n-u)+h*u;
        return (top*(n-v)+bot*v)/(float)(n*n*TILE);
    }
    int px=lx-BM_BLK/2,pz=lz-BM_BLK/2,ex=px>=0,ez=pz>=0;
    int u=px<0?px+BM_BLK:px,v=pz<0?pz+BM_BLK:pz,n=BM_BLK;
    int top=bm_hill(bx-1+ex,bz-1+ez)*(n-u)+bm_hill(bx+ex,bz-1+ez)*u;
    int bot=bm_hill(bx-1+ex,bz+ez)*(n-u)+bm_hill(bx+ex,bz+ez)*u;
    return (top*(n-v)+bot*v)/(float)(n*n*TILE);
}
static Point land(int x,int z,float y){return P(x,height(x,z)+y,z);}
static void patch(Scene*s,int x,int z,int X,int Z,Color c,float offset){
    slope(s,land(x,z,offset),land(X,z,offset),land(X,Z,offset),land(x,Z,offset),c);
}
static void flat(Scene*s,float x,float z,float X,float Z,float y,Color c){
    quad(s,P(x,y,z),P(X,y,z),P(X,y,Z),P(x,y,Z),c);
}
static void box(Scene*s,float x,float z,float X,float Z,float base,float top,Color c){
    quad(s,P(x,base,z),P(X,base,z),P(X,top,z),P(x,top,z),lit(c,.82f));
    quad(s,P(X,base,z),P(X,base,Z),P(X,top,Z),P(X,top,z),lit(c,.67f));
    quad(s,P(X,base,Z),P(x,base,Z),P(x,top,Z),P(X,top,Z),lit(c,.92f));
    quad(s,P(x,base,Z),P(x,base,z),P(x,top,z),P(x,top,Z),lit(c,.78f));
    flat(s,x,z,X,Z,top,c);
}
static void tree(Scene*s,int x,int z,unsigned seed){
    float h=height(x,z),r=.78f+(seed&3)*.12f,base=h+.95f;
    box(s,x-.12f,z-.12f,x+.12f,z+.12f,h,base,C(0x67513e));
    Color c=C((seed&4)?0x456c45:0x527d4b);
    Point a=P(x-r,base,z-r),b=P(x+r,base,z-r),d=P(x+r,base,z+r),e=P(x-r,base,z+r),tip=P(x,base+1.8f,z);
    tri(s,a,b,tip,lit(c,.85f));tri(s,b,d,tip,lit(c,.67f));tri(s,d,e,tip,c);tri(s,e,a,tip,lit(c,.76f));
}
static Color wallcolor(const LotPlan*p,int lot,unsigned hash){
    static const unsigned houses[]={0xe6cdb0,0xceb39d,0xe4ddc9,0xd0c6ae,0xc1d1c1,0xb8c6cd,0xd6b9a7,0xe3d6bc};
    static const unsigned venues[]={0xc7b8a2,0xb5c5c4,0xd2cdbd,0xc8a99a,0xa4b4b8,0xc2ad93};
    if(lot>=LOT_HOUSE&&lot<=LOT_TWO)return C(houses[(p->pal==255?hash:p->pal*2u+p->wall)%8]);
    return C(venues[(unsigned)(lot+hash%3)%6]);
}
static float roofy(const BoreFoot*f,int x,int z,int pitch,float y){return y+(pitch<0?0:.48f*bore_roof_height(f,x,z,pitch));}
// Wall tiles have real edge positions; the common wall of a wing is omitted.
static void wall(Scene*s,const BoreFoot*f,const LotPlan*p,int ix,int iz,int x,int z,int dx,int dz,float base,int floors,int pitch,Color c,int home,int detail){
    int ax=ix+(dx>0),az=iz+(dz>0),bx=ax+(dz!=0),bz=az+(dx!=0);
    float A=x+ax,B=z+az,X=x+bx,Z=z+bz,top=base+2.f*floors;
    float f1=height(x+ax,z+az),f2=height(x+bx,z+bz);
    Color k=lit(c,dx<0?.82f:dx>0?.70f:dz<0?.86f:1.f);
    if(base>f1+.02f||base>f2+.02f)
        quad(s,P(A,fminf(f1,base),B),P(X,fminf(f2,base),Z),P(X,base,Z),P(A,base,B),lit(c,.55f));
    quad(s,P(A,base,B),P(X,base,Z),P(X,roofy(f,bx,bz,pitch,top),Z),P(A,roofy(f,ax,az,pitch,top),B),k);
    if(!detail)return;
    // Frames are raised slightly from the solid wall, never coplanar.
    float off=.027f;
    A+=dx*off;X+=dx*off;B+=dz*off;Z+=dz*off;
    int door=home&&dz&&iz==p->dy&&(ix==p->dx||(p->d2x&&ix==p->d2x));
    float a=A+(X-A)*.16f,b=B+(Z-B)*.16f,x1=A+(X-A)*.84f,z1=B+(Z-B)*.84f;
    for(int level=0;level<floors;level++){
        int isDoor=door&&level==0;
        if(!isDoor&&((ix+iz)&1))continue;
        float y=base+level*2.f,lo=y+(isDoor?.04f:.76f),hi=y+1.57f;
        Color frame=lit(C(0xf5e8d3),.85f),glass=lit(C(isDoor?0x685248:0x436777),.92f);
        quad(s,P(a,lo,b),P(x1,lo,z1),P(x1,hi,z1),P(a,hi,b),frame);
        float aa=a+(x1-a)*.09f,bb=b+(z1-b)*.09f,xx=x1-(x1-a)*.09f,zz=z1-(z1-b)*.09f;
        aa+=dx*.012f;xx+=dx*.012f;bb+=dz*.012f;zz+=dz*.012f;
        quad(s,P(aa,lo+.07f,bb),P(xx,lo+.07f,zz),P(xx,hi-.07f,zz),P(aa,hi-.07f,bb),glass);
    }
}
static void building(Scene*s,const BInfo*b,int bx,int bz,int detail){
    LotPlan p;u8 f[5];if(!bm_lot_foot(bx,bz,b,f,&p))return;
    BoreFoot foot={f[0],f[1],f[2],f[3],0,0,0,0};
    if(f[4])foot=(BoreFoot){p.x0,p.y0,p.x1,p.y1,p.wx0,p.wy0,p.wx1,p.wy1};
    int x=bx*BM_BLK,z=bz*BM_BLK;
    int floors=CLAMP(bm_storeys(b),1,6);
    int pitch=f[4]?(p.roof<=2?p.roof:2):-1;
    // Each building is planted on one continuous foundation; the visible
    // outside skirt meets terrain even along sloped or irregular lots.
    float base=height(x+BM_LOT0,z+BM_LOT0);
    for(int iz=BM_LOT0;iz<BM_BLK;iz++)for(int ix=BM_LOT0;ix<BM_BLK;ix++)if(bore_foot_has(&foot,ix,iz)){
        base=fmaxf(base,height(x+ix,z+iz));base=fmaxf(base,height(x+ix+1,z+iz));
        base=fmaxf(base,height(x+ix,z+iz+1));base=fmaxf(base,height(x+ix+1,z+iz+1));
    }
    base+=.018f;float y=base+floors*2.f;
    Color c=wallcolor(&p,b->lot,b->h),roof=C(f[4]?(p.rcol?0x685e56:0x915e49):0x788581);
    // Retain exact union for every view, including L-shaped and terrace lots.
    for(int iz=BM_LOT0;iz<BM_BLK;iz++)for(int ix=BM_LOT0;ix<BM_BLK;ix++)if(bore_foot_has(&foot,ix,iz)){
        float a=roofy(&foot,ix,iz,pitch,y),b0=roofy(&foot,ix+1,iz,pitch,y),
              d=roofy(&foot,ix,iz+1,pitch,y),e=roofy(&foot,ix+1,iz+1,pitch,y);
        Color k=lit(roof,((ix+iz)&1)?.985f:1.f);
        slope(s,P(x+ix,a,z+iz),P(x+ix+1,b0,z+iz),P(x+ix+1,e,z+iz+1),P(x+ix,d,z+iz+1),k);
        if(bore_foot_edge(&foot,ix,iz,-1,0))wall(s,&foot,&p,ix,iz,x,z,-1,0,base,floors,pitch,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,1,0))wall(s,&foot,&p,ix,iz,x,z,1,0,base,floors,pitch,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,0,-1))wall(s,&foot,&p,ix,iz,x,z,0,-1,base,floors,pitch,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,0,1))wall(s,&foot,&p,ix,iz,x,z,0,1,base,floors,pitch,c,f[4],detail);
    }
    if(f[4]&&p.face!=LF_NONE){
        int zz=p.face==LF_NORTH?p.dy: p.dy+1,edge=p.face==LF_NORTH?BM_LOT0:BM_BLK;
        int lo=MIN(zz,edge),hi=MAX(zz,edge);
        if(hi>lo)patch(s,x+p.dx,z+lo,x+p.dx+1,z+hi,C(0xc4b9a6),.024f);
    }
}
static void block(Scene*s,int bx,int bz,int detail){
    BInfo b;bm_block(bx,bz,&b);int x=bx*BM_BLK,z=bz*BM_BLK;
    static const unsigned terrain[]={0x548a9d,0xdac5a0,0x7e9b68,0x537749,0xb8a078,0x8f958a};
    Color ground=C(terrain[MIN(b.terrain,5)]);
    int river=b.lot==LOT_RIVER,rd=b.road&3;
    if(!river&&(b.road&BR_MEGA)){if(BR_PART(b.road)&1)rd&=~2;if(BR_PART(b.road)&2)rd&=~1;}
    // Roads, shoulders and lot pads share the mesh edges (no painted overlay
    // fighting the terrain). 1..4 is the carriageway, 0..1 and 4..5 sidewalk.
    const int k[]={0,1,4,5,9,18};
    for(int j=0;j<5;j++)for(int i=0;i<5;i++){
        int street=((rd&1)&&j<3)||((rd&2)&&i<3),road=((rd&1)&&j==1)||((rd&2)&&i==1);
        Color c=street?C(road?0x555d61:0xb6b9af):ground;
        patch(s,x+k[i],z+k[j],x+k[i+1],z+k[j+1],c,0);
    }
    if(b.terrain==BT_SEA)return;
    if(river){
        if(b.road&BR_MEGA)patch(s,x+5,z,x+13,z+18,C(0x5b91a2),.012f);
        else patch(s,x,z+5,x+18,z+13,C(0x5b91a2),.012f);
        if((b.road&BR_MEGA)?(rd&1):(rd&2)){
            if(b.road&BR_MEGA)patch(s,x+5,z+1,x+13,z+4,C(0x5b6062),.06f);
            else patch(s,x+1,z+5,x+4,z+13,C(0x5b6062),.06f);
        }return;
    }
    building(s,&b,bx,bz,detail);
    if(b.lot==LOT_POOL){patch(s,x+7,z+7,x+16,z+15,C(0xc6c8bb),.018f);patch(s,x+8,z+8,x+15,z+14,C(0x66b6c1),.035f);}
    else if(b.lot==LOT_SQUARE||b.lot==LOT_SKATE)patch(s,x+5,z+5,x+17,z+17,C(0xb4b6ae),.018f);
    else if(b.lot==LOT_FARM){
        for(int i=0;i<3;i++)patch(s,x+6+i*2,z+12,x+7+i*2,z+17,C((i&1)?0x9a7751:0x718a49),.02f);
    }
    else if(b.lot==LOT_PARK||b.terrain==BT_FOREST&&b.lot==LOT_WILD){
        if(b.lot==LOT_PARK)patch(s,x+5,z+10,x+17,z+11,C(0xc4bda7),.018f);
        tree(s,x+7,z+7,b.h);tree(s,x+15,z+15,b.h>>3);
        if(detail)tree(s,x+15,z+7,b.h>>7);
    }
}
int bore_scene_build(int cx,int cy,int r,BoreMesh *out){
    if(!out)return 0;
    memset(out,0,sizeof *out);
    if(r<1||r>BORE_SCENE_RADIUS)return 0;
    cx=bore_window_center(cx,r,BM_N);cy=bore_window_center(cy,r,BM_N);
    if(cx<0||cy<0)return 0;
    bm_hill_center(cx,cy);
    Scene s={out,cx*BM_BLK+BM_BLK/2,cy*BM_BLK+BM_BLK/2};
    for(int z=-r;z<=r;z++)for(int x=-r;x<=r;x++)
        block(&s,cx+x,cy+z,ABS(x)<=2&&ABS(z)<=2);
    if(out->failed||!out->count){bore_scene_free(out);return 0;}
    return 1;
}
void bore_scene_free(BoreMesh*m){if(m){free(m->v);memset(m,0,sizeof *m);}}
