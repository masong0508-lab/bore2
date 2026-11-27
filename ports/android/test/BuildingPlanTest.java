package com.bore2.game;
import java.util.Arrays;
public final class BuildingPlanTest {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    static int[] raw(int lot,int w,int h,boolean wing){
        int[] a=new int[327];a[0]=lot;a[1]=6;a[2]=5+h-1;
        for(int y=5;y<5+h;y++)for(int x=5;x<5+w;x++)a[3+y*18+x]=1;
        if(wing)for(int y=6;y<9;y++)for(int x=5+w-1;x<5+w+2;x++)a[3+y*18+x]=1;
        return a;
    }
    static void verify(int[] a,int seed,int levels,boolean commercial){
        BuildingPlan b=BuildingPlan.create(a,seed,levels,commercial);
        check(b!=null&&b.count()==levels,"native floor count");
        BuildingPlan duplicate=BuildingPlan.create(a,seed,levels,commercial);
        check(duplicate!=null&&b.landing==duplicate.landing,"stable landing");
        for(int n=0;n<levels;n++){
            InteriorPlan p=b.floor(n);
            check(p!=null&&p.commercial==commercial&&p.lot==a[0],"floor identity");
            check(Arrays.equals(p.objects,duplicate.floor(n).objects),"stable furniture");
            for(int i=0;i<324;i++){
                check(p.floor(i)==(a[i+3]==1),"no invented footprint");
                if(p.walkable(i))check(p.route(p.entry,i)!=null,"connected floor");
            }
            if(levels>1){
                check(p.walkable(b.landing)&&p.route(p.entry,b.landing)!=null,"aligned reachable stairs");
                check(n==0?p.entry!=b.landing:p.entry==b.landing,"upstairs arrival at landing");
                check(b.canChange(n,b.landing,1)==(n<levels-1),"upper floor bound");
                check(b.canChange(n,b.landing,-1)==(n>0),"lower floor bound");
                check(!b.canChange(n,(b.landing+1)%324,1),"no remote stairs");
                check(!b.canChange(n,b.landing,2),"no floor skipping");
            }
            for(int k=0;k<p.actionCount();k++){
                int[] path=p.routeToObject(p.entry,k);check(path!=null,"reachable furniture");
                int at=path.length==0?p.entry:path[path.length-1];check(p.canUse(at,k),"adjacent use");
            }
        }
        int[] services=commercial?VenueRules.actions(a[0]):new int[]{0,1,2,3,4};
        for(int action:services)check(b.floorFor(action)>=0,"no service lost");
        check(b.restoreFloor(-1)==0&&b.restoreFloor(999)==0,"invalid saves return to ground");
        check(b.restoreFloor(levels-1)==levels-1,"valid saved floor");
    }
    public static void main(String[] args){
        check(BuildingPlan.create(raw(1,4,4,false),0,0,false)==null,"invalid count");
        check(BuildingPlan.create(raw(1,4,4,false),0,7,false)==null,"too many floors");
        check(BuildingPlan.create(raw(1,4,4,false),0,2,true)==null,"private home not public");
        int total=0;
        for(int seed=0;seed<6;seed++)for(int levels=1;levels<=6;levels++){
            verify(raw(4,4,4,false),seed,levels,false);total++;
            verify(raw(5,7,8,true),seed,levels,false);total++;
            for(int lot=6;lot<=34;lot++)if(VenueRules.supported(lot)){verify(raw(lot,7,8,false),seed,levels,true);total++;}
        }
        BuildingPlan tower=BuildingPlan.create(raw(14,7,8,false),12,6,true);
        check(tower.floorFor(VenueRules.WORK)>0,"tower work upstairs");
        BuildingPlan house=BuildingPlan.create(raw(4,7,8,false),12,2,false);
        check(house.floor(0).slotFor(InteriorPlan.BED)>=0&&house.floor(1).slotFor(BuildingPlan.STUDY)>=0,"old home actions retained plus new study");
        System.out.println("BuildingPlan: "+total+" building layouts, aligned stairs, all stations reachable, restore bounds PASS");
    }
}
