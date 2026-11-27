package com.bore2.game;

import java.util.Arrays;

/** Floors share the native exterior envelope; a reserved landing connects them. */
final class BuildingPlan {
    static final int STUDY=10,FITNESS=11,MAX_FLOORS=6;
    final int landing;
    private final InteriorPlan[] floors;
    private BuildingPlan(InteriorPlan[] floors,int landing){this.floors=floors;this.landing=landing;}
    int count(){return floors.length;}
    InteriorPlan floor(int n){return n>=0&&n<count()?floors[n]:null;}
    int restoreFloor(int saved){return floor(saved)==null?0:saved;}
    int floorFor(int action){for(int n=0;n<count();n++)if(floors[n].slotFor(action)>=0)return n;return -1;}
    boolean canChange(int n,int at,int direction){return count()>1&&at==landing&&(direction==1||direction==-1)&&floor(n)!=null&&floor(n+direction)!=null&&floor(n).walkable(at)&&floor(n+direction).walkable(at);}
    private static int[] groundServices(int lot,boolean commercial,int count){
        if(!commercial)return new int[]{InteriorPlan.BED,InteriorPlan.KITCHEN,InteriorPlan.HOBBY,InteriorPlan.PHONE,InteriorPlan.SOFA};
        int[] all=VenueRules.actions(lot);if(count==1)return all;
        int[] out=new int[all.length];int n=0;
        for(int k:all)if(k!=VenueRules.WORK&&k!=VenueRules.ACTIVITY)out[n++]=k;
        return Arrays.copyOf(out,n);
    }
    private static int[] upperServices(int lot,boolean commercial,int floor){
        if(!commercial){
            if(floor==1)return new int[]{InteriorPlan.BED,STUDY,FITNESS,InteriorPlan.HOBBY};
            return (floor&1)==0?new int[]{STUDY,InteriorPlan.PHONE,InteriorPlan.SOFA}:new int[]{FITNESS,InteriorPlan.HOBBY,InteriorPlan.SOFA};
        }
        int[] out=new int[4];int n=0;
        if(VenueRules.has(lot,VenueRules.WORK))out[n++]=VenueRules.WORK;
        if(VenueRules.has(lot,VenueRules.ACTIVITY))out[n++]=VenueRules.ACTIVITY;
        out[n++]=VenueRules.MEET;out[n++]=VenueRules.WAIT;
        return Arrays.copyOf(out,n);
    }
    static BuildingPlan create(int[] raw,int seed,int count,boolean commercial){
        if(raw==null||raw.length!=InteriorPlan.N+3||count<1||count>MAX_FLOORS||
            (commercial?!VenueRules.supported(raw[0]):raw[0]<1||raw[0]>5))return null;
        InteriorPlan[] floors=new InteriorPlan[count];
        floors[0]=InteriorPlan.layout(raw,seed,commercial,groundServices(raw[0],commercial,count));
        if(floors[0]==null)return null;
        if(count==1)return new BuildingPlan(floors,-1);
        InteriorPlan base=floors[0];int best=-1,bestScore=Integer.MAX_VALUE;
        // Prefer a central, reachable, unoccupied landing away from the front door.
        for(int i=0;i<InteriorPlan.N;i++)if(i!=base.entry&&base.walkable(i)){
            int d=Math.abs(i%18-base.entry%18)+Math.abs(i/18-base.entry/18);
            int score=16*(Math.abs(i%18-9)+Math.abs(i/18-9))+(d<2?256:0)+(i&7);
            if(score<bestScore){best=i;bestScore=score;}
        }
        if(best<0)return null;
        int[] upper=raw.clone();upper[1]=best%18;upper[2]=best/18;
        for(int n=1;n<count;n++){
            floors[n]=InteriorPlan.layout(upper,seed^(n*0x9e3779b9),commercial,upperServices(raw[0],commercial,n));
            if(floors[n]==null||!floors[n].walkable(best))return null;
        }
        return new BuildingPlan(floors,best);
    }
}
