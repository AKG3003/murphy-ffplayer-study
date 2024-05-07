#ifndef NDK22_COMPILE_STUDY_SAFEQUEUE_H
#define NDK22_COMPILE_STUDY_SAFEQUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

template<typename T>
class SafeQueue {
private:
    typedef void (*ReleaseCallback)(T *);//函数指针 释放队列中元素的回调函数

    queue<T> queue;
    // 互斥锁
    pthread_mutex_t mutex;
    // 等待&唤醒
    pthread_cond_t cond;
    int work;
    ReleaseCallback releaseCallback;


public:
    SafeQueue() {
        // 初始化互斥锁
        pthread_mutex_init(&mutex, 0);
        // 初始化条件变量
        pthread_cond_init(&cond, 0);
    }

    ~SafeQueue() {
        // 销毁互斥锁
        pthread_mutex_destroy(&mutex);
        // 销毁条件变量
        pthread_cond_destroy(&cond);
    }

    void push(T value) {
        // 加锁
        pthread_mutex_lock(&mutex);
        if (work) {
            queue.push(value);
            // 通知消费者
            pthread_cond_signal(&cond);
        } else {
            // TODO：释放value，可能会和clear产生冲突
            if (releaseCallback) {
                releaseCallback(&value);
            }
        }
        // 解锁
        pthread_mutex_unlock(&mutex);
    }

    int pop(T &value) {
        pthread_mutex_lock(&mutex);
        while (work && queue.empty()) {
            // 等待
            pthread_cond_wait(&cond, &mutex);
        }
        if (!queue.empty()) {
            value = queue.front();
            queue.pop();
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    void setWork(int ws) {
        pthread_mutex_lock(&mutex);
        this->work = ws;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    void clear() {
        pthread_mutex_lock(&mutex);
        unsigned int size = queue.size();
        for (int i = 0; i < size; ++i) {
            T value = queue.front();
            if (releaseCallback) {
                releaseCallback(&value);
            }
            queue.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback callback) {
        this->releaseCallback = callback;
    }
};

#endif //NDK22_COMPILE_STUDY_SAFEQUEUE_H
