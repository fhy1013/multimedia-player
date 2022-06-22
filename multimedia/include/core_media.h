#pragma once
#include "core_media_status.h"
#include "core_thread_demux.h"
#include "core_thread_audio.h"
#include "core_thread_video.h"
#include "core_decoder_audio.h"
#include "core_decoder_video.h"
#include "sdl_proxy.h"
#include "core_frame_queue.h"

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

    // bool pause();

    bool open(const std::string& media);

    // bool isOpen() const { return is_open_.load(); }

    bool close();

    void statusCallback(AVMediaType type, MediaStatus status, long pts);

    MediaStatus status() const { return status_; }

    void loop();

private:
    bool initVideo();
    bool initAudio();

    bool startThreads();

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

    // std::atomic<bool> is_open_;
    SwrContext* swr_context_;

    CoreThreadVideo* thread_video_;
    CoreThreadAudio* thread_audio_;
    CoreThreadDemux* thread_demux_;

    std::atomic<MediaStatus> status_;

    SDL_Event event_;

public:
    SDL2Proxy* sdl_proxy_;

    std::shared_ptr<FrameQueue> audio_frame_queue_;
    std::shared_ptr<FrameQueue> video_frame_queue_;

    friend CoreThreadVideo;
    friend CoreThreadAudio;
    friend CoreThreadDemux;
};