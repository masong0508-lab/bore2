package com.bore2.game;

import java.util.Arrays;

/** Local tile routing, in absolute world tile coordinates. The native mask owns collision. */
final class OutdoorPath {
    static final int SIDE=54,N=SIDE*SIDE,WORLD=1024*18;
    private final int ox,oy;
    private final int[] cells;
    private OutdoorPath(int ox,int oy,int[] cells){this.ox=ox;this.oy=oy;this.cells=cells.clone();}
    static OutdoorPath create(int ox,int oy,int[] cells){
        return ox>=0&&oy>=0&&ox<=WORLD-SIDE&&oy<=WORLD-SIDE&&cells!=null&&cells.length==N?new OutdoorPath(ox,oy,cells):null;
    }
    static boolean valid(int x,int y){return x>=0&&y>=0&&x<WORLD&&y<WORLD;}
    static int pack(int x,int y){return x|(y<<16);}
    static int x(int q){return q&65535;}
    static int y(int q){return q>>>16;}
    static boolean adjacent(int a,int b){return valid(x(a),y(a))&&valid(x(b),y(b))&&Math.abs(x(a)-x(b))+Math.abs(y(a)-y(b))==1;}
    private int index(int q){int x=x(q)-ox,y=y(q)-oy;return x>=0&&x<SIDE&&y>=0&&y<SIDE?y*SIDE+x:-1;}
    private int cell(int i){return pack(ox+i%SIDE,oy+i/SIDE);}
    boolean open(int q){int i=index(q);return i>=0&&cells[i]==1;}
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
                if(j>=0&&cells[j]==1&&parent[j]<0){parent[j]=i;queue[tail++]=j;}
            }
        }
        if(parent[goal]<0)return null;
        int n=0;for(int i=goal;i!=start;i=parent[i])n++;
        int[] path=new int[n];for(int i=goal,k=n-1;k>=0;k--,i=parent[i])path[k]=cell(i);
        return path;
    }
}
