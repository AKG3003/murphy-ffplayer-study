package com.study.ndk22_compile_study.push

import android.app.Activity
import android.view.SurfaceHolder

class Pusher(
    activity: Activity,
    cameraId: Int,
    private val width: Int,
    private val height: Int,
    private val fps: Int,
    private val bitRate: Int
) {

    companion object {
        init {
            System.loadLibrary("native-rtmp")
        }
    }

    private var audioChannel: AudioChannel = AudioChannel(this)
    private var videoChannel: VideoChannel = VideoChannel(this, activity, cameraId, width, height, fps, bitRate)

    // 1.初始化native层的需要加载。
    // 2.实例化视频通道并传递基本参数。
    init {
        native_init()
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        videoChannel.switchCamera()
    }

    fun startPush(path: String) {
        native_start(path)
        videoChannel.startPush()
        audioChannel.startPush()
    }

    fun stopPush() {
        videoChannel.stopPush()
        audioChannel.stopPush()
        native_stop()
    }

    fun release() {
        videoChannel.release()
        audioChannel.release()
        native_release()
    }

    // 初始化
    external fun native_init()

    // 开始直播
    external fun native_start(path: String)

    // 停止直播
    external fun native_stop()

    // 释放资源
    external fun native_release()

    // 视频发送数据到native层，NV21格式
    external fun native_pushVideo(data: ByteArray?)

    // 视频初始化x264编码器
    external fun native_initVideoEncoder(width: Int, height: Int, fps: Int, bitRate: Int)

    // 音频初始化faac编码器
    external fun native_initAudioEncoder(i: Int, channels: Int)

    // 获取音频采样率
    external fun native_getInpushSamples(): Int

    // 音频发送数据到native层
    external fun native_pushAudio(buffer: ByteArray, len: Int)
}