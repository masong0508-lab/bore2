#pragma once
// All plan endpoints are inclusive wall tiles. Mesh corners use exclusive edges.
typedef struct { int x0,z0,x1,z1,wx0,wz0,wx1,wz1; } BoreFoot;
static inline int bore_foot_has(const BoreFoot *f,int x,int z){
    return (x>=f->x0&&x<=f->x1&&z>=f->z0&&z<=f->z1)||
           (f->wx1>0&&x>=f->wx0&&x<=f->wx1&&z>=f->wz0&&z<=f->wz1);
}
static inline int bore_foot_edge(const BoreFoot *f,int x,int z,int dx,int dz){
    return bore_foot_has(f,x,z)&&!bore_foot_has(f,x+dx,z+dz);
}
static inline int bore_min(int a,int b){return a<b?a:b;}
static inline int bore_roof_height(const BoreFoot *f,int x,int z,int type){
    int n=0,s=0,w=0,e=0;
    // Distance from a corner to the eaves along each axis of the footprint union.
    while(n<18&&(bore_foot_has(f,x-1,z-n-1)||bore_foot_has(f,x,z-n-1)))n++;
    while(s<18&&(bore_foot_has(f,x-1,z+s)||bore_foot_has(f,x,z+s)))s++;
    while(w<18&&(bore_foot_has(f,x-w-1,z-1)||bore_foot_has(f,x-w-1,z)))w++;
    while(e<18&&(bore_foot_has(f,x+e,z-1)||bore_foot_has(f,x+e,z)))e++;
    int dz=bore_min(n,s),dx=bore_min(w,e);
    return bore_min(3,type==0?dz:type==1?dx:bore_min(dx,dz));
}
