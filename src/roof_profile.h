#pragma once
// Rectangular roof profile in caller's units. Match the detailed renderer's
// three-tile pitch cap; the resulting inner ring can collapse to a ridge.
typedef struct { int x,z,run; } BoreRoofRun;
static inline BoreRoofRun bore_roof_run(int w,int d,int style,int cap){
    BoreRoofRun r={0,0,0};
    if(w<=0||d<=0||cap<=0||style<1||style>3)return r;
    int along_x=style==2&&w>=d;
    int short_side=style==3?w:w<d?w:d;
    r.run=short_side/2;
    if(r.run>cap)r.run=cap;
    r.x=(style==1||!along_x)?r.run:0;
    r.z=(style==1||along_x)?r.run:0;
    return r;
}
