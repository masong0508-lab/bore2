package com.bore2.game;

public final class IndoorRoutineTest {
    static int c(int x,int y){return y*18+x;}
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static void verifyRoom(IndoorCrowd crowd,int player,int entry,int stairs,int[][] stations){
        check(crowd.route(player,entry)!=null&&crowd.route(player,stairs)!=null,"doors and stairs reachable");
        for(int[] station:stations)check(crowd.routeToAny(player,station)!=null,"station remains accessible");
        for(int i=0;i<crowd.size();i++){
            check(crowd.cell(i)!=player&&crowd.cell(i)!=entry&&crowd.cell(i)!=stairs,"reserved tile protected");
            check(crowd.routeToPerson(player,crowd.id(i))!=null,"resident approachable");
            for(int k=0;k<i;k++)check(crowd.cell(i)!=crowd.cell(k),"no overlaps");
        }
    }
    public static void main(String[] args){
        boolean[] floor=new boolean[324];int[] walls=new int[324];
        for(int y=2;y<=8;y++)for(int x=2;x<=11;x++)floor[c(x,y)]=true;
        for(int y=3;y<=7;y++){walls[c(6,y)]|=1;walls[c(7,y)]|=4;}
        int entry=c(2,2),stairs=c(11,2),player=c(3,3),target=c(10,5),nextTarget=c(4,5);
        int[][] stations={{target,c(11,5)},{nextTarget,c(4,6)}};
        IndoorCrowd crowd=IndoorCrowd.create(floor,walls,stations,entry,stairs,player,new int[]{42},c(3,5));
        check(crowd.size()==1&&crowd.cell(0)==c(3,5),"resident starts at preferred tile");
        int[][] goals={{target}};int moves=0;boolean detour=false;
        for(int tick=0;tick<40;tick++){
            IndoorCrowd.Step s=crowd.advanceToward(player,entry,stairs,stations,goals,tick);
            if(s!=null){moves++;detour|=s.to/18==2||s.to/18==8;int dx=s.to%18-s.from%18,dy=s.to/18-s.from/18;
                check(Math.abs(dx)+Math.abs(dy)==1,"only cardinal moves");
                int d=dx==1?0:dy==1?1:dx==-1?2:3;check((walls[s.from]&(1<<d))==0&&(walls[s.to]&(1<<((d+2)&3)))==0,"no wall crossing");}
            verifyRoom(crowd,player,entry,stairs,stations);
        }
        check(detour&&moves>7&&crowd.cell(0)==target,"walk around partition to scheduled activity");
        check(crowd.advanceToward(player,entry,stairs,stations,goals,41)==null,"remain at activity instead of pacing");
        goals[0]=new int[]{nextTarget};
        for(int tick=0;tick<40;tick++)crowd.advanceToward(player,entry,stairs,stations,goals,tick);
        check(crowd.cell(0)==nextTarget,"updated routine changes destination");
        int old=crowd.cell(0);goals[0]=new int[]{player,entry,stairs,-1,324};
        check(crowd.advanceToward(player,entry,stairs,stations,goals,0)==null&&crowd.cell(0)==old,"invalid and occupied goals wait");
        IndoorCrowd several=IndoorCrowd.create(floor,walls,stations,entry,stairs,player,new int[]{1,2,3,4},c(5,4));
        check(several.size()==4,"four people in open room");
        int steps=0;
        for(int tick=0;tick<600;tick++){
            int[][] targets=tick%80<40?new int[][]{{target},{nextTarget},{c(3,7)},{c(10,7)}}:new int[][]{{nextTarget},{target},{c(10,7)},{c(3,7)}};
            if(several.advanceToward(player,entry,stairs,stations,targets,tick)!=null)steps++;
            verifyRoom(several,player,entry,stairs,stations);
        }
        check(steps>20,"repeated schedule changes actually move crowd");
        IndoorCrowd scarce=IndoorCrowd.create(floor,walls,new int[][]{{target}},entry,stairs,player,new int[]{42},c(10,4));
        for(int tick=0;tick<40;tick++)scarce.advanceToward(player,entry,stairs,new int[][]{{target}},new int[][]{{target}},tick);
        check(scarce.clear(target)&&scarce.route(player,target)!=null,"resident waits rather than blocking last station standing tile");
        System.out.println("IndoorRoutine: wall detour, changed destination, stable arrivals, "+steps+" safe multi-resident steps and blocked-goal handling PASS");
    }
}
