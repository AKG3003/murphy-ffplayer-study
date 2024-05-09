#include "AudioChannel.h"

AudioChannel::AudioChannel(int i, AVCodecContext *cContext)
        : BaseChannel(i, cContext) {


    // 2. 重采样 音频三要素：声道数、采样位数、采样率
    // 2.1 获取声道数
    out_channels = 2;//frame->ch_layout.nb_channels;//av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);好像没有这个函数
    // 2.2 获取采样位数
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    // 2.3 获取采样率
    out_sample_rate = 44100;
    // 这样一来，我们就确定了输出视频的格式，而输入视频的格式存储在frame中。
    // 2.6.1 创建SwrContext
    // 这边应该是ffmpeg7.0.0版本的问题，swr_alloc_set_opts已经被废弃了，需要使用swr_alloc_set_opts2
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    swr_alloc_set_opts2(&swrContext,
                        &out_ch_layout,
                        AV_SAMPLE_FMT_S16,
                        out_sample_rate,
                        &(cContext->ch_layout),
                        static_cast<AVSampleFormat>(cContext->sample_fmt),
                        cContext->sample_rate,
                        0, nullptr);
    LOGI("cContext->sample_rate:%d", cContext->sample_rate);
//    swr_alloc_set_opts2(&swrContext,
//                              &out_ch_layout,
//                              AV_SAMPLE_FMT_S16,
//                              out_sample_rate,
//                              &(frame->ch_layout),
//                              static_cast<AVSampleFormat>(frame->format),
//                              in_sample_rate,
//                              0, nullptr);

    if (swrContext) {
        // 必须要初始化一下。
        swr_init(swrContext);
    }
    // 2.4 进行临时性的大小计算，因为如果不计算，你就无法预先开辟空间。
    // 而实际上的大小需要根据系统计算出来实际的size，换句话说，先有空间再有大小。
    // swr_get_out_samples(swrContext, frame->nb_samples)
    // 这个就是系统帮助你获取的最大可能空间量，保证不溢出，虽然可能有少许空间浪费，但也是正常的。
    out_buffer_size_pre =
            out_channels * out_samplesize * swr_get_out_samples(swrContext, 1024);
    // 2.5 创建缓冲区
    // 根据计算的临时大小开辟堆空间（注意要回收）
    if (!out_buffer) {
        out_buffer = static_cast<uint8_t *>(av_malloc(out_buffer_size_pre));
    }
}

AudioChannel::~AudioChannel() {

}

void AudioChannel::stop() {

}

void *task_audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_decode();
    return nullptr;
}

void *task_audio_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_play();
    return nullptr;
}

//typedef void (*SLBufferQueueCallback)(SLBufferQueueItf caller, void *pContext);
// 每次需要的时候进行回调，执行一个frame的数据量
void audioBufferCallback(SLBufferQueueItf caller, void *pContext) {
    // 获取环境对象，否则无法操作
    AudioChannel *audioChannel = static_cast<AudioChannel *>(pContext);
    int pcm_size = audioChannel->get_pcm_size();
    (*caller)->Enqueue(caller, audioChannel->out_buffer, pcm_size);
    // 释放out_buffer
}

// 1. 解码，把队列中的数据包取出来，解码，再放入队列中。(AvPacket -> AvFrame)
// 2. 播放，把队列中的数据包取出来，播放，再放入队列中。(AvFrame -> OpenSL ES)
// 其中音频播放是通过OpenSL ES来实现的。
// (AvFormatContext -> AvCodecContext -> SwrContext -> OpenSL ES)
void AudioChannel::start() {
    // 1. 设置正在播放的标志
    isPlaying = true;

    // 2. 设置队列工作标志
    packets.setWork(1);
    frames.setWork(1);

    // 3. 启动解码线程
    pthread_create(&pid_audio_decode, nullptr, task_audio_decode, this);
    // 4. 启动播放线程
    pthread_create(&pid_audio_play, nullptr, task_audio_play, this);
}

void AudioChannel::audio_decode() {
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
        ret = avcodec_send_packet(codecContext, packet);
        // 发送完成后，释放数据包，因为ffmpeg是拷贝了一份数据包
        freePacket(&packet);
        if (ret) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        // 有可能音频帧也会获取失败，需要重新获取
        if (ret == AVERROR(EAGAIN)) {
            freeFrame(&frame);
            continue;
        } else if (ret) {
            // 如果出错，有值就需要释放
            freeFrame(&frame);
            break;
        }
        // 音频是压缩的，需要解码成PCM格式
        frames.push(frame);
    }
    freePacket(&packet);
}

