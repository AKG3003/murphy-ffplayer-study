//
// Created by 王一 on 2024/5/4.
//

#ifndef NDK22_COMPILE_STUDY_BASECHANNEL_H
#define NDK22_COMPILE_STUDY_BASECHANNEL_H

#include "SafeQueue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

class BaseChannel {

public:
    int stream_index; //音频或视频流的索引
    AVCodecContext *codecContext = nullptr;//解码器上下文
//    AVFormatContext *fContext = nullptr;//封装格式上下文
    bool isPlaying = false;//是否正在播放
    SafeQueue<AVPacket *> packets;//队列中存放的是压缩数据包（视频或音频）
    SafeQueue<AVFrame *> frames;//队列中存放的是解码后的数据包（视频或音频）

    BaseChannel(int i, AVCodecContext *cContext) {
        stream_index = i;
        this->codecContext = cContext;
//        this->fContext = pfContext;
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

private:
    static void releaseAVPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            *packet = nullptr;
        }
    }

    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = nullptr;
        }
    }
};

#endif //NDK22_COMPILE_STUDY_BASECHANNEL_H
