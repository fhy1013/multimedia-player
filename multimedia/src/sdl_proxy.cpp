#include "sdl_proxy.h"
#include "glog_proxy.h"
#include "core_media.h"
#include "audio_frame_buffer.h"

#include <fstream>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

SDL2Audio::SDL2Audio()
    : audio_buf_(nullptr), audio_buf_size_(0), audio_buf_index_(0), audio_callback_time_(0), core_media_(nullptr) {
    LOG(INFO) << "SDL2Audio() ";
}

SDL2Audio::~SDL2Audio() { LOG(INFO) << "~SDL2Audio() "; }

SDL2Audio *SDL2Audio::instance() {
    static SDL2Audio self;
    return &self;
}

bool SDL2Audio::init(SDL_AudioSpec *spec, CoreMedia *core_media) {
    spec_ = *spec;
    core_media_ = core_media;

    // The second paramter of SDL_OpenAudio must bo nullptr.
    // if it is an object of SDL_AudioSpec audio playback has no sound.
    if (SDL_OpenAudio(&spec_, nullptr) < 0) {
        LOG(ERROR) << "failed to open audio device: " << SDL_GetError();
        return false;
    }
    // start player audio
    SDL_PauseAudio(0);
    return true;
}

void SDL2Audio::uninit() { SDL_CloseAudio(); }

void SDL2Audio::refresh(void *udata, Uint8 *stream, int len) {
    CoreMedia *core_media = (CoreMedia *)udata;
    int audio_size;
    int len1;

    if (core_media_->status() == STOP) {
        SDL_PauseAudio(1);
        return;
    }

    // static size_t count = 0;
    // LOG(INFO) << "audio refresh frames: " << ++count;
    SDL_memset(stream, 0, len);
    while (len > 0) {
        if (audio_buf_index_ >= audio_buf_size_) {
            if (core_media_->status() == PAUSE)
                audio_size = -1;
            else {
                // get audio frame form audio frame queue
                audio_size = getPcmFromAudioFrameQueue();
            }
            if (audio_size < 0) {
                audio_buf_ = new uint8_t[SDL_AUDIO_BUFFER_SIZE];
                memset(audio_buf_, 0x00, SDL_AUDIO_BUFFER_SIZE);
                audio_buf_size_ = SDL_AUDIO_BUFFER_SIZE;
                // LOG(WARNING) << "audio refresh get buffer failed, set silence ";
            } else {
                // LOG(INFO) << "audio refresh frames: " << ++count << ", size: " << audio_size;
                audio_buf_size_ = audio_size;
            }
            audio_buf_index_ = 0;
        }
        len1 = audio_buf_size_ - audio_buf_index_;
        if (len1 > len) len1 = len;
        SDL_MixAudio(stream, (uint8_t *)audio_buf_ + audio_buf_index_, len1, SDL_MIX_MAXVOLUME);

        len -= len1;
        stream += len1;
        audio_buf_index_ += len1;

        // delete convert buffer
        if (audio_buf_index_ >= audio_buf_size_ && audio_buf_) {
            delete[] audio_buf_;
            audio_buf_ = nullptr;
        }
    }
    core_media_->setAudioPts(clock_.pts);
    LOG(INFO) << "audio refresh frames pts: " << clock_.pts << ", duration: " << clock_.duration;
}

