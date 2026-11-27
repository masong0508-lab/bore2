package com.bore2.game;
import java.util.Arrays;
public final class ResidentScheduleTest {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    static int code(int lot){return 2|(lot<<3);}
    static void put(int[] a,int x,int y,int lot){a[(y-90)*21+x-90]=code(lot);}
    public static void main(String[] args){
        int[] a=new int[441];Arrays.fill(a,code(0));
        put(a,100,100,1);put(a,101,100,9);put(a,100,101,21);put(a,99,100,16);
        put(a,102,100,1);put(a,98,100,1);put(a,100,102,1);
        ResidentSchedule.Grid g=ResidentSchedule.Grid.create(100,100,10,a);
        int home=ResidentSchedule.pack(100,100);
        check(g!=null,"valid grid");
        check(g.where(1234,home,0,120).location==home,"sleep at home");
        ResidentSchedule.Entry morning=g.where(1234,home,0,540),afternoon=g.where(1234,home,0,840);
        check(morning.phase==ResidentSchedule.WORK&&morning.location!=home&&morning.location==afternoon.location,"stable work location");
        check(g.where(1234,home,0,740).phase==ResidentSchedule.LUNCH,"lunch schedule");
        ResidentSchedule.Entry depart=g.located(1234,home,0,480);
        check(depart.phase==ResidentSchedule.COMMUTE&&depart.location==home,"commute starts at home");
        check(g.located(1234,home,0,516).location==morning.location,"commute reaches work");
        for(int minute=480;minute<516;minute++){
            ResidentSchedule.Entry moving=g.located(1234,home,0,minute);
            check(g.lot(ResidentSchedule.x(moving.location),ResidentSchedule.y(moving.location))>=0,"commute uses land");
            int n=0;for(int y=97;y<=103;y++)for(int x=97;x<=103;x++)
                for(ResidentSchedule.Entry e:g.snapshot(1234,x,y,0,0,minute,-1))if(e.home==home)n++;
            check(n==1,"commuter occupies exactly one block");
        }
        check(g.where(1234,home,6,600).phase!=ResidentSchedule.WORK,"weekend off");
        int count=0;for(int y=97;y<=103;y++)for(int x=97;x<=103;x++)for(ResidentSchedule.Entry e:g.snapshot(1234,x,y,0,0,540,-1))if(e.home==home)count++;
        check(count==1,"one resident, exactly one destination across different camera centers");
        for(ResidentSchedule.Entry e:g.snapshot(1234,100,100,3,0,540,home))check(e.home!=home,"leased home excluded");
        int[] actors=g.streetActors(1234,100,100,2,0,540,-1,16);
        check(actors.length>0&&actors.length%3==0&&actors.length<=48,"bounded street residents");
        for(int i=0;i<actors.length;i+=3){check(actors[i]%18>=1&&actors[i]%18<=4&&actors[i+1]%18==3,"outside building strip");for(int j=0;j<i;j+=3)check(actors[i]!=actors[j]||actors[i+1]!=actors[j+1],"distinct street cells");}
        check(Arrays.equals(actors,g.streetActors(1234,100,100,2,0,540,-1,16)),"stable render snapshot");
        int[] barrier=new int[441];Arrays.fill(barrier,0);put(barrier,100,100,1);put(barrier,102,100,9);
        ResidentSchedule.Grid blocked=ResidentSchedule.Grid.create(100,100,10,barrier);
        check(blocked.where(1234,home,0,540).location==home,"no movement across isolated water");
        check(ResidentSchedule.Grid.create(100,100,10,new int[1])==null,"invalid grid");
        System.out.println("Resident schedules: unique location, daily rhythm, safe street slots, water barriers and stable snapshots PASS");
    }
}
