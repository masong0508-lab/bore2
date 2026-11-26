#pragma once
// Interleaved position and lit material RGB; each three vertices are a triangle.
// One bounded window, local coordinates measured in native BORE tiles.
#define BORE_SCENE_RADIUS 4
#define BORE_SCENE_MAX_VERTICES 400000
typedef struct {float *v; int count,capacity,failed;} BoreMesh;
int bore_scene_build(int cx,int cy,int radius,BoreMesh *out);
void bore_scene_free(BoreMesh *m);
