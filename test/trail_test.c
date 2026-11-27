#include <assert.h>
#include <stdio.h>
#include "trail_plan.h"
static unsigned mix(unsigned x){x^=x>>16;x*=0x7feb352du;x^=x>>15;return x;}
int main(void){
    int plans=0;
    assert(!bore_trail_join(0,383,1));assert(bore_trail_join(383,384,1));
    assert(!bore_trail_join(384,3000,1));assert(!bore_trail_join(384,384,0));
    for(unsigned seed=1;seed<=1024;seed++){
        uint32_t edge[4];for(int d=0;d<4;d++)edge[d]=mix(seed*31u+d);
        for(unsigned mask=0;mask<16;mask++){
            BoreTrail p;bore_trail_build(&p,mask,TRAIL_WALK,mix(seed),edge);
            assert(p.kind==(mask?TRAIL_WALK:TRAIL_NONE));
            int n=0,head=0,q[324],seen[324]={0};
            if(mask){int k=p.cy*18+p.cx;q[n++]=k;seen[k]=1;}
            while(head<n){int k=q[head++],x=k%18,y=k/18;
                const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
                for(int d=0;d<4;d++){int a=x+dx[d],b=y+dy[d];
                    if(!bore_trail_cell(&p,a,b))continue;
                    int v=b*18+a;if(!seen[v]){seen[v]=1;q[n++]=v;}
                }
            }
            for(int y=0;y<18;y++)for(int x=0;x<18;x++)assert(seen[y*18+x]==bore_trail_cell(&p,x,y));
            for(int d=0;d<4;d++){
                unsigned e[4]={0};e[(d+2)&3]=edge[d];BoreTrail other;
                bore_trail_build(&other,(mask&(1u<<d))?1u<<((d+2)&3):0,TRAIL_WALK,mix(seed+1),e);
                for(int u=0;u<18;u++){
                    int a=bore_trail_cell(&p,d==1?17:d==3?0:u,d==0?0:d==2?17:u);
                    int b=bore_trail_cell(&other,d==1?0:d==3?17:u,d==0?17:d==2?0:u);
                    assert(a==b);
                }
                int a=(int)(seed%6145u),b=(int)(mix(seed)%6145u);
                assert(bore_trail_join(a,b,edge[d])==bore_trail_join(b,a,edge[d]));
            }
            plans++;
        }
    }
    printf("trail plan: %d connected layouts and matching shared boundaries PASS\n",plans);
}
