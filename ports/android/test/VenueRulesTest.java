package com.bore2.game;
import java.util.*;
public final class VenueRulesTest {
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static int[] raw(int lot,int w,int h,boolean wing){int[] r=new int[327];r[0]=lot;r[1]=6;r[2]=5+h-1;for(int y=5;y<5+h;y++)for(int x=5;x<5+w;x++)r[3+y*18+x]=1;if(wing)for(int y=6;y<9;y++)for(int x=5+w-1;x<5+w+2;x++)r[3+y*18+x]=1;return r;}
    public static void main(String[] args){
        check(!VenueRules.supported(1)&&!VenueRules.supported(16)&&!VenueRules.supported(20)&&!VenueRules.supported(32),"no home, park, farm or river interiors");
        check(VenueRules.has(21,VenueRules.WORK)&&VenueRules.has(21,VenueRules.MEAL)&&VenueRules.has(21,VenueRules.ACTIVITY),"cafe work food activities");
        check(VenueRules.has(9,VenueRules.WORK)&&!VenueRules.has(9,VenueRules.MEAL),"office does not sell meals");
        check(VenueRules.has(13,VenueRules.ACTIVITY)&&!VenueRules.has(13,VenueRules.WORK),"arcade has activities not invented shifts");
        int plans=0;
        for(int lot=0;lot<35;lot++)if(VenueRules.supported(lot))for(int seed=0;seed<8;seed++)for(int shape=0;shape<3;shape++){
            int[] r=raw(lot,shape==0?4:7,shape==0?4:8,shape==2);InteriorPlan p=InteriorPlan.createVenue(r,seed),q=InteriorPlan.createVenue(r,seed);
            check(p!=null&&q!=null&&p.commercial,"furnished venue");check(Arrays.equals(p.objects,q.objects)&&Arrays.equals(p.walls,q.walls),"stable layout");
            check(p.actionCount()>=3&&p.actionCount()<=5,"bounded actions");Set<Integer> seen=new HashSet<>();
            for(int i=0;i<324;i++){check(p.floor(i)==(r[i+3]==1),"native footprint");if(p.walkable(i))check(p.route(p.entry,i)!=null,"all floors reachable");}
            for(int i=0;i<p.actionCount();i++){int c=p.objects[i];check(seen.add(p.actionAt(i)),"unique service");check(VenueRules.has(lot,p.actionAt(i)),"actual service");check(p.floor(c)&&!p.walkable(c)&&!p.labelAt(i).isEmpty(),"visible solid object");int[] path=p.routeToObject(p.entry,i);check(path!=null,"reachable service");int at=p.entry;for(int n:path){check(p.canStep(at,n),"cardinal walk");at=n;}check(p.canUse(at,i)&&!p.canUse(c,i),"adjacent use only");}
            for(int i=p.actionCount();i<InteriorPlan.COUNT;i++)check(p.objects[i]==-1&&p.routeToObject(p.entry,i)==null,"unused slot");plans++;
        }
        check(InteriorPlan.createVenue(raw(1,4,4,false),0)==null,"private home denied");
        check(InteriorPlan.create(raw(21,4,4,false),0)==null,"no home at cafe");
        check(VenueRules.mealOpen(21,420)&&!VenueRules.mealOpen(21,1250)&&VenueRules.mealOpen(31,1430),"meal opening hours");
        System.out.println("Venue rules and "+plans+" furnished plans PASS");
    }
}
