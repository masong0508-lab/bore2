package com.bore2.game;
import java.util.Arrays;
public final class RoommateScheduleTest {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    static void put(int[] a,int x,int y,int lot){a[(y-90)*21+x-90]=2|(lot<<3);}
    public static void main(String[] args){
        int[] a=new int[441];Arrays.fill(a,2);
        put(a,100,100,4);put(a,103,100,1);put(a,100,99,9);put(a,105,100,9);put(a,100,101,16);put(a,103,101,16);
        int home=ResidentSchedule.pack(100,100),id=ResidentSchedule.pack(103,100);
        ResidentSchedule.Grid g=ResidentSchedule.Grid.create(100,100,10,a);
        check(g.located(1234,id,0,450).location==id,"original residence before moving");
        check(g.locatedAt(1234,id,home,0,450).location==home,"new residence after moving");
        check(g.locatedAt(1234,id,home,0,450).home==id,"same person after moving");
        for(int minute:new int[]{0,450,480,486,540,720,780,1020,1100,1300,1439}){
            ResidentSchedule.Entry expected=g.locatedAt(1234,id,home,0,minute);int count=0;
            for(int y=97;y<=103;y++)for(int x=97;x<=103;x++){
                for(ResidentSchedule.Entry e:g.snapshot(1234,x,y,0,0,minute,home,id)){
                    check(e.home!=home,"vacant leased-home proxy suppressed");
                    if(e.home==id){count++;check(e.location==expected.location,"new address reflected in local snapshot");}
                }
            }
            check(count==1,"roommate never duplicated");
        }
        check(g.locatedAt(1234,id,home,0,480).phase==ResidentSchedule.COMMUTE,"commute from shared home");
        check(g.locatedAt(1234,id,home,0,540).location==ResidentSchedule.pack(100,99),"work near new home not old neighborhood");
        check(g.snapshot(1234,103,100,0,0,450,-1).stream().anyMatch(e->e.home==id),"move-out restores original schedule");
        System.out.println("Roommate schedule: same identity, new residence, commuting, no duplicate and move-out PASS");
    }
}
