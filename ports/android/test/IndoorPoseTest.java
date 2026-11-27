package com.bore2.game;
import java.util.Arrays;
public final class IndoorPoseTest {
    private static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    private static float[] frame(int mode,int facing,float wave){float[] p=new float[IndoorPose.SIZE];IndoorPose.sample(mode,facing,wave,p);return p;}
    public static void main(String[] args){
        int[] expected={IndoorPose.REST,IndoorPose.COOK,IndoorPose.WORK,IndoorPose.TALK,IndoorPose.REST,IndoorPose.WORK,IndoorPose.EAT,IndoorPose.WORK,IndoorPose.TALK,IndoorPose.REST,IndoorPose.READ,IndoorPose.EXERCISE};
        for(int action=0;action<expected.length;action++)check(IndoorPose.forAction(action)==expected[action],"station action maps to its own pose");
        check(IndoorPose.forAction(-1)==IndoorPose.IDLE&&IndoorPose.forAction(100)==IndoorPose.IDLE,"unknown action remains idle");
        check(!IndoorPose.atGoal(25,new int[]{24,26,-1})&&IndoorPose.atGoal(26,new int[]{24,26,-1})&&!IndoorPose.atGoal(-1,new int[]{-1}),"pose only after reaching actual goal");
        check(Arrays.equals(frame(IndoorPose.IDLE,0,-1),frame(IndoorPose.IDLE,0,1)),"idle never animates");
        float[] standing=frame(IndoorPose.IDLE,0,0),rest=frame(IndoorPose.REST,1,0),eat=frame(IndoorPose.EAT,1,0);
        check(rest[IndoorPose.BODY_Y]>standing[IndoorPose.BODY_Y]+.08f,"rest lowers body");
        check(eat[IndoorPose.RIGHT_Y]<standing[IndoorPose.RIGHT_Y]-.15f,"eating lifts hand toward mouth");
        check(!Arrays.equals(frame(IndoorPose.COOK,1,1),frame(IndoorPose.COOK,1,-1)),"cooking moves arms");
        check(!Arrays.equals(frame(IndoorPose.WALK,1,1),frame(IndoorPose.WALK,1,-1)),"walking alternates feet");
        float[] right=frame(IndoorPose.CLEAN,1,.6f),left=frame(IndoorPose.CLEAN,-1,.6f);
        check(Math.abs(right[IndoorPose.BODY_X]+left[IndoorPose.BODY_X])<.0001f,"cleaning follows station direction");
        int n=0;
        for(int mode=0;mode<=IndoorPose.CLEAN;mode++)for(int facing=-1;facing<=1;facing++)for(int t=-20;t<=20;t++){
            float[] p=frame(mode,facing,t/20f);
            for(float v:p)check(Float.isFinite(v)&&Math.abs(v)<.5f,"pose remains within tile");n++;
        }
        check(Arrays.equals(frame(IndoorPose.WALK,1,Float.NaN),frame(IndoorPose.WALK,1,0)),"invalid phase falls back safely");
        System.out.println("IndoorPose: "+n+" bounded poses, action mapping, arrival gating, distinct animations and direction PASS");
    }
}
