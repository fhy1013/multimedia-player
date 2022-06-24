#include "core_thread.h"
#include "glog_proxy.h"

CoreThread::CoreThread(CoreMedia *core_media) : core_media_(core_media) {
    LOG(INFO) << "CoreThread() ";
    handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CoreThread::~CoreThread() { LOG(INFO) << "~CoreThread() "; }