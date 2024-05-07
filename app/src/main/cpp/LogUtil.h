#ifndef NDK22_COMPILE_STUDY_LOGUTIL_H
#define NDK22_COMPILE_STUDY_LOGUTIL_H

#include <android/log.h>
#define LOG_TAG "JNI_TAG"
#define LOGI(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //NDK22_COMPILE_STUDY_LOGUTIL_H
