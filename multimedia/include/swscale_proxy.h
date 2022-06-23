#pragma once

#include "core_common.h"
typedef struct VideoParams {
    int width;
    int height;
    enum AVPixelFormat format;
};

class SwscaleProxy {
public:
    static SwscaleProxy* instance();

    bool init(const VideoParams src, const VideoParams des);
    int scaled(const uint8_t* const srcSlice[], const int srcStride[], int srcSliceY, int srcSliceH,
               uint8_t* const dst[], const int dstStride[]);
    void uninit();

private:
    SwscaleProxy();
    ~SwscaleProxy();

private:
    struct SwsContext* sws_context_;
};