void AudioChannel::audio_play() {
    // 创建SLresult对象，用于接受成功或失败的返回值
    SLresult result;

    // 1. 创建引擎
    // 1.1 创建引擎对象：SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("创建引擎对象失败");
        return;
    }
    LOGI("创建引擎对象成功");
    // 1.2 实现引擎对象
    // SL_BOOLEAN_FALSE 表示阻塞式的实现 (SL_BOOLEAN_TRUE 表示非阻塞式的实现)
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("实现引擎对象失败");
        return;
    }
    LOGI("实现引擎对象成功");
    // 1.3 获取引擎接口：SLEngineItf engineEngine.
    // SL_IID_ENGINE 表示获取引擎接口
    // SL_IID_OUTPUTMIX 表示获取混音器接口
    // SL_IID_BUFFERQUEUE 表示获取缓冲队列接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("获取引擎接口失败");
        return;
    }
    LOGI("获取引擎接口成功");
    // 2. 创建混音器
    // 2.1 创建混音器 (outputMixObject) 用于混音 (混音器是一个特殊的音频输出设备) (OpenSL ES中的音频设备)
    // SLEngineItf self:这是一个指向引擎接口的指针，它是调用此函数的引擎对象。
    // SLObjectItf *pMix:这是一个指向混音器对象的指针，它是一个输出参数，用于接收混音器对象。
    // SLuint32 numInterfaces: 这是一个无符号整数，表示要在混音器对象上实现的接口数量。
    // const SLInterfaceID *pInterfaceIds:这是一个指向接口ID数组的指针，表示要在混音器对象上实现的接口。
    // const SLboolean *pInterfaceRequired:这是一个指向布尔值数组的指针，表示每个接口是否是必需的。
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("创建混音器失败");
        return;
    }
    LOGI("创建混音器成功");
    // 2.2 实现混音器对象
    // SLboolean async: SL_BOOLEAN_FALSE 表示阻塞式的实现 (SL_BOOLEAN_TRUE 表示非阻塞式的实现)
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("实现混音器对象失败");
        return;
    }
    LOGI("实现混音器对象成功");
    // 2.3 获取混音器接口
    // SL_IID_ENVIRONMENTALREVERB 表示获取混响接口
    // void * pInterface:这是一个指向接口的指针，用于接收接口。类型是SLInterfaceID。
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = nullptr;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        // 设置混响效果
        // SL_I3DL2_ENVIRONMENT_PRESET_CAVE 表示洞穴
        // SL_I3DL2_ENVIRONMENT_PRESET_FOREST 表示森林
        // SL_I3DL2_ENVIRONMENT_PRESET_CITY 表示城市
        LOGI("设置混响效果");
        // 暂时不用
