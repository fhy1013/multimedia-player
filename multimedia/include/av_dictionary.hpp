#pragma once
#include <string>

#include "core_common.h"

class AVDictionaryCfg {
public:
    AVDictionaryCfg()
        : transport_protocol_("tcp"), max_delay_(100), buffer_size_(1024000), timeout_(3000000), options_(nullptr) {}
    ~AVDictionaryCfg() {
        if (options_) av_dict_free(&options_);
    }

    AVDictionary **options() { return &options_; }

    void configAVDictionary() {
        av_dict_set(&options_, "rtsp_transport", transport_protocol_.c_str(), 0);
        av_dict_set(&options_, "max_delay", std::to_string(max_delay_).c_str(), 0);
        av_dict_set(&options_, "buffer_size", std::to_string(buffer_size_).c_str(), 0);
        av_dict_set(&options_, "stimeout", std::to_string(timeout_).c_str(), 0);
    }

    void setTransportProtocol(const std::string &transport_protocol) { transport_protocol_ = transport_protocol; }
    void setMaxDelay(const size_t max_delay) { max_delay_ = max_delay; }
    void setBufferSize(const size_t buffer_size) { buffer_size_ = buffer_size; }
    void setTimeout(const size_t timeout) { timeout_ = timeout; }

private:
    std::string transport_protocol_;
    size_t max_delay_ = 0;
    size_t buffer_size_ = 0;
    size_t timeout_ = 0;

    AVDictionary *options_;
};