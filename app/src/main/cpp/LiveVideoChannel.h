#ifndef NDK22_COMPILE_STUDY_LIVEVIDEOCHANNEL_H
#define NDK22_COMPILE_STUDY_LIVEVIDEOCHANNEL_H

#include <pthread.h>
#include "LogUtil.h"
#include <string.h>
#include <rtmp.h>
#include <x264.h>

typedef void (*VideoCallback)(RTMPPacket *packet);
class LiveVideoChannel {

private:
    pthread_mutex_t mutex;
    int width;
    int height;
    int fps;
    int bitrate;

    int y_len;//Y数据的长度
    int uv_len;//UV数据的长度

    x264_t *videoEncoder = nullptr;
    x264_picture_t *pic_in = 0;// 这是每一张图片

    VideoCallback videoCallback;

public:

    LiveVideoChannel();

    ~LiveVideoChannel();

    void initVideoEncoder(int width, int height, int fps, int bitrate);

    void encodeData(signed char *data);

    void sendSpsPps(unsigned char sps[100], int sps_len, unsigned char pps[100], int pps_len);

    void setVideoCallback(VideoCallback videoCallback);

    void setFrame(int type, int i_payload, uint8_t *p_payload);

    void release();
};


#endif //NDK22_COMPILE_STUDY_LIVEVIDEOCHANNEL_H
