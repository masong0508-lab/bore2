package com.bore2.game;

import java.util.Arrays;

/** Low-poly bevelled humanoid. XYZ RGB + signed limb (legs +/-1, arms +/-2). */
final class AvatarMesh {
    static final int STRIDE=7;
    private final float[] data=new float[750*3*STRIDE];
    private int at;
    private final int sides;
    private AvatarMesh(boolean detail){sides=detail?8:6;}
    static float[] create(AvatarStyle style,boolean detail){
        if(style==null)style=AvatarStyle.defaults();
        AvatarMesh m=new AvatarMesh(detail);
        int skin=style.color(0),hair=style.color(1),top=style.color(2),pants=style.color(3),shoe=0x323438;
        for(int side=-1;side<=1;side+=2){
            float x=side*.125f;
            m.part(side,pants,x,0,new float[]{.12f,.40f,.76f},new float[]{.09f,.103f,.12f},new float[]{.09f,.102f,.13f});
            m.part(side,shoe,x,.045f,new float[]{0,.16f},new float[]{.102f,.088f},new float[]{.15f,.105f});
            float arm=side*.32f;
            m.part(side*2,top,arm,0,new float[]{1.14f,1.24f,1.34f},new float[]{.071f,.088f,.083f},new float[]{.075f,.091f,.079f});
            m.part(side*2,skin,arm,0,new float[]{.81f,.99f,1.16f},new float[]{.052f,.057f,.064f},new float[]{.056f,.062f,.069f});
            m.part(side*2,skin,arm,.004f,new float[]{.75f,.82f,.89f},new float[]{.046f,.064f,.053f},new float[]{.049f,.065f,.052f});
        }
        m.part(0,top,0,0,new float[]{.75f,1.03f,1.35f},new float[]{.22f,.23f,.265f},new float[]{.135f,.145f,.129f});
        m.part(0,skin,0,0,new float[]{1.34f,1.40f},new float[]{.085f,.093f},new float[]{.079f,.083f});
        m.part(0,skin,0,.006f,new float[]{1.40f,1.58f,1.73f},new float[]{.112f,.174f,.139f},new float[]{.11f,.149f,.126f});
        int cut=style.choice(4);
        if(cut==0)m.part(0,hair,0,-.016f,new float[]{1.64f,1.76f,1.82f},new float[]{.178f,.182f,.128f},new float[]{.151f,.149f,.11f});
        else if(cut==1)m.part(0,hair,.024f,-.022f,new float[]{1.66f,1.79f,1.88f},new float[]{.174f,.184f,.087f},new float[]{.151f,.16f,.085f});
        else if(cut==2){
            m.part(0,hair,0,-.058f,new float[]{1.40f,1.69f,1.83f},new float[]{.157f,.193f,.129f},new float[]{.095f,.113f,.105f});
            m.part(0,hair,0,-.01f,new float[]{1.68f,1.78f},new float[]{.181f,.16f},new float[]{.143f,.129f});
        }else m.part(0,hair,0,-.006f,new float[]{1.69f,1.765f,1.785f},new float[]{.147f,.14f,.108f},new float[]{.129f,.12f,.088f});
        if(detail){
            for(int side=-1;side<=1;side+=2){
                float x=side*.068f;
                m.box(0,0xeee5db,x-.027f,1.593f,.132f,x+.027f,1.625f,.154f);
                m.box(0,0x30333b,x-.009f,1.597f,.154f,x+.009f,1.622f,.159f);
            }
            m.box(0,skin,-.024f,1.54f,.147f,.024f,1.59f,.179f);
            m.box(0,0x825446,-.038f,1.503f,.135f,.038f,1.515f,.148f);
        }
        return Arrays.copyOf(m.data,m.at);
    }
    private void vertex(float[] p,int color,float shade,int role){
        data[at++]=p[0];data[at++]=p[1];data[at++]=p[2];
        data[at++]=((color>>>16)&255)/255f*shade;data[at++]=((color>>>8)&255)/255f*shade;data[at++]=(color&255)/255f*shade;data[at++]=role;
    }
    private void tri(float[] a,float[] b,float[] c,int color,float shade,int role){vertex(a,color,shade,role);vertex(b,color,shade,role);vertex(c,color,shade,role);}
    private void part(int role,int color,float x,float z,float[] ys,float[] rx,float[] rz){
        float[][][] ring=new float[ys.length][sides][3];
        for(int j=0;j<ys.length;j++)for(int i=0;i<sides;i++){
            double angle=2*Math.PI*i/sides;ring[j][i]=new float[]{x+(float)Math.cos(angle)*rx[j],ys[j],z+(float)Math.sin(angle)*rz[j]};
        }
        for(int i=0;i<sides;i++){
            int n=(i+1)%sides;float shade=.73f+.24f*(float)(.5+.5*Math.sin((i+.5)*2*Math.PI/sides));
            for(int j=1;j<ys.length;j++){tri(ring[j-1][i],ring[j-1][n],ring[j][n],color,shade,role);tri(ring[j-1][i],ring[j][n],ring[j][i],color,shade,role);}
            int top=ys.length-1;
            tri(new float[]{x,ys[0],z},ring[0][n],ring[0][i],color,.62f,role);
            tri(new float[]{x,ys[top],z},ring[top][i],ring[top][n],color,1,role);
        }
    }
    private void box(int role,int color,float x,float y,float z,float X,float Y,float Z){
        float[][] p={{x,y,z},{X,y,z},{X,y,Z},{x,y,Z},{x,Y,z},{X,Y,z},{X,Y,Z},{x,Y,Z}};
        int[][] f={{0,1,2,3},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
        for(int[] v:f){tri(p[v[0]],p[v[1]],p[v[2]],color,.97f,role);tri(p[v[0]],p[v[2]],p[v[3]],color,.97f,role);}
    }
}
