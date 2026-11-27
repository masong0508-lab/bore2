package com.bore2.game;
import java.util.Arrays;
public final class InteriorPlanTest {
    static void check(boolean ok,String what){if(!ok)throw new AssertionError(what);}
    static int[] shape(int x0,int y0,int x1,int y1,boolean wing){
        int[] raw=new int[327];raw[0]=1;raw[1]=x0+2;raw[2]=y1;
        for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)raw[3+y*18+x]=1;
        if(wing)for(int y=y0+1;y<=y0+4;y++)for(int x=x1;x<=x1+3;x++)raw[3+y*18+x]=1;
        return raw;
    }
    static void verify(int[] raw,int seed){
        InteriorPlan p=InteriorPlan.create(raw,seed);check(p!=null,"valid furnished plan");
        InteriorPlan other=InteriorPlan.create(raw,seed);check(Arrays.equals(p.objects,other.objects)&&Arrays.equals(p.walls,other.walls),"deterministic plan");
        check(p.entry==raw[2]*18+raw[1]&&p.walkable(p.entry),"real door entry");
        for(int i=0;i<324;i++){
            check(p.floor(i)==(raw[i+3]==1),"exact generated footprint");
            if(p.walkable(i))check(p.route(p.entry,i)!=null,"no stranded floor cells");
        }
        for(int kind=0;kind<InteriorPlan.COUNT;kind++){
            int cell=p.objects[kind];check(p.floor(cell)&&!p.walkable(cell),"solid furniture");
            int[] route=p.routeToObject(p.entry,kind);check(route!=null,"object reachable");
            int from=p.entry;for(int next:route){check(p.canStep(from,next),"valid path edge");from=next;}
            check(p.canUse(from,kind)&&!p.canUse(cell,kind),"interaction at adjacent walkable cell");
            check(p.route(p.entry,cell)==null,"cannot walk through furniture");
        }
        check(!p.canStep(17,18),"no row-wrap edge");
    }
    public static void main(String[] args){
        for(int seed=0;seed<25;seed++){verify(shape(5,5,12,13,false),seed);verify(shape(5,5,11,12,true),seed);verify(shape(7,7,10,10,false),seed);}
        check(InteriorPlan.create(new int[2],1)==null,"invalid payload");
        int[] bad=shape(5,5,12,13,false);bad[1]=0;bad[2]=0;check(InteriorPlan.create(bad,1)==null,"door outside footprint");
        int[] isolated=shape(5,5,12,13,false);isolated[3]=1;check(InteriorPlan.create(isolated,1)==null,"reject disconnected floor");
        System.out.println("Interior plans: 75 deterministic layouts, exact footprints, collision-safe walking and reachable furniture PASS");
    }
}
