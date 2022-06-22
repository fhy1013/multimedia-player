#include "core_media_status.h"

std::string statusTostr(MediaStatus &media_status) {
    std::string status;
    switch (media_status) {
        case STOP:
            status = "stop";
            break;
        case START:
            status = "start";
            break;
        case PAUSE:
            status = "pause";
            break;
        case END:
            status = "end";
            break;

        default:
            status = "none";
            break;
    }
    return status;
}