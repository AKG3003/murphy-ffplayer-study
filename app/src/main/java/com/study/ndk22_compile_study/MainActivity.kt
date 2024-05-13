package com.study.ndk22_compile_study

import android.content.Intent
import android.graphics.Color
import android.hardware.Camera
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.view.SurfaceView
import android.view.View
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.study.ndk22_compile_study.databinding.ActivityMainBinding
import com.study.ndk22_compile_study.player.PlayerErrorListener
import com.study.ndk22_compile_study.player.PlayerProgressListener
import com.study.ndk22_compile_study.player.PlayerStateListener
import com.study.ndk22_compile_study.push.Pusher

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var pusher: Pusher

    private val REQUEST_CODE_PERMISSIONS = 1001

    private var player: MurphyPlayer = MurphyPlayer()

    private lateinit var surfaceView: SurfaceView

    private var seekBar: SeekBar? = null
    private var tvTime: TextView? = null
    private var isTouch: Boolean = false
    private var durationStr: String = "00:00"

    private var playerStateListener = object : PlayerStateListener {
        override fun onPrepared() {
            changeShowText("onPrepared")
            runOnUiThread {
                val duration = player.getDuration()
                Log.i("Murphy", "onPrepared: duration=$duration")
                if (duration != 0L) {
                    durationStr = convertDurationToMinSec(duration)
                    tvTime?.text = "00:00/" + durationStr
                    tvTime?.visibility = View.VISIBLE

                    seekBar?.max = duration.toInt()
                    seekBar?.progress = 0
                    seekBar?.visibility = View.VISIBLE
                }
            }
            player.start()
        }

        override fun onStarted() {
            changeShowText("onStarted")
        }

        override fun onPaused() {
            changeShowText("onPaused")
        }

        override fun onStopped() {
            changeShowText("onStopped")
        }

        override fun onCompleted() {
            changeShowText("onCompleted")
        }
    }

    private var playerErrorListener = object : PlayerErrorListener {

        override fun onError(errorCode: Int, msg: String?) {
            changeShowText(msg ?: "未知错误", Color.RED)
        }
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!Environment.isExternalStorageManager()) {
            val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
            intent.addCategory("android.intent.category.DEFAULT")
            intent.data = Uri.parse(String.format("package:%s", applicationContext.packageName))
            startActivityForResult(intent, REQUEST_CODE_PERMISSIONS)
        }

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        surfaceView = binding.surfaceView

        seekBar = binding.seekBar
        tvTime = binding.tvTime

        seekBar?.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    val currentTimeStr = convertDurationToMinSec(progress.toLong())
                    tvTime?.text = "$currentTimeStr/$durationStr"
//                    Log.i("Murphy", "onProgressChanged: $progress")
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                isTouch = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                isTouch = false

                player.seekTo(seekBar?.progress ?: 0)
            }
        })

        player.let { player ->
            getExternalStorageDemoMp4Path().let { path ->
                player.setDataSource(path)
                player.setSurfaceView(surfaceView)
                player.setStateListener(playerStateListener)
                player.setErrorListener(playerErrorListener)
            }
            player.setProgressListener(object : PlayerProgressListener {
                override fun onProgress(progress: Int) {
                    if (!isTouch) {
                        runOnUiThread {
                            seekBar?.progress = progress
                            val currentTimeStr = convertDurationToMinSec(progress.toLong())
                            tvTime?.text = "$currentTimeStr/$durationStr"
                        }
                    }
                }
            })
        }

        player.showRTMP()

        pusher = Pusher(this, Camera.CameraInfo.CAMERA_FACING_BACK, 640, 480, 25, 800 * 1000)
        pusher.setPreviewDisplay(surfaceView.holder)
    }

    private fun changeShowText(text: String, color: Int = Color.BLACK) {
        Log.i("MainActivity", "changeShowText: $text")
        runOnUiThread {
            binding.sampleText.let {
                it.setTextColor(color)
                it.text = text
            }
        }
    }

    private fun convertDurationToMinSec(duration: Long): String {
        val totalSeconds = duration
        val minutes = totalSeconds / 60
        val seconds = totalSeconds % 60
        return String.format("%02d:%02d", minutes, seconds)
    }

    override fun onResume() {
        super.onResume()
        player.prepare()
    }

    override fun onStop() {
        super.onStop()
        player.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        player.release()
        pusher.release()
    }

    private fun getExternalStorageDemoMp4Path(): String {
        return "rtmp://live.hkstv.hk.lxdns.com/live/hks"
//        return Environment.getExternalStorageDirectory().absolutePath + "/Download/demo.mp4"
    }

    fun switchCamera() {
        pusher.switchCamera()
    }

    fun startPush(path: String) {
        pusher.startPush(path)
    }

    fun stopPush() {
        pusher.stopPush()
    }


    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (Environment.isExternalStorageManager()) {
                // Permission granted
            } else {
                // Permission not granted
            }
        }
    }

}