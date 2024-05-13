package com.study.ndk22_compile_study.push

import android.app.Activity
import android.hardware.Camera
import android.view.SurfaceHolder

class VideoChannel(
    private val pusher: Pusher,
    activity: Activity,
    cameraId: Int,
    private val width: Int,
    private val height: Int,
    private val fps: Int,
    private val bitRate: Int
) : Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {

    private var cameraHelper: CameraHelper = CameraHelper(activity, cameraId, width, height, 0)
    private var isLive: Boolean = false //是否直播,通过这个标记控制是否发送数据给native层

    init {
        cameraHelper.setPreviewCallback(this)
        cameraHelper.setOnChangedSizeListener(this)
    }

    // 绑定SurfaceHolder
    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        cameraHelper.switchCamera()
    }

    fun startPush() {
        isLive = true
    }

    fun stopPush() {
        isLive = false
    }

    fun release() {
        cameraHelper.startPreview()
    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
        if (isLive) {
            pusher.native_pushVideo(data)
        }
    }

    override fun onChanged(height: Int, width: Int) {
        pusher.native_initVideoEncoder(width, height, fps, bitRate)
    }
}