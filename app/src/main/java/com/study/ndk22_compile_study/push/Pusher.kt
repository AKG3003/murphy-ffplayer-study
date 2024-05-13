package com.study.ndk22_compile_study.push

import android.app.Activity
import android.hardware.Camera
import android.view.SurfaceHolder
import android.view.View

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

    private var audioChannel: AudioChannel = AudioChannel()
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

    // 视频发送数据到native层
    external fun native_pushVideo(data: ByteArray?)

    // 视频初始化x264编码器
    external fun native_initVideoEncoder(width: Int, height: Int, fps: Int, bitRate: Int)
}