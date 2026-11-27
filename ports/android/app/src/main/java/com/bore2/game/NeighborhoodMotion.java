package com.bore2.game;

import java.util.Arrays;
import java.util.function.IntBinaryOperator;

/** Deterministic game-clock commutes and bounded exterior animation. */
final class NeighborhoodMotion {
    static final int REACH=3, MINUTES_PER_BLOCK=6;
    private static final int[] BOUNDARIES={0,420,480,540,600,660,720,780,960,1020,1140,1260,1320,1380};
    static int pack(int x,int y){return x|(y<<16);}
    static int x(int p){return p&65535;}
    static int y(int p){return p>>>16;}
    static int distance(int a,int b){return Math.abs(x(a)-x(b))+Math.abs(y(a)-y(b));}
    private static boolean inside(int x,int y){return x>=0&&y>=0&&x<1024&&y<1024;}
    static int boundary(int minute){int b=0;for(int t:BOUNDARIES){if(t>minute)break;b=t;}return b;}
    static int[] route(IntBinaryOperator lot,int home,int from,int to){
        int hx=x(home),hy=y(home);
        if(!inside(hx,hy)||!inside(x(from),y(from))||!inside(x(to),y(to))||distance(home,from)>REACH||distance(home,to)>REACH||lot.applyAsInt(x(from),y(from))<0||lot.applyAsInt(x(to),y(to))<0)return null;
        int start=(y(from)-hy+3)*7+x(from)-hx+3,end=(y(to)-hy+3)*7+x(to)-hx+3;
        int[] parent=new int[49],queue=new int[49];Arrays.fill(parent,-1);
        int head=0,tail=1;queue[0]=start;parent[start]=start;
        while(head<tail&&parent[end]<0){int at=queue[head++],u=at%7,v=at/7;
            for(int d=0;d<4;d++){
                int U=u+(d==0?1:d==2?-1:0),V=v+(d==1?1:d==3?-1:0);
                if(U<0||V<0||U>=7||V>=7||Math.abs(U-3)+Math.abs(V-3)>REACH)continue;
                int next=V*7+U,X=hx+U-3,Y=hy+V-3;
                if(parent[next]<0&&inside(X,Y)&&lot.applyAsInt(X,Y)>=0){parent[next]=at;queue[tail++]=next;}
            }
        }
        if(parent[end]<0)return null;
        int n=1;for(int i=end;i!=start;i=parent[i])n++;
        int[] path=new int[n];for(int i=end,k=n-1;k>=0;k--,i=parent[i])path[k]=pack(hx+i%7-3,hy+i/7-3);
        return path;
    }
    static int blockAt(int[] path,int elapsed){return path[Math.min(path.length-1,Math.max(0,elapsed)/MINUTES_PER_BLOCK)];}
    // Each resident has a separate x lane, and moves only along the four
    // exterior tile centers in that lane. Time is monotonic, no frame count.
    static float pace(int id,long ms){long offset=((long)id*137)&4095;float t=Math.floorMod(ms+offset,6000L)/1000f;return t<=3?t:6-t;}
    static float facing(int id,long ms){long offset=((long)id*137)&4095;return Math.floorMod(ms+offset,6000L)<3000?0f:3.1415927f;}
    static float swing(int id,long ms){return (float)Math.sin((Math.floorMod(ms,1000L)+Math.floorMod(id,11)*73)*Math.PI/500)*.42f;}
    static float[] light(int minute){
        int m=Math.floorMod(minute,1440);
        float day=Math.max(0f,Math.min(1f,Math.min((m-300)/180f,(1260-m)/180f)));
        float t=day*day*(3-2*day),warm=(float)Math.sin(Math.PI*t);
        return new float[]{.15f+.53f*t+.08f*warm,.20f+.57f*t,.31f+.49f*t-.05f*warm,.48f+.52f*t,.53f+.47f*t,.67f+.33f*t-.08f*warm};
    }
    private NeighborhoodMotion(){}
}
