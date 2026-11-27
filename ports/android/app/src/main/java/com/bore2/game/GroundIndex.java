package com.bore2.game;

/** Exact x/z triangle bins for terrain-height probes; bounded auxiliary RAM. */
final class GroundIndex {
    private static final int SIDE=9,MAX_REFS=1_000_000;
    private final float[] mesh;
    private final int[][] bins;
    private GroundIndex(float[] mesh,int[][] bins){this.mesh=mesh;this.bins=bins;}
    private static int bin(float x){return (int)Math.floor((x+81f)/18f);}
    private static int clamp(int x){return Math.max(0,Math.min(SIDE-1,x));}
    static GroundIndex build(float[] mesh){
        if(mesh==null||mesh.length%18!=0)return null;
        try{
            int[] counts=new int[SIDE*SIDE],pos=new int[SIDE*SIDE];int[][] bins=new int[SIDE*SIDE][];
            long total=0;
            for(int pass=0;pass<2;pass++){
                for(int i=0;i<mesh.length;i+=18){
                    float ax=mesh[i],az=mesh[i+2],bx=mesh[i+6],bz=mesh[i+8],cx=mesh[i+12],cz=mesh[i+14];
                    float minX=Math.min(ax,Math.min(bx,cx)),maxX=Math.max(ax,Math.max(bx,cx)),minZ=Math.min(az,Math.min(bz,cz)),maxZ=Math.max(az,Math.max(bz,cz));
                    if(!Float.isFinite(minX)||!Float.isFinite(maxX)||!Float.isFinite(minZ)||!Float.isFinite(maxZ)||maxX< -81||minX>81||maxZ< -81||minZ>81)continue;
                    float det=(bx-ax)*(cz-az)-(bz-az)*(cx-ax);if(Math.abs(det)<1e-6f)continue;
                    for(int z=clamp(bin(minZ));z<=clamp(bin(maxZ));z++)for(int x=clamp(bin(minX));x<=clamp(bin(maxX));x++){
                        int k=z*SIDE+x;if(pass==0){counts[k]++;if(++total>MAX_REFS)return null;}else bins[k][pos[k]++]=i;
                    }
                }
                if(pass==0)for(int k=0;k<bins.length;k++)bins[k]=new int[counts[k]];
            }
            return new GroundIndex(mesh,bins);
        }catch(OutOfMemoryError e){return null;}
    }
    private static float sample(float[] mesh,int i,float x,float z){
        float ax=mesh[i],az=mesh[i+2],bx=mesh[i+6],bz=mesh[i+8],cx=mesh[i+12],cz=mesh[i+14];
        if(x<Math.min(ax,Math.min(bx,cx))||x>Math.max(ax,Math.max(bx,cx))||z<Math.min(az,Math.min(bz,cz))||z>Math.max(az,Math.max(bz,cz)))return Float.POSITIVE_INFINITY;
        float ux=bx-ax,uz=bz-az,vx=cx-ax,vz=cz-az,det=ux*vz-uz*vx;
        if(Math.abs(det)<1e-6f)return Float.POSITIVE_INFINITY;
        float u=((x-ax)*vz-(z-az)*vx)/det,v=(ux*(z-az)-uz*(x-ax))/det;
        if(u<-.0001f||v<-.0001f||u+v>1.0001f)return Float.POSITIVE_INFINITY;
        float h=mesh[i+1]+u*(mesh[i+7]-mesh[i+1])+v*(mesh[i+13]-mesh[i+1]);return Float.isFinite(h)?h:Float.POSITIVE_INFINITY;
    }
    static float scan(float[] mesh,float x,float z){float low=Float.POSITIVE_INFINITY;for(int i=0;i+17<mesh.length;i+=18)low=Math.min(low,sample(mesh,i,x,z));return low;}
    float height(float x,float z){
        if(!Float.isFinite(x)||!Float.isFinite(z))return Float.POSITIVE_INFINITY;
        if(x< -81||x>81||z< -81||z>81)return scan(mesh,x,z);
        float low=Float.POSITIVE_INFINITY;for(int i:bins[clamp(bin(z))*SIDE+clamp(bin(x))])low=Math.min(low,sample(mesh,i,x,z));return low;
    }
    int maxBucket(){int max=0;for(int[] b:bins)max=Math.max(max,b.length);return max;}
}
