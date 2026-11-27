package com.bore2.game;

import java.util.Arrays;

/** Bounded indoor residents. Placement never sacrifices access to the room. */
final class IndoorCrowd {
    private static final int SIDE=18,N=SIDE*SIDE,MAX=4;
    private final boolean[] floor,blocked=new boolean[N];
    private final int[] walls,ids=new int[MAX],cells=new int[MAX];
    private int size;
    private IndoorCrowd(boolean[] floor,int[] walls){this.floor=floor.clone();this.walls=walls.clone();}
    static IndoorCrowd create(boolean[] floor,int[] walls,int[][] stations,int entry,int stairs,int player,int[] people,int preferred){
        if(floor==null||walls==null||floor.length!=N||walls.length!=N)throw new IllegalArgumentException("room dimensions");
        IndoorCrowd room=new IndoorCrowd(floor,walls);
        if(people==null||stations==null||!room.clear(player)||!room.clear(entry))return room;
        for(int id:people){
            if(room.size==MAX)break;
            if(id<0||room.index(id)>=0)continue;
            boolean[] tried=new boolean[N];
            for(int attempt=0;attempt<N;attempt++){
                int best=-1,score=Integer.MAX_VALUE;
                for(int at=0;at<N;at++)if(!tried[at]&&room.clear(at)&&at!=entry&&at!=stairs&&at!=player){
                    int distance=preferred>=0&&preferred<N?Math.abs(at%SIDE-preferred%SIDE)+Math.abs(at/SIDE-preferred/SIDE):0;
                    int s=distance*16+((at*31^id*17)&15);
                    if(s<score){score=s;best=at;}
                }
                if(best<0)break;
                tried[best]=true;room.blocked[best]=true;
                if(room.safe(player,entry,stairs,stations)){
                    room.ids[room.size]=id;room.cells[room.size++]=best;break;
                }
                room.blocked[best]=false;
            }
        }
        return room;
    }
    static final class Step {
        final int index,from,to;
        Step(int index,int from,int to){this.index=index;this.from=from;this.to=to;}
    }
    // Only one resident moves per tick. Validate the entire post-step room
    // before committing occupancy; failed candidates leave everything intact.
    Step advance(int player,int entry,int stairs,int[][] stations,int tick){
        if(size==0||!clear(player)||!clear(entry)||stations==null)return null;
        int first=Math.floorMod(tick,size);
        for(int n=0;n<size;n++){
            int i=(first+n)%size,from=cells[i],hash=(ids[i]*31)^(tick*0x9e3779b9);
            int direction=(hash^(hash>>>16))&3;
            for(int d=0;d<4;d++){
                int to=neighbor(from,(direction+d)&3);
                if(!clear(to)||to==player||to==entry||to==stairs||!edge(from,to))continue;
                blocked[from]=false;blocked[to]=true;
                if(safe(player,entry,stairs,stations)){cells[i]=to;return new Step(i,from,to);}
                blocked[to]=false;blocked[from]=true;
            }
        }
        return null;
    }
    int size(){return size;}
    int id(int i){return i>=0&&i<size?ids[i]:-1;}
    int cell(int i){return i>=0&&i<size?cells[i]:-1;}
    private int index(int id){for(int i=0;i<size;i++)if(ids[i]==id)return i;return -1;}
    int personAt(int at){for(int i=0;i<size;i++)if(cells[i]==at)return ids[i];return -1;}
    boolean clear(int at){return at>=0&&at<N&&floor[at]&&!blocked[at];}
    private static int neighbor(int at,int d){
        if(at<0||at>=N)return -1;
        int x=at%SIDE,y=at/SIDE;
        x+=d==0?1:d==2?-1:0;y+=d==1?1:d==3?-1:0;
        return x>=0&&x<SIDE&&y>=0&&y<SIDE?y*SIDE+x:-1;
    }
    private boolean edge(int a,int b){
        if(a<0||a>=N||b<0||b>=N||!floor[a]||!floor[b])return false;
        for(int d=0;d<4;d++)if(neighbor(a,d)==b)return (walls[a]&(1<<d))==0&&(walls[b]&(1<<((d+2)&3)))==0;
        return false;
    }
    boolean canStep(int a,int b){return clear(a)&&clear(b)&&edge(a,b);}
    private int[] search(int start){
        int[] parent=new int[N],queue=new int[N];Arrays.fill(parent,-1);
        if(!clear(start))return parent;
        int head=0,tail=0;parent[start]=start;queue[tail++]=start;
        while(head<tail){int at=queue[head++];for(int d=0;d<4;d++){
            int next=neighbor(at,d);if(next>=0&&parent[next]<0&&canStep(at,next)){parent[next]=at;queue[tail++]=next;}
        }}return parent;
    }
    private boolean safe(int player,int entry,int stairs,int[][] stations){
        int[] parent=search(player);
        if(parent[entry]<0||(stairs>=0&&(stairs>=N||parent[stairs]<0)))return false;
        for(int i=0;i<N;i++)if(clear(i)&&parent[i]<0)return false;
        for(int[] station:stations){boolean ok=false;if(station!=null)for(int at:station)if(at>=0&&at<N&&parent[at]>=0){ok=true;break;}if(!ok)return false;}
        // Everyone must still have a reachable standing tile next to them.
        for(int i=0;i<N;i++)if(blocked[i]){boolean ok=false;for(int d=0;d<4;d++){int at=neighbor(i,d);if(at>=0&&parent[at]>=0&&edge(at,i)){ok=true;break;}}if(!ok)return false;}
        return true;
    }
    private int[] path(int start,int goal,int[] parent){
        if(goal<0||goal>=N||parent[goal]<0)return null;
        int n=0;for(int i=goal;i!=start;i=parent[i])n++;
        int[] out=new int[n];for(int i=goal,k=n-1;k>=0;i=parent[i],k--)out[k]=i;
        return out;
    }
    int[] route(int start,int goal){return path(start,goal,search(start));}
    int[] routeToAny(int start,int[] goals){
        if(goals==null)return null;
        int[] parent=search(start),best=null;
        for(int goal:goals){int[] route=path(start,goal,parent);if(route!=null&&(best==null||route.length<best.length))best=route;}
        return best;
    }
    boolean adjacent(int at,int id){int i=index(id);return i>=0&&clear(at)&&edge(at,cells[i]);}
    int[] routeToPerson(int start,int id){
        int i=index(id);if(i<0)return null;
        int[] goals=new int[4];for(int d=0;d<4;d++){int at=neighbor(cells[i],d);goals[d]=adjacent(at,id)?at:-1;}
        return routeToAny(start,goals);
    }
}
