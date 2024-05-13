#include <jni.h>
#include <unistd.h>
#include <string>
#include "LogUtil.h"

extern "C" {
#include <rtmp.h>
#include <x264.h>
}

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// 定义一个全局的JavaVM，这样就可以在其他线程中使用JNI了。
// env是线程相关的，不能跨线程使用。
JavaVM *g_javaVM = nullptr;

extern "C"
JNIEXPORT void JNICALL
do_init(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_start(JNIEnv *env, jobject /* this */, jstring path) {

}

extern "C"
JNIEXPORT void JNICALL
do_stop(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_release(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_pushVideo(JNIEnv *env, jobject /* this */, jbyteArray data) {

}

extern "C"
JNIEXPORT void JNICALL
do_initVideoEncoder(JNIEnv *env, jobject /* this */, jint width, jint height, jint fps,
                    jint bitrate) {

}


static const JNINativeMethod gMethods[] = {
        {"native_init",             "()V",                   (void *) do_init},
        {"native_start",            "(Ljava/lang/String;)V", (void *) do_start},
        {"native_stop",             "()V",                   (void *) do_stop},
        {"native_release",          "()V",                   (void *) do_release},
        {"native_pushVideo",        "([B)V",                 (void *) do_pushVideo},
        {"native_initVideoEncoder", "(IIII)V",               (void *) do_initVideoEncoder},
};

// 通过JNI_OnLoad函数动态注册
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    ::g_javaVM = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass clazz = env->FindClass("com/study/ndk22_compile_study/push/Pusher");
    if (clazz == nullptr) {
        return JNI_ERR;
    }

    // 第一个车参数，你实现在Java中那个类里面就是那个类的全路径。
    // 第二个参数，是一个数组，里面是JNINativeMethod结构体，这个结构体里面是方法名，方法签名，方法指针。
    // 第三个参数，是数组的长度。一般通过NELEM(gMethods)来获取数组的长度。
    if (0 > env->RegisterNatives(clazz, gMethods, NELEM(gMethods))) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}