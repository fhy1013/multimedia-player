#pragma once
#include "core_frame_queue.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif

#define MAX_AUDIO_FRAME_SIZE 192000
#define SDL_AUDIO_BUFFER_SIZE 512

class SDL2Audio {
public:
    static SDL2Audio *instance();

    bool init(SDL_AudioSpec *spec, std::shared_ptr<FrameQueue> &audio_frame_queue);

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

    std::shared_ptr<FrameQueue> audio_frame_queue_;
    int64_t audio_callback_time_;
};

class SDL2Video {
public:
    static SDL2Video *instance();

    bool init(int width, int height);

    void refresh();

private:
    SDL2Video();
    ~SDL2Video();

private:
    SDL_Window *window_;
    SDL_Renderer *render_;
    SDL_Texture *texture_;
};

class SDL2Event {
public:
private:
    SDL_Event event_;
};

class SDL2Proxy {
public:
    static SDL2Proxy *instance();

    bool initAudio(SDL_AudioSpec *spec, std::shared_ptr<FrameQueue> &audio_frame_queue);
    bool initVideo(int width, int height);

    static void refreshAudio(void *udata, Uint8 *stream, int len);
    static void refreshVideo();

private:
    SDL2Proxy();
    ~SDL2Proxy();
};