#include "core_thread.h"

CoreThread::CoreThread(CoreMedia *core_media) : core_media_(core_media) {
    // handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CoreThread::~CoreThread() {}

// bool CoreThread::start() {
//     setStatus(START);
//     return true;
// }

// std::string CoreThread::status(ThreadStatus &thread_status) {
//     std::string status;
//     switch (thread_status) {
//         case STOP:
//             status = "stop";
//             break;
//         case START:
//             status = "start";
//             break;
//         case PAUSE:
//             status = "pause";
//             break;
//         case END:
//             status = "end";
//             break;

//         default:
//             status = "none";
//             break;
//     }
//     return status;
// }