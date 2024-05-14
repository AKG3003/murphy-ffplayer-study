//
// Created by 王一 on 2024/5/14.
//

#ifndef NDK22_COMPILE_STUDY_LIVEAUDIOCHANNEL_H
#define NDK22_COMPILE_STUDY_LIVEAUDIOCHANNEL_H


#include <sys/types.h>
#include <faac.h>
#include <jni.h>
#include "LogUtil.h"
#include <rtmp.h>

typedef void (*AudioCallback)(RTMPPacket *packet);

class LiveAudioChannel {

public:
    LiveAudioChannel();
    virtual ~LiveAudioChannel();

    void initAudioEncoder(int sample_rate, int num_channels);

    jint getInputSamples();

    void encodeData(int32_t *data);

    void setVideoCallback(AudioCallback callback);

    void addAudioHeader();

    void release();

private:
    u_long inputSamples = 0;
    u_long maxOutputBytes;
    int m_chanels = 2;
    faacEncHandle audioCodec = nullptr;
    u_char *buffer = nullptr;
    AudioCallback audioCallback = nullptr;
};


#endif //NDK22_COMPILE_STUDY_LIVEAUDIOCHANNEL_H
