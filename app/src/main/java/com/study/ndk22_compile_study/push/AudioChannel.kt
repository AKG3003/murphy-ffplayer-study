package com.study.ndk22_compile_study.push

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

@SuppressLint("MissingPermission")
class AudioChannel(private val pusher: Pusher) {

    private var isLive: Boolean = false
    private val channels: Int = 2
    private val audioRecord: AudioRecord
    private val executorService: ExecutorService = Executors.newSingleThreadExecutor()// 线程池
    private var inputSamples: Int = 4096

    init {

        pusher.native_initAudioEncoder(44100, channels)

        inputSamples = pusher.native_getInpushSamples() * 2

        val minBufferSize = AudioRecord.getMinBufferSize(
            44100,
            channels,
            AudioFormat.ENCODING_PCM_16BIT
        )

        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            44100,
            channels,
            AudioFormat.ENCODING_PCM_16BIT,
            Math.max(minBufferSize, inputSamples)
        )

    }

    fun startPush() {
        isLive = true
        executorService.submit {
            // 录制音频数据，把数据传给Native层，封包进行发送。
            audioRecord.startRecording()
            val buffer = ByteArray(inputSamples)
            while (isLive) {
                val len = audioRecord.read(buffer, 0, buffer.size)
                if (len > 0) {
                    pusher.native_pushAudio(buffer, len)
                }
            }
        }
    }

    fun stopPush() {
        isLive = false
    }

    fun release() {
        audioRecord.release()
    }

}