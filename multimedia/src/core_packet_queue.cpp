#include "core_packet_queue.h"
#include "glog_proxy.h"

#include <memory>

static const size_t g_max_size = 10;

PacketQueue::PacketQueue() : max_size_(g_max_size) { LOG(INFO) << "PacketQueue() "; }

PacketQueue::~PacketQueue() { LOG(INFO) << "~PacketQueue() "; }

bool PacketQueue::empty() {
    std::lock_guard<SpinMutex> lck(mtx_);
    return packet_queue_.empty();
}

bool PacketQueue::full() {
    std::lock_guard<SpinMutex> lck(mtx_);
    return packet_queue_.size() >= max_size_ ? true : false;
}

bool PacketQueue::clear() {
    std::lock_guard<SpinMutex> lck(mtx_);
    for (auto &e : packet_queue_) {
        av_packet_unref(e);
        av_packet_free(&e);
    }

    packet_queue_.clear();
    return true;
}

bool PacketQueue::push(AVPacket *packet) {
    {
        std::lock_guard<SpinMutex> lck(mtx_);
        if (packet_queue_.size() >= max_size_) return false;
    }

    AVPacket *tmp = av_packet_alloc();
    if (!tmp) {
        av_packet_free(&tmp);
        return false;
    }
    av_packet_move_ref(tmp, packet);

    {
        std::lock_guard<SpinMutex> lck(mtx_);
        packet_queue_.push_back(tmp);
    }

    return true;
}

bool PacketQueue::pop(AVPacket *packet) {
    std::lock_guard<SpinMutex> lck(mtx_);
    if (packet_queue_.empty()) return false;

    AVPacket *tmp = packet_queue_.front();
    av_packet_move_ref(packet, tmp);
    av_packet_free(&tmp);
    packet_queue_.pop_front();

    return true;
}