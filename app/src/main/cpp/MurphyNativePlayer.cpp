#include "MurphyNativePlayer.h"

MurphyNativePlayer::MurphyNativePlayer(const char *data_source, JNICallbackHelper *pHelper) {

    //注意要加1，因为strlen不包括字符串结束符
    //深拷贝
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source);
    this->callbackHelper = pHelper;
}

MurphyNativePlayer::~MurphyNativePlayer() {
    //注意释放内存
    if (data_source) {
        delete data_source;
        data_source = nullptr;
    }
    if (callbackHelper) {
        delete callbackHelper;
        callbackHelper = nullptr;
    }
}

// 子线程调用
void *prepare_thread_func(void *arg) {
    MurphyNativePlayer *player = static_cast<MurphyNativePlayer *>(arg);
    player->prepare_();

    return nullptr;//必须返回
}

void MurphyNativePlayer::prepare_() {
    pFormatCtx = avformat_alloc_context();
    AVDictionary *options = nullptr;
    int i, ret;
    av_dict_set(&options, "timeout", "5000000", 0);
    ret = avformat_open_input(&pFormatCtx, data_source, nullptr, &options);
    av_dict_free(&options);
    if (ret != 0) {
        LOGI("打开媒体失败：%s", av_err2str(ret));
        if (callbackHelper) {
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
            // char * errorInfo = av_err2str(r); // 根据你的返回值 得到错误详情
        }
        return;
    }
    // 查找流信息
    ret = avformat_find_stream_info(pFormatCtx, nullptr);
    if (ret != 0) {
        LOGI("查找流信息失败：%s", av_err2str(ret));
        if (callbackHelper) {
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    LOGI("流的数量：%d", pFormatCtx->nb_streams);
    for (i = 0; i < pFormatCtx->nb_streams; ++i) {
        // 获取流
        AVStream *stream = pFormatCtx->streams[i];
        // 获取流的编解码参数
        AVCodecParameters *codecParameters = stream->codecpar;
        // 获取流的编解码类型
        AVMediaType type = codecParameters->codec_type;
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            // 如果是附图，跳过
            continue;
        }
        if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
            LOGI("不是视频流或音频流，跳过");
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
            }
            continue;
        }
        // 查找解码器
        const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            LOGI("找不到解码器");
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        // 分配解码器上下文，版本3，但是这里缺乏参数
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            LOGI("分配解码器上下文失败");
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        // 将流参数拷贝到解码器上下文
        if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
            LOGI("拷贝流参数到解码器上下文失败");
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        // 打开解码器
        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            LOGI("打开解码器失败");
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }
        AVRational time_base = stream->time_base;

        if (type == AVMEDIA_TYPE_VIDEO) {
            // 视频流
            int fps = av_q2d(stream->avg_frame_rate);
            video_channel = new VideoChannel(i, codecContext, time_base,fps);
            if (renderFrame) {
                video_channel->setRenderFrame(renderFrame);
            }
        } else {
            // 音频流
            audio_channel = new AudioChannel(i, codecContext, time_base);
        }
    }
    // 健壮性校验
    if (callbackHelper != nullptr) {
        callbackHelper->onPrepared(THREAD_CHILD);
    }

}

// 主线程调用
void MurphyNativePlayer::prepare() {
    pthread_create(&pid_prepare, nullptr, prepare_thread_func, this);
}

void *start_thread_func(void *arg) {
    MurphyNativePlayer *player = static_cast<MurphyNativePlayer *>(arg);
    player->start_();

    return nullptr;//必须返回
}

void MurphyNativePlayer::start_() {
    while (isPlaying) {
        // 初步优化方案-如果队列已经满了，就休眠一下
        if (video_channel && video_channel->packets.size() > MAX_PACKET_SIZE) {
            av_usleep(SLEEP_TIME);
            continue;
        }
        if (audio_channel && audio_channel->packets.size() > MAX_PACKET_SIZE) {
            av_usleep(SLEEP_TIME);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(pFormatCtx, packet);
        if (ret == 0) {
            // 将数据包发送给对应的流
            if (video_channel && video_channel->stream_index == packet->stream_index) {
                // !!!作为生产者，一直在生产数据，可能会撑爆内存。
                video_channel->packets.push(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                audio_channel->packets.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            // TODO 读取完毕
            if (video_channel != nullptr && audio_channel != nullptr) {
                if (video_channel->packets.empty() && video_channel->frames.empty() &&
                    audio_channel->packets.empty() && audio_channel->frames.empty()) {
                    break;
                }
            }
        } else {
            break; // 出错
        }
    }
    isPlaying = false;
    if (video_channel) {
        video_channel->stop();
    }
    if (audio_channel) {
        audio_channel->stop();
    }
}

void MurphyNativePlayer::start() {
    isPlaying = true;

    if (audio_channel) {
        audio_channel->start();
    }

    if (video_channel) {
        video_channel->setAudioChannel(audio_channel);
        video_channel->start();
    }

    pthread_create(&pid_start, nullptr, start_thread_func, this);
}

void MurphyNativePlayer::setRenderFrame(RenderFrame rf) {
    renderFrame = rf;
}
