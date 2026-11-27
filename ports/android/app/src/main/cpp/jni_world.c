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

JNIEXPORT jboolean JNICALL Java_com_bore2_game_NativeWorld_canVisit(JNIEnv *env,jclass cls,jint x,jint y){
    (void)env;(void)cls;return bore_android_can_visit(x,y)?JNI_TRUE:JNI_FALSE;
}
JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_activity(JNIEnv *env,jclass cls,jint x,jint y,jint day,jint slot,jint minute,jint funds,jint rest){
    (void)cls;int v[BORE_ANDROID_ACTIVITY_FIELDS];
    if(!bore_android_activity(x,y,day,slot,minute,funds,rest,v))return NULL;
    jintArray a=(*env)->NewIntArray(env,BORE_ANDROID_ACTIVITY_FIELDS);
    if(a)(*env)->SetIntArrayRegion(env,a,0,BORE_ANDROID_ACTIVITY_FIELDS,(const jint *)v);
    return a;
}
JNIEXPORT jstring JNICALL Java_com_bore2_game_NativeWorld_activityText(JNIEnv *env,jclass cls,jint id,jint field){
    (void)cls;return (*env)->NewStringUTF(env,bore_android_activity_text(id,field));
}

JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_route(JNIEnv *env,jclass cls,jint x,jint y,jint X,jint Y){
    (void)cls;uint32_t path[BORE_ANDROID_ROUTE_MAX];int n=bore_android_route(x,y,X,Y,path,BORE_ANDROID_ROUTE_MAX);
    if(n<0)return NULL;
    jintArray a=(*env)->NewIntArray(env,n);
    if(a&&n)(*env)->SetIntArrayRegion(env,a,0,n,(const jint*)path);
    return a;
}
JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_shift(JNIEnv *env,jclass cls,jint x,jint y,jint day,jint minute,jint energy,jint food){
    (void)cls;int v[BORE_ANDROID_SHIFT_FIELDS];
    if(!bore_android_shift(x,y,day,minute,energy,food,v))return NULL;
    jintArray a=(*env)->NewIntArray(env,BORE_ANDROID_SHIFT_FIELDS);
    if(a)(*env)->SetIntArrayRegion(env,a,0,BORE_ANDROID_SHIFT_FIELDS,(const jint*)v);
    return a;
}

JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_interior(JNIEnv *env,jclass cls,jint x,jint y){
    (void)cls;int v[BORE_ANDROID_INTERIOR_FIELDS];
    if(!bore_android_interior(x,y,v,BORE_ANDROID_INTERIOR_FIELDS))return NULL;
    jintArray a=(*env)->NewIntArray(env,BORE_ANDROID_INTERIOR_FIELDS);
    if(a)(*env)->SetIntArrayRegion(env,a,0,BORE_ANDROID_INTERIOR_FIELDS,(const jint*)v);
    return a;
}

JNIEXPORT jintArray JNICALL Java_com_bore2_game_NativeWorld_walkMask(JNIEnv *env,jclass cls,jint x,jint y){
    (void)cls;int v[BORE_ANDROID_WALK_FIELDS];
    if(!bore_android_walk_mask(x,y,v,BORE_ANDROID_WALK_FIELDS))return NULL;
    jintArray a=(*env)->NewIntArray(env,BORE_ANDROID_WALK_FIELDS);
    if(a)(*env)->SetIntArrayRegion(env,a,0,BORE_ANDROID_WALK_FIELDS,(const jint*)v);
    return a;
}
