package com.bore2.game;

/** Tiny depth-tested resident model, in the same tile units as the neighborhood. */
final class ActorMesh {
    static float ground(float[] mesh,float x,float z){
        float low=Float.POSITIVE_INFINITY;
        if(mesh==null)return low;
        for(int i=0;i+17<mesh.length;i+=18){
            float ax=mesh[i],az=mesh[i+2],bx=mesh[i+6],bz=mesh[i+8],cx=mesh[i+12],cz=mesh[i+14];
            if(x<Math.min(ax,Math.min(bx,cx))||x>Math.max(ax,Math.max(bx,cx))||z<Math.min(az,Math.min(bz,cz))||z>Math.max(az,Math.max(bz,cz)))continue;
            float ux=bx-ax,uz=bz-az,vx=cx-ax,vz=cz-az,det=ux*vz-uz*vx;
            if(Math.abs(det)<1e-6f)continue;
            float u=((x-ax)*vz-(z-az)*vx)/det,v=(ux*(z-az)-uz*(x-ax))/det;
            if(u<-.0001f||v<-.0001f||u+v>1.0001f)continue;
            float h=mesh[i+1]+u*(mesh[i+7]-mesh[i+1])+v*(mesh[i+13]-mesh[i+1]);
            if(Float.isFinite(h))low=Math.min(low,h);
        }
        return low;
    }
    static float[] create(){
        // 7 cuboids, all sides, two triangles per face, 6 floats/vertex.
        float[] out=new float[7*36*6];int i=0;
        i=box(out,i,-.22f,0,-.12f,-.03f,.72f,.12f,0x293949);
        i=box(out,i,.03f,0,-.12f,.22f,.72f,.12f,0x293949);
        i=box(out,i,-.26f,.70f,-.15f,.26f,1.36f,.15f,0x3c9b89);
        i=box(out,i,-.39f,.73f,-.10f,-.27f,1.33f,.10f,0xb98560);
        i=box(out,i,.27f,.73f,-.10f,.39f,1.33f,.10f,0xb98560);
        i=box(out,i,-.19f,1.36f,-.18f,.19f,1.74f,.18f,0xd9a67c);
        box(out,i,-.20f,1.68f,-.19f,.20f,1.82f,.19f,0x46332b);
        return out;
    }
    private static int box(float[] o,int at,float x,float y,float z,float X,float Y,float Z,int color){
        float[][] v={{x,y,z},{X,y,z},{X,y,Z},{x,y,Z},{x,Y,z},{X,Y,z},{X,Y,Z},{x,Y,Z}};
        int[][] faces={{0,1,2,3},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
        float[] shade={.62f,1f,.84f,.71f,.92f,.76f};int[] order={0,1,2,0,2,3};
        for(int f=0;f<6;f++)for(int k:order){float[] p=v[faces[f][k]];o[at++]=p[0];o[at++]=p[1];o[at++]=p[2];o[at++]=((color>>16)&255)/255f*shade[f];o[at++]=((color>>8)&255)/255f*shade[f];o[at++]=(color&255)/255f*shade[f];}
        return at;
    }
    private ActorMesh(){}
}
