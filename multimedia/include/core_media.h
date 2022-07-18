#pragma once
#include "core_media_status.h"
#include "core_thread_demux.h"
#include "core_thread_audio.h"
#include "core_thread_video.h"
#include "core_decoder_audio.h"
#include "core_decoder_video.h"
#include "sdl_proxy.h"
#include "core_frame_queue.h"
#include "clock.h"

#include <memory>
#include <functional>

typedef std::function<void(AVMediaType type, MediaStatus status, long pts)> StatusCallback;

class CoreMedia {
public:
    CoreMedia();
    CoreMedia(const CoreMedia&) = delete;
    virtual ~CoreMedia();

    bool init();
    void unInit();

    bool start();
    bool stop();
    bool pause();

    bool open(const std::string& media);
    bool close();

    void statusCallback(AVMediaType type, MediaStatus status, long pts);
    MediaStatus status() const { return status_; }
    void loop();

    std::shared_ptr<FrameQueue>& videoFrameQueue();
    std::shared_ptr<FrameQueue>& audioFrameQueue();

    void initClock() {
        video_clock_.pts = 0.0;
        video_clock_.pts_drift = 0.0;
        video_clock_.last_updated = 0.0;

        audio_clock_.pts = 0.0;
        audio_clock_.pts_drift = 0.0;
        audio_clock_.last_updated = 0.0;
    }
    void setVideoPts(const double pts) {
        double now_time = av_gettime_relative() / 1000000.0;
        video_clock_.pts = pts;
        video_clock_.pts_drift = pts - now_time;
        video_clock_.last_updated = now_time;
    }
    double videoPts() const {
        double now_time = av_gettime_relative() / 1000000.0;
        return video_clock_.pts_drift + now_time;
    }
    void setAudioPts(const double pts) {
        double now_time = av_gettime_relative() / 1000000.0;
        audio_clock_.pts = pts;
        audio_clock_.pts_drift = pts - now_time;
        audio_clock_.last_updated = now_time;
    }
    double audioPts() const {
        double now_time = av_gettime_relative() / 1000000.0;
        return audio_clock_.pts_drift + now_time;
    }

private:
    bool initVideo();
    bool initAudio();

    bool startThreads();
    void stopThreads();

public:
    StatusCallback status_cb_;

private:
    std::string media_;
    AVFormatContext* format_context_;

    CoreDecoderVideo* video_;
    CoreDecoderAudio* audio_;

    int video_stream_index_;
    int audio_stream_index_;

    AVRational tbn_;
    size_t media_time_;
    SwrContext* swr_context_;

    CoreThreadVideo* thread_video_;
    CoreThreadAudio* thread_audio_;
    CoreThreadDemux* thread_demux_;

    std::atomic<MediaStatus> status_;
    SDL_Event event_;

    Clock video_clock_;
    Clock audio_clock_;

public:
    std::shared_ptr<SDL2Proxy> sdl_proxy_;

    std::shared_ptr<FrameQueue> audio_frame_queue_;
    std::shared_ptr<FrameQueue> video_frame_queue_;

    friend CoreThreadVideo;
    friend CoreThreadAudio;
    friend CoreThreadDemux;
};