#ifndef NDK22_COMPILE_STUDY_JNICALLBACKHELPER_H
#define NDK22_COMPILE_STUDY_JNICALLBACKHELPER_H


#include <jni.h>
#include "ThreadUtil.h"

class JNICallbackHelper {

private:
    JavaVM *pVm;
    JNIEnv *pEnv;
    jobject obj;
    jmethodID jmd_prepared;
    jmethodID jmd_started;
    jmethodID jmd_paused;
    jmethodID jmd_stopped;
    jmethodID jmd_completed;

    jmethodID jmd_error;
    jmethodID jmd_progress;

public:
    JNICallbackHelper(JavaVM *pVm, JNIEnv *pEnv, jobject obj);

    virtual ~JNICallbackHelper();

    void onPrepared(int thread_mode);

    void onError(int thread_mode, int error_code);

    void onProgress(int thread_mode, int progress);

};


#endif //NDK22_COMPILE_STUDY_JNICALLBACKHELPER_H
