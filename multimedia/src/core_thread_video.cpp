#include "core_thread_video.h"
#include "glog_proxy.h"
#include "core_media.h"

CoreThreadVideo::CoreThreadVideo(CoreMedia* core_media) : CoreThread(core_media) {}

CoreThreadVideo::~CoreThreadVideo() {}

bool CoreThreadVideo::start() {
    std::thread([&] { this->videoDecode(); }).detach();
    return true;
}

void CoreThreadVideo::frameCallback(long pts) {
    core_media_->status_cb_(AVMEDIA_TYPE_VIDEO, core_media_->status(), pts);
}

void CoreThreadVideo::videoDecode() {
    LOG(INFO) << "video decoder thread start";
    auto packets = core_media_->video_->packets();
    AVPacket* packet = av_packet_alloc();
    while (true) {
        if (STOP == core_media_->status()) break;

        // if (END == core_media_->status() && packets->empty()) break;

        if (PAUSE == core_media_->status()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            continue;
        }

        if (packets->empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        if (!packets->pop(packet)) {
            // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            av_packet_unref(packet);
            continue;
        }

        core_media_->video_->decode(packet, std::bind(&CoreThreadVideo::frameCallback, this, std::placeholders::_1));
    }
    av_packet_free(&packet);
    packets->clear();

    // SetEvent(handle_);
    LOG(INFO) << "video decoder thread is stop";
    return;
}