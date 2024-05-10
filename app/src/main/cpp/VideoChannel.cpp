#include "VideoChannel.h"

void DropAVPacket(queue<AVPacket *> &queue) {
    while (!queue.empty()) {
        AVPacket *packet = queue.front();
        if (packet->flags != AV_PKT_FLAG_KEY) {
            queue.pop();
            av_packet_unref(packet);
            BaseChannel::releaseAVPacket(&packet);
        } else {
            break;
        }
    }
}

void DropAVFrame(queue<AVFrame *> &queue) {
    if (!queue.empty()) {
        AVFrame *frame = queue.front();
        queue.pop();
        av_frame_free(&frame);
        BaseChannel::releaseAVFrame(&frame);
    }
}

VideoChannel::VideoChannel(int i, AVCodecContext *cContext, AVRational timeBase, int _fps)
        : BaseChannel(i, cContext, timeBase) {
    // 1.1 创建上下文环境
    fps = _fps;

    packets.setSyncHandle(DropAVPacket);
    frames.setSyncHandle(DropAVFrame);

    swsContext = sws_getContext(cContext->width,
                                cContext->height,
                                codecContext->pix_fmt,
                                cContext->width,
                                cContext->height,
                                AV_PIX_FMT_RGBA,
                                SWS_BILINEAR,
                                nullptr,
                                nullptr,
                                nullptr);
}

VideoChannel::~VideoChannel() {
    sws_freeContext(swsContext);
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
        if (isPlaying && frames.size() > MAX_FRAME_SIZE) {
            av_usleep(SLEEP_TIME);
            continue;
        }
        int ret = packets.pop(packet);
        // 阻塞式的，所以要判断是否正在播放
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            freePacket(&packet);
            continue;
        }
        // 发送数据包到解码器
        // !!! 同样生产者队列要对数据进行控制，等待被消费。
        ret = avcodec_send_packet(codecContext, packet);
        // 发送完成后，释放数据包，因为ffmpeg是拷贝了一份数据包
        freePacket(&packet);

        if (ret) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        // 从解码器中获取解码后的数据
        ret = avcodec_receive_frame(codecContext, frame);
        // 如果没有数据，或者数据解码失败
        if (ret == AVERROR(EAGAIN)) {
            freeFrame(&frame);
            continue;
        } else if (ret != 0) {
            // 如果出错，有值就需要释放
            freeFrame(&frame);
            break;
        }
        // 将解码后的数据放入队列
        frames.push(frame);
    }
    freePacket(&packet);
}

void VideoChannel::video_play() {
    // YUV --> RGBA --> NativeWindow
    AVFrame *frame = nullptr;
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            freeFrame(&frame);
            break;
        }
        if (!ret) {
            freeFrame(&frame);
            continue;
        }

        // 渲染
        // 1. 将AVFrame转换成RGB格式
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

        // 2.1 计算延迟时间
        double extra_delay = frame->repeat_pict / (2 * fps);
        double delay = 1.0 / fps + extra_delay;
        LOGI("extra_delay: %f, delay: %f", extra_delay, delay);
        av_usleep(delay * 1000000);

        renderFrame(rgb_dst[0], rgb_dstStride[0], frame->width, frame->height);

        // 3. 控制播放速度


        // 4. 控制音视频同步
        double videoTime = frame->best_effort_timestamp * av_q2d(time_base);
        double audioTime = audio_channel->audio_time;
        double time_diff = videoTime - audioTime;

        // 4.1 如果视频比音频快，延迟一点时间
        if (time_diff > 0) {
            // 视频比音频快非常多，延迟一段时间，但并不睡眠两个差值。
            if (time_diff > 1) {
                av_usleep(delay * 2 * 1000 * 1000);
            } else {
                av_usleep((delay + time_diff) * 1000 * 1000);
            }
        } else if (time_diff < 0) {
            // 音频比视频快，要用视频追音频，要让视频快一点，所以丢包。
            if (time_diff > -0.05) {
                // 视频比音频快
                frames.sync();
                packets.sync();
                continue;
            }
        }

        // 5. 释放资源
        freeFrame(&frame);
        av_freep(&rgb_dst[0]);
    }
    freeFrame(&frame);
    isPlaying = false;
}

void VideoChannel::setRenderFrame(RenderFrame rf) {
    this->renderFrame = rf;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audio_channel = audioChannel;
}
