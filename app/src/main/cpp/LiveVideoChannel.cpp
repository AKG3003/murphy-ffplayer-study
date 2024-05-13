
#include "LiveVideoChannel.h"


void LiveVideoChannel::initVideoEncoder(int width, int height, int fps, int bitrate) {
    pthread_mutex_lock(&mutex);
    this->width = width;
    this->height = height;
    this->fps = fps;
    this->bitrate = bitrate;

    y_len = width * height;
    uv_len = y_len / 4;

    if (videoEncoder) {
        x264_encoder_close(videoEncoder);
        videoEncoder = nullptr;
    }

    if (pic_in) {
        x264_picture_clean(pic_in);
        delete pic_in;
        pic_in = nullptr;
    }

    // 初始化x264编码器
    x264_param_t param;

    // 设置编码器属性
    // ultrafast 最快
    // zerolatency 零延迟
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    // 编码输入的像素格式
    param.i_csp = X264_CSP_I420;
    // 编码规格
    param.i_level_idc = 32;
    // 直播不能有B帧，会影响编码效率，所以设置为0
    param.i_bframe = 0;
    // 码率的控制方式：bitrate(恒定码率)，crf(恒定质量)，abr(平均码率)
    param.rc.i_rc_method = X264_RC_ABR;
    // 码率(比特率，单位：kbps)
    param.rc.i_bitrate = bitrate / 1000;
    // 瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    // 设置码率控制区大小，单位：kbps
    param.rc.i_vbv_buffer_size = bitrate / 1000;
    // 码率控制不是通过时间控制，而是通过比特率控制
    param.b_vfr_input = 0;
    // 帧率分子
    param.i_fps_num = fps;
    // 帧率分母
    param.i_fps_den = 1;
    // 时间基分子
    param.i_timebase_den = param.i_fps_num;
    // 时间基分母
    param.i_timebase_num = param.i_fps_den;
    // 关键帧最大间隔，一般设置为帧率的两倍
    param.i_keyint_max = fps * 2;
    // 设置sps序列参数，pps图像参数及，所以需要设置header
    // 是否复制sps和pps到每个关键帧前面， 让每个关键帧带上sps和pps
    param.b_repeat_headers = 1;
    // 并行编码数
    param.i_threads = 1;
    // 设置profile，baseline是基本的，main是主流，high是高级，把上面参数进行提交
    x264_param_apply_profile(&param, "baseline");

    // 输入图像初始化
    // 本身空间的初始化
    pic_in = new x264_picture_t;
    // pic_in内部成员初始化等
    x264_picture_alloc(pic_in, param.i_csp, param.i_width, param.i_height);

    // 打开编码器 x264_encoder_open
    videoEncoder = x264_encoder_open(&param);
    if (videoEncoder) {
        LOGI("x264编码器打开成功");
    } else {
        LOGI("x264编码器打开失败");
    }

    pthread_mutex_unlock(&mutex);
}

LiveVideoChannel::LiveVideoChannel() {
    pthread_mutex_init(&mutex, nullptr);
}

LiveVideoChannel::~LiveVideoChannel() {
    pthread_mutex_destroy(&mutex);
}

