package com.bore2.game;
import java.util.Arrays;
public final class OutdoorPathTest {
    private static void check(boolean b,String m){if(!b)throw new AssertionError(m);}
    public static void main(String[] args){
        int[] cells=new int[54*54];Arrays.fill(cells,1);
        for(int y=0;y<54;y++)if(y!=27)cells[y*54+26]=0;
        OutdoorPath p=OutdoorPath.create(100,200,cells);
        check(p!=null,"valid grid");
        int a=OutdoorPath.pack(105,205),b=OutdoorPath.pack(148,205);
        int[] path=p.route(a,b);check(path!=null&&path.length==87,"shortest route via gap");
        int prior=a;for(int q:path){check(p.open(q)&&OutdoorPath.adjacent(prior,q),"no blocked/diagonal movement");prior=q;}
        check(prior==b,"destination");
        check(p.route(a,OutdoorPath.pack(126,205))==null,"wall target refused");
        check(p.route(a,OutdoorPath.pack(99,205))==null,"out of window refused");
        check(p.route(a,a).length==0,"same tile");
        check(p.nearest(OutdoorPath.pack(126,205))==OutdoorPath.pack(125,205),"nearest deterministic spawn");
        cells[27*54+26]=0;p=OutdoorPath.create(100,200,cells);
        check(p.route(a,b)==null,"sealed barrier");
        check(!OutdoorPath.adjacent(OutdoorPath.pack(18431,0),OutdoorPath.pack(0,1)),"no row wrap");
        check(OutdoorPath.create(18400,0,cells)==null,"window beyond map");
        check(OutdoorPath.create(0,0,new int[1])==null,"bad mask");
        int[] slope=new int[54*54];Arrays.fill(cells,1);
        for(int y=0;y<54;y++)for(int x=27;x<54;x++)slope[y*54+x]=200;
        p=OutdoorPath.create(100,200,cells,slope);
        check(p.route(OutdoorPath.pack(126,205),OutdoorPath.pack(127,205))==null,"cliff disconnects walking graph");
        Arrays.fill(slope,0);slope[5*54+6]=64;
        p=OutdoorPath.create(100,200,cells,slope);
        int c=OutdoorPath.pack(106,205);
        check(p.canStep(a,c)&&p.duration(a,c,160)==214,"moderate uphill slower");
        check(p.duration(c,a,160)==214,"moderate downhill slower");
        check(p.duration(a,OutdoorPath.pack(104,205),160)==160,"flat pace unchanged");
        slope[5*54+6]=110;p=OutdoorPath.create(100,200,cells,slope);
        check(p.duration(a,c,160)==640,"near-cliff careful step");
        slope[5*54+6]=111;p=OutdoorPath.create(100,200,cells,slope);
        check(!p.canStep(a,c)&&p.duration(a,c,160)==0,"steep edge refused");
        check(p.route(a,c)==null,"isolated high tile unreachable");
        check(OutdoorPath.create(100,200,cells,new int[1])==null,"bad terrain array");
        Arrays.fill(slope,0);Arrays.fill(cells,1);
        int start=OutdoorPath.pack(105,205),goal=OutdoorPath.pack(109,205);
        slope[5*54+6]=110;slope[5*54+8]=110;
        p=OutdoorPath.create(100,200,cells,slope);
        path=p.route(start,goal);
        check(path!=null&&path.length==6,"take flat six-step detour instead of four steep steps");
        int total=0;prior=start;
        for(int step:path){total+=p.duration(prior,step,160);prior=step;}
        check(total==960&&prior==goal,"fastest walking time");
        System.out.println("Outdoor routes: cardinal paths, safe spawn, slope costs and fastest-time detours PASS");
    }
}
