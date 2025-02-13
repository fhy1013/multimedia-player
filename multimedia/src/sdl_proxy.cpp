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
    // LOG(INFO) << "SDL2Audio() ";
}

SDL2Audio::~SDL2Audio() {
    // LOG(INFO) << "~SDL2Audio() ";
}

SDL2Audio *SDL2Audio::instance() {
    static SDL2Audio self;
    return &self;
}

bool SDL2Audio::init(SDL_AudioSpec *spec, CoreMedia *core_media) {
    spec_ = *spec;
    core_media_ = core_media;

    // clock_ = {0.0, 0.0, {2, 0, 0, 0}};
    clock_ = {0.0, {0, AV_SAMPLE_FMT_NONE, 0}};

    audio_volume_ = SDL_MIX_MAXVOLUME;
    muted_ = false;
    seek_ = false;
    update_audio_clock_ = false;

    // The second parameter of SDL_OpenAudio must bo nullptr.
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
                seek();
                // get audio frame form audio frame queue
                audio_size = getPcmFromAudioFrameQueue();
            }

            if (audio_size < 0) {
                audio_buf_ = nullptr;
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
        if (audio_buf_) {
            if (!muted_)
                SDL_MixAudio(stream, (uint8_t *)audio_buf_ + audio_buf_index_, len1, audio_volume_);
            else
                SDL_MixAudio(stream, (uint8_t *)audio_buf_ + audio_buf_index_, len1, 0);
        } else {
            memset(stream, 0, len1);
        }

        len -= len1;
        stream += len1;
        audio_buf_index_ += len1;

        // delete convert buffer
        if (audio_buf_index_ >= audio_buf_size_ && audio_buf_) {
            delete[] audio_buf_;
            audio_buf_ = nullptr;
        }
    }
    if (!update_audio_clock_) {
        if (clock_.params.channel != 0 && clock_.params.format != AV_SAMPLE_FMT_NONE && clock_.params.sample_rate != 0)
            core_media_->setAudioPts(clock_.pts - (audio_buf_size_ - audio_buf_index_) /
                                                      (av_get_bytes_per_sample(clock_.params.format) *
                                                       clock_.params.channel * clock_.params.sample_rate));
    } else {
        clock_.pts = (double)core_media_->seek_.seek_pos / AV_TIME_BASE;
        core_media_->setAudioPts(clock_.pts);
        update_audio_clock_ = false;
    }

    // LOG(INFO) << "audio refresh frames pts: " << core_media_->audioPts();
}

void SDL2Audio::setAudioVolume(const int audio_volume) {
    if (audio_volume < 0)
        audio_volume_ = 0;
    else if (audio_volume > 128)
        audio_volume_ = 128;
    else
        audio_volume_ = audio_volume;
}

int SDL2Audio::audioVolume() const { return audio_volume_; }

void SDL2Audio::setMuted() { muted_ = !muted_; }

bool SDL2Audio::muted() const { return muted_; }