//        SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_FOREST;
//        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
//                outputMixEnvironmentalReverb, &settings);
    }
    // 3. 创建播放器
    // 在OpenSL ES中，SLDataLocator_后缀的类型主要用于描述音频数据的来源或目的地。
    // 在Android的OpenSL ES实现中，主要有以下几种类型的Queue：
    // @1.SLDataLocator_AndroidSimpleBufferQueue:
    // 这是一个简单的缓冲队列，它只包含一个缓冲区数量的字段。
    // 这个队列类型通常用于存储PCM音频数据，这些数据可以直接被音频设备播放。
    // @2.SLDataLocator_AndroidBufferQueue:
    // 这是一个更复杂的缓冲队列，它也包含一个缓冲区数量的字段.
    // 但是它可以存储更复杂的音频数据，如AAC或MP3压缩的音频数据，这些数据在被播放前需要先解码。
    // @3.SLDataLocator_AndroidFD:
    // 这是一个文件描述符数据定位器，它包含一个文件描述符、一个偏移量和一个长度。
    // 这个定位器类型通常用于从文件中读取音频数据。
    // 这三种类型的Queue都是用于描述音频数据的来源或目的地，但是它们的使用场景和能力有所不同。
    // SLDataLocator_AndroidSimpleBufferQueue和SLDataLocator_AndroidBufferQueue主要用于内存中的音频数据，而SLDataLocator_AndroidFD主要用于文件中的音频数据。
    // 同时，SLDataLocator_AndroidBufferQueue相比SLDataLocator_AndroidSimpleBufferQueue能处理更复杂的音频数据格式。
    // 3.1 创建队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            5};
    // 3.2 PCM格式
    // SLDataFormat_PCM是一个结构体，用于描述PCM音频数据的格式。
    // @SLuint32 formatType: 这是一个无符号整数，表示音频数据的格式类型。SL_DATAFORMAT_PCM表示PCM格式。SL_DATAFORMAT_MIME表示MIME格式。SL_DATAFORMAT_RESERVED3表示保留格式。
    // @SLuint32 numChannels: 这是一个无符号整数，表示音频数据的通道数。通道数是指音频数据中的声道数。
    // 在 OpenSL ES 中，numChannels 字段的最大值主要取决于你的音频设备和音频数据的能力。理论上，它可以支持任意数量的声道。然而，在实际应用中，大多数音频设备和音频数据只支持到 7.1 声道，即 8 个声道。
    // 通道数可以是1（单声道）或2（立体声）。在立体声中，通常有左声道和右声道两个声道。
    // 7.1 声道音频系统包含以下声道：左前声道、右前声道、中央声道、低音声道、左后环绕声道、右后环绕声道、左环绕声道和右环绕声道。
    // @SLuint32 samplesPerSec: 这是一个无符号整数，表示音频数据的采样率。采样率是指音频数据中的采样点数。采样率通常以赫兹（Hz）为单位，表示每秒的采样点数。
    // 在 OpenSL ES 中，samplesPerSec 字段的值主要取决于你的音频设备和音频数据的能力。理论上，它可以支持任意采样率。然而，在实际应用中，大多数音频设备和音频数据只支持几种常见的采样率，如 8kHz、16kHz、44.1kHz、48kHz 等。
    // @SLuint32 bitsPerSample: 这是一个无符号整数，表示音频数据的采样位数。采样位数是指音频数据中的每个采样点的位数。
    // 在 OpenSL ES 中，bitsPerSample 字段的值主要取决于你的音频设备和音频数据的能力。理论上，它可以支持任意采样位数。然而，在实际应用中，大多数音频设备和音频数据只支持几种常见的采样位数，如 8 位、16 位、24 位、32 位等。
    // @SLuint32 containerSize: 这是一个无符号整数，表示音频数据的容器大小。容器大小是指音频数据中的每个采样点的容器大小。
    // 在 OpenSL ES 中，containerSize 字段的值主要取决于你的音频设备和音频数据的能力。理论上，它可以支持任意容器大小。然而，在实际应用中，大多数音频设备和音频数据只支持几种常见的容器大小，如 8 位、16 位、24 位、32 位等。
    // bitsPerSample 和 containerSize 通常是相等的，但是在某些情况下，它们可能不相等。
    // TODO(不太理解)例如，当音频数据是 24 位的时候，bitsPerSample 是 24，containerSize 是 32。
    // @SLuint32 channelMask: 这是一个无符号整数，表示音频数据的声道掩码。声道掩码是指音频数据中的声道布局。
    // 在 OpenSL ES 中，channelMask 字段的值主要取决于你的音频设备和音频数据的能力。理论上，它可以支持任意声道布局。然而，在实际应用中，大多数音频设备和音频数据只支持几种常见的声道布局，如立体声、环绕声等。
    // numChannels 和 channelMask 通常是相等的，但是在某些情况下，它们可能不相等。
    // TODO（不太理解）：例如，当音频数据是立体声的时候，numChannels 是 2，channelMask 是 SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT。
    // @SLuint32 endianness: 这是一个无符号整数，表示音频数据的字节序。字节序是指音频数据中的字节顺序。
    // SL_BYTEORDER_BIGENDIAN 表示大端字节序。SL_BYTEORDER_LITTLEENDIAN 表示小端字节序。
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    // 3.3 数据源
    // SLDataSource是一个结构体，用于描述音频数据的来源。
    SLDataSource slDataSource = {&android_queue, &pcm};
    // 3.4 配置音轨
    // SLDataLocator_OutputMix设置混音器。
    // SL_DATALOCATOR_OUTPUTMIX表示音频数据的目的地是混音器。
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    // SLDataSink是一个结构体，最终成果。
    // pLocator：这是一个指向数据定位器的指针，表示音频数据的目的地。
    // pFormat：这是一个指向数据格式的指针，表示音频数据的格式。
    SLDataSink audioSink = {&outputMix, nullptr};
    // 3.5 播放器接口
    // SL_IID_BUFFERQUEUE 表示获取缓冲队列接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    // 3.6 创建播放器
    // 创建播放器对象：SLObjectItf playerObject
    // 最后三个参数用于打开队列的工作。
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &slDataSource,
                                                &audioSink, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("创建播放器失败");
        return;
    }
    LOGI("创建播放器成功");

    // 3.7 实现播放器对象
    // SL_BOOLEAN_FALSE 表示阻塞式的实现 (SL_BOOLEAN_TRUE 表示非阻塞式的实现)
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("实现播放器对象失败");
        return;
    }
    LOGI("实现播放器对象成功");

    // 3.8 获取播放器接口
    // SL_IID_PLAY 表示获取播放接口

    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("获取播放器接口失败");
        return;
    }
    LOGI("获取播放器接口成功");

    // 3.9 获取缓冲队列接口
    // SL_IID_BUFFERQUEUE 表示获取缓冲队列接口

    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("获取缓冲队列接口失败");
        return;
    }
    LOGI("获取缓冲队列接口成功");

    // 3.10 设置回调函数
    // SLBufferQueueItf playerBufferQueue:这是一个指向缓冲队列接口的指针，表示要设置回调函数的缓冲队列。
    // void * pCallback:这是一个指向回调函数的指针，表示要设置的回调函数。回调函数的类型是SLBufferQueueCallback。
    // void * pContext:这是一个指向上下文的指针，表示要传递给回调函数的上下文。
    // SLBufferQueueCallback是一个函数指针类型，用于描述缓冲队列的回调函数。
    // typedef void (*SLBufferQueueCallback)(SLBufferQueueItf caller, void *pContext);
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, audioBufferCallback, this);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("设置回调函数失败");
        return;
    }
    LOGI("设置回调函数成功");

    // 3.11 设置播放状态
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != result) {
        LOGI("设置播放状态失败");
        return;
    }
    LOGI("设置播放状态成功");

    // 3.12 播放 激活回调函数 (回调函数是在子线程中执行的)
    audioBufferCallback(playerBufferQueue, this);
}

