//
// Created by 王一 on 2024/5/14.
//

#include <string.h>
#include "LiveAudioChannel.h"


LiveAudioChannel::LiveAudioChannel() {

}

LiveAudioChannel::~LiveAudioChannel() {
    if (buffer) {
        delete buffer;
        buffer = nullptr;
    }
    if (audioCodec) {
        faacEncClose(audioCodec);
        audioCodec = nullptr;
    }
}

void LiveAudioChannel::initAudioEncoder(int sample_rate, int num_channels) {
    this->m_chanels = num_channels;
    // 1.打开编码器
    audioCodec = faacEncOpen(sample_rate, num_channels, &inputSamples, &maxOutputBytes);
    if (!audioCodec) {
        LOGI("faacEncOpen failed");
        return;
    }
    // 2.设置编码参数
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioCodec);
    config->mpegVersion = MPEG4;
    config->aacObjectType = LOW; // aac LC标准，低规格
    config->inputFormat = FAAC_INPUT_16BIT;
    config->outputFormat = 0; // 0代表原始数据 Raw
    config->useTns = 1; // 时域噪音控制
    config->useLfe = 0; // 低频增强

    // 3.把配置参数传给faac编码器
    int ret = faacEncSetConfiguration(audioCodec, config);
    if (!ret) {
        LOGI("faacEncSetConfiguration failed");
        return;
    }

    // 4.初始化buffer
    buffer = new u_char[maxOutputBytes];

}

jint LiveAudioChannel::getInputSamples() {
    return inputSamples;
}

void LiveAudioChannel::encodeData(int32_t *data) {
    if (!audioCodec) {
        return;
    }
    // 5.编码
    // faacEncHandle hEncoder 初始化好的编码器
    // int32_t *inputBuffer 输入数据
    // unsigned int samplesInput 输入数据的样本数
    // unsigned char *outputBuffer 输出数据
    // unsigned int bufferSize 输出数据的大小
    u_long len = faacEncEncode(audioCodec, data, inputSamples, buffer, maxOutputBytes);
    if (len > 0) {
        // 6.发送数据
        // 7.发送AAC头
        // 8.发送AAC数据
        RTMPPacket *packet = new RTMPPacket;
        int body_size = 2 + len;
        RTMPPacket_Alloc(packet, body_size);
        // 2.发送AAC头 二进制数据 1010 1111 0000 0001
        // 0xAF 0x01 前4位代表了格式 10代表AAC。后2位代表音频采样率（AAC总是3），再1位代表采样长度（AAC总是1，16bit），再1位代表音频类型（AAC总是1，双声道）
        packet->m_body[0] = 0xAF;
        // 0x00 是序列头，0x01 是数据
        // 只有序列头才会增加AudioSpecificConfig字段
        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2], buffer, len);

        // 9.发送数据
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = body_size;
        packet->m_nChannel = 0x06;
        packet->m_nTimeStamp = -1;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

        // 10.发送
        audioCallback(packet);

    }
}

void LiveAudioChannel::setVideoCallback(AudioCallback callback) {
    audioCallback = callback;
}

void LiveAudioChannel::addAudioHeader() {
    // 获取音频头
    u_char *buf;
    u_long len;
    faacEncGetDecoderSpecificInfo(audioCodec, &buf, &len);
    RTMPPacket *packet = new RTMPPacket;
    int body_size = 2 + len; // 一般来说len就是2
    RTMPPacket_Alloc(packet, body_size);
    packet->m_body[0] = 0xAF;
    packet->m_body[1] = 0x00;
    memcpy(&packet->m_body[2], buf, len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x06;
    packet->m_nTimeStamp = 0;//头一般不添加时间戳
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    audioCallback(packet);
}

void LiveAudioChannel::release() {

}
