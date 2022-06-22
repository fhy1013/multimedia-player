#pragma once
#include "core_common.h"

class AudioFrameBuffer {
public:
    AudioFrameBuffer(size_t channel, size_t nb_samples, enum AVSampleFormat sample_fmt)
        : channel_(channel), nb_samples_(nb_samples), sample_fmt_(sample_fmt), buffer(nullptr) {
        // buffer = new uint8_t*[channel_];
        // for (auto i = 0; i < channel; ++i) {
        // av_samples_alloc(buffer, nullptr, channel_, nb_samples_, sample_fmt_, 0);

        auto ret = av_samples_alloc_array_and_samples(&buffer, nullptr, channel_, nb_samples_, sample_fmt_, 0);
        // }
        // int x = 0;
    }

    ~AudioFrameBuffer() {
        av_freep(&buffer[0]);
        buffer = nullptr;
    }

    uint8_t** getBuffer() { return buffer; }

    // size_t bufferSize() { return channel_ * per_channel_size_; }

private:
    size_t channel_;
    size_t nb_samples_;
    enum AVSampleFormat sample_fmt_;
    uint8_t** buffer;
};