int SDL2Audio::getPcmFromAudioFrameQueue() {
    Frame *af = nullptr;

    while (true) {
        if (core_media_->audioFrameQueue()->frameRemaining() == 0) {
            // if ((av_gettime_relative() - audio_callback_time_) > )
            return -1;
        }
        af = core_media_->audioFrameQueue()->peekReadable();
        if (!af) return -1;
        core_media_->audioFrameQueue()->frameNext();
        if (seek_) {
            auto seek_pos = (double)core_media_->seek_.seek_pos / AV_TIME_BASE;
            if ((af->pts >= seek_pos) && af->pts - seek_pos < 1.0) {
                seek_ = false;
                break;
            }
        } else
            break;
    }
    // LOG(INFO) << "get audio frames pts: " << af->pts << ", duration: " << af->duration;

    auto swresample_proxy = SwresampleProxy::instance();
    auto out_samples = swresample_proxy->swrGetOutSamples(af->frame->nb_samples);
    auto sample_buff_size = swresample_proxy->avSamplesGetBufferSize(out_samples);
    auto out_swr_context = swresample_proxy->outSwrContextParam();

    if (sample_buff_size <= 0) {
        // LOG(ERROR) << "avSamplesGetBufferSize() <= 0, nb_samples: " << nb_samples;
        LOG(ERROR) << "avSamplesGetBufferSize() <= 0, out_samples: " << out_samples;
        return -1;
    }

    // new out buffer
    audio_buf_ = new uint8_t[sample_buff_size];
    memset(audio_buf_, 0x00, sample_buff_size);

    // new convert buffer
    std::shared_ptr<AudioFrameBuffer> audio_buf =
        std::make_shared<AudioFrameBuffer>(out_swr_context.channel, sample_buff_size, out_swr_context.format);
    auto nb_convert = swresample_proxy->resample(audio_buf.get()->getBuffer(), out_samples,
                                                 (const uint8_t **)af->frame->data, af->frame->nb_samples);
    if (nb_convert < 0) {
        LOG(ERROR) << "swr_convert failed";
        delete[] audio_buf_;
        audio_buf_ = nullptr;
        return -1;
    }
    auto index = 0;
    auto sample_sum = 0;
    do {
        sample_sum += nb_convert;
        if (av_sample_fmt_is_planar(out_swr_context.format))
            index += copyPlanar(audio_buf_ + index, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                                out_swr_context.channel);
        else
            index += copyPacked(audio_buf_ + index, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                                out_swr_context.channel);
        nb_convert = swresample_proxy->resample(audio_buf.get()->getBuffer(), out_samples, nullptr, 0);
        if (nb_convert < 0) {
            LOG(ERROR) << "swr_convert failed";
            delete[] audio_buf_;
            audio_buf_ = nullptr;
            return -1;
        }
    } while (nb_convert > 0);

    if (sample_sum > 0) {
        clock_.pts = af->pts + sample_sum / out_swr_context.sample_rate;
        clock_.params = out_swr_context;
    }

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

void SDL2Audio::seek() {
    if (!(core_media_->seek_.seek_req & 0x01) && (core_media_->seek_.seek_req & 0x04)) {
        LOG(INFO) << "audio frame queue clear";
        core_media_->audioFrameQueue()->clear();
        core_media_->seek_.seek_req &= 0xFB;
        seek_ = true;
        update_audio_clock_ = true;
    }
}

SDL2Video::SDL2Video()
    : window_(nullptr),
      render_(nullptr),
      texture_(nullptr),
      rect_({0, 0, 0, 0}),
      core_media_(nullptr),
      frame_(nullptr) {
    // LOG(INFO) << "SDL2Video() ";
    frame_ = av_frame_alloc();
}

SDL2Video::~SDL2Video() {
    if (!frame_) av_frame_free(&frame_);
    // LOG(INFO) << "~SDL2Video() ";
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

    window_ = SDL_CreateWindow("L7Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, video_params.width,
                               video_params.height, SDL_WINDOW_SHOWN);
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

    seek();
    double time;

retry:
    if (core_media_->videoFrameQueue()->frameRemaining() == 0) {
        cb(1);
        return false;
    }

    Frame *vp = nullptr;
    Frame *last_vp = nullptr;
    double last_duration, duration, delay;

    last_vp = core_media_->videoFrameQueue()->peekLast();
    vp = core_media_->videoFrameQueue()->peekCurrent();

    last_duration = vpDuration(last_vp, vp);
    delay = computeTargetDelay(last_duration, vp);

    time = av_gettime_relative() / 1000000.0;

    if (frame_timer_ + delay > time) {
        auto remaining_time = frame_timer_ + delay - time;
        cb(remaining_time * 1000);
        return false;
    }

    frame_timer_ += delay;

    if (delay > 0 && time - frame_timer_ > AV_SYNC_THRESHOLD_MAX) {
        frame_timer_ = time;
    }

    if (!isnan(vp->pts)) {
        core_media_->setVideoPts(vp->pts);
    }

    if (core_media_->videoFrameQueue()->frameRemaining() > 1) {
        auto *next_vp = core_media_->videoFrameQueue()->peekNext();
        duration = vpDuration(vp, next_vp);
        if (time > frame_timer_ + duration) {
            core_media_->videoFrameQueue()->frameNext();
            goto retry;
        }
    }

    core_media_->videoFrameQueue()->frameNext();

display:
    render(core_media_->videoFrameQueue()->peekLast());
    cb(1);

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
    if (!af) return;

    // LOG(INFO) << "video refresh pts " << af->pts << ", duration " << af->duration;

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
    auto diff = core_media_->videoPts() - core_media_->audioPts();
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

void SDL2Video::seek() {
    if (!(core_media_->seek_.seek_req & 0x01) && (core_media_->seek_.seek_req & 0x02)) {
        LOG(INFO) << "video frame queue clear";
        core_media_->videoFrameQueue()->clear();
        core_media_->seek_.seek_req &= 0xFD;
        last_pts_ = (double)core_media_->seek_.seek_pos / AV_TIME_BASE;
    }
}

double SDL2Video::vpDuration(Frame *vp, Frame *next_vp) {
    double duration = next_vp->pts - vp->pts;
    if (isnan(duration) || duration <= 0)
        return vp->duration;
    else
        return duration;
}

SDL2Proxy::SDL2Proxy() {
    // LOG(INFO) << "SDL2Proxy() ";
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        LOG(ERROR) << "failed SDL_Init";
        return;
    }
}

SDL2Proxy::~SDL2Proxy() {
    uninit();
    SDL_Quit();
    // LOG(INFO) << "~SDL2Proxy() ";
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

int SDL2Proxy::audioVolumeUp() {
    auto volume = SDL2Audio::instance()->audioVolume();
    volume += 5;
    SDL2Audio::instance()->setAudioVolume(volume);
    return SDL2Audio::instance()->audioVolume();
}
int SDL2Proxy::audioVolumeDown() {
    auto volume = SDL2Audio::instance()->audioVolume();
    volume -= 5;
    SDL2Audio::instance()->setAudioVolume(volume);
    return SDL2Audio::instance()->audioVolume();
}

int SDL2Proxy::audioVolume() const { return SDL2Audio::instance()->audioVolume(); }

bool SDL2Proxy::setMuted() {
    SDL2Audio::instance()->setMuted();
    return SDL2Audio::instance()->muted();
}

bool SDL2Proxy::muted() const { return SDL2Audio::instance()->muted(); }