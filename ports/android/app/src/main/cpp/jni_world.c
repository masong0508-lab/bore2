#include <jni.h>
#include "world_bridge.h"
#include "window_math.h"

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
