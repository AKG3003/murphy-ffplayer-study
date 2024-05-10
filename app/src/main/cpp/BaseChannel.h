//
// Created by 王一 on 2024/5/4.
//

#ifndef NDK22_COMPILE_STUDY_BASECHANNEL_H
#define NDK22_COMPILE_STUDY_BASECHANNEL_H

#include "SafeQueue.h"
#include "ThreadUtil.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

class BaseChannel {

public:
    int stream_index; //音频或视频流的索引
    AVCodecContext *codecContext = nullptr;//解码器上下文
//    AVFormatContext *fContext = nullptr;//封装格式上下文
    bool isPlaying = false;//是否正在播放
    SafeQueue<AVPacket *> packets;//队列中存放的是压缩数据包（视频或音频）
    SafeQueue<AVFrame *> frames;//队列中存放的是解码后的数据包（视频或音频）
    AVRational time_base;//流的时间基

    BaseChannel(int i, AVCodecContext *cContext, AVRational timeBase) {
        stream_index = i;
        this->codecContext = cContext;
        this->time_base = timeBase;
//        this->fContext = pfContext;
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        packets.clear();
        frames.clear();
    }

    void freePacket(AVPacket **packet) {
        if (packet) {
            av_packet_unref(*packet);
            av_packet_free(packet);
            *packet = nullptr;
        }
    }

    void freeFrame(AVFrame **frame) {
        if (frame) {
            av_frame_unref(*frame);
            av_frame_free(frame);
            *frame = nullptr;
        }
    }

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
