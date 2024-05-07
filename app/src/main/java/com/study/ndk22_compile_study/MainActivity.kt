package com.study.ndk22_compile_study

import android.content.Intent
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity
import com.study.ndk22_compile_study.databinding.ActivityMainBinding
import com.study.ndk22_compile_study.player.PlayerErrorListener
import com.study.ndk22_compile_study.player.PlayerStateListener

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private val REQUEST_CODE_PERMISSIONS = 1001

    private var player: MurphyPlayer = MurphyPlayer()

    private lateinit var surfaceView: SurfaceView

    private var playerStateListener = object : PlayerStateListener {
        override fun onPrepared() {
            changeShowText("onPrepared")
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

        player.let { player ->
            getExternalStorageDemoMp4Path().let { path ->
                player.setDataSource(path)
                player.setSurfaceView(surfaceView)
                player.setStateListener(playerStateListener)
                player.setErrorListener(playerErrorListener)
            }
        }
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

    override fun onResume() {
        super.onResume()
        player.prepare()
    }

    override fun onStop() {
        super.onStop()
    }

    override fun onDestroy() {
        super.onDestroy()
    }

    private fun getExternalStorageDemoMp4Path(): String {
        return Environment.getExternalStorageDirectory().absolutePath + "/Download/demo.mp4"
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