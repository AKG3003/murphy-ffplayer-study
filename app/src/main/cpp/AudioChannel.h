#ifndef NDK22_COMPILE_STUDY_AUDIOCHANNEL_H
#define NDK22_COMPILE_STUDY_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "LogUtil.h"

extern "C" {
#include <libswresample/swresample.h>
}

class AudioChannel : public BaseChannel {

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

public:
    AudioChannel(int i, AVCodecContext *cContext);

    virtual ~AudioChannel();

    void stop();

    void start();

    void audio_decode();

    void audio_play();

};


#endif //NDK22_COMPILE_STUDY_AUDIOCHANNEL_H
