#include <jni.h>
#include <unistd.h>
#include <string>
#include "MurphyNativePlayer.h"
#include "JNICallbackHelper.h"
#include "LogUtil.h"
#include <android/native_window_jni.h>


#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// 定义一个全局的JavaVM，这样就可以在其他线程中使用JNI了。
// env是线程相关的，不能跨线程使用。
JavaVM *g_javaVM = nullptr;
JNICallbackHelper *helper = nullptr;
MurphyNativePlayer *player = nullptr;
ANativeWindow *window = nullptr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 渲染一帧数据
void render_frame(uint8_t *data, int linesize, int w, int h) {
    pthread_mutex_lock(&mutex);

    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    // 设置窗口的属性
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;
    // 锁定窗口，获取绘图缓冲区，如果失败就释放窗口，然后返回。
    if (ANativeWindow_lock(window, &windowBuffer, nullptr) < 0) {
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex);
        return;
    }

    // 获取绘图缓冲区的首地址
    uint8_t *dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    // 获取绘图缓冲区的一行的字节数
    int dst_linesize = windowBuffer.stride * 4;
    // 拷贝一行的数据
    for (int i = 0; i < windowBuffer.height; ++i) {
        memcpy(dst_data + i * dst_linesize, data + i * linesize, dst_linesize);
    }

    // 解锁绘图缓冲区
    ANativeWindow_unlockAndPost(window);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
do_prepare(JNIEnv *env, jobject thiz /* this */,jstring data_source) {
    const char *dataSource = env->GetStringUTFChars(data_source, nullptr);

    helper = new JNICallbackHelper(g_javaVM, env, thiz);

    player = new MurphyNativePlayer(dataSource, helper);
    player->setRenderFrame(render_frame);
    player->prepare();

    env->ReleaseStringUTFChars(data_source, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
do_start(JNIEnv *env, jobject /* this */) {
    if (player){
        player->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
do_stop(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_complete(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_pause(JNIEnv *env, jobject /* this */) {

}

extern "C"
JNIEXPORT void JNICALL
do_surface_setup(JNIEnv *env, jobject /* this */, jobject surface) {
    pthread_mutex_lock(&mutex);
    // 释放之前的窗口
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    // 创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);
//    if (player) {
//        player->setRenderFrame(render_frame);
//    }
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT jlong JNICALL
do_get_duration(JNIEnv *env, jobject /* this */){
    if (player){
        return player->getDuration();
    }
    return 0;
}

static const JNINativeMethod gMethods[] = {
        {"prepareNative", "(Ljava/lang/String;)V", (void *) do_prepare},
        {"startNative", "()V", (void *) do_start},
        {"pauseNative", "()V", (void *) do_pause},
        {"stopNative", "()V", (void *) do_stop},
        {"completeNative", "()V", (void *) do_complete},
        {"setSurfaceNative", "(Landroid/view/Surface;)V", (void *) do_surface_setup},
        {"getDurationNative", "()J", (void *) do_get_duration},
};

// 通过JNI_OnLoad函数动态注册
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    ::g_javaVM = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass clazz = env->FindClass("com/study/ndk22_compile_study/MurphyPlayer");
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

