#pragma once

#include "core_thread.h"

class CoreThreadAudio : public CoreThread {
public:
    CoreThreadAudio(CoreMedia* core_media);

    CoreThreadAudio(const CoreThreadAudio&) = delete;

    ~CoreThreadAudio() override;

    bool start() override;

    void frameCallback(long pts) override;

private:
    void audioDecode();
};