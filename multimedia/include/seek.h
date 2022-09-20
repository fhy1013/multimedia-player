#pragma once
#include <cstdint>

typedef struct Seek {
    char seek_req;     // 0001 demux, 0010 video, 0100 audio
    int seek_flags;    // default AVSEEK_FLAG_BACKWARD
    int64_t seek_pos;  // timestamp(ms)
} Seek;