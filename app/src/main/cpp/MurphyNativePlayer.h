#ifndef NDK22_COMPILE_STUDY_MURPHYNATIVEPLAYER_H
#define NDK22_COMPILE_STUDY_MURPHYNATIVEPLAYER_H

#include <string.h>
#include <pthread.h>
#include "JNICallbackHelper.h"
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "ThreadUtil.h"
#include "LogUtil.h"

//注意导入指定C的头文件
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

typedef void (*RenderFrame)(uint8_t *, int, int, int);

class MurphyNativePlayer {

private:
    char *data_source = nullptr;

    bool isPlaying = false;

    pthread_t pid_prepare;
    pthread_t pid_start;

    AVFormatContext *pFormatCtx = nullptr;
    JNICallbackHelper *callbackHelper = nullptr;

    VideoChannel *video_channel = nullptr;
    AudioChannel *audio_channel = nullptr;

    RenderFrame renderFrame = nullptr;

public:
    MurphyNativePlayer(const char *data_source, JNICallbackHelper *pHelper);

    virtual ~MurphyNativePlayer();

    void prepare();

    void prepare_();

    void start();

    void start_();

    void setRenderFrame(RenderFrame rf);
};

#endif //NDK22_COMPILE_STUDY_MURPHYNATIVEPLAYER_H