void LiveVideoChannel::encodeData(signed char *data) {
    pthread_mutex_lock(&mutex);
    //NV21 -> I420
    //NV21: YYYYYYYY VU VU
    //I420: YYYYYYYY UU VV
    memcpy(pic_in->img.plane[0], data, y_len);
    for (int i = 0; i < uv_len; ++i) {
        //U数据
        *(pic_in->img.plane[1] + i) = *(data + y_len + i * 2 + 1);
        //V数据
        *(pic_in->img.plane[2] + i) = *(data + y_len + i * 2);
    }

    x264_nal_t *pp_nal = nullptr;//通过H.264编码得到的NAL数组
    int pi_nal;//pi_nal是pp_nal的个数
    x264_picture_t pic_out;//输出的图片（压缩后的）

    //视频编码器
    int ret = x264_encoder_encode(videoEncoder, &pp_nal, &pi_nal, pic_in, &pic_out);
    if (ret < 0) {
        LOGI("x264编码失败");
        pthread_mutex_unlock(&mutex);
        return;
    }

    // 发送packet 入队
    // sps序列参数及，pps图像参数集，告诉我们如何解码图像数据。
    int sps_len, pps_len;
    unsigned char sps[100];// 100是随便写的，实际上不知道多大
    unsigned char pps[100];// 100是随便写的，实际上不知道多大
    pic_in->i_pts += 1;// pts是时间戳，每次加1，目标是让pts和dts一样，dts是解码时间戳

    // NAL的单元是不同的，有的是sps，有的是pps，有的是关键帧，有的是非关键帧
    for (int i = 0; i < pi_nal; ++i) {
        if (pp_nal[i].i_type == NAL_SPS) {
            // 计算SPS的长度。pp_nal[i].i_payload是NAL单元的负载长度，减去4是因为H.264的NAL单元头部通常是4个字节。
            sps_len = pp_nal[i].i_payload - 4;
            // 将SPS的数据（跳过前4个字节的头部）复制到sps数组中。
            memcpy(sps, pp_nal[i].p_payload + 4, sps_len);
        } else if (pp_nal[i].i_type == NAL_PPS) {
            // 这两行代码的作用和处理SPS的代码类似，只不过这里处理的是PPS
            pps_len = pp_nal[i].i_payload - 4;
            memcpy(pps, pp_nal[i].p_payload + 4, pps_len);

            // 发送sps和pps，这两个数据是固定的，只需要发送一次,pps在sps后面。
            // 这里拿到sps和pps后，就可以发送视频数据了。
            // 1.发送sps和pps
            sendSpsPps(sps, sps_len, pps, pps_len);
        } else {
            // 发送视频数据
            // 1.发送sps和pps
            // 2.发送关键帧和非关键帧
            setFrame(pp_nal[i].i_type, pp_nal[i].i_payload, pp_nal[i].p_payload);
        }
    }

    pthread_mutex_unlock(&mutex);
}

void
LiveVideoChannel::sendSpsPps(unsigned char *sps, int sps_len, unsigned char *pps, int pps_len) {
    // 5代表NALU的前缀长度，8代表NALU的类型，3代表NALU的后缀长度，这些都是固定的。
    int body_size = 5 + 8 + sps_len + 3 + pps_len;
    RTMPPacket *packet = new RTMPPacket;// 开始封包
    RTMPPacket_Alloc(packet, body_size); //堆区实例化
    int i = 0;
    // 固定的NALU前缀
    packet->m_body[i++] = 0x17;// 1:I帧，7:AVC
    packet->m_body[i++] = 0x00;// AVC sequence header
    packet->m_body[i++] = 0x00;// composition time
    packet->m_body[i++] = 0x00;// composition time
    packet->m_body[i++] = 0x00;// composition time
    // sps
    packet->m_body[i++] = 0x01;// configurationVersion
    packet->m_body[i++] = sps[1];// AVCProfileIndication
    packet->m_body[i++] = sps[2];// profile_compatibility
    packet->m_body[i++] = sps[3];// AVCLevelIndication
    packet->m_body[i++] = 0xff;// lengthSizeMinusOne
    packet->m_body[i++] = 0xe1;// numOfSequenceParameterSets
    // sps长度
    packet->m_body[i++] = (sps_len >> 8) & 0xff;
    packet->m_body[i++] = sps_len & 0xff;
    // sps数据
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;
    // pps
    packet->m_body[i++] = 0x01;// numOfPictureParameterSets
    // pps长度
    packet->m_body[i++] = (pps_len >> 8) & 0xff;
    packet->m_body[i++] = pps_len & 0xff;
    memcpy(&packet->m_body[i], pps, pps_len);
    i += pps_len;

    // 封包处理
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;//视频通道
    packet->m_nTimeStamp = -1;//绝对时间
    packet->m_hasAbsTimestamp = 0;//相对时间
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    // 发送
    videoCallback(packet);
}

void LiveVideoChannel::setVideoCallback(VideoCallback callback) {
    this->videoCallback = callback;
}

// type: 0x05:关键帧，0x01:非关键帧
// i_payload: 数据长度
// p_payload: 数据
void LiveVideoChannel::setFrame(int type, int i_payload, uint8_t *p_payload) {
    if (p_payload == nullptr) {
        return;
    }
    if (p_payload[2] == 0x00) {
        i_payload -= 4;
        p_payload += 4;
    } else if (p_payload[2] == 0x01) {
        i_payload -= 3;
        p_payload += 3;
    }
    int body_size = 5 + 4 + i_payload;
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, body_size);
    if (type == NAL_SLICE_IDR) {
        packet->m_body[0] = 0x17;
    } else {
        packet->m_body[0] = 0x27;
    }
    packet->m_body[1] = 0x01;
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    // 需要用四个字节表达一个长度，需要位移。
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = i_payload & 0xff;
    memcpy(&packet->m_body[9], p_payload, i_payload);

    // 封包
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = RTMP_GetTime();
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

        videoCallback(packet);

}
