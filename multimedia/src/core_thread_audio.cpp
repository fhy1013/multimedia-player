#include "core_thread_audio.h"
#include "glog_proxy.h"
#include "core_media.h"

CoreThreadAudio::CoreThreadAudio(CoreMedia* core_media) : CoreThread(core_media) {
    // LOG(INFO) << "CoreThreadAudio() ";
}

CoreThreadAudio::~CoreThreadAudio() {
    //  LOG(INFO) << "~CoreThreadAudio() ";
}

bool CoreThreadAudio::start() {
    std::thread([&] { this->audioDecode(); }).detach();
    return true;
}

void CoreThreadAudio::frameCallback(long pts) {
    core_media_->status_cb_(AVMEDIA_TYPE_AUDIO, core_media_->status(), pts);
}

void CoreThreadAudio::audioDecode() {
    LOG(INFO) << "audio decoder thread start";
    auto packets = core_media_->audio_->packets();
    AVPacket* packet = av_packet_alloc();
    while (true) {
        // TODO
        if (STOP == core_media_->status()) break;

        // if (END == core_media_->status() && packets->empty()) break;

        if (PAUSE == core_media_->status()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            continue;
        }

        // if (packets->empty()) {
        //     std::this_thread::sleep_for(std::chrono::microseconds(1));
        //     continue;
        // }

        if (!packets->pop(packet)) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            av_packet_unref(packet);
            continue;
        }

        core_media_->audio_->decode(packet, std::bind(&CoreThreadAudio::frameCallback, this, std::placeholders::_1));
    }
    av_packet_free(&packet);
    packets->clear();

    // SetEvent(handle_);
    LOG(INFO) << "audio decoder thread is stop";
    return;
}