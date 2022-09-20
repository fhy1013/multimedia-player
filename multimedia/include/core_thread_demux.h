#pragma once
#include "core_thread.h"
#include "core_common.h"

class CoreThreadDemux : public CoreThread {
public:
    CoreThreadDemux(CoreMedia* core_media);

    ~CoreThreadDemux() override;

    bool start();

    void frameCallback(long pts) override{};

private:
    void demux();

    void seek();
};