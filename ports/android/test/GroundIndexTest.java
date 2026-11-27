package com.bore2.game;
import java.util.Random;
public final class GroundIndexTest {
    static void check(boolean b,String m){if(!b)throw new AssertionError(m);}
    static void tri(float[] m,int i,float x,float z,float h,float size){float[] v={x,h,z,1,1,1,x+size,h+1,z,1,1,1,x,h+2,z+size,1,1,1};System.arraycopy(v,0,m,i,v.length);}
    public static void main(String[] args){
        float[] m=new float[18*400];Random r=new Random(17);
        for(int i=0;i<400;i++)tri(m,i*18,r.nextInt(157)-80,r.nextInt(157)-80,r.nextInt(30),1+r.nextInt(22));
        GroundIndex g=GroundIndex.build(m);check(g!=null,"index built");
        for(int i=0;i<10000;i++){float x=r.nextFloat()*162-81,z=r.nextFloat()*162-81;float expected=GroundIndex.scan(m,x,z),actual=g.height(x,z);check(Float.floatToIntBits(expected)==Float.floatToIntBits(actual),"exact indexed height");}
        for(int x=-81;x<=81;x+=18)for(int z=-81;z<=81;z+=18)check(Float.floatToIntBits(g.height(x,z))==Float.floatToIntBits(GroundIndex.scan(m,x,z)),"boundary height");
        check(g.maxBucket()<400,"bounded query cost");check(GroundIndex.build(new float[3])==null,"bad mesh");
        System.out.println("GroundIndex: 10000+ exact indexed ground queries and bin edges PASS");
    }
}