int SDL2Audio::getPcmFromAudioFrameQueue() {
    Frame *af = nullptr;

    if (core_media_->audioFrameQueue()->frameRemaining() == 0) {
        // if ((av_gettime_relative() - audio_callback_time_) > )
        return -1;
    }
    af = core_media_->audioFrameQueue()->peekReadable();
    if (!af) return -1;
    core_media_->audioFrameQueue()->frameNext();
    // LOG(INFO) << "audio refresh frames pts: " << af->pts << ", duration: " << af->duration;

    auto swresample_proxy = SwresampleProxy::instance();
    auto out_samples = swresample_proxy->swrGetOutSamples(af->frame->nb_samples);
    auto sambuf_size = swresample_proxy->avSamplesGetBufferSize(out_samples);
    auto out_swr_context = swresample_proxy->outSwrContextParam();

    if (sambuf_size <= 0) {
        // LOG(ERROR) << "avSamplesGetBufferSize() <= 0, nb_samples: " << nb_samples;
        LOG(ERROR) << "avSamplesGetBufferSize() <= 0, out_samples: " << out_samples;
        return -1;
    }

    // new outbuffer
    audio_buf_ = new uint8_t[sambuf_size];
    memset(audio_buf_, 0x00, sambuf_size);

    // new convert buffer
    std::shared_ptr<AudioFrameBuffer> audio_buf =
        std::make_shared<AudioFrameBuffer>(out_swr_context.channel, sambuf_size, out_swr_context.format);
    auto nb_convert = swresample_proxy->resample(audio_buf.get()->getBuffer(), out_samples,
                                                 (const uint8_t **)af->frame->data, af->frame->nb_samples);
    if (nb_convert < 0) {
        LOG(ERROR) << "swr_convert fialed";
        delete[] audio_buf_;
        audio_buf_ = nullptr;
        return -1;
    }
    auto index = 0;
    do {
        if (av_sample_fmt_is_planar(out_swr_context.format))
            index += copyPlanar(audio_buf_ + index, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                                out_swr_context.channel);
        else
            index += copyPacked(audio_buf_ + index, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                                out_swr_context.channel);
        nb_convert = swresample_proxy->resample(audio_buf.get()->getBuffer(), out_samples, nullptr, 0);
        if (nb_convert < 0) {
            LOG(ERROR) << "swr_convert fialed";
            delete[] audio_buf_;
            audio_buf_ = nullptr;
            return -1;
        }
    } while (nb_convert > 0);

    clock_.pts = af->pts;
    clock_.duration = af->duration;
    clock_.samples = index;

    return index;
}

// Planar: L L L L R R R R
uint64_t SDL2Audio::copyPlanar(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels) {
    auto per_sample_size = av_get_bytes_per_sample(format);
    size_t offset = 0;
    for (auto i = 0; i < nb_samples; ++i)
        for (auto channel = 0; channel < channels; ++channel) {
            // out_.write(reinterpret_cast<char*>(buff[channel] + per_sample_size * i), per_sample_size);
            memcpy(des + offset, reinterpret_cast<char *>(src[channel] + per_sample_size * i), per_sample_size);
            offset += per_sample_size;
        }
    return per_sample_size * nb_samples * channels;
}

// Packed: L R L R L R L R
uint64_t SDL2Audio::copyPacked(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels) {
    auto per_sample_size = av_get_bytes_per_sample(format);
    memcpy(des, reinterpret_cast<char *>(src[0]), per_sample_size * nb_samples * channels);
    return per_sample_size * nb_samples * channels;
}

SDL2Video::SDL2Video()
    : window_(nullptr),
      render_(nullptr),
      texture_(nullptr),
      rect_({0, 0, 0, 0}),
      core_media_(nullptr),
      frame_(nullptr) {
    LOG(INFO) << "SDL2Video() ";
    frame_ = av_frame_alloc();
}

SDL2Video::~SDL2Video() {
    if (!frame_) av_frame_free(&frame_);
    LOG(INFO) << "~SDL2Video() ";
}

SDL2Video *SDL2Video::instance() {
    static SDL2Video self;
    return &self;
}

bool SDL2Video::init(VideoParams video_params, CoreMedia *core_media) {
    core_media_ = core_media;
    auto err =
        av_image_alloc(frame_->data, frame_->linesize, video_params.width, video_params.height, video_params.format, 1);
    if (err < 0) {
        LOG(INFO) << "av_image_alloc failed" << err2str(err);
        return false;
    }

    window_ = SDL_CreateWindow("Multimedia-Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               video_params.width, video_params.height, SDL_WINDOW_SHOWN);
    if (!window_) {
        LOG(ERROR) << "failed to create window by sdl";
        return false;
    }
    render_ = SDL_CreateRenderer(window_, -1, 0);
    if (!render_) {
        LOG(ERROR) << "failed to create render";
        return false;
    }
    texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_params.width,
                                 video_params.height);
    if (!texture_) {
        LOG(ERROR) << "failed to create texture";
        return false;
    }
    return true;
}

