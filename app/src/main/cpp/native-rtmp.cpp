#include <jni.h>
#include <unistd.h>
#include <string>
#include "LogUtil.h"
#include "LiveVideoChannel.h"
#include "SafeQueue.h"

extern "C" {
#include <rtmp.h>
#include <x264.h>
}

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

// 定义一个全局的JavaVM，这样就可以在其他线程中使用JNI了。
// env是线程相关的，不能跨线程使用。
JavaVM *g_javaVM = nullptr;
LiveVideoChannel *videoChannel = nullptr;
bool isStart = false;
pthread_t *pid_start;
bool isReady = false;
SafeQueue<RTMPPacket *> packets; //rtmp包队列，不区分音频视频，start直接拿出去给流媒体服务器。

void release_packets(RTMPPacket **packet) {
    if (packet) {
        RTMPPacket_Free(*packet);
        delete *packet;
        *packet = nullptr;
    }
}

void rtmp_callback(RTMPPacket *packet) {
    if (packet) {
        if (packet->m_nBodySize <= 0) {
            release_packets(&packet);
            return;
        }
//        packet->m_nTimeStamp = RTMP_GetTime() - packet->m_nTimeStamp;
//        packet->m_nInfoField2 = 1;
        packets.push(packet);
    }
}

void *start_thread_func(void *args) {
    char *url = static_cast<char *>(args);
    int ret = 0; // rtmp这里0代表失败
    // RTMPDump API 执行流程
    // 1.创建RTMP对象，申请内存空间
    RTMP *rtmp = RTMP_Alloc();
    do {
        if (!rtmp) {
            LOGI("RTMP_Alloc failed");
            break;
        }
        // 2.初始化RTMP对象
        RTMP_Init(rtmp);
        // 2.1 设置超时时间
        rtmp->Link.timeout = 5;

        // 3.设置RTMP连接参数
        ret = RTMP_SetupURL(rtmp, url);
        if (!ret) {
            LOGI("RTMP_SetupURL failed");
            break;
        }

        // 4.开启RTMP输出模式
        RTMP_EnableWrite(rtmp);

        // 5.建立RTMP连接
        ret = RTMP_Connect(rtmp, nullptr);
        if (!ret) {
            LOGI("RTMP_Connect failed");
            break;
        }

        // 6.建立RTMP流
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) {
            LOGI("RTMP_ConnectStream failed");
            break;
        }

        // 准备好了，可以开始向服务器推流了。
        isReady = true;

        // 队列开始工作
        packets.setWork(1);
        RTMPPacket *packet = nullptr;
        while (isReady) {
            // 从队列中取出RTMPPacket
            packets.pop(packet);
            if (!isReady) {
                break;
            }
            if (!packet) {
                continue;
            }
            // 7.发送数据
            // 给rtmp的id字段赋值
            packet->m_nInfoField2 = rtmp->m_stream_id;
            // queue 1 代表开启内部缓存队列
            ret = RTMP_SendPacket(rtmp, packet, 1);
            if (!ret) {
                LOGI("RTMP_SendPacket failed");
                break;
            }
            // 释放RTMPPacket
            release_packets(&packet);
        }
        release_packets(&packet);
    } while (false);

    // 释放资源
    isStart = false;
    isReady = false;
    packets.setWork(0);
    packets.clear();
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    delete url;

    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
do_init(JNIEnv *env, jobject /* this */) {
    LOGI("do_init");
    videoChannel = new LiveVideoChannel();
    videoChannel->setVideoCallback(rtmp_callback);
    // 队列开始工作
    packets.setReleaseCallback(release_packets);
}

extern "C"
JNIEXPORT void JNICALL
do_start(JNIEnv *env, jobject /* this */, jstring path) {
    // 1.链接流媒体服务器，2.发包
    if (isStart) {
        return;
    }
    isStart = true;
    const char *url = env->GetStringUTFChars(path, nullptr);
    // 必须执行深拷贝,因为子线程中会释放这个url
    char *globel_url = new char[strlen(url) + 1];
    strcpy(globel_url, url);

//    pthread_create(pid_start, nullptr, [](void *args) -> void * {
//        LiveVideoChannel *liveVideoChannel = static_cast<LiveVideoChannel *>(args);
//        liveVideoChannel->init(globel_url);
//        return nullptr;
//    }, globel_url);
    pthread_create(pid_start, nullptr, start_thread_func, globel_url);

    env->ReleaseStringUTFChars(path, url);

    pthread_t pid;

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
    // 往队列里面加入RTMP包，必须经过x264编码
    if (!videoChannel || !isReady) {
        return;
    }
    jbyte *yuv = env->GetByteArrayElements(data, nullptr);

    videoChannel->encodeData(yuv);

    env->ReleaseByteArrayElements(data, yuv, 0);
}

extern "C"
JNIEXPORT void JNICALL
do_initVideoEncoder(JNIEnv *env, jobject /* this */, jint width, jint height, jint fps,
                    jint bitrate) {
    // 初始化x264编码器
    if (videoChannel) {
        videoChannel->initVideoEncoder(width, height, fps, bitrate);
    }

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