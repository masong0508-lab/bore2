package com.bore2.game;

public final class IndoorMotionTest {
    private static int c(int x,int y){return y*18+x;}
    private static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    public static void main(String[] args){
        boolean[] floor=new boolean[324];int[] walls=new int[324];
        for(int y=3;y<=9;y++)for(int x=3;x<=11;x++)if(x!=7||y==6)floor[c(x,y)]=true;
        walls[c(9,7)]=1;walls[c(10,7)]=4;
        int entry=c(3,6),stairs=c(11,6),player=c(4,6),passage=c(7,6);
        int[][] stations={{c(3,4),c(4,3)},{c(10,8),c(11,9)}};
        int[] ids={21,32,43,54};
        IndoorCrowd crowd=IndoorCrowd.create(floor,walls,stations,entry,stairs,player,ids,c(10,7));
        int moved=0,seen=0;
        for(int tick=0;tick<1200;tick++){
            int[] before=new int[crowd.size()];for(int i=0;i<before.length;i++)before[i]=crowd.cell(i);
            IndoorCrowd.Step step=crowd.advance(player,entry,stairs,stations,tick);
            if(step!=null){
                moved++;seen|=1<<step.index;
                check(before[step.index]==step.from&&crowd.cell(step.index)==step.to,"real move committed");
                int dx=step.to%18-step.from%18,dy=step.to/18-step.from/18;
                check(Math.abs(dx)+Math.abs(dy)==1,"cardinal step");
                int d=dx==1?0:dy==1?1:dx==-1?2:3;
                check((walls[step.from]&(1<<d))==0&&(walls[step.to]&(1<<((d+2)&3)))==0,"never cross wall");
                check(step.to!=entry&&step.to!=stairs&&step.to!=player&&step.to!=passage,"protected cells");
                for(int i=0;i<before.length;i++)if(i!=step.index)check(crowd.cell(i)==before[i],"only one moves per tick");
                check(crowd.clear(step.from)&&!crowd.clear(step.to),"occupancy matches committed tile");
            }
            check(crowd.route(player,entry)!=null&&crowd.route(player,stairs)!=null,"exits reachable throughout motion");
            for(int[] station:stations)check(crowd.routeToAny(player,station)!=null,"stations stay reachable");
            for(int i=0;i<crowd.size();i++){
                check(crowd.routeToPerson(player,crowd.id(i))!=null,"everyone approachable");
                for(int k=0;k<i;k++)check(crowd.cell(i)!=crowd.cell(k),"no overlap");
            }
            // Simulate a new player position between motion ticks.
            if(tick%9==0){int[] route=crowd.route(player,tick%18==0?entry:stairs);if(route.length>0)player=route[0];}
        }
        check(moved>500&&seen==15,"all four residents move repeatedly");
        boolean[] corridor=new boolean[324];for(int x=1;x<10;x++)corridor[c(x,5)]=true;
        IndoorCrowd narrow=IndoorCrowd.create(corridor,new int[324],new int[][]{{c(9,5)}},c(1,5),c(9,5),c(2,5),ids,c(5,5));
        check(narrow.advance(c(2,5),c(1,5),c(9,5),new int[][]{{c(9,5)}},0)==null,"empty narrow room stays safe");
        System.out.println("IndoorMotion: "+moved+" safe moves in 1200 ticks, all residents, walls, player, stairs and stations PASS");
    }
}
