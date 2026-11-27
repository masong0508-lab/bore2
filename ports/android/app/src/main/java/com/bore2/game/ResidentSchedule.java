package com.bore2.game;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;

/** Bounded mobile resident schedule; every identity belongs to one generated home block. */
final class ResidentSchedule {
    static final int HOME=0,WORK=1,LUNCH=2,LEISURE=3,SLEEP=4,COMMUTE=5,REACH=3,MAP=1024;
    static int pack(int x,int y){return x|(y<<16);}
    static int x(int key){return key&65535;}
    static int y(int key){return key>>>16;}
    static boolean inside(int x,int y){return x>=0&&y>=0&&x<MAP&&y<MAP;}
    static int distance(int a,int b){return Math.abs(x(a)-x(b))+Math.abs(y(a)-y(b));}
    static int mix(int v){v^=v>>>16;v*=0x7feb352d;v^=v>>>15;v*=0x846ca68b;return v^(v>>>16);}
    static int trait(int seed,int home){return mix(seed^home)&15;}
    static boolean homeLot(int lot){return lot>=1&&lot<=5;}
    static boolean workLot(int lot){switch(lot){case 6:case 7:case 8:case 9:case 14:case 20:case 21:case 25:case 27:case 29:case 30:case 31:return true;default:return false;}}
    static boolean foodLot(int lot){switch(lot){case 6:case 7:case 10:case 15:case 21:case 25:case 30:case 31:return true;default:return false;}}
    static boolean leisureLot(int lot){switch(lot){case 8:case 11:case 13:case 16:case 17:case 18:case 20:case 21:case 22:case 23:case 25:case 27:case 29:case 33:case 34:return true;default:return false;}}
    static boolean openAt(int lot,int minute){
        switch(lot){
            case 27:case 29:case 20:return minute>=480&&minute<1140;
            case 11:return minute>=540&&minute<1140;
            case 22:return minute>=960&&minute<1440;
            case 13:case 23:return minute>=660&&minute<1380;
            case 8:return minute>=360&&minute<1320;
            case 21:case 6:return minute>=420&&minute<1260;
            default:return true;
        }
    }
    static int preference(int trait,int lot){
        switch(trait){
            case 3:return lot==8||lot==11||lot==18?5:0;
            case 4:return lot==29||lot==27||lot==23?5:0;
            case 5:return lot==13||lot==34||lot==33?5:0;
            case 6:return lot==16||lot==25||lot==21?4:0;
            case 7:return lot==7||lot==21||lot==25?5:0;
            case 8:return lot==27||lot==16||lot==20?5:0;
            case 9:return lot==17||lot==22||lot==21?5:0;
            case 13:return lot==27||lot==29?6:0;
            case 14:return lot==22||lot==21||lot==23?4:0;
            case 15:return lot==16||lot==11||lot==17?5:0;
            default:return lot==16||lot==21?2:0;
        }
    }
    static final class Entry {
        final int home,location,phase;
        Entry(int home,int location,int phase){this.home=home;this.location=location;this.phase=phase;}
    }
    static final class Grid {
        final int cx,cy,radius,side;
        private final int[] cells;
        private Grid(int cx,int cy,int radius,int[] cells){this.cx=cx;this.cy=cy;this.radius=radius;side=2*radius+1;this.cells=cells.clone();}
        static Grid create(int cx,int cy,int radius,int[] cells){
            if(radius<REACH||radius>16||cx<radius||cy<radius||cx>=MAP-radius||cy>=MAP-radius||cells==null||cells.length!=(2*radius+1)*(2*radius+1))return null;
            return new Grid(cx,cy,radius,cells);
        }
        boolean covers(int X,int Y,int r){return inside(X,Y)&&Math.max(0,X-r)>=cx-radius&&Math.min(MAP-1,X+r)<=cx+radius&&Math.max(0,Y-r)>=cy-radius&&Math.min(MAP-1,Y+r)<=cy+radius;}
        int lot(int X,int Y){
            if(!covers(X,Y,0))return -1;
            int code=cells[(Y-cy+radius)*side+X-cx+radius],lot=(code>>>3)&63;
            return (code&7)==0||lot==32?-1:lot;
        }
        Entry where(int seed,int home,int day,int minute){
            int hx=x(home),hy=y(home);
            if(!covers(hx,hy,REACH)||!homeLot(lot(hx,hy)))return null;
            if(day<0||day>6||minute<0||minute>=1440||minute<420||minute>=1320)return new Entry(home,home,SLEEP);
            int t=trait(seed,home),phase=HOME;
            if(day<5&&minute>=480&&minute<1020)phase=minute>=720&&minute<780?LUNCH:WORK;
            else if((day>=5&&minute>=600&&minute<1260)||(day<5&&minute>=1020&&minute<1260)){
                if(t!=8||(mix(seed^home^day)&3)==0)phase=LEISURE;
            }
            if(phase==HOME)return new Entry(home,home,HOME);
            // Search the same bounded diamond around the home for every query.
            // Water/river blocks are deliberately excluded (conservative travel).
            int[] queue=new int[49],dist=new int[49];Arrays.fill(dist,-1);
            int head=0,tail=1,best=home,score=Integer.MIN_VALUE;queue[0]=24;dist[24]=0;
            while(head<tail){
                int at=queue[head++],u=at%7,v=at/7,d=dist[at],X=hx+u-3,Y=hy+v-3,lot=lot(X,Y);
                boolean match=phase==WORK?workLot(lot):phase==LUNCH?foodLot(lot)&&openAt(lot,minute):leisureLot(lot)&&openAt(lot,minute);
                if(d>0&&match){int key=pack(X,Y);int s=5*preference(t,lot)-2*d+(mix(seed^home^key)&7);if(s>score||(s==score&&key<best)){score=s;best=key;}}
                if(d>=REACH)continue;
                for(int k=0;k<4;k++){
                    int U=u+(k==0?1:k==2?-1:0),V=v+(k==1?1:k==3?-1:0);
                    if(U<0||V<0||U>=7||V>=7)continue;
                    int i=V*7+U;if(dist[i]>=0||lot(hx+U-3,hy+V-3)<0)continue;
                    dist[i]=d+1;queue[tail++]=i;
                }
            }
            return new Entry(home,best,best==home?HOME:phase);
        }
        Entry located(int seed,int home,int day,int minute){
            Entry now=where(seed,home,day,minute);
            if(now==null||minute<0||minute>=1440)return now;
            int boundary=NeighborhoodMotion.boundary(minute),elapsed=minute-boundary;
            if(boundary==0||elapsed>=2*REACH*NeighborhoodMotion.MINUTES_PER_BLOCK)return now;
            Entry before=where(seed,home,day,boundary-1);
            if(before==null||before.location==now.location)return now;
            int[] path=NeighborhoodMotion.route(this::lot,home,before.location,now.location);
            if(path==null||elapsed>=(path.length-1)*NeighborhoodMotion.MINUTES_PER_BLOCK)return now;
            return new Entry(home,NeighborhoodMotion.blockAt(path,elapsed),COMMUTE);
        }
        ArrayList<Entry> snapshot(int seed,int X,int Y,int radius,int day,int minute,int excluded){
            ArrayList<Entry> out=new ArrayList<>();
            if(radius<0||radius>4||!covers(X,Y,radius+2*REACH))return out;
            for(int y=Math.max(0,Y-radius-REACH);y<=Math.min(MAP-1,Y+radius+REACH);y++)for(int x=Math.max(0,X-radius-REACH);x<=Math.min(MAP-1,X+radius+REACH);x++){
                int key=pack(x,y);if(key==excluded||!homeLot(lot(x,y)))continue;
                Entry e=located(seed,key,day,minute);
                if(e!=null&&Math.abs(x(e.location)-X)<=radius&&Math.abs(y(e.location)-Y)<=radius)out.add(e);
            }
            return out;
        }
        int[] streetActors(int seed,int X,int Y,int radius,int day,int minute,int excluded,int limit){
            if(limit<=0)return new int[0];limit=Math.min(16,limit);
            ArrayList<Entry> list=snapshot(seed,X,Y,radius,day,minute,excluded);
            list.sort(Comparator.comparingInt((Entry e)->distance(e.location,pack(X,Y))).thenComparingInt(e->e.home));
            HashMap<Integer,Integer> slots=new HashMap<>();int[] out=new int[limit*3];int n=0;
            for(Entry e:list){
                if(e.phase==HOME||e.phase==SLEEP)continue;
                int slot=slots.getOrDefault(e.location,0);if(slot>=4)continue;
                slots.put(e.location,slot+1);out[n++]=x(e.location)*18+slot+1;out[n++]=y(e.location)*18+3;out[n++]=e.home;
                if(n>=out.length)break;
            }
            return Arrays.copyOf(out,n);
        }
    }
    private ResidentSchedule(){}
}
