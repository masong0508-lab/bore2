package com.bore2.game;

import java.util.Arrays;

/** Local tile routing, in absolute world tile coordinates. The native mask owns collision. */
final class OutdoorPath {
    static final int SIDE=54,N=SIDE*SIDE,WORLD=1024*18;
    private final int ox,oy;
    private final int[] cells,heights;
    private OutdoorPath(int ox,int oy,int[] cells,int[] heights){
        this.ox=ox;this.oy=oy;this.cells=cells.clone();this.heights=heights.clone();
    }
    static OutdoorPath create(int ox,int oy,int[] cells){return create(ox,oy,cells,new int[N]);}
    static OutdoorPath create(int ox,int oy,int[] cells,int[] heights){
        if(ox<0||oy<0||ox>WORLD-SIDE||oy>WORLD-SIDE||cells==null||cells.length!=N||heights==null||heights.length!=N)return null;
        for(int h:heights)if(h<0||h>6144)return null;
        return new OutdoorPath(ox,oy,cells,heights);
    }
    static boolean valid(int x,int y){return x>=0&&y>=0&&x<WORLD&&y<WORLD;}
    static int pack(int x,int y){return x|(y<<16);}
    static int x(int q){return q&65535;}
    static int y(int q){return q>>>16;}
    static boolean adjacent(int a,int b){return valid(x(a),y(a))&&valid(x(b),y(b))&&Math.abs(x(a)-x(b))+Math.abs(y(a)-y(b))==1;}
    private int index(int q){int x=x(q)-ox,y=y(q)-oy;return x>=0&&x<SIDE&&y>=0&&y<SIDE?y*SIDE+x:-1;}
    private int cell(int i){return pack(ox+i%SIDE,oy+i/SIDE);}
    boolean open(int q){int i=index(q);return i>=0&&cells[i]==1;}
    private int grade(int a,int b){return Math.abs(heights[a]-heights[b])*100;}
    private boolean pass(int a,int b){return a>=0&&b>=0&&cells[a]==1&&cells[b]==1&&grade(a,b)<=43*256;}
    boolean canStep(int a,int b){return adjacent(a,b)&&pass(index(a),index(b));}
    int duration(int a,int b,int baseMs){
        if(!canStep(a,b)||baseMs<=0)return 0;
        int grade=grade(index(a),index(b)),pace=grade<=10*256?4:grade<=25*256?3:grade<=40*256?2:1;
        return (int)Math.min(Integer.MAX_VALUE,((long)baseMs*4+pace-1)/pace);
    }
    int nearest(int q){
        if(open(q))return q;
        int best=-1,d=Integer.MAX_VALUE;
        for(int i=0;i<N;i++)if(cells[i]==1){int p=cell(i),n=Math.abs(x(q)-x(p))+Math.abs(y(q)-y(p));if(n<d){best=p;d=n;}}
        return best;
    }
    int[] route(int from,int to){
        if(!open(from)||!open(to))return null;
        int start=index(from),goal=index(to),head=0,tail=0;
        int[] parent=new int[N],queue=new int[N];Arrays.fill(parent,-1);
        parent[start]=start;queue[tail++]=start;
        while(head<tail&&parent[goal]<0){
            int i=queue[head++],x=i%SIDE,y=i/SIDE;
            for(int d=0;d<4;d++){
                int j=d==0?(x+1<SIDE?i+1:-1):d==1?(y+1<SIDE?i+SIDE:-1):d==2?(x>0?i-1:-1):(y>0?i-SIDE:-1);
                if(j>=0&&parent[j]<0&&pass(i,j)){parent[j]=i;queue[tail++]=j;}
            }
        }
        if(parent[goal]<0)return null;
        int n=0;for(int i=goal;i!=start;i=parent[i])n++;
        int[] path=new int[n];for(int i=goal,k=n-1;k>=0;k--,i=parent[i])path[k]=cell(i);
        return path;
    }
}
