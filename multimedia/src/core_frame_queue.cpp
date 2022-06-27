#include "core_frame_queue.h"
#include "glog_proxy.h"

FrameQueue::FrameQueue() {
    LOG(INFO) << "FrameQueue() ";
    init();
}

FrameQueue::~FrameQueue() {
    destroy();
    LOG(INFO) << "~FrameQueue() ";
}

int FrameQueue::init(int max_size, int keep_last) {
    memset(queue_, 0, sizeof(Frame) * FRAME_QUEUE_SIZE);
    rindex_ = 0;
    windex_ = 0;
    size_ = 0;
    max_size_ = max_size;
    keep_last_ = !!keep_last;
    rindex_shown_ = 0;

    for (auto i = 0; i < max_size_; ++i)
        if (!(queue_[i].frame = av_frame_alloc())) return AVERROR(ENOMEM);

    status_ = STOP;
    return 0;
}

void FrameQueue::destroy() {
    for (auto i = 0; i < max_size_; ++i) {
        frameUnref(&queue_[i]);
        av_frame_free(&queue_[i].frame);
    }
}

Frame *FrameQueue::peekWritable() {
    {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this] { return size_ < max_size_ || status_ == STOP; });
        if (status_ == STOP) return nullptr;
    }

    return &queue_[windex_];
}

Frame *FrameQueue::peekReadable() {
    {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [this] { return (size_ - rindex_shown_ > 0 && status_ != PAUSE) || status_ == STOP; });
        if (status_ == STOP) return nullptr;
    }

    return &queue_[(rindex_ + rindex_shown_) % max_size_];
}

void FrameQueue::framePush() {
    if (++windex_ == max_size_) windex_ = 0;
    std::unique_lock<std::mutex> lck(mtx_);
    ++size_;
    cv_.notify_one();
}

void FrameQueue::frameNext() {
    if (keep_last_ && !rindex_shown_) {
        rindex_shown_ = 1;
        return;
    }
    frameUnref(&queue_[rindex_]);
    if (++rindex_ == max_size_) rindex_ = 0;
    std::unique_lock<std::mutex> lck(mtx_);
    --size_;
    cv_.notify_one();
}

int64_t FrameQueue::lastPos() {
    if (rindex_shown_)
        return queue_[rindex_].pos;
    else
        return -1;
}