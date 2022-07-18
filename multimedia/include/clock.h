#pragma once
#include <atomic>

typedef struct Clock {
    // 当前帧(待播放)显示时间戳，播放后，当前帧变成上一帧
    std::atomic<double> pts; /* clock base */
    // 当前帧显示时间戳与当前系统时钟时间的差值
    std::atomic<double> pts_drift; /* clock base minus time at which we updated the clock */
    // 当前时钟(如视频时钟)最后一次更新时间，也可称当前时钟时间
    std::atomic<double> last_updated;
} Clock;