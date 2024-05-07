package com.study.ndk22_compile_study.player

interface PlayerErrorListener {
    fun onError(errorCode: Int, msg:String? = null)

    companion object {
        // 错误代码 ================ 如下
        // 打不开视频
        // #define FFMPEG_CAN_NOT_OPEN_URL 1
        val FFMPEG_CAN_NOT_OPEN_URL = 1

        // 找不到流媒体
        // #define FFMPEG_CAN_NOT_FIND_STREAMS 2
        val FFMPEG_CAN_NOT_FIND_STREAMS = 2

        // 找不到解码器
        // #define FFMPEG_FIND_DECODER_FAIL 3
        val FFMPEG_FIND_DECODER_FAIL = 3

        // 无法根据解码器创建上下文
        // #define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
        val FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 4

        //  根据流信息 配置上下文参数失败
        // #define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
        val FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 6

        // 打开解码器失败
        // #define FFMPEG_OPEN_DECODER_FAIL 7
        val FFMPEG_OPEN_DECODER_FAIL = 7

        // 没有音视频
        // #define FFMPEG_NOMEDIA 8
        val FFMPEG_NOMEDIA = 8
    }
}