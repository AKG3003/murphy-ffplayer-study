#ifndef NDK22_COMPILE_STUDY_SAFEQUEUE_H
#define NDK22_COMPILE_STUDY_SAFEQUEUE_H

#include <queue>
#include <pthread.h>

#define MAX_NUM 100
#define MIN_NUM 50

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

    // 次级优化方案，加一个锁，如果内部变量出现过多，就进行wait等待，降低到一定程度后再唤醒
    pthread_mutex_t mutex_ctrl_num;
    pthread_cond_t cond_ctrl_num;
    int work;
    ReleaseCallback releaseCallback;


public:
    SafeQueue() {
        // 初始化互斥锁
        pthread_mutex_init(&mutex, 0);
        // 初始化条件变量
        pthread_cond_init(&cond, 0);

        pthread_mutex_init(&mutex_ctrl_num, 0);
        pthread_cond_init(&cond_ctrl_num, 0);
    }

    ~SafeQueue() {
        // 销毁互斥锁
        pthread_mutex_destroy(&mutex);
        // 销毁条件变量
        pthread_cond_destroy(&cond);

        pthread_mutex_destroy(&mutex_ctrl_num);
        pthread_cond_destroy(&cond_ctrl_num);
    }

    void push(T value) {
//        pthread_mutex_lock(&mutex_ctrl_num);
//        if (work && queue.size() > MAX_NUM) {
//            // 阻塞
//            pthread_cond_wait(&cond_ctrl_num, &mutex_ctrl_num);
//        }
//        pthread_mutex_unlock(&mutex_ctrl_num);
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
//        if (work) {
//            pthread_mutex_lock(&mutex_ctrl_num);
//            if (queue.size() < MIN_NUM) {
//                pthread_cond_signal(&cond_ctrl_num);
//            }
//            pthread_mutex_unlock(&mutex_ctrl_num);
//        }
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

    int size() {
        pthread_mutex_lock(&mutex);
        int size = queue.size();
        pthread_mutex_unlock(&mutex);
        return size;
    }

    bool empty() {
        pthread_mutex_lock(&mutex);
        bool empty = queue.empty();
        pthread_mutex_unlock(&mutex);
        return empty;
    }

    void setReleaseCallback(ReleaseCallback callback) {
        this->releaseCallback = callback;
    }
};

#endif //NDK22_COMPILE_STUDY_SAFEQUEUE_H
