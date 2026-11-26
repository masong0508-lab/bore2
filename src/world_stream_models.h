// Open-world block handoff for PS1 far-house visibility and roof work.
// No resident cache, dynamic allocation, geometry or save-format changes.
// A planned house proxy disappears only when the local block and its
// roof/building metadata are ready in the same rendered frame.
#ifndef BORE2_WORLD_STREAM_MODELS_H
#define BORE2_WORLD_STREAM_MODELS_H

enum { STREAM_MODEL_READY, STREAM_MODEL_ROOF_PENDING, STREAM_MODEL_GENERATING };
static inline int stream_tile_valid(int x,int y,int blocks,int span){
    return blocks>0&&span>0&&(unsigned)x<(unsigned)(blocks*span)&&(unsigned)y<(unsigned)(blocks*span);
}

static inline int stream_model_state(const unsigned char *generation,
                                     const unsigned char *roof, int n,
                                     int bx, int by) {
    if (!generation || !roof || n <= 0 || bx < 0 || by < 0 ||
        bx >= n || by >= n) return STREAM_MODEL_READY;
    int i = by * n + bx;
    return generation[i] ? STREAM_MODEL_GENERATING
                         : roof[i] ? STREAM_MODEL_ROOF_PENDING
                                   : STREAM_MODEL_READY;
}

// If a block has not been generated, nothing else draws the house at all.
// Once its walls exist but the roof list is pending, do not add a fake box
// over detailed nearby geometry (which would cause wall-through-floor).
static inline int stream_house_proxy_needed(int state, int overlaps_detail) {
    return state == STREAM_MODEL_GENERATING ||
           (state == STREAM_MODEL_ROOF_PENDING && !overlaps_detail);
}

static inline int stream_axis_gap(int p, int min_tile, int max_tile) {
    return p < min_tile ? min_tile - p : p > max_tile ? p - max_tile : 0;
}
static inline int stream_block_gap(int x, int y, int wx, int wy, int tile_count) {
    int gap_x = stream_axis_gap(x, wx * tile_count, (wx + 1) * tile_count - 1);
    int gap_y = stream_axis_gap(y, wy * tile_count, (wy + 1) * tile_count - 1);
    return gap_x + gap_y;
}

// Roof metadata is scheduled nearest the real eye/Sim first, not by raw
// memory row order. Runs on at most a 5x5 mask and processes <=1 block
// per frame. Stable row-major tie break prevents nondeterministic updates.
static inline int stream_pick_roof(const unsigned char *dirty, int n, int span,
                                   int sim_x, int sim_y, int eye_x, int eye_y) {
    int best = -1, score_best = 0x7fffffff;
    if (!dirty || n <= 0 || span <= 0) return -1;
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
        int i = y * n + x;
        if (!dirty[i]) continue;
        int ds = stream_block_gap(sim_x, sim_y, x, y, span);
        int de = stream_block_gap(eye_x, eye_y, x, y, span);
        int score = ds * 2 + de * 3;
        if (score < score_best) { score_best = score; best = i; }
    }
    return best;
}

// Generation gives collision-critical blocks first refusal, then uses the
// same camera/Sim score as roofs. Do not increase blocks generated per frame.
static inline int stream_pick_generation(const unsigned char *dirty,int n,int span,
                                        int sx,int sy,int ex,int ey){
    int best=-1,dist=3;
    if(!dirty||n<=0||span<=0)return -1;
    for(int y=0;y<n;y++)for(int x=0;x<n;x++){
        int d=stream_block_gap(sx,sy,x,y,span);
        if(dirty[y*n+x]&&d<dist){best=y*n+x;dist=d;}
    }
    return best>=0?best:stream_pick_roof(dirty,n,span,sx,sy,ex,ey);
}
// Preserve unfinished blocks that remain in the window, discard those
// leaving it, and mark only newly entering blocks as pending. No forced
// generation of the old fringe when the player crosses twice quickly.
static inline int stream_shift_pending(unsigned char *pending,int n,int sx,int sy){
    if(!pending||n<1||n>5)return 0;
    unsigned char next[25];
    int count=0;
    for(int y=0;y<n;y++)for(int x=0;x<n;x++){
        int ox=x+sx,oy=y+sy;
        int p=ox<0||oy<0||ox>=n||oy>=n?1:!!pending[oy*n+ox];
        next[y*n+x]=(unsigned char)p;count+=p;
    }
    for(int i=0;i<n*n;i++)pending[i]=next[i];
    return count;
}
#endif
