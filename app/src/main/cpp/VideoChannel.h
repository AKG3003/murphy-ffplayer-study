#ifndef NDK22_COMPILE_STUDY_VIDEOCHANNEL_H
#define NDK22_COMPILE_STUDY_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
};

typedef void (*RenderFrame)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {

private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderFrame renderFrame = nullptr;
    SwsContext *swsContext = nullptr;
    uint8_t *rgb_dst[4];
    int rgb_dstStride[4];

    int fps;
    AudioChannel* audio_channel = nullptr;

public:
    VideoChannel(int i, AVCodecContext *cContext, AVRational timeBase, int _fps);

    virtual ~VideoChannel();

    void stop();

    void start();

    void video_decode();

    void video_play();

    void setRenderFrame(RenderFrame rf);

    void setAudioChannel(AudioChannel *audioChannel);

};


#endif //NDK22_COMPILE_STUDY_VIDEOCHANNEL_H22211
