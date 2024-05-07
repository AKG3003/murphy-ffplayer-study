package com.study.ndk22_compile_study

import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.study.ndk22_compile_study.player.PlayerErrorListener
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_ALLOC_CODEC_CONTEXT_FAIL
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_CAN_NOT_FIND_STREAMS
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_CAN_NOT_OPEN_URL
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_FIND_DECODER_FAIL
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_NOMEDIA
import com.study.ndk22_compile_study.player.PlayerErrorListener.Companion.FFMPEG_OPEN_DECODER_FAIL
import com.study.ndk22_compile_study.player.PlayerProgressListener
import com.study.ndk22_compile_study.player.PlayerStateListener

class MurphyPlayer : SurfaceHolder.Callback {
    private var stateListener: PlayerStateListener? = null
    private var errorListener: PlayerErrorListener? = null
    private var progressListener: PlayerProgressListener? = null
    private var dataSource: String? = null
    private var surfaceHolder: SurfaceHolder? = null

    fun setDataSource(dataSource: String) {
        Log.i("MurphyPlayer", "setDataSource: $dataSource")
        this.dataSource = dataSource
    }

    fun setStateListener(listener: PlayerStateListener) {
        this.stateListener = listener
    }

    fun setErrorListener(listener: PlayerErrorListener) {
        this.errorListener = listener
    }

    fun setProgressListener(listener: PlayerProgressListener) {
        this.progressListener = listener
    }


    fun prepare() {
        dataSource?.let {
            prepareNative(it)
        }
    }

    fun start() {
        startNative()
    }

    fun onPrepared() {
        Log.i("MurphyPlayer", "native:onPrepared")
        stateListener?.onPrepared()
    }

    fun onError(errorCode: Int) {
        errorListener?.let {
            var msg: String? = null
            when (errorCode) {
                FFMPEG_CAN_NOT_OPEN_URL -> msg = "打不开视频"
                FFMPEG_CAN_NOT_FIND_STREAMS -> msg = "找不到流媒体"
                FFMPEG_FIND_DECODER_FAIL -> msg = "找不到解码器"
                FFMPEG_ALLOC_CODEC_CONTEXT_FAIL -> msg = "无法根据解码器创建上下文"
                FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL -> msg = "根据流信息 配置上下文参数失败"
                FFMPEG_OPEN_DECODER_FAIL -> msg = "打开解码器失败"
                FFMPEG_NOMEDIA -> msg = "没有音视频"
            }
            it.onError(errorCode, msg)
        }
    }

    //native methods

    private external fun prepareNative(dataSource: String)
    private external fun startNative()
    private external fun pauseNative()
    private external fun stopNative()
    private external fun completeNative()

    private external fun setSurfaceNative(surface: Surface)


    companion object {
        // Used to load the 'ndk22_compile_study' library on application startup.
        init {
            System.loadLibrary("ndk22_compile_study")
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        setSurfaceNative(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
    }

    fun setSurfaceView(surfaceView: SurfaceView) {
        surfaceHolder?.removeCallback(this)
        surfaceHolder = surfaceView.holder
        surfaceHolder?.addCallback(this)
    }
}