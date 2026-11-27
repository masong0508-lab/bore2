package com.bore2.game;
public final class ActorMeshTest {
    static void check(boolean b,String why){if(!b)throw new AssertionError(why);}
    static float[] tri(float y){return new float[]{-2,y,-2,1,1,1,2,y,-2,1,1,1,0,y,2,1,1,1};}
    public static void main(String[] args){
        float[] a=new float[36];System.arraycopy(tri(4),0,a,0,18);System.arraycopy(tri(1),0,a,18,18);
        check(Math.abs(ActorMesh.ground(a,0,0)-1)<.0001f,"ground not roof");
        check(!Float.isFinite(ActorMesh.ground(a,8,8)),"no invented off-mesh footing");
        float[] model=ActorMesh.create();check(model.length%18==0&&model.length>300,"solid humanoid mesh");
        float min=Float.POSITIVE_INFINITY,max=Float.NEGATIVE_INFINITY;
        for(int i=0;i<model.length;i+=6){for(int j=0;j<6;j++)check(Float.isFinite(model[i+j]),"finite vertex");min=Math.min(min,model[i+1]);max=Math.max(max,model[i+1]);}
        check(min==0&&max>=1.5f&&max<=2.5f,"human-scale model, feet anchored");
        System.out.println("Android actor footing and geometry PASS");
    }
}
