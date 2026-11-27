package com.bore2.game;
import java.util.Arrays;
public final class NeighborhoodMotionTest {
    static void check(boolean b,String m){if(!b)throw new AssertionError(m);}
    public static void main(String[] args){
        int home=NeighborhoodMotion.pack(100,100),from=NeighborhoodMotion.pack(98,100),to=NeighborhoodMotion.pack(102,100);
        int[] path=NeighborhoodMotion.route((x,y)->x==100&&y==100? -1:0,home,from,to);
        check(path!=null&&path.length==7,"six-step detour around obstacle");
        check(path[0]==from&&path[path.length-1]==to,"endpoints");
        for(int i=1;i<path.length;i++)check(NeighborhoodMotion.distance(path[i-1],path[i])==1&&path[i]!=home,"cardinal clear route");
        check(NeighborhoodMotion.route((x,y)->x==100?-1:0,home,from,to)==null,"sealed barrier");
        check(NeighborhoodMotion.route((x,y)->0,home,from,NeighborhoodMotion.pack(104,100))==null,"bounded home area");
        check(NeighborhoodMotion.blockAt(path,0)==from&&NeighborhoodMotion.blockAt(path,6)==path[1]&&NeighborhoodMotion.blockAt(path,999)==to,"progress follows time");
        check(NeighborhoodMotion.boundary(479)==420&&NeighborhoodMotion.boundary(480)==480&&NeighborhoodMotion.boundary(1031)==1020,"schedule boundaries");
        for(int id=0;id<40;id++)for(long t=0;t<16000;t+=37){float a=NeighborhoodMotion.pace(id,t);check(a>=0&&a<=3&&Float.isFinite(a),"bounded lane motion");check(Math.abs(a-NeighborhoodMotion.pace(id,t+1))<.01f,"continuous motion");}
        check(NeighborhoodMotion.pace(9,0)!=NeighborhoodMotion.pace(9,700),"actually moves");
        float[] night=NeighborhoodMotion.light(0),noon=NeighborhoodMotion.light(720),dawn=NeighborhoodMotion.light(360);
        check(Arrays.equals(night,NeighborhoodMotion.light(1440)),"wrap midnight");
        check(night[3]<dawn[3]&&dawn[3]<noon[3]&&noon[3]==1,"smooth day light");
        for(int i=0;i<1440;i++){float[] a=NeighborhoodMotion.light(i),b=NeighborhoodMotion.light(i+1);for(int j=0;j<6;j++)check(a[j]>=0&&a[j]<=1&&Math.abs(a[j]-b[j])<.03,"bounded continuous lighting");}
        System.out.println("NeighborhoodMotion: commute, bounded animation, day/night PASS");
    }
}
