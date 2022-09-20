#include "core_decoder_audio.h"
#include "glog_proxy.h"

CoreDecoderAudio::CoreDecoderAudio() { LOG(INFO) << "CoreDecoderAudio() "; }

CoreDecoderAudio::~CoreDecoderAudio() {
    unInit();
    LOG(INFO) << "~CoreDecoderAudio() ";
}

bool CoreDecoderAudio::init(AVFormatContext* format_context, int stream_index, AudioCallback cb,
                            std::shared_ptr<FrameQueue>& audio_frame_queue) {
    auto ret = CoreDecoder::init(format_context, stream_index, audio_frame_queue);
    if (!ret) return ret;

    LOG(INFO) << "multimedia file audio config: channel " << codec_context_->channels << ", sample_fmt "
              << codec_context_->sample_fmt << ", sample_rate " << codec_context_->sample_rate << ", frame_size "
              << codec_context_->frame_size;

    AudioParams in{codec_context_->channels, codec_context_->sample_fmt, codec_context_->sample_rate};

    return cb(in);
}

void CoreDecoderAudio::unInit() {}

bool CoreDecoderAudio::decode(AVPacket* pack, DecodeCallback cb) {
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

        if (!pushFrame(frame_)) {
            av_frame_unref(frame_);
            return false;
        }
        cb(frame_->pts);
    }

    return true;
}

// void CoreDecoderAudio::setAudioConfig(int out_channel, int sample_rate, AVSampleFormat out_format) {
//     out_channel_ = out_channel;
//     sample_rate_ = sample_rate;
//     out_format_ = out_format;
// }

bool CoreDecoderAudio::pushFrame(AVFrame* frame) {
    Frame* af = nullptr;
    af = frame_queue_->peekWritable();
    if (!af) return false;

    AVRational tb = {1, frame->sample_rate};
    if (frame->pts != AV_NOPTS_VALUE) frame->pts = av_rescale_q(frame->pts, stream_->time_base, tb);

    af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    af->pos = frame->pkt_pos;
    af->duration = av_q2d({frame->nb_samples, frame->sample_rate});
    av_frame_move_ref(af->frame, frame);
    frame_queue_->framePush();

    // static size_t count = 0;
    // LOG(INFO) << "audio pushFrame " << ++count;
    return true;
}