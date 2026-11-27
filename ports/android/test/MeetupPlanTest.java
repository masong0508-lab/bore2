package com.bore2.game;
import java.util.Arrays;
public final class MeetupPlanTest {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    static int pack(int x,int y){return x|(y<<16);}
    static void put(int[] a,int x,int y,int lot){a[(y-90)*21+x-90]=2|(lot<<3);}
    public static void main(String[] args){
        int[] a=new int[441];Arrays.fill(a,2);
        int resident=pack(100,100), park=pack(100,101),office=pack(101,100);
        put(a,100,100,1);put(a,100,101,16);put(a,101,100,9);
        ResidentSchedule.Grid grid=ResidentSchedule.Grid.create(100,100,10,a);
        MeetupPlan.Schedule schedule=(day,minute)->grid.located(1234,resident,day%7,minute);
        check(MeetupPlan.find(resident,9,0,600,schedule,k->k==park?16:9,k->3)==null,"stranger cannot arrange a meeting");
        MeetupPlan p=MeetupPlan.find(resident,30,0,600,schedule,k->k==park?16:9,k->3);
        check(p!=null&&p.contact==resident&&p.place==park&&p.day==0&&p.start>=1026,"meet at actual leisure destination after commute");
        check(p.state(0,p.start-1)==MeetupPlan.UPCOMING&&p.state(0,p.start)==MeetupPlan.READY&&p.state(0,p.start+31)==MeetupPlan.MISSED,"arrival window");
        check(!p.completeAt(resident,office,0,p.start)&&!p.completeAt(resident+1,park,0,p.start),"wrong lot or person cannot complete");
        check(p.completeAt(resident,park,0,p.start+30),"arrive on time at correct lot");
        check(MeetupPlan.find(resident,30,0,600,schedule,k->k==park?16:9,k->-1)==null,"no route, no meetup");
        MeetupPlan later=MeetupPlan.find(resident,30,0,1300,schedule,k->k==park?16:9,k->3);
        check(later!=null&&later.day==1&&later.start>=1026,"rollover finds tomorrow's real leisure window");
        check(MeetupPlan.find(resident,30,0,600,(d,m)->new ResidentSchedule.Entry(resident,park,ResidentSchedule.WORK),k->16,k->3)==null,"no booking during work");
        check(MeetupPlan.find(resident,30,0,600,(d,m)->new ResidentSchedule.Entry(resident,park,m%60==0?ResidentSchedule.COMMUTE:ResidentSchedule.LEISURE),k->16,k->3)==null,"full duration checked, not just start and end");
        String encoded=p.save(1234);MeetupPlan loaded=MeetupPlan.load(encoded,1234,0,600,k->k==resident?30:0,k->k==park?16:9);
        check(loaded!=null&&loaded.contact==resident&&loaded.place==park&&loaded.start==p.start,"saved appointment retains identity and window");
        check(MeetupPlan.load(encoded,99,0,600,k->30,k->16)==null,"different world rejected");
        check(MeetupPlan.load(encoded,1234,0,600,k->0,k->16)==null,"removed contact rejected");
        check(MeetupPlan.load(encoded,1234,0,600,k->30,k->1)==null,"private home not valid meetup venue");
        check(MeetupPlan.load(encoded,1234,1,0,k->30,k->16)==null,"expired plans not revived");
        System.out.println("MeetupPlan: actual schedule, 60-minute availability, arrival window, travel, save validation PASS");
    }
}