bool SDL2Video::refresh(VideoRefreshCallbacks cb) {
    if (core_media_->status() == STOP) return true;
    if (core_media_->status() == PAUSE) {
        cb(1);
        return true;
    }
    Frame *af = nullptr;
    if (getFrame(&af)) {
        // cb(af->duration * 1000);  // seconds to milliseconds
        // render(af);
        // core_media_->videoFrameQueue()->frameNext();

        auto delay = af->pts - last_pts_;
        if (delay <= 0 || delay >= 1.0) delay = last_delay_;
        // auto delay = computeTargetDelay(delay, af);

        auto diff = af->pts - core_media_->audioPts();
        auto sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
        LOG(INFO) << "video refresh delay: " << delay * 1000 + 1;
        cb(delay * 1000 + 1);

        last_delay_ = delay;

        if (diff <= 0.0) {
            render(af);
            last_pts_ = af->pts;
            core_media_->videoFrameQueue()->frameNext();
        } else {
            LOG(INFO) << "video refresh delay: " << delay << ", diff: " << diff;
        }

    } else {
        cb(1);
        return false;
    }
    return true;
}

void SDL2Video::uninit() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (render_) {
        SDL_DestroyRenderer(render_);
        render_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    av_freep(&frame_->data[0]);
}

bool SDL2Video::getFrame(Frame **frame) {
    if (core_media_->videoFrameQueue()->frameRemaining() == 0) return false;
    *frame = core_media_->videoFrameQueue()->peekReadable();
    if (!*frame) return false;
    // core_media_->videoFrameQueue()->frameNext();
    return true;
}

void SDL2Video::render(Frame *af) {
    LOG(INFO) << "video refresh pts " << af->pts << ", duration " << af->duration;

    auto height = SwscaleProxy::instance()->scaled((uint8_t const *const *)af->frame, af->frame->linesize, 0,
                                                   af->height, frame_->data, frame_->linesize);

    SDL_UpdateYUVTexture(texture_, nullptr, frame_->data[0], frame_->linesize[0], frame_->data[1], frame_->linesize[1],
                         frame_->data[2], frame_->linesize[2]);
    rect_.x = 0;
    rect_.y = 0;
    // rect_.w = af->width;
    // rect_.h = af->height;
    rect_.w = frame_->linesize[0];
    rect_.h = frame_->linesize[1];

    SDL_RenderClear(render_);
    SDL_RenderCopy(render_, texture_, nullptr, &rect_);
    SDL_RenderPresent(render_);

    // static size_t count = 0;
    // LOG(INFO) << "video refresh frames: " << ++count;
}

double SDL2Video::computeTargetDelay(double delay, Frame *af) {
    /* if video is slave, we try to correct big delays by
       duplicating or deleting a frame */
    auto diff = af->pts - core_media_->audioPts();
    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    auto sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
        if (diff <= -sync_threshold)
            delay = FFMAX(0, delay + diff);
        else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
            delay = delay + diff;
        else if (diff >= sync_threshold)
            delay = 2 * delay;
    }

    return delay;
}

SDL2Proxy::SDL2Proxy() {
    LOG(INFO) << "SDL2Proxy() ";
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        LOG(ERROR) << "failed SDL_Init";
        return;
    }
}

SDL2Proxy::~SDL2Proxy() {
    uninit();
    SDL_Quit();
    LOG(INFO) << "~SDL2Proxy() ";
}

bool SDL2Proxy::initAudio(SDL_AudioSpec *spec, CoreMedia *core_media) {
    return SDL2Audio::instance()->init(spec, core_media);
}

bool SDL2Proxy::initVideo(VideoParams video_params, CoreMedia *core_media) {
    return SDL2Video::instance()->init(video_params, core_media);
}

void SDL2Proxy::uninit() {
    SDL2Audio::instance()->uninit();
    SDL2Video::instance()->uninit();
}

void SDL2Proxy::refreshAudio(void *udata, Uint8 *stream, int len) {
    SDL2Audio::instance()->refresh(udata, stream, len);
}

Uint32 SDL2Proxy::refreshVideo(Uint32 interval, void *opaque) {
    SDL2Video::instance()->refresh([&opaque](int milliseconds_delay) { scheduleRefreshVideo(milliseconds_delay); });
    return 0;
}

void SDL2Proxy::scheduleRefreshVideo(int millisecond_time) { SDL_AddTimer(millisecond_time, refreshVideo, nullptr); }