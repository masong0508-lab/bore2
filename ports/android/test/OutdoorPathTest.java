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
        System.out.println("Outdoor routes: exact cardinal shortest path, blocked cells, safe spawn, bounds PASS");
    }
}
