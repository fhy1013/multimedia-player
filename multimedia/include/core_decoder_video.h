#pragma once

#include "core_decoder.h"
#include "swscale_proxy.h"

typedef std::function<bool(VideoParams in)> VideoCallback;

class CoreDecoderVideo : public CoreDecoder {
public:
    CoreDecoderVideo();

    ~CoreDecoderVideo() override;

    bool init(AVFormatContext* format_context, int stream_index, VideoCallback cb,
              std::shared_ptr<FrameQueue>& video_frame_queue);

    void unInit() override;

    bool decode(AVPacket* pack, DecodeCallback cb) override;

private:
    bool pushFrame(AVFrame* frame, double pts, double duration, int64_t pos);

private:
    AVRational tb_;
    AVRational frame_rate_;
};