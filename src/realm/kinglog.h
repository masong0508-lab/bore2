// kinglog.h - EP6 THE LAND BOREFORE TIME, Phase 3 step b: HAULING. Carriers (free Nubbins) move goods along the player's roads.
// Needs kingdom.h and kingsim.h before it. Pure C, tested on a PC (tools/test_kingsim.c). Every function is static inline.
//
// HOW IT WORKS
//  - A building that WANTS a good (a blueprint its cost, a processor its input) asks for a DISTANCE MAP: how many road steps every road cell is from
//    the building (all its road-side cells count as 0). Up to KS_MAPS maps live at once; one is built at a time, 64 cells a frame (a "slice"),
//    so the worst frame stays near the average. A map is per building and is thrown away (marked not ready) when a road changes.
//  - The matcher (6 buildings a frame, at most 2 jobs a frame) finds the nearest producer that holds the good (else the nearest storehouse with
//    the good in the stockpile) whose road side is joined to the wanted building, and a free carrier. The carrier takes the good AT ONCE (it
//    appears at the source's road side), so nothing is promised on a shelf: the books always balance. Producers' surplus goes to the
//    nearest storehouse the same way.
//  - A carrier takes a step every KS_WALK frames to the neighbour with the lowest distance. At 0 it is at the door: it hands the good over.
//  - HEALING: a map not ready yet, or a road cut, just makes a carrier wait. Cut off for KS_CUT frames: the good goes back in the stockpile
//    (aborts). Waiting for any other reason for KS_TIMEOUT frames: the same (timeouts). A road cell lost under a carrier: it steps aside.
//    A demolition sends every carrier home with its good first.
//  - A route may be at most 254 road steps long.
#define KS_WALK     4       // frames a carrier needs for one road step
#define KS_SLICE    64      // road cells one frame of map building handles
#define KS_SCAN     6       // buildings the matcher looks at a frame
#define KS_JOBS     2       // jobs the matcher may start a frame
#define KS_CUT      180     // frames a carrier waits when the map says it cannot get there, before giving up
#define KS_TIMEOUT  900     // frames a carrier may wait for anything else
#define KS_MAPWAIT  300     // frames a map may wait to be built before another building may take its slot (else requesters would evict each other for ever)

