#pragma once

#include "core_common.h"

typedef struct AudioParams {
    int channel;                 // number of audio channels
    enum AVSampleFormat format;  // sample format
    int sample_rate;             // samples per second
};

class SwresampleProxy {
public:
    static SwresampleProxy* instance();

    bool init(const AudioParams& in, const AudioParams& out);
    void uninit();

    int resample(uint8_t** out, int out_count, const uint8_t** in, int in_count);

    int64_t avRescaleRnd(AVFrame* frame);

    int avSamplesGetBufferSize(int64_t nb_samples);

    AudioParams outSwrContextParam() const { return out_; };

    int swrGetOutSamples(int in_samples) { return swr_get_out_samples(swr_context_, in_samples); }

private:
    SwresampleProxy();
    ~SwresampleProxy();

private:
    SwrContext* swr_context_;
    AudioParams in_;
    AudioParams out_;
    uint8_t* buffer_;
};