#include "scene_mesh.h"
#include "mesh_footprint.h"
#include "scene_surface.h"
#include "window_math.h"
#include "bigmap.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
extern void bm_hill_center(int bx,int by);

typedef struct {float x,y,z;} Point;
typedef struct {float r,g,b;} Color;
#define HILL_SIDE (2*BORE_SCENE_RADIUS+4)
typedef struct {
    BoreMesh *m; int ox,oz,bx,bz,hx,hz,hn;
    float ground[BM_BLK+1][BM_BLK+1];
    // The world generator's block-center samples already encode a town
    // pad flag. Cache the small halo once for all interpolated corners.
    u16 hills[HILL_SIDE][HILL_SIDE];
} Scene;
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
static void scene_hills(Scene*s,int cx,int cy,int radius){
    s->hx=cx-radius-1;s->hz=cy-radius-1;s->hn=2*radius+4;
    for(int z=0;z<s->hn;z++)for(int x=0;x<s->hn;x++){
        int town=0,h=bm_hill_t(s->hx+x,s->hz+z,&town);
        s->hills[z][x]=(u16)(h|(town?0x8000:0));s->m->hill_reads++;
    }
}
static int scene_hill(Scene*s,int bx,int bz,int*town){
    int x=bx-s->hx,z=bz-s->hz;
    if((unsigned)x>=(unsigned)s->hn||(unsigned)z>=(unsigned)s->hn){
        s->m->hill_reads++;return bm_hill_t(bx,bz,town);
    }
    u16 packed=s->hills[z][x];int h=packed&0x7fff;
    if(town)*town=packed>>15;
#ifdef BORE_MESH_VERIFY_HILL_CACHE
    // Host-only equivalence assertion; production avoids the extra call.
    int check_town=0,check_h=bm_hill_t(bx,bz,&check_town);
    if(check_h!=h||check_town!=(packed>>15))abort();
#endif
    return h;
}
// Single world-coordinate height function: all adjacent blocks use the very
// same corner values. Town lots are level pads, street strips blend them.
// This follows world.c gh_raw0 rather than inventing a second elevation map.
static float height_raw(Scene*s,int ax,int az){
    s->m->height_samples++;
    int bx=ax/BM_BLK,bz=az/BM_BLK,lx=ax%BM_BLK,lz=az%BM_BLK,town=0;
    int h=scene_hill(s,bx,bz,&town);
    if(town){
        int u=MIN(lx,BM_LOT0),v=MIN(lz,BM_LOT0),n=BM_LOT0;
        int top=scene_hill(s,bx-1,bz-1,0)*(n-u)+scene_hill(s,bx,bz-1,0)*u;
        int bot=scene_hill(s,bx-1,bz,0)*(n-u)+h*u;
        return (top*(n-v)+bot*v)/(float)(n*n*TILE);
    }
    int px=lx-BM_BLK/2,pz=lz-BM_BLK/2,ex=px>=0,ez=pz>=0;
    int u=px<0?px+BM_BLK:px,v=pz<0?pz+BM_BLK:pz,n=BM_BLK;
    int top=scene_hill(s,bx-1+ex,bz-1+ez,0)*(n-u)+scene_hill(s,bx+ex,bz-1+ez,0)*u;
    int bot=scene_hill(s,bx-1+ex,bz+ez,0)*(n-u)+scene_hill(s,bx+ex,bz+ez,0)*u;
    return (top*(n-v)+bot*v)/(float)(n*n*TILE);
}
static float height_at(Scene*s,int x,int z){
    int dx=x-s->bx,dz=z-s->bz;
    if((unsigned)dx<=BM_BLK&&(unsigned)dz<=BM_BLK){
        s->m->height_hits++;return s->ground[dz][dx];
    }
    return height_raw(s,x,z);
}
static Point land(Scene*s,int x,int z,float y){return P(x,height_at(s,x,z)+y,z);}
static void patch(Scene*s,int x,int z,int X,int Z,Color c,float offset){
    // Overlay follows the same tile triangulation as terrain. No large
    // diagonal surfaces that cut across the slopes under paths/pools.
    for(int zz=z;zz<Z;zz++)for(int xx=x;xx<X;xx++)
        slope(s,land(s,xx,zz,offset),land(s,xx+1,zz,offset),land(s,xx+1,zz+1,offset),land(s,xx,zz+1,offset),c);
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
    float h=height_at(s,x,z),r=.78f+(seed&3)*.12f,base=h+.95f;
    box(s,x-.12f,z-.12f,x+.12f,z+.12f,h,base,C(0x67513e));
    Color c=C((seed&4)?0x456c45:0x527d4b);
    Point a=P(x-r,base,z-r),b=P(x+r,base,z-r),d=P(x+r,base,z+r),e=P(x-r,base,z+r),tip=P(x,base+1.8f,z);
    tri(s,a,b,tip,lit(c,.85f));tri(s,b,d,tip,lit(c,.67f));tri(s,d,e,tip,c);tri(s,e,a,tip,lit(c,.76f));
}
static Color wallcolor(const LotPlan*p,int lot,unsigned hash){
    // The same six planned material averages as DS PWALL15/world.c.
    static const unsigned houses[]={0xa55a42,0xd6d6b5,0xd6b584,0x8c6b4a,0xcec6b5,0x736b63};
    static const unsigned other[]={0xe2d6be,0xb0604c,0xeceae2,0xd6be96,0xa0a0a4,0xb4c8d6,0xbaceaa,0xe2b4b4,0x788ca0};
    if(lot>=LOT_HOUSE&&lot<=LOT_TWO&&p->pal!=255)
        return C(houses[(p->pal*2u+p->wall)%6]);
    return C(other[lot==LOT_TOWER?8:(hash>>20)%8u]);
}
static float roofy(const BoreFoot*f,int x,int z,int pitch,float y){
    if(pitch<0)return y;
    return y+((pitch&4)?250.f:170.f)/256.f*bore_roof_height(f,x,z,pitch&3);
}
// Wall tiles have real edge positions; the common wall of a wing is omitted.
static void wall(Scene*s,const float roof_height[BM_BLK+1][BM_BLK+1],const LotPlan*p,int ix,int iz,int x,int z,int dx,int dz,float base,int floors,Color c,int home,int detail){
    int ax=ix+(dx>0),az=iz+(dz>0),bx=ax+(dz!=0),bz=az+(dx!=0);
    float A=x+ax,B=z+az,X=x+bx,Z=z+bz;
    float f1=height_at(s,x+ax,z+az),f2=height_at(s,x+bx,z+bz);
    Color k=lit(c,dx<0?.82f:dx>0?.70f:dz<0?.86f:1.f);
    if(base>f1+.02f||base>f2+.02f)
        quad(s,P(A,fminf(f1,base),B),P(X,fminf(f2,base),Z),P(X,base,Z),P(A,base,B),lit(c,.55f));
    quad(s,P(A,base,B),P(X,base,Z),P(X,roof_height[bz][bx],Z),P(A,roof_height[az][ax],B),k);
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
    int pitch=-1;
    if(f[4]){
        int width=foot.x1-foot.x0+1,depth=foot.z1-foot.z0+1;
        pitch=p.roof==RF_HIP||p.type==HT_MANSION?2:
            (p.roof==RF_FRONT&&(p.type==HT_COMPACT||p.type==HT_BOX||p.type==HT_TWO))?1:width>=depth?0:1;
        if(p.type>HT_RANCH)pitch|=4;
    }
    // Each building is planted on one continuous foundation; the visible
    // outside skirt meets terrain even along sloped or irregular lots.
    float base=height_at(s,x+BM_LOT0,z+BM_LOT0);
    for(int iz=BM_LOT0;iz<BM_BLK;iz++)for(int ix=BM_LOT0;ix<BM_BLK;ix++)if(bore_foot_has(&foot,ix,iz)){
        base=fmaxf(base,height_at(s,x+ix,z+iz));base=fmaxf(base,height_at(s,x+ix+1,z+iz));
        base=fmaxf(base,height_at(s,x+ix,z+iz+1));base=fmaxf(base,height_at(s,x+ix+1,z+iz+1));
    }
    base+=.018f;float y=base+floors*2.f;
    static const unsigned roofs[]={0xc45c40,0x687080,0x966842,0x5c8454,0x46568c,0xceb480};
    Color c=wallcolor(&p,b->lot,b->h),roof=C(f[4]?roofs[(p.pal+p.rcol*3u)%6]:0x788581);
    // Each roof corner is evaluated once even when four roof tiles and
    // an exterior wall meet there. Preserve the exact old geometry.
    float rh[BM_BLK+1][BM_BLK+1]={{0}};
    for(int iz=BM_LOT0;iz<=BM_BLK;iz++)for(int ix=BM_LOT0;ix<=BM_BLK;ix++)
        if(bore_foot_has(&foot,ix,iz)||bore_foot_has(&foot,ix-1,iz)||
           bore_foot_has(&foot,ix,iz-1)||bore_foot_has(&foot,ix-1,iz-1)){
            rh[iz][ix]=roofy(&foot,ix,iz,pitch,y);s->m->roof_samples++;
        }
    // Retain exact union for every view, including L-shaped and terrace lots.
    for(int iz=BM_LOT0;iz<BM_BLK;iz++)for(int ix=BM_LOT0;ix<BM_BLK;ix++)if(bore_foot_has(&foot,ix,iz)){
        float a=rh[iz][ix],b0=rh[iz][ix+1],d=rh[iz+1][ix],e=rh[iz+1][ix+1];
        Color k=lit(roof,((ix+iz)&1)?.985f:1.f);
        slope(s,P(x+ix,a,z+iz),P(x+ix+1,b0,z+iz),P(x+ix+1,e,z+iz+1),P(x+ix,d,z+iz+1),k);
        if(bore_foot_edge(&foot,ix,iz,-1,0))wall(s,rh,&p,ix,iz,x,z,-1,0,base,floors,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,1,0))wall(s,rh,&p,ix,iz,x,z,1,0,base,floors,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,0,-1))wall(s,rh,&p,ix,iz,x,z,0,-1,base,floors,c,f[4],detail);
        if(bore_foot_edge(&foot,ix,iz,0,1))wall(s,rh,&p,ix,iz,x,z,0,1,base,floors,c,f[4],detail);
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
    // A single stitched mesh, including roads and water. Merge only flat
    // rectangles of identical material; all nonflat cells retain their
    // exact corner heights, so building foundations sit on the actual mesh.
    s->bx=x;s->bz=z;
    float (*h)[BM_BLK+1]=s->ground;
    unsigned char mat[BM_BLK][BM_BLK],done[BM_BLK][BM_BLK]={{0}};
    for(int j=0;j<=BM_BLK;j++)for(int i=0;i<=BM_BLK;i++)h[j][i]=height_raw(s,x+i,z+j);
    for(int j=0;j<BM_BLK;j++)for(int i=0;i<BM_BLK;i++)mat[j][i]=(unsigned char)bore_surface(&b,i,j);
    const Color surface[]={ground,C(0x555d61),C(0xb6b9af),C(0x5b91a2)};
    for(int j=0;j<BM_BLK;j++)for(int i=0;i<BM_BLK;i++){
        if(done[j][i])continue;
        int w=1,d=1;float y=h[j][i];unsigned m=mat[j][i];
        #define FLATCELL(I,J) (!done[J][I]&&mat[J][I]==m&&h[J][I]==y&&h[J+1][I]==y&&h[J][I+1]==y&&h[J+1][I+1]==y)
        if(FLATCELL(i,j)){
            while(i+w<BM_BLK&&FLATCELL(i+w,j))w++;
            while(j+d<BM_BLK){
                int k=0;while(k<w&&FLATCELL(i+k,j+d))k++;
                if(k<w)break;d++;
            }
        }
        #undef FLATCELL
        for(int v=0;v<d;v++)for(int u=0;u<w;u++)done[j+v][i+u]=1;
        slope(s,P(x+i,h[j][i],z+j),P(x+i+w,h[j][i+w],z+j),
              P(x+i+w,h[j+d][i+w],z+j+d),P(x+i,h[j+d][i],z+j+d),surface[m]);
    }
    if(b.terrain==BT_SEA||b.lot==LOT_RIVER)return;
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
    Scene s={.m=out,.ox=cx*BM_BLK+BM_BLK/2,.oz=cy*BM_BLK+BM_BLK/2};
    scene_hills(&s,cx,cy,r);
    for(int z=-r;z<=r;z++)for(int x=-r;x<=r;x++)
        block(&s,cx+x,cy+z,ABS(x)<=2&&ABS(z)<=2);
    if(out->failed||!out->count){bore_scene_free(out);return 0;}
    return 1;
}
void bore_scene_free(BoreMesh*m){if(m){free(m->v);memset(m,0,sizeof *m);}}
