#include "JNICallbackHelper.h"


JNICallbackHelper::JNICallbackHelper(JavaVM *pVm, JNIEnv *pEnv, jobject obj) {
    this->pVm = pVm;
    this->pEnv = pEnv;
    this->obj = pEnv->NewGlobalRef(obj); // 不能跨越线程，不能跨越函数，必须全局引用

    jclass clazz = pEnv->GetObjectClass(obj);
    if (clazz == nullptr) {
        return;
    }
    jmd_prepared = pEnv->GetMethodID(clazz, "onPrepared", "()V");
//    jmd_started = pEnv->GetMethodID(callbackClazz, "startedNative", "()V");
//    jmd_paused = pEnv->GetMethodID(callbackClazz, "pausedNative", "()V");
//    jmd_stopped = pEnv->GetMethodID(callbackClazz, "stoppedNative", "()V");
//    jmd_completed = pEnv->GetMethodID(callbackClazz, "completedNative", "()V");
    jmd_error = pEnv->GetMethodID(clazz, "onError", "(I)V");
    jmd_progress = pEnv->GetMethodID(clazz, "onProgress", "(I)V");

}

JNICallbackHelper::~JNICallbackHelper() {
    pEnv->DeleteGlobalRef(obj);
    pVm = nullptr;
    pEnv = nullptr;
    obj = nullptr;
}

void JNICallbackHelper::onPrepared(int thread_mode) {
    if (thread_mode == THREAD_MAIN) {
        pEnv->CallVoidMethod(obj, jmd_prepared);
    } else {
        JNIEnv *env;
        pVm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(obj, jmd_prepared);
        pVm->DetachCurrentThread();
    }
}

void JNICallbackHelper::onError(int thread_mode, int error_code) {
    if (thread_mode == THREAD_MAIN) {
        //主线程
        pEnv->CallVoidMethod(obj, jmd_error);
    } else {
        //子线程
        //当前子线程的 JNIEnv
        JNIEnv *env_child;
        pVm->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(obj, jmd_error, error_code);
        pVm->DetachCurrentThread();
    }
}

void JNICALL JNICallbackHelper::onProgress(int thread_mode, int progress) {
    if (thread_mode == THREAD_MAIN) {
        //主线程
        pEnv->CallVoidMethod(obj, jmd_progress, progress);
    } else {
        //子线程
        //当前子线程的 JNIEnv
        JNIEnv *env_child;
        pVm->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(obj, jmd_progress, progress);
        pVm->DetachCurrentThread();
    }
}
