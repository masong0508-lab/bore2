package com.bore2.game;

import java.util.Arrays;

/** Furnished cutaway on the native lot footprint. Walls are edges, furniture occupies cells. */
final class InteriorPlan {
    static final int SIDE=18,N=SIDE*SIDE,BED=0,KITCHEN=1,HOBBY=2,PHONE=3,SOFA=4,COUNT=5;
    static final String[] LABEL={"Bed","Kitchen","Desk","Phone","Sofa"};
    static final int[] DX={1,0,-1,0},DY={0,1,0,-1};
    final boolean[] floors=new boolean[N];
    final int[] walls=new int[N],objects=new int[COUNT],occupied=new int[N];
    final int entry,lot;
    final boolean commercial;
    private final int[] actions;
    private InteriorPlan(int[] raw,boolean commercial,int[] actions){
        this.lot=raw[0];this.commercial=commercial;this.actions=actions;
        entry=raw[2]*SIDE+raw[1];Arrays.fill(objects,-1);Arrays.fill(occupied,-1);
        for(int i=0;i<N;i++)floors[i]=raw[i+3]==1;
        for(int i=0;i<N;i++)if(floor(i))for(int d=0;d<4;d++)if(!floor(neighbor(i,d)))walls[i]|=1<<d;
    }
    static InteriorPlan create(int[] raw,int seed){
        if(raw==null||raw.length!=N+3||raw[0]<1||raw[0]>5)return null;
        return layout(raw,seed,false,new int[]{BED,KITCHEN,HOBBY,PHONE,SOFA});
    }
    static InteriorPlan createVenue(int[] raw,int seed){
        if(raw==null||raw.length!=N+3||!VenueRules.supported(raw[0]))return null;
        return layout(raw,seed,true,VenueRules.actions(raw[0]));
    }
    int actionCount(){return actions.length;}
    int actionAt(int slot){return slot>=0&&slot<actions.length?actions[slot]:-1;}
    int slotFor(int action){for(int i=0;i<actions.length;i++)if(actions[i]==action)return i;return -1;}
    String labelAt(int slot){int action=actionAt(slot);return action<0?"":commercial?VenueRules.label(lot,action):LABEL[action];}
    private static InteriorPlan layout(int[] raw,int seed,boolean commercial,int[] actions){
        if(!inside(raw[1],raw[2]))return null;
        InteriorPlan p=new InteriorPlan(raw,commercial,actions);
        if(!p.walkable(p.entry)||!p.connected())return null;
        int lo=SIDE,hi=-1,top=SIDE,bottom=-1;
        for(int i=0;i<N;i++)if(p.floor(i)){lo=Math.min(lo,i%SIDE);hi=Math.max(hi,i%SIDE);top=Math.min(top,i/SIDE);bottom=Math.max(bottom,i/SIDE);}
        // Divide a large plan into two rooms with a passage, but keep irregular
        // wings connected. The generated footprint and entry never change.
        if(hi-lo>=5&&bottom-top>=4){
            int split=(lo+hi)/2,gap=-1,best=Integer.MAX_VALUE;
            for(int y=top;y<=bottom;y++)if(p.floor(y*SIDE+split)&&p.floor(y*SIDE+split+1)){
                int d=Math.abs(y-p.entry/SIDE);if(d<best){best=d;gap=y;}
            }
            if(gap>=0){int[] original=p.walls.clone();for(int y=top;y<=bottom;y++)if(y!=gap){
                int a=y*SIDE+split,b=a+1;if(p.floor(a)&&p.floor(b)){p.walls[a]|=1;p.walls[b]|=4;}
            }if(!p.connected())System.arraycopy(original,0,p.walls,0,N);}
        }
        int[][] anchors={{lo,top},{hi,bottom},{hi,top},{lo,bottom},{(lo+hi)/2,(top+bottom)/2}};
        if((seed&1)!=0)for(int[] anchor:anchors)anchor[0]=lo+hi-anchor[0];
        for(int kind=0;kind<p.actionCount();kind++){
            int choice=-1,score=Integer.MAX_VALUE;
            for(int i=0;i<N;i++)if(i!=p.entry&&p.walkable(i)){
                int s=16*(Math.abs(i%SIDE-anchors[kind][0])+Math.abs(i/SIDE-anchors[kind][1]))+((i^seed)&7);
                if(s>=score)continue;
                p.occupied[i]=kind;p.objects[kind]=i;
                boolean ok=p.connected()&&p.accessibleObjects();
                p.occupied[i]=-1;p.objects[kind]=-1;
                if(ok){choice=i;score=s;}
            }
            if(choice<0)return null;
            p.objects[kind]=choice;p.occupied[choice]=kind;
        }
        return p;
    }
    static boolean inside(int x,int y){return x>=0&&y>=0&&x<SIDE&&y<SIDE;}
    static int neighbor(int i,int d){if(i<0||i>=N||d<0||d>=4)return -1;int x=i%SIDE+DX[d],y=i/SIDE+DY[d];return inside(x,y)?y*SIDE+x:-1;}
    boolean floor(int i){return i>=0&&i<N&&floors[i];}
    boolean walkable(int i){return floor(i)&&occupied[i]<0;}
    private boolean clearEdge(int a,int b){if(!floor(a)||!floor(b))return false;for(int d=0;d<4;d++)if(neighbor(a,d)==b)return (walls[a]&(1<<d))==0&&(walls[b]&(1<<((d+2)&3)))==0;return false;}
    boolean canStep(int a,int b){return walkable(a)&&walkable(b)&&clearEdge(a,b);}
    boolean canUse(int at,int kind){return kind>=0&&kind<COUNT&&walkable(at)&&clearEdge(at,objects[kind]);}
    private int[] search(int start){
        int[] parent=new int[N],queue=new int[N];Arrays.fill(parent,-1);
        if(!walkable(start))return parent;
        int head=0,tail=0;parent[start]=start;queue[tail++]=start;
        while(head<tail){int at=queue[head++];for(int d=0;d<4;d++){int next=neighbor(at,d);if(next>=0&&parent[next]<0&&canStep(at,next)){parent[next]=at;queue[tail++]=next;}}}
        return parent;
    }
    private boolean connected(){int[] parent=search(entry);for(int i=0;i<N;i++)if(walkable(i)&&parent[i]<0)return false;return true;}
    private boolean accessibleObjects(){int[] parent=search(entry);for(int k=0;k<COUNT;k++)if(objects[k]>=0){boolean ok=false;for(int d=0;d<4;d++){int a=neighbor(objects[k],d);if(a>=0&&parent[a]>=0&&canUse(a,k))ok=true;}if(!ok)return false;}return true;}
    private int[] reconstruct(int start,int goal,int[] parent){if(goal<0||goal>=N||parent[goal]<0)return null;int n=0;for(int i=goal;i!=start;i=parent[i])n++;int[] route=new int[n];for(int i=goal,k=n-1;k>=0;k--,i=parent[i])route[k]=i;return route;}
    int[] route(int start,int goal){return reconstruct(start,goal,search(start));}
    int[] routeToObject(int start,int kind){
        if(kind<0||kind>=COUNT||objects[kind]<0)return null;
        int[] parent=search(start),best=null;
        for(int d=0;d<4;d++){int at=neighbor(objects[kind],d);if(!canUse(at,kind))continue;int[] route=reconstruct(start,at,parent);if(route!=null&&(best==null||route.length<best.length))best=route;}
        return best;
    }
}
