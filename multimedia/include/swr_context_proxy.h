#pragma once

#include "core_common.h"

typedef struct SwrContextParam {
    int channel;                 // number of audio channels
    enum AVSampleFormat format;  // sample format
    int sample_rate;             // samples per second
};

class SwrContextProxy {
public:
    static SwrContextProxy* instance();

    bool init(const SwrContextParam& in, const SwrContextParam& out);
    void uninit();

    int swrConvert(uint8_t** out, int out_count, const uint8_t** in, int in_count);

    int64_t avRescaleRnd(AVFrame* frame);

    int avSamplesGetBufferSize(int64_t nb_samples);

    SwrContextParam outSwrContextParam() const { return out_; };

private:
    SwrContextProxy();
    ~SwrContextProxy();

private:
    SwrContext* swr_context_;
    SwrContextParam in_;
    SwrContextParam out_;
    uint8_t* buffer_;
};