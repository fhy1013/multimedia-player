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
#include "seek.h"
#include "av_dictionary.hpp"

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

    void initClock();
    void setVideoPts(const double pts);
    double videoPts() const;
    void setAudioPts(const double pts);
    double audioPts() const;

    void initSeek(int seek_flags = AVSEEK_FLAG_BACKWARD);
    void seek(double timestamp);

private:
    bool initVideo();
    bool initAudio();

    bool startThreads();
    void stopThreads();

public:
    StatusCallback status_cb_;

private:
    std::string media_url_;
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

    Seek seek_;

    std::shared_ptr<AVDictionaryCfg> av_dictionary_ptr_;

public:
    std::shared_ptr<SDL2Proxy> sdl_proxy_;

    std::shared_ptr<FrameQueue> audio_frame_queue_;
    std::shared_ptr<FrameQueue> video_frame_queue_;

    friend CoreThreadVideo;
    friend CoreThreadAudio;
    friend CoreThreadDemux;
    friend SDL2Audio;
    friend SDL2Video;
};