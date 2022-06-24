#include "core_thread_demux.h"
#include "glog_proxy.h"
#include "core_media.h"

CoreThreadDemux::CoreThreadDemux(CoreMedia* core_media) : CoreThread(core_media) { LOG(INFO) << "CoreThreadDemux() "; }

CoreThreadDemux::~CoreThreadDemux() { LOG(INFO) << "~CoreThreadDemux() "; }

bool CoreThreadDemux::start() {
    std::thread([&] { this->demux(); }).detach();
    return true;
}

void CoreThreadDemux::demux() {
    LOG(INFO) << "frame read thread start";
    auto video_packets = core_media_->video_->packets();
    auto audio_packets = core_media_->audio_->packets();

    bool ret = true;
    AVPacket* packet = av_packet_alloc();
    size_t acount = 0;
    size_t vcount = 0;
    while (true) {
        if (STOP == core_media_->status()) {
            if (packet) av_packet_unref(packet);
            break;
        }

        if (PAUSE == core_media_->status()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (ret) {
            if (av_read_frame(core_media_->format_context_, packet) < 0) {
                av_packet_unref(packet);
                break;
            }
        }

        if (packet->stream_index == core_media_->video_stream_index_) {
            ret = video_packets->push(packet);
            // av_packet_unref(packet);
            // if (ret) LOG(INFO) << "read video " << ++vcount;
        } else if (packet->stream_index == core_media_->audio_stream_index_) {
            ret = audio_packets->push(packet);
            // av_packet_unref(packet);
            // if (ret) LOG(INFO) << "read audio " << ++acount;
        } else {
            av_packet_unref(packet);
            ret = true;
        }

        if (!ret) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    av_packet_free(&packet);

    SetEvent(handle_);
    LOG(INFO) << "demux thread is stop";
    return;
}