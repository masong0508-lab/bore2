package com.bore2.game;
import java.util.Arrays;
public final class IndoorCrowdTest {
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static int c(int x,int y){return y*18+x;}
    public static void main(String[] args){
        boolean[] floor=new boolean[324];int[] walls=new int[324];
        for(int y=3;y<=9;y++)for(int x=3;x<=11;x++)if(x!=7||y==6)floor[c(x,y)]=true;
        int entry=c(3,6),stairs=c(11,6),player=c(4,6),passage=c(7,6);
        int[][] use={{c(3,4),c(4,3)},{c(10,8),c(11,9)}};
        int[] ids={21,32,43,54,65,21};
        IndoorCrowd crowd=IndoorCrowd.create(floor,walls,use,entry,stairs,player,ids,c(10,7));
        check(crowd.size()==4,"bounded crowd fills valid room");
        for(int i=0;i<crowd.size();i++){
            int at=crowd.cell(i);check(floor[at]&&at!=entry&&at!=stairs&&at!=player&&at!=passage,"never occupy doors, stairs, player or chokepoint");
            for(int j=0;j<i;j++)check(at!=crowd.cell(j)&&crowd.id(i)!=crowd.id(j),"distinct identity and tile");
            int[] route=crowd.routeToPerson(player,crowd.id(i));check(route!=null,"resident can be reached");
            int end=route.length==0?player:route[route.length-1];check(crowd.adjacent(end,crowd.id(i)),"talk from a clear adjacent tile");
            check(crowd.route(player,at)==null,"cannot walk through a resident");
        }
        check(crowd.route(player,entry)!=null&&crowd.route(player,stairs)!=null,"exit and stairs always accessible");
        for(int[] station:use)check(crowd.routeToAny(player,station)!=null,"every station reachable");
        IndoorCrowd same=IndoorCrowd.create(floor,walls,use,entry,stairs,player,ids,c(10,7));
        for(int i=0;i<crowd.size();i++)check(crowd.cell(i)==same.cell(i),"stable layout");
        boolean[] corridor=new boolean[324];for(int x=1;x<10;x++)corridor[c(x,5)]=true;
        IndoorCrowd narrow=IndoorCrowd.create(corridor,new int[324],new int[][]{{c(9,5)}},c(1,5),c(9,5),c(2,5),ids,c(5,5));
        check(narrow.size()==0&&narrow.route(c(2,5),c(9,5))!=null,"no unsafe crowd in narrow corridor");
        boolean[] square=new boolean[324];for(int y=1;y<=3;y++)for(int x=1;x<=3;x++)square[c(x,y)]=true;
        int[] barrier=new int[324];barrier[c(1,2)]=1;barrier[c(2,2)]=4;
        IndoorCrowd open=IndoorCrowd.create(square,barrier,new int[0][],c(1,1),-1,c(1,2),new int[0],-1);
        check(!open.canStep(c(1,2),c(2,2)),"wall edges respected");
        check(open.route(c(1,2),c(2,2)).length==3,"route around wall");
        check(open.routeToPerson(c(1,2),999)==null,"absent resident cannot be targeted");
        System.out.println("IndoorCrowd: distinct residents, blocked tiles, protected routes, safe adjacency and deterministic layout PASS");
    }
}
