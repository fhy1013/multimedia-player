#pragma once
#include "core_common.h"
#include "spin_mutex.h"

#include <mutex>
#include <list>
#include <atomic>

class PacketQueue {
public:
    PacketQueue();

    virtual ~PacketQueue();

    bool empty();

    bool full();

    bool clear();

    bool push(AVPacket *packet);

    // bool front();

    bool pop(AVPacket *packet);

private:
    size_t max_size_;
    // std::atomic<size_t> size_;
    // std::mutex mtx_;
    SpinMutex mtx_;

    std::list<AVPacket *> packet_queue_;
};