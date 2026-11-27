package com.bore2.game;
import java.util.Arrays;
public final class AvatarMeshTest {
    static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    static void verify(float[] a,boolean detailed){
        check(a.length%21==0&&a.length>=100*21&&a.length<750*21,"triangle budget and stride");
        float min=Float.POSITIVE_INFINITY,max=Float.NEGATIVE_INFINITY;
        int roles=0;
        for(int i=0;i<a.length;i+=7){
            for(int k=0;k<7;k++)check(Float.isFinite(a[i+k]),"finite vertex data");
            for(int k=3;k<=5;k++)check(a[i+k]>=0&&a[i+k]<=1,"valid colors");
            check(a[i+6]==(int)a[i+6]&&Math.abs(a[i+6])<=2,"valid limb tag");
            roles|=1<<((int)a[i+6]+2);min=Math.min(min,a[i+1]);max=Math.max(max,a[i+1]);
            check(Math.abs(a[i])<.55f&&Math.abs(a[i+2])<.4f,"walkable body width");
        }
        check(roles==31&&min==0&&max>=1.7f&&max<=2.05f,"feet anchored, all four independently rigged limbs");
        for(int i=0;i<a.length;i+=21){
            float ux=a[i+7]-a[i],uy=a[i+8]-a[i+1],uz=a[i+9]-a[i+2],vx=a[i+14]-a[i],vy=a[i+15]-a[i+1],vz=a[i+16]-a[i+2];
            float x=uy*vz-uz*vy,y=uz*vx-ux*vz,z=ux*vy-uy*vx;
            check(x*x+y*y+z*z>1e-12,"no degenerate faces");
            check(a[i+6]==a[i+13]&&a[i+6]==a[i+20],"one articulated part per triangle");
        }
    }
    public static void main(String[] args){
        AvatarStyle base=AvatarStyle.defaults();
        float[] model=AvatarMesh.create(base,true),low=AvatarMesh.create(base,false);
        verify(model,true);verify(low,false);check(low.length<model.length,"crowd LOD cheaper");
        check(Arrays.equals(model,AvatarMesh.create(base,true)),"stable mesh");
        float[] recolor=AvatarMesh.create(base.with(2,3),true);
        check(model.length==recolor.length,"color change retains topology");boolean different=false;
        for(int i=0;i<model.length;i+=7){for(int k=0;k<3;k++)check(model[i+k]==recolor[i+k],"recolor geometry unchanged");different|=model[i+3]!=recolor[i+3]||model[i+4]!=recolor[i+4]||model[i+5]!=recolor[i+5];}
        check(different,"outfit changes actual vertex colors");
        for(int cut=0;cut<4;cut++){float[] hair=AvatarMesh.create(base.with(4,cut),true);verify(hair,true);if(cut>0)check(!Arrays.equals(hair,model),"hair shape changes mesh");}
        for(int i=0;i<40;i++)verify(AvatarMesh.create(AvatarStyle.resident(1234,i),false),false);
        System.out.println("Avatar mesh: detailed and low-LOD bodies, four joint groups, outfits and hair PASS");
    }
}
