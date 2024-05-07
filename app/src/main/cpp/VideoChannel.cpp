#include "VideoChannel.h"

VideoChannel::VideoChannel(int i, AVCodecContext *cContext)
        : BaseChannel(i, cContext) {

}

VideoChannel::~VideoChannel() {

}

void VideoChannel::stop() {

}

void *task_video_decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();
    return nullptr;
}

void *task_video_play(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_play();
    return nullptr;
}

void VideoChannel::start() {
    // 1. 设置正在播放的标志
    isPlaying = true;

    // 2. 设置队列工作标志
    packets.setWork(1);
    frames.setWork(1);

    // 3. 启动解码线程
    pthread_create(&pid_video_decode, nullptr, task_video_decode, this);
    // 4. 启动播放线程
    pthread_create(&pid_video_play, nullptr, task_video_play, this);
}

void VideoChannel::video_decode() {
    AVPacket *packet = nullptr;
    while (isPlaying) {
        int ret = packets.pop(packet);
        // 阻塞式的，所以要判断是否正在播放
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 发送数据包到解码器
        ret = avcodec_send_packet(codecContext, packet);
        // 发送完成后，释放数据包，因为ffmpeg是拷贝了一份数据包
//        av_packet_free(&packet);
//        packet = nullptr;

        if (ret) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        // 从解码器中获取解码后的数据
        ret = avcodec_receive_frame(codecContext, frame);
        // 如果没有数据，或者数据解码失败
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        // 将解码后的数据放入队列
        frames.push(frame);
    }
    av_packet_free(&packet);
    packet = nullptr;
}

void VideoChannel::video_play() {
    // YUV --> RGBA --> NativeWindow
    AVFrame *frame = nullptr;
    uint8_t *rgb_dst[4];
    int rgb_dstStride[4];

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        // 渲染
        // 1. 将AVFrame转换成RGB格式
        // 1.1 创建上下文环境
        SwsContext *swsContext = sws_getContext(frame->width,
                                                frame->height,
                                                codecContext->pix_fmt,
                                                frame->width,
                                                frame->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);
        // 1.2 申请RGBA格式的内存空间
        av_image_alloc(rgb_dst,
                       rgb_dstStride,
                       frame->width,
                       frame->height,
                       AV_PIX_FMT_RGBA,
                       1);

        // 1.3 将YUV格式的数据转换成RGBA格式的数据
        sws_scale(swsContext,
                  frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  rgb_dst,
                  rgb_dstStride);
        // 2. 绘制一帧画面，需要宽，高，像素格式，数据。这里是RGBA格式的数据。
        renderFrame(rgb_dst[0], rgb_dstStride[0], frame->width, frame->height);

        // 3. 控制播放速度


        // 4. 控制音视频同步


        // 5. 释放资源
        av_frame_free(&frame);
        frame = nullptr;
        av_freep(&rgb_dst);
        sws_freeContext(swsContext);
    }
    av_frame_free(&frame);
    frame = nullptr;
    isPlaying = false;
}

void VideoChannel::setRenderFrame(RenderFrame rf) {
    this->renderFrame = rf;
}
