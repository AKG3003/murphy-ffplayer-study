package com.study.ndk22_compile_study.player

interface PlayerStateListener {
    fun onPrepared()
    fun onStarted()
    fun onPaused()
    fun onStopped()
    fun onCompleted()
}