int AudioChannel::get_pcm_size() {
    // 需要计算的重采样之后的数据大小。之所以有actual，是因为需要提前开辟空间，有个临时的size。（不够准确）
    int out_buffer_size_actual = 0;
    int ret = 0;
    AVFrame *frame = nullptr;
    while (isPlaying) {
        // 1. 从队列中取出一个AVFrame
        ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            freeFrame(&frame);
            continue;
        }

        // 2.6 重采样

        // 输入视频的采样率（测试视频的是48000hz,2声道,16bit仅供参考）
        int in_sample_rate = frame->sample_rate;
        // 计算转换后的sample个数 a * b / c
        // 要注意这个nb_samples的含义，它就是指每次回调之后，这一个数据包所持续时间内的采样数，也就是一帧的时间x采样率。
        // 但是这个输入采样数是系统给的，千万不能自己算，这里打印输出是1024，大体上是0.021s，也就是48000x0.021（大约）
        int in_nb_samples = frame->nb_samples;
        // 这里通过av_rescale_rnd计算输出样本数，也是指相同时间内输出的样本个数，用来保证不同频率下时间一致，因为不同频率采集的样本量是不同的。
        // 但也要明白这里只是理论上，实际上会有一定出入，所以这里并不是最终答案。
        // av_rescale_rnd 这个本身是ffmpeg方舟数据溢出的算法，本质等于 axb/c 也就是 1024 * 44100 / 48000 = 941（向上取整）跟打印输出一致。
        int out_nb_samples = av_rescale_rnd(
                swr_get_delay(swrContext, in_sample_rate) + in_nb_samples,
                out_sample_rate, in_sample_rate, AV_ROUND_UP);
        // 2.6.2 重采样 这里是真正执行，也就是把重采样的数据放到out_buffer当中，而返回值就是真正意义上的重采样后的输出样本数。
        int out_convert_samples = swr_convert(swrContext, &out_buffer, out_nb_samples, frame->data,
                                              in_nb_samples);
        if (out_convert_samples < 0) {
            freeFrame(&frame);
            continue;
        }
        // 3. 播放
        // 为什么传递自己进去，因为不是C++没有this指针
        // const void *pBuffer : 这是一个指向缓冲区的指针，表示要播放的音频数据。这里是PCM数据。
        // SLuint32 out_buffer_size_actual : 这是一个无符号整数，表示要播放的音频数据的大小。这里是PCM数据的大小。
        // 实际数值就是：实际样本数x位数x通道数
        out_buffer_size_actual = static_cast<SLuint32>(out_convert_samples * out_channels *
                                                       out_samplesize);
        freeFrame(&frame);
        break;
    }
    // 输出数据，输出数据大小，就是这么得到的。
    freeFrame(&frame);
    return out_buffer_size_actual;
}
