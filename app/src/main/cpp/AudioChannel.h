#ifndef NDK22_COMPILE_STUDY_AUDIOCHANNEL_H
#define NDK22_COMPILE_STUDY_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "LogUtil.h"
#include "JNICallbackHelper.h"

extern "C" {
#include <libswresample/swresample.h>
}

class AudioChannel : public BaseChannel {

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    SLObjectItf engineObject = nullptr;
    SLEngineItf engineEngine = nullptr;
    SLObjectItf outputMixObject = nullptr;
    SLObjectItf playerObject = nullptr;
    SLPlayItf playerPlay = nullptr;
    SLBufferQueueItf playerBufferQueue = nullptr;
    JNICallbackHelper *pHelper = nullptr;


public:
    AudioChannel(int i, AVCodecContext *cContext, AVRational timeBase);

    virtual ~AudioChannel();

    void stop();

    void start();

    void audio_decode();

    void audio_play();

    int get_pcm_size();

    void setJNICallbackHelper(JNICallbackHelper *pHelper);

    SwrContext *swrContext = nullptr;
    uint8_t *out_buffer = nullptr;
    int out_sample_rate = 0;
    int out_channels = 0;
    int out_samplesize = 0;

    int out_buffer_size_pre = 0;
    double audio_time = 0;

};


#endif //NDK22_COMPILE_STUDY_AUDIOCHANNEL_H
