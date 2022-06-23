#include "swscale_proxy.h"
#include "glog_proxy.h"

SwscaleProxy* SwscaleProxy::instance() {
    static SwscaleProxy self;
    return &self;
}

bool SwscaleProxy::init(const VideoParams src, const VideoParams des) {
    sws_context_ = sws_getContext(src.width, src.height, src.format, des.width, des.height, des.format,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_context_) {
        LOG(ERROR) << "sws_getContext failed";
        return false;
    }
    return true;
}

int SwscaleProxy::scaled(const uint8_t* const srcSlice[], const int srcStride[], int srcSliceY, int srcSliceH,
                         uint8_t* const dst[], const int dstStride[]) {
    return sws_scale(sws_context_, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
}

void SwscaleProxy::uninit() {
    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
}

SwscaleProxy::SwscaleProxy() : sws_context_(nullptr) {}

SwscaleProxy::~SwscaleProxy() { uninit(); }