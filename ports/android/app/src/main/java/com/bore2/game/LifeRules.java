package com.bore2.game;

import java.util.ArrayList;
import java.util.Comparator;

/** Platform-neutral mobile life rules; lot IDs match src/bigmap.h. */
final class LifeRules {
    static final int HOMES=0, WORK=1, FOOD=2, VENUES=3, TALL=4, HIGHLANDS=5;
    static final int LEASE=75, DAILY_RENT=12, LIMIT=1_000_000;
    static boolean isHome(int lot){return lot>=1&&lot<=5;}
    static boolean isWork(int lot){switch(lot){case 6:case 7:case 8:case 9:case 14:case 20:case 21:case 25:case 27:case 29:case 30:case 31:return true;default:return false;}}
    static boolean isFood(int lot){switch(lot){case 6:case 7:case 10:case 15:case 21:case 25:case 30:case 31:return true;default:return false;}}
    static boolean isVenue(int lot){switch(lot){case 8:case 10:case 11:case 12:case 13:case 15:case 16:case 20:case 21:case 23:case 27:case 28:case 29:case 33:case 34:return true;default:return false;}}
    static boolean inside(int x,int y){return x>=0&&x<1024&&y>=0&&y<1024;}
    static int pack(int x,int y){return x|(y<<16);}
    static int x(int pos){return pos&65535;}
    static int y(int pos){return pos>>>16;}
    static int distance(int x,int y,int X,int Y){return Math.abs(x-X)+Math.abs(y-Y);}
    static boolean validRoute(int x,int y,int X,int Y,int[] path){
        if(path==null||path.length>1089||!inside(x,y)||!inside(X,Y))return false;
        for(int p:path){int nx=x(p),ny=y(p);if(!inside(nx,ny)||distance(x,y,nx,ny)!=1)return false;x=nx;y=ny;}
        return x==X&&y==Y;
    }
    static int travelMinutes(int n){return Math.max(0,Math.min(1089,n))*3;}
    static int travelEnergy(int n){return (Math.max(0,Math.min(1089,n))+3)/4;}
    static int credit(int funds,int amount){return (int)Math.max(0,Math.min(LIMIT,(long)funds+amount));}
    static int[] rent(int funds,int debt,int days,boolean leased){
        long bill=Math.max(0,Math.min(LIMIT,(long)debt+(leased?(long)Math.max(0,days)*DAILY_RENT:0)));
        int cash=Math.max(0,Math.min(LIMIT,funds)),paid=(int)Math.min(bill,cash);
        return new int[]{cash-paid,(int)bill-paid};
    }
    static int elapsedBuckets(int minute,int elapsed,int interval){return (minute+elapsed)/interval-minute/interval;}
    static int payBonus(int shifts){return Math.min(50,Math.max(0,shifts)/4*10);}
    private static boolean matches(int lot,int kind){
        switch(kind){case HOMES:return isHome(lot);case WORK:return isWork(lot);case FOOD:return isFood(lot);case VENUES:return isVenue(lot);default:return false;}
    }
    static int[] nearby(int[] tiles,int cx,int cy,int radius,int px,int py,int kind,int max){
        if(radius<0||radius>16||max<=0||tiles==null)return new int[0];
        int side=2*radius+1;if(tiles.length!=side*side)return new int[0];
        ArrayList<Integer> candidates=new ArrayList<>();
        for(int row=0;row<side;row++)for(int col=0;col<side;col++){
            int code=tiles[row*side+col],x=cx-radius+col,y=cy-radius+row;
            if(inside(x,y)&&(code&7)!=0&&(kind==TALL?(((code>>>3)&63)==14||((code>>>3)&63)==15)&&((code>>>17)&7)>1:kind==HIGHLANDS?((code&7)==5&&((code>>>3)&63)==0):matches((code>>>3)&63,kind)))candidates.add(pack(x,y));
        }
        candidates.sort(Comparator.comparingInt((Integer p)->distance(px,py,x(p),y(p))).thenComparingInt(p->p));
        int[] out=new int[Math.min(max,candidates.size())];for(int i=0;i<out.length;i++)out[i]=candidates.get(i);
        return out;
    }
    private LifeRules(){}
}
