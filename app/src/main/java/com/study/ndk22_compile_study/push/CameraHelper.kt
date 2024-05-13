package com.study.ndk22_compile_study.push

import android.app.Activity
import android.graphics.ImageFormat
import android.hardware.Camera
import android.view.SurfaceHolder

class CameraHelper(activity: Activity, cameraId: Int, width: Int, height: Int, rotation: Int) :
    SurfaceHolder.Callback, Camera.PreviewCallback {

    private val TAG = "CameraHelper"

    private var mActivity: Activity? = null
    private var mHeight: Int = 0
    private var mWidth: Int = 0
    private var mRotation: Int = 0
    private var mCameraId: Int = 0
    private var mBuffer: ByteArray? = null

    private var mCamera: Camera? = null
    private var mSurfaceHolder: SurfaceHolder? = null
    private var mPreviewCallback: Camera.PreviewCallback? = null
    private var mOnChangedSizeListener: OnChangedSizeListener? = null

    init {
        mActivity = activity
        mCameraId = cameraId
        mWidth = width
        mHeight = height
        mRotation = rotation
    }

    fun switchCamera() {
        mCameraId = if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            Camera.CameraInfo.CAMERA_FACING_FRONT
        } else {
            Camera.CameraInfo.CAMERA_FACING_BACK
        }
        stopPreview()
        startPreview()
    }

    fun startPreview() {
        if (mCamera != null) {
            stopPreview()
        }
        mCamera = Camera.open(mCameraId)
        mCamera?.let {
            val parameters = it.parameters
            val size = getBestSupportedSize(parameters.supportedPreviewSizes, mWidth, mHeight)
            parameters.setPreviewSize(size.width, size.height)
            parameters.previewFormat = ImageFormat.NV21
            setPreviewSize(parameters, mWidth, mHeight)
            setPreviewOrientation(it, mCameraId)
            it.parameters = parameters
            mBuffer = ByteArray(size.width * size.height * 3 / 2)
            try {
                it.addCallbackBuffer(mBuffer)
                it.setPreviewCallbackWithBuffer(this)
                //                it.setPreviewCallback(this)
                it.setPreviewDisplay(mSurfaceHolder)
                if (mOnChangedSizeListener != null) {
                    mOnChangedSizeListener?.onChanged(size.height, size.width)
                }
                it.startPreview()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    private fun getBestSupportedSize(
        supportedPreviewSizes: List<Camera.Size>,
        width: Int,
        height: Int
    ): Camera.Size {
        var bestSize = supportedPreviewSizes[0]
        var minDiff = Int.MAX_VALUE
        for (size in supportedPreviewSizes) {
            val diff = Math.abs(size.width - width) + Math.abs(size.height - height)
            if (diff < minDiff) {
                bestSize = size
                minDiff = diff
            }
        }
        return bestSize
    }

    private fun setPreviewOrientation(camera: Camera, cameraId: Int) {
        val info = Camera.CameraInfo()
        Camera.getCameraInfo(cameraId, info)
        val rotation = mActivity?.windowManager?.defaultDisplay?.rotation
        var degrees = 0
        when (rotation) {
            0 -> degrees = 0
            1 -> degrees = 90
            2 -> degrees = 180
            3 -> degrees = 270
        }
        var result = 0
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360
            result = (360 - result) % 360
        } else {
            result = (info.orientation - degrees + 360) % 360
        }
        camera.setDisplayOrientation(result)
    }

    private fun setPreviewSize(parameters: Camera.Parameters, width: Int, height: Int) {
        val size = getBestSupportedSize(parameters.supportedPreviewSizes, width, height)
        parameters.setPreviewSize(size.width, size.height)
    }

    fun stopPreview() {
        mCamera?.let {
            it.setPreviewCallback(null)
            it.stopPreview()
            it.release()
            mCamera = null
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        stopPreview()
        startPreview()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        stopPreview()
    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
        if (mPreviewCallback != null) {
            val rotatedData = data?.let { rotateYUV420Degree90(it, camera?.parameters?.previewSize?.width ?: 0, camera?.parameters?.previewSize?.height ?: 0) }
            mPreviewCallback?.onPreviewFrame(rotatedData, camera)
        }
        camera?.addCallbackBuffer(mBuffer)
    }

    fun setPreviewCallback(previewCallback: Camera.PreviewCallback) {
        mPreviewCallback = previewCallback
    }

    fun setOnChangedSizeListener(listener: OnChangedSizeListener) {
        mOnChangedSizeListener = listener
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        mSurfaceHolder = surfaceHolder
        mSurfaceHolder?.addCallback(this)
    }

    private fun rotateYUV420Degree90(data: ByteArray, imageWidth: Int, imageHeight: Int): ByteArray {
        val yuv = ByteArray(imageWidth * imageHeight * 3 / 2)
        var i = 0
        for (x in 0 until imageWidth) {
            for (y in imageHeight - 1 downTo 0) {
                yuv[i] = data[y * imageWidth + x]
                i++
            }
        }
        i = imageWidth * imageHeight * 3 / 2 - 1
        for (x in imageWidth - 1 downTo 0) {
            for (y in imageHeight / 2 - 1 downTo 0) {
                yuv[i] = data[imageWidth * imageHeight + y * imageWidth + x]
                i--
                yuv[i] = data[imageWidth * imageHeight + y * imageWidth + x - 1]
                i--
            }
        }
        return yuv
    }



    interface OnChangedSizeListener {
        fun onChanged(height: Int, width: Int) {
            // do nothing
        }
    }
}