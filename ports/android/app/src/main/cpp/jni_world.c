#include <jni.h>
#include "world_bridge.h"
#include "window_math.h"
#include "scene_mesh.h"

JNIEXPORT jint JNICALL Java_com_bore2_game_NativeWorld_init(JNIEnv *env,jclass cls,jint seed){
    (void)env;(void)cls;return (jint)bore_android_init((uint32_t)seed);
}
JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_tiles(JNIEnv *env,jclass cls,jint cx,jint cy,jint r){
    (void)cls;uint32_t cells[33*33];int n=bore_android_window(cx,cy,r,cells,33*33);
    if(!n)return NULL;
    jintArray result=(*env)->NewIntArray(env,n);
    if(result)(*env)->SetIntArrayRegion(env,result,0,n,(const jint *)cells);
    return result;
}
JNIEXPORT jstring JNICALL Java_com_bore2_game_NativeWorld_describe(JNIEnv *env,jclass cls,jint x,jint y,jint day){
    (void)cls;char line[256];
    if(!bore_android_describe(x,y,day,line,sizeof line))line[0]=0;
    return (*env)->NewStringUTF(env,line);
}

JNIEXPORT jfloatArray JNICALL Java_com_bore2_game_NativeWorld_mesh(JNIEnv *env,jclass cls,jint x,jint y,jint radius){
    (void)cls;BoreMesh m;
    if(!bore_scene_build(x,y,radius,&m))return NULL;
    jfloatArray a=(*env)->NewFloatArray(env,m.count*6);
    if(a)(*env)->SetFloatArrayRegion(env,a,0,m.count*6,m.v);
    bore_scene_free(&m);return a;
}
