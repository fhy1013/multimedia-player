#pragma once

#include "core_common.h"
#include "core_packet_queue.h"
#include "core_frame_queue.h"

#include <functional>

typedef std::function<void(long pts)> DecodeCallback;

class CoreDecoder {
public:
    CoreDecoder();

    virtual ~CoreDecoder();

    bool init(AVFormatContext* format_context, int stream_index, std::shared_ptr<FrameQueue>& frame_queue);

    virtual void unInit();

    virtual bool decode(AVPacket* pack, DecodeCallback cb) = 0;

    std::shared_ptr<PacketQueue> packets() { return packet_queue_; }

    AVFormatContext* formatContext() { return format_context_; }

    bool avcodecFlushBuffer(AVPacket* pack);

protected:
    int stream_index_;
    AVFormatContext* format_context_;

    AVCodecParameters* codec_parameters_;
    AVCodecContext* codec_context_;

    AVCodec* codec_;
    AVStream* stream_;
    AVFrame* frame_;

    std::shared_ptr<PacketQueue> packet_queue_;
    std::shared_ptr<FrameQueue> frame_queue_;
};