static inline int ksMapFor(KSim*S,int b){   // the map slot of a building (made if there is room), or -1
    for(int m=0;m<KS_MAPS;m++) if(S->mapB[m]==b){ S->mapUse[m]=S->t; return m; }
    int v=-1; for(int m=0;m<KS_MAPS;m++){ if(S->mapB[m]==0xFF){ v=m; break; }
        if(!S->mapRef[m]&&(S->mapReady[m]||ksAge(S,S->mapUse[m])>KS_MAPWAIT)&&(v<0||ksAge(S,S->mapUse[m])>ksAge(S,S->mapUse[v]))) v=m; }
    if(v<0) return -1;
    if(S->bfsSlot==v) S->bfsSlot=-1;
    S->mapB[v]=(u8)b; S->mapReady[v]=0; S->mapUse[v]=S->t; return v;
}
static inline void ksBfs(KSim*S){   // one slice of one map
    if(S->bfsSlot<0){ int k=-1;
        for(int m=0;m<KS_MAPS;m++) if(S->mapB[m]!=0xFF&&!S->mapReady[m]&&(k<0||S->mapRef[m]>S->mapRef[k])) k=m;
        if(k<0) return;
        __builtin_memset(S->dist[k],255,KS_MAXN); S->bfsHead=S->bfsTail=0; S->bfsSlot=k;
        int adj[8], n=ksAdj(S,&S->B[S->mapB[k]],adj);
        for(int i=0;i<n;i++){ int nd=ksNode(S,adj[i]); if(nd>=0&&S->dist[k][nd]==255){ S->dist[k][nd]=0; S->queue[S->bfsTail++]=(u16)nd; } } }
    int k=S->bfsSlot, d=S->dim;
    for(int s=0;s<KS_SLICE&&S->bfsHead<S->bfsTail;s++){
        int nd=S->queue[S->bfsHead++], c=S->nodeCell[nd], dv=S->dist[k][nd]; S->opBfs++;
        if(dv>=254) continue;
        int nb[4]={c+1,c-1,c+d,c-d};
        for(int j=0;j<4;j++){ if(!ksIsRoad(S,nb[j])) continue; int m=ksNode(S,nb[j]); if(m>=0&&S->dist[k][m]==255){ S->dist[k][m]=(u8)(dv+1); S->queue[S->bfsTail++]=(u16)m; } } }
    if(S->bfsHead>=S->bfsTail){ S->mapReady[k]=1; S->bfsSlot=-1; }
}
static inline int ksDoor(KSim*S,int src,int slot){   // a road cell beside building src that is joined to the map's building, or -1
    int adj[8], n=ksAdj(S,&S->B[src],adj), best=-1, bd=255;
    for(int i=0;i<n;i++){ if(!ksIsRoad(S,adj[i])) continue; int nd=ksNode(S,adj[i]); if(nd>=0&&S->dist[slot][nd]<bd){ bd=S->dist[slot][nd]; best=adj[i]; } }
    return best;
}
static inline int ksDist(const KSim*S,int a,int b){ int x=S->B[a].x-S->B[b].x, y=S->B[a].y-S->B[b].y; return (x<0?-x:x)+(y<0?-y:y); }
static inline int ksIsProd(const KBld*b){ const KsType*T=ksT(b->type); return b->lvl&&(T->kind==KK_GATHER||T->kind==KK_PROC); }
static inline int ksStart(KSim*S,int src,int pool,int dst,int slot,int g,int cell){   // a carrier takes the good at once
    int free=-1, act=0; for(int w=0;w<KS_MAXW;w++){ if(S->wk[w].st) act++; else if(free<0) free=w; }
    if(free<0||act>=S->nWt) return 0;
    if(pool){ if(!S->R->goods[g]) return 0; S->R->goods[g]--; } else { KBld*b=&S->B[src]; if(ksOut(b)<1) return 0; ksSetOut(b,ksOut(b)-1); }
    KsWk*k=&S->wk[free]; k->cell=(u16)cell; k->st=1; k->src=(u8)src; k->dst=(u8)dst; k->good=(u8)g; k->md=(u8)slot; k->stall=0;
    S->mapRef[slot]++; S->inRes[dst]++; S->opMatch+=4; return 1;
}
static inline int ksWant(const KSim*S,int i,int*g){   // how many more of which good a building asks for
    const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int have=ksIn(b)+S->inRes[i];
    if(!b->lvl){ int cn=0; ksCost(S,b,g,&cn); return cn-have; }
    if(T->kind==KK_PROC&&b->work){ *g=T->inG; return KS_BUF-have; }
    return 0;
}
static inline int ksMatch(KSim*S){
    KRealm*R=S->R; int jobs=0; if(R->nB<=0) return 0;
    for(int s=0;s<KS_SCAN&&jobs<KS_JOBS;s++){
        if(S->mrr>=(1u<<30)) S->mrr=0; int i=(int)(S->mrr++%(u32)R->nB); S->opMatch++; int g=0;
        int want=ksWant(S,i,&g), started=0;
        if(want>0){   // someone must bring it
            int any=S->nStore>0&&R->goods[g]>0;   // no source anywhere: do not take a map slot (wanters with nothing to get used to evict each other's maps)
            for(int j=0;j<R->nB&&!any;j++){ const KBld*pb=&S->B[j]; S->opMatch++; if(j!=i&&ksIsProd(pb)&&ksT(pb->type)->outG==g&&ksOut(pb)>0) any=1; }
            if(!any) goto sink;
            int slot=ksMapFor(S,i); if(slot<0||!S->mapReady[slot]) goto sink;
            int best=-1, bd=1<<20, bcell=-1, pool=0;
            for(int j=0;j<R->nB;j++){ S->opMatch++; const KBld*b=&S->B[j]; const KsType*T=ksT(b->type);
                if(j==i||!ksIsProd(b)||T->outG!=g||ksOut(b)<1){ continue; }
                int dd=ksDist(S,i,j); if(dd>=bd){ continue; }
                int c=ksDoor(S,j,slot); if(c>=0){ best=j; bd=dd; bcell=c; } }
            if(best<0&&S->nStore>0&&R->goods[g]>0){ pool=1;
                for(int j=0;j<R->nB;j++){ S->opMatch++; const KBld*b=&S->B[j]; if(j==i||!b->lvl||ksT(b->type)->kind!=KK_STORE) continue; int dd=ksDist(S,i,j); if(dd>=bd) continue;
                    int c=ksDoor(S,j,slot); if(c>=0){ best=j; bd=dd; bcell=c; } } }
            if(best>=0&&ksStart(S,best,pool,i,slot,g,bcell)){ jobs++; started=1; }
        }
        sink:
        if(!started&&jobs<KS_JOBS&&ksIsProd(&S->B[i])&&ksOut(&S->B[i])>0&&S->nStore>0){   // a producer's surplus goes to a storehouse (also when it wants something itself)
            const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int best=-1, bd=1<<20, bslot=-1, bcell=-1;
            for(int j=0;j<R->nB;j++){ S->opMatch++; if(!R->nB||!S->B[j].lvl||ksT(S->B[j].type)->kind!=KK_STORE) continue; int dd=ksDist(S,i,j); if(dd>=bd) continue;
                int slot=ksMapFor(S,j); if(slot<0||!S->mapReady[slot]) continue;
                int c=ksDoor(S,i,slot); if(c>=0){ best=j; bd=dd; bslot=slot; bcell=c; } }
            if(best>=0&&ksStart(S,i,0,best,bslot,T->outG,bcell)) jobs++;
        }
    }
    return jobs;
}
static inline void ksWalk(KSim*S){
    for(int w=0;w<KS_MAXW;w++){ KsWk*k=&S->wk[w]; if(!k->st) continue; S->opWalk++;
        if(k->stall>=KS_TIMEOUT){ ksInc(&S->timeouts); ksDrop(S,k,1); continue; }
        if((S->t+(u32)w)%KS_WALK) continue;
        int m=k->md, n=ksNode(S,k->cell);
        if(n<0){ ksSnap(S,k); if(k->cell==0xFFFF){ ksInc(&S->aborts); ksDrop(S,k,1); } else k->stall++; continue; }
        if(!S->mapReady[m]||S->mapB[m]==0xFF){ k->stall+=KS_WALK; continue; }
        int dv=S->dist[m][n];
        if(dv==255){ k->stall+=KS_WALK; if(k->stall>=KS_CUT){ ksInc(&S->aborts); ksDrop(S,k,1); } continue; }
        if(dv==0){   // at the door: hand it over
            KBld*b=&S->B[k->dst]; const KsType*T=ksT(b->type);
            if(b->lvl&&T->kind==KK_STORE) ksPool(S,k->good,1); else ksSetIn(b,ksIn(b)+1);
            if(S->inRes[k->dst]) S->inRes[k->dst]--;
            if(S->mapRef[m]) S->mapRef[m]--;
            k->st=0; ksInc(&S->delivered); continue; }
        int c=k->cell, d=S->dim, nb[4]={c+1,c-1,c+d,c-d}, moved=0;
        for(int j=0;j<4&&!moved;j++){ if(!ksIsRoad(S,nb[j])) continue; int q=ksNode(S,nb[j]); if(q>=0&&S->dist[m][q]<dv){ k->cell=(u16)nb[j]; moved=1; } }
        if(moved) k->stall=0; else k->stall+=KS_WALK; }
}
static inline void ksStep(KSim*S){ if(++S->t>=KS_TMAX) S->t=0; ksEcon(S); ksBfs(S); ksMatch(S); ksWalk(S); }
static inline void ksPrepSave(KSim*S){ ksRecall(S); }   // before kgEncode: the carriers' goods go back in the stockpile (the save has no carriers)

// ---- the checks (the PC test runs them all the time; the cart may run ksLedger now and then) ----
// 0 = all well; else a code: 1..8 a good's books (ksLedger), 11 roads, 12 a carrier, 13 a claim, 14 a map reference, 15 a building, 16 a carrier's map
static inline int ksCheck(KSim*S){
    int e=ksLedger(S); if(e) return e;
    for(int n=0;n<S->nN;n++){ if(!ksRoadV(S->cells[S->nodeCell[n]])||(n&&S->nodeCell[n-1]>=S->nodeCell[n])) return 11; }
    { int cnt=0; for(int c=0;c<S->dim*S->dim;c++) cnt+=ksRoadV(S->cells[c]); if(cnt!=S->nN) return 11; }
    int res[KS_MAXB]={0}, mr[KS_MAPS]={0}, act=0;
    for(int w=0;w<KS_MAXW;w++){ const KsWk*k=&S->wk[w]; if(!k->st) continue; act++;
        if(k->dst>=S->R->nB||k->good>=KG_GOODS||k->md>=KS_MAPS) return 12;
        if(S->mapB[k->md]!=k->dst) return 16;
        if(k->cell!=0xFFFF&&k->cell>=S->dim*S->dim) return 12;
        res[k->dst]++; mr[k->md]++; }
    (void)act;   // (more carriers out than free Nubbins is fine for a while: a new job needs a free one, the ones out finish theirs)
    for(int i=0;i<S->R->nB;i++){ if(res[i]!=S->inRes[i]) return 13; const KBld*b=&S->B[i]; const KsType*T=ksT(b->type);
        if(ksIn(b)>KS_BUF||ksOut(b)>KS_BUF){ return 15; }
        if(!b->lvl){ int cg=0,cn=0; ksCost(S,b,&cg,&cn); if(ksIn(b)>cn){ return 15; } } (void)T; }
    for(int m=0;m<KS_MAPS;m++) if(mr[m]!=S->mapRef[m]) return 14;
    return 0;
}
