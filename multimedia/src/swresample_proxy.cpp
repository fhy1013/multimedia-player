#include "swresample_proxy.h"
#include "glog_proxy.h"

SwresampleProxy* SwresampleProxy::instance() {
    static SwresampleProxy self;
    return &self;
}

SwresampleProxy::SwresampleProxy()
    : swr_context_(nullptr), in_({0, AV_SAMPLE_FMT_NONE, 0}), out_({0, AV_SAMPLE_FMT_NONE, 0}) {}

SwresampleProxy::~SwresampleProxy() {}

bool SwresampleProxy::init(const AudioParams& in, const AudioParams& out) {
    in_ = in;
    out_ = out;
    swr_context_ = swr_alloc_set_opts(nullptr, av_get_default_channel_layout(out.channel), out.format, out.sample_rate,
                                      av_get_default_channel_layout(in.channel), in.format, in.sample_rate, 0, nullptr);
    if (!swr_context_) {
        LOG(ERROR) << "swr_alloc_set_opts failed";
        return false;
    }
    auto res = swr_init(swr_context_);
    if (res < 0) {
        LOG(ERROR) << "swr_init failed: " << err2str(res);
        return false;
    }
    return true;
}

void SwresampleProxy::uninit() {
    if (swr_context_) swr_free(&swr_context_);
}

int SwresampleProxy::resample(uint8_t** out, int out_count, const uint8_t** in, int in_count) {
    return swr_convert(swr_context_, out, out_count, in, in_count);
}

int64_t SwresampleProxy::avRescaleRnd(AVFrame* frame) {
    return av_rescale_rnd(swr_get_delay(swr_context_, frame->sample_rate) + frame->nb_samples, out_.sample_rate,
                          in_.sample_rate, AV_ROUND_ZERO);
}

int SwresampleProxy::avSamplesGetBufferSize(int64_t nb_samples) {
    return av_samples_get_buffer_size(NULL, out_.channel, static_cast<int>(nb_samples), out_.format, 1);
}