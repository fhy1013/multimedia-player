#pragma once
#include "core_frame_queue.h"
#include "swscale_proxy.h"
#include "swresample_proxy.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif

#define MAX_AUDIO_FRAME_SIZE 192000
#define SDL_AUDIO_BUFFER_SIZE 1024

class CoreMedia;

typedef struct AudioClockParams {
    int channels;          // current number of channels
    int bytes_per_sample;  // bytes per second
    int sample_rate;       // current frame samples
    int index;             // current frame read index
};

typedef struct AudioClock {
    double pts;
    // double duration;
    AudioParams params;
};

class SDL2Audio {
public:
    static SDL2Audio *instance();

    bool init(SDL_AudioSpec *spec, CoreMedia *core_media);
    void uninit();

    void refresh(void *udata, Uint8 *stream, int len);

    void setAudioVolume(const int audio_volume);
    int audioVolume() const;
    void setMuted();
    void cancelMuted();
    bool muted() const;

private:
    SDL2Audio();
    ~SDL2Audio();

    int getPcmFromAudioFrameQueue();

    // int getPcmFromFile();

    uint64_t copyPlanar(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels);

    uint64_t copyPacked(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels);

    // void writePcm(uint8_t *data, size_t len);

    void seek();

private:
    SDL_AudioSpec spec_;

    uint8_t *audio_buf_;
    unsigned int audio_buf_size_;
    int audio_buf_index_;
    AudioClock clock_;

    CoreMedia *core_media_;
    int64_t audio_callback_time_;

    int audio_volume_;
    bool muted_;
    bool seek_;
    bool update_audio_clock_;
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
    void render(Frame *af);
    double computeTargetDelay(double delay, Frame *af);
    void seek();
    double vpDuration(Frame *vp, Frame *next_vp);

private:
    SDL_Window *window_;
    SDL_Renderer *render_;
    SDL_Texture *texture_;
    SDL_Rect rect_;

    CoreMedia *core_media_;
    AVFrame *frame_;

    double next_cb_timer_;  // 下一次回调时间
    double last_pts_;       // 上一帧pts
    double last_delay_;     // 上一帧delay时间
    double frame_timer_;    // 视频帧播放时间
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

    int audioVolumeUp();
    int audioVolumeDown();
    int audioVolume() const;
    bool setMuted();
    bool muted() const;
};