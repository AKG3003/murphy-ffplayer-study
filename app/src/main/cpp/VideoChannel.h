#ifndef NDK22_COMPILE_STUDY_VIDEOCHANNEL_H
#define NDK22_COMPILE_STUDY_VIDEOCHANNEL_H

#include "BaseChannel.h"

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

public:
    VideoChannel(int i, AVCodecContext *cContext);

    virtual ~VideoChannel();

    void stop();

    void start();

    void video_decode();

    void video_play();

    void setRenderFrame(RenderFrame rf);

};


#endif //NDK22_COMPILE_STUDY_VIDEOCHANNEL_H
