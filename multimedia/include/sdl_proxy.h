#pragma once
#include "core_frame_queue.h"
#include "swscale_proxy.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif

#define MAX_AUDIO_FRAME_SIZE 192000
#define SDL_AUDIO_BUFFER_SIZE 512

class CoreMedia;

class SDL2Audio {
public:
    static SDL2Audio *instance();

    bool init(SDL_AudioSpec *spec, CoreMedia *core_media);
    void uninit();

    void refresh(void *udata, Uint8 *stream, int len);

private:
    SDL2Audio();
    ~SDL2Audio();

    int getPcmFromAudioFrameQueue();

    // int getPcmFromFile();

    void copyPlanar(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels);

    void copyPacked(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels);

    // void writePcm(uint8_t *data, size_t len);

private:
    SDL_AudioSpec spec_;

    uint8_t *audio_buf_;
    unsigned int audio_buf_size_;
    int audio_buf_index_;

    CoreMedia *core_media_;
    int64_t audio_callback_time_;
};

typedef std::function<void(int milliseconds_delay)> VideoRefreshCallbacks;
class SDL2Video {
public:
    static SDL2Video *instance();

private:
    SDL2Video();
    ~SDL2Video();

public:
    bool init(VideoParams video_params, CoreMedia *core_media);
    bool refresh(VideoRefreshCallbacks cb);
    void uninit();

private:
    bool getFrame(Frame **frame);

private:
    SDL_Window *window_;
    SDL_Renderer *render_;
    SDL_Texture *texture_;
    SDL_Rect rect_;

    CoreMedia *core_media_;
    AVFrame *frame_;
};

class SDL2Event {
public:
private:
    SDL_Event event_;
};

class SDL2Proxy {
public:
    SDL2Proxy();
    ~SDL2Proxy();

    bool initAudio(SDL_AudioSpec *spec, CoreMedia *core_media);
    bool initVideo(VideoParams video_params, CoreMedia *core_media);

    void uninit();

    static void refreshAudio(void *udata, Uint8 *stream, int len);
    static Uint32 refreshVideo(Uint32 interval, void *opaque);

    static void scheduleRefreshVideo(int millisecond_time);
};