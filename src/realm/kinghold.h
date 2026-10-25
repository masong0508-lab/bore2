// kinghold.h - EP6 THE LAND BOREFORE TIME, Phase 4 step h: PICK UP AND PUT DOWN. PURE C (needs kingdom.h, kingsim.h, kinglog.h before it; kinghand.h includes it).
// The giant has two hands' worth of carrying: ONE thing at a time, kept in KSim.hd (kingsim.h, so the books can see it):
//   GOODS  (HAND mode)   A on a finished STOREHOUSE takes a load of what the sites and sawmills are waiting for; A on a SITE (or a processor that wants it) puts it in,
//                        skipping the Nubbins' hauling; A on a producer takes the goods it holds to send out; A on a storehouse with a load in hand puts the load back.
//   MOVE   (MOVE mode)   A on a building lifts it (its goods go to the stockpile, a home's residents and a workplace's worker are remembered); A on a free spot puts it down. Free.
//   NUBBIN (NUBBIN mode) A on a home lifts one RESIDENT (to put in another home); A on a workplace lifts its WORKER (to staff another workplace). Nubbins stay a head count:
//                        what is carried is "a resident" or "a worker", not one particular Nubbin.
// NOTE (step 4c): main.c #defines W, D and H. No single capital letters as names here. [ASSUMED values, tune freely]
#ifndef KINGHOLD_H
#define KINGHOLD_H
static inline int khLoad(int giant){ return khStat(giant,1); }   // goods in a load by giant (the carry column of khStatT in kinghand.h)
// how many more of good g the sites and the working sawmills / masons still ask for (their want minus what is already on its way to them)
static inline int khWantOf(const KSim*S,int i,int*g){
    const KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int have=ksIn(b)+S->inRes[i], w=0; *g=-1;
    if(!b->lvl){ int cn=0; ksCost(S,b,g,&cn); w=cn-have; }
    else if(T->kind==KK_PROC&&b->work){ *g=T->inG; w=KS_BUF-have; }
    return w>0?w:0;
}
// ---- GOODS: A with the HAND on building i ----
static inline int khGoods(KSim*S,int i,int x,int y,int giant){
    KHold*h=&S->hd; KBld*b=&S->B[i]; const KsType*T=ksT(b->type);
    if(h->k==KHD_BLD||h->k==KHD_NUB) return KHR_BUSY;
    if(h->k==KHD_GOODS){
        if(T->kind==KK_STORE&&b->lvl){ ksPool(S,h->g,h->n); h->k=KHD_NONE; h->n=0; return KHR_BACK; }   // a load goes back on the pile (a full pile loses it, the books know)
        int g=0, w=khWantOf(S,i,&g);
        if(g!=h->g) return KHR_REFUSE;                                // not the good this building asks for (a finished home, a woodcutter, the wrong good)
        if(w<=0) return KHR_ENOUGH;                                   // it has what it needs, or what is on its way covers it
        int n=h->n<w?h->n:w; ksSetIn(b,ksIn(b)+n); h->n=(u8)(h->n-n); if(!h->n) h->k=KHD_NONE;
        if(!b->lvl&&n==w&&khBuild(giant)>1) ksGift(S,g,khBuild(giant)-1);   // (step 5e) the builder's hands finish a site: some of its goods come back (ksGift, so the books stay balanced)
        return KHR_GIVE;
    }
    if(b->lvl&&T->kind==KK_STORE){   // empty hands on a storehouse: take what is asked for, from the stockpile
        int bg=-1, bd=1<<20;
        for(int j=0;j<S->R->nB;j++){ int g=0; if(!khWantOf(S,j,&g)||S->R->goods[g]<1) continue; int dd=(S->B[j].x>x?S->B[j].x-x:x-S->B[j].x)+(S->B[j].y>y?S->B[j].y-y:y-S->B[j].y); if(dd<bd){ bd=dd; bg=g; } }
        if(bg<0) return KHR_NOWANT;
        long ask=0; for(int j=0;j<S->R->nB;j++){ int g=0; int w=khWantOf(S,j,&g); if(w&&g==bg) ask+=w; }
        long n=khLoad(giant); if(n>ask) n=ask; if(n>S->R->goods[bg]) n=S->R->goods[bg];
        S->R->goods[bg]=(u16)(S->R->goods[bg]-n); h->k=KHD_GOODS; h->g=(u8)bg; h->n=(u8)n; return KHR_TAKE;
    }
    if(b->lvl&&(T->kind==KK_GATHER||T->kind==KK_PROC)&&ksOut(b)>0){   // empty hands on a producer: take what it holds to send out
        int n=ksOut(b); if(n>khLoad(giant)) n=khLoad(giant); ksSetOut(b,ksOut(b)-n); h->k=KHD_GOODS; h->g=T->outG; h->n=(u8)n; return KHR_TAKE;
    }
    return KHR_NOLOAD;
}
// ---- MOVE: A on a cell ----
static inline int khMove(KSim*S,int x,int y){
    KHold*h=&S->hd; int d=S->dim; if(x<0||y<0||x>=d||y>=d) return KHR_ARG;
    if(h->k==KHD_BLD){   // put it down, top left corner on the cell; a finished building stays finished, a site stays a site
        int e=ksPlace(S,h->bt,x,y,0); if(e<0) return e==KSE_CAP?KHR_FULL:KHR_SPOT;
        KBld*b=&S->B[e]; const KsType*T=ksT(b->type); b->lvl=h->bl; b->work=h->bw;
        if(T->kind==KK_HOME&&b->work>T->house){ b->work=(u8)T->house; }
        if(T->kind!=KK_HOME&&b->work>1){ b->work=1; }
        if(!b->lvl){ b->work=0; }
        h->k=KHD_NONE; ksCensus(S); return KHR_SET;
    }
    if(h->k) return KHR_BUSY;
    int i=-1; for(int j=0;j<S->R->nB;j++){ const KBld*q=&S->B[j]; if(x>=q->x&&x<q->x+kgFootW(q->type)&&y>=q->y&&y<q->y+kgFootH(q->type)){ i=j; break; } }
    if(i<0) return KHR_NOTHING;
    if(S->B[i].type==KB_CASTLE) return KHR_KEEP;   // (the castle stays where it was founded)
    { KBld q=S->B[i]; if(ksDemolish(S,i)!=KSE_OK) return KHR_ARG; h->k=KHD_BLD; h->bt=q.type; h->bl=q.lvl; h->bw=q.work; h->g=0; h->n=0; }   // (ksDemolish: its goods to the stockpile, every carrier home, the cells free)
    return KHR_LIFT;
}
// ---- NUBBIN: A on building i ----
static inline int khNubbin(KSim*S,int i){
    KHold*h=&S->hd; KBld*b=&S->B[i]; const KsType*T=ksT(b->type); int home=T->kind==KK_HOME, work=T->kind==KK_GATHER||T->kind==KK_PROC;
    if(h->k==KHD_NUB){
        if(!b->lvl) return KHR_NOTYET;
        if(h->bt){   // a resident: only a home takes him
            if(!home) return KHR_NOWORK;
            if(b->work>=T->house) return KHR_HOMEFULL;
            b->work++; h->k=KHD_NONE; ksCensus(S); return KHR_SETTLE; }
        if(!work) return KHR_NOWORK;
        if(b->work) return KHR_STAFFED;   // a worker: only an empty workplace takes him
        { int pop=S->pop, lim=pop-1-pop/4; if(lim<0) lim=0; if(S->emp+1>lim) return KHR_LASTHAND; }
        b->work=1; h->k=KHD_NONE; ksCensus(S); return KHR_SETTLE;
    }
    if(h->k) return KHR_BUSY;
    if(!b->lvl) return KHR_NOTYET;
    if(home&&b->work){ b->work--; h->k=KHD_NUB; h->bt=1; ksCensus(S); return KHR_HEAD; }
    if(work&&b->work){ b->work=0; h->k=KHD_NUB; h->bt=0; ksCensus(S); return KHR_HEAD; }
    return KHR_NOBODY;
}
// what A does in the MOVE and NUBBIN modes, from khAct; the building under the cell, if any, is found here
static inline int khHoldAct(KSim*S,int mode,int x,int y){
    if(mode==KHM_MOVE) return khMove(S,x,y);
    int i=-1; for(int j=0;j<S->R->nB;j++){ const KBld*q=&S->B[j]; if(x>=q->x&&x<q->x+kgFootW(q->type)&&y>=q->y&&y<q->y+kgFootH(q->type)){ i=j; break; } }
    if(i<0) return KHR_NOTHING;
    return khNubbin(S,i);
}
// what the giant holds, as text: "5 WOOD", "A HUT", "A RESIDENT", "A WORKER", or "" (out: at least 24 bytes)
static inline void khHeld(const KSim*S,char*out){
    const KHold*h=&S->hd; char*e=out; *e=0;
    if(h->k==KHD_GOODS){ e=khNum(e,h->n); e=khCat(e," "); khCat(e,khGood[h->g<8?h->g:0]); }
    else if(h->k==KHD_BLD){ e=khCat(e,"A "); khCat(e,khName[KHM_HUT+(h->bt<KS_NTYPES?h->bt:0)]); }
    else if(h->k==KHD_NUB) khCat(e,h->bt?"A RESIDENT":"A WORKER");
}
#endif
