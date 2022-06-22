#pragma once
#include "core_media_status.h"

#include <windows.h>
#include <thread>
#include <atomic>
#include <string>

class CoreMedia;

class CoreThread {
public:
    CoreThread(CoreMedia *core_media);

    CoreThread(const CoreThread &) = delete;

    virtual ~CoreThread();

    virtual bool start() = 0;

    virtual void frameCallback(long pts) = 0;

    // static std::string status(ThreadStatus &thread_status);

    // HANDLE &waitCloseHandle() { return handle_; }

protected:
    CoreMedia *core_media_;

    // HANDLE handle_;
};