#pragma once

#include "core_thread.h"

class CoreThreadVideo : public CoreThread {
public:
    CoreThreadVideo(CoreMedia* core_media);

    CoreThreadVideo(const CoreThreadVideo&) = delete;

    ~CoreThreadVideo() override;

    bool start() override;

    void frameCallback(long pts) override;

private:
    void videoDecode();
};