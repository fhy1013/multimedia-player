#pragma once

#include "core_common.h"
#include "core_media_status.h"

#include <condition_variable>
#include <atomic>

#define FRAME_QUEUE_SIZE 10

typedef struct Frame {
    AVFrame *frame;
    // AVSubtitle sub;
    int serial;
    double pts;      /* presentation timestamp for the frame */
    double duration; /* estimated duration of the frame */
    int64_t pos;     /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    // int flip_v;
} Frame;

class FrameQueue {
public:
    FrameQueue();
    ~FrameQueue();

    int init(int max_size = FRAME_QUEUE_SIZE, int keep_last = 1);

    void destroy();

    Frame *peekCurrent() { return &queue_[(rindex_ + rindex_shown_) % max_size_]; }

    Frame *peekNext() { return &queue_[(rindex_ + rindex_shown_ + 1) % max_size_]; }

    Frame *peekLast() { return &queue_[rindex_]; }

    Frame *peekWritable();

    Frame *peekReadable();

    void framePush();

    void frameNext();

    int frameRemaining() {
        //
        return size_ - rindex_shown_;
    }

    int64_t lastPos();

    void frameUnref(Frame *vp) { av_frame_unref(vp->frame); }

    void setStatus(MediaStatus status) { status_ = status; }

    void notify() { cv_.notify_all(); }

protected:
    Frame queue_[FRAME_QUEUE_SIZE];
    int rindex_;        // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex_;        // 写索引
    int size_;          // 总帧数
    int max_size_;      // 队列可存储最大帧数
    int keep_last_;     // 是否保留已播放的最后一帧使能标志
    int rindex_shown_;  // 是否保留已播放的最后一帧实现手段
    // SDL_mutex *mutex;
    // SDL_cond *cond;
    // PacketQueue *pktq;  // 指向对应的packet_queue
    std::mutex mtx_;
    std::condition_variable cv_;

    std::atomic<MediaStatus> status_;
};