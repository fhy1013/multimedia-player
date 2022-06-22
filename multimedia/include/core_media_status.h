#pragma once
#include <string>

enum MediaStatus {
    STOP = 0,
    START,
    PAUSE,
    END,
};

std::string statusTostr(MediaStatus &media_status);