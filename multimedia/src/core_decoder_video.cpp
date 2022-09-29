#include "core_decoder_video.h"
#include "glog_proxy.h"

CoreDecoderVideo::CoreDecoderVideo() {
    // LOG(INFO) << "CoreDecoderVideo() ";
}

CoreDecoderVideo::~CoreDecoderVideo() {
    unInit();
    // LOG(INFO) << "~CoreDecoderVideo() ";
}

bool CoreDecoderVideo::init(AVFormatContext *format_context, int stream_index, VideoCallback cb,
                            std::shared_ptr<FrameQueue> &video_frame_queue) {
    auto ret = CoreDecoder::init(format_context, stream_index, video_frame_queue);
    if (!ret) return ret;

    VideoParams in{codec_context_->width, codec_context_->height, codec_context_->pix_fmt};
    if (!cb(in)) return false;
    tb_ = stream_->time_base;
    frame_rate_ = av_guess_frame_rate(formatContext(), stream_, NULL);
    return ret;
}

void CoreDecoderVideo::unInit() {}

bool CoreDecoderVideo::decode(AVPacket *pack, DecodeCallback cb) {
    if (avcodecFlushBuffer(pack)) {
        return false;
    }

    auto ret = avcodec_send_packet(codec_context_, pack);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOG(ERROR) << "Error in avcodec_send_packet: " << err2str(ret);
        av_packet_unref(pack);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_context_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG(ERROR) << "Error avcodec_receive_frame: " << err2str(ret);
            return false;
        }

        double duration = (frame_rate_.num && frame_rate_.den ? av_q2d({frame_rate_.den, frame_rate_.num}) : 0);
        double pts = (frame_->pts == AV_NOPTS_VALUE) ? NAN : frame_->pts * av_q2d(tb_);
        // pushFrame(frame_, pts, duration, frame_->pkt_pos);
        if (!pushFrame(frame_, pts, duration, frame_->pkt_pos)) {
            av_frame_unref(frame_);
            return false;
        }

        cb(frame_->pts);
        // av_frame_unref(frame_);
    }
}

bool CoreDecoderVideo::pushFrame(AVFrame *frame, double pts, double duration, int64_t pos) {
    Frame *vp = nullptr;
    vp = frame_queue_->peekWritable();
    if (!vp) return false;

    vp->sar = frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = frame->width;
    vp->height = frame->height;
    vp->format = frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    // vp->serial = serial;
    // set_default_window_size(vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->frame, frame);
    frame_queue_->framePush();

    // static size_t count = 0;
    // LOG(INFO) << "video pushFrame " << ++count;

    return true;
}