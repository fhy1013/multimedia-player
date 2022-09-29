#include "core_thread_demux.h"
#include "glog_proxy.h"
#include "core_media.h"

CoreThreadDemux::CoreThreadDemux(CoreMedia* core_media) : CoreThread(core_media) {
    // LOG(INFO) << "CoreThreadDemux() ";
}

CoreThreadDemux::~CoreThreadDemux() {
    // LOG(INFO) << "~CoreThreadDemux() ";
}

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

        if (core_media_->seek_.seek_req & 0x01) {
            if (!ret) {
                av_packet_unref(packet);
                ret = true;
            }
            seek();
            core_media_->seek_.seek_req &= 0xFE;
        }
        if (core_media_->seek_.seek_req) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

void CoreThreadDemux::seek() {
    auto video_packets = core_media_->video_->packets();
    auto audio_packets = core_media_->audio_->packets();

    video_packets->clear();
    audio_packets->clear();

    LOG(INFO) << "seek_pos: " << core_media_->seek_.seek_pos;
    if (0 <=
        av_seek_frame(core_media_->format_context_, -1, core_media_->seek_.seek_pos, core_media_->seek_.seek_flags)) {
        if (core_media_->video_stream_index_ >= 0) {
            AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket));
            av_new_packet(packet, 10);
            strcpy((char*)packet->data, FLUSH_DATA);
            video_packets->push(packet);
            free(packet);
        }
        if (core_media_->audio_stream_index_ >= 0) {
            AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket));
            av_new_packet(packet, 10);
            strcpy((char*)packet->data, FLUSH_DATA);
            audio_packets->push(packet);
            free(packet);
        }
    }
}