#include "sdl_proxy.h"
#include "glog_proxy.h"
#include "core_media.h"
#include "swresample_proxy.h"
#include "audio_frame_buffer.h"

#include <fstream>

SDL2Audio::SDL2Audio() : audio_buf_(nullptr), audio_buf_size_(0), audio_buf_index_(0), audio_callback_time_(0) {}

SDL2Audio::~SDL2Audio() {}

SDL2Audio *SDL2Audio::instance() {
    static SDL2Audio self;
    return &self;
}

bool SDL2Audio::init(SDL_AudioSpec *spec, std::shared_ptr<FrameQueue> &audio_frame_queue) {
    spec_ = *spec;
    audio_frame_queue_ = audio_frame_queue;

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

void SDL2Audio::refresh(void *udata, Uint8 *stream, int len) {
    CoreMedia *core_media = (CoreMedia *)udata;
    int audio_size;
    int len1;

    static size_t count = 0;
    // LOG(INFO) << "refresh len: " << len;
    SDL_memset(stream, 0, len);
    while (len > 0) {
        if (audio_buf_index_ >= audio_buf_size_) {
            // get audio frame form audio frame queue
            audio_size = getPcmFromAudioFrameQueue();
            // audio_size = getPcmFromFile();
            if (audio_size < 0) {
                audio_buf_ = new uint8_t[SDL_AUDIO_BUFFER_SIZE];
                memset(audio_buf_, 0x00, SDL_AUDIO_BUFFER_SIZE);
                audio_buf_size_ = SDL_AUDIO_BUFFER_SIZE;
                LOG(WARNING) << "audio refresh get buffer failed, set silence ";
            } else {
                // LOG(INFO) << "audio refresh get buffer " << ++count << ", size: " << audio_size;
                audio_buf_size_ = audio_size;
            }
            audio_buf_index_ = 0;
        }
        len1 = audio_buf_size_ - audio_buf_index_;
        if (len1 > len) len1 = len;
        SDL_MixAudio(stream, (uint8_t *)audio_buf_ + audio_buf_index_, len1, SDL_MIX_MAXVOLUME);
        // writePcm(audio_buf_ + audio_buf_index_, len1);

        len -= len1;
        stream += len1;
        audio_buf_index_ += len1;

        // delete convert buffer
        if (audio_buf_index_ >= audio_buf_size_ && audio_buf_) {
            delete[] audio_buf_;
            audio_buf_ = nullptr;
        }
    }
}

int SDL2Audio::getPcmFromAudioFrameQueue() {
    Frame *af = nullptr;

    while (audio_frame_queue_->frameRemaining() == 0) {
        // if ((av_gettime_relative() - audio_callback_time_) > )
        return -1;
    }
    af = audio_frame_queue_->peekReadable();
    if (!af) return -1;
    audio_frame_queue_->frameNext();

    auto swresample_proxy = SwresampleProxy::instance();
    auto nb_samples = swresample_proxy->avRescaleRnd(af->frame);
    auto sambuf_size = swresample_proxy->avSamplesGetBufferSize(nb_samples);

    auto out_swr_context = swresample_proxy->outSwrContextParam();

    // new convert buffer
    std::shared_ptr<AudioFrameBuffer> audio_buf =
        std::make_shared<AudioFrameBuffer>(out_swr_context.channel, sambuf_size, out_swr_context.format);

    auto nb_convert = swresample_proxy->resample(audio_buf.get()->getBuffer(), nb_samples,
                                                 (const uint8_t **)af->frame->data, af->frame->nb_samples);
    if (nb_convert < 0) {
        LOG(ERROR) << "swr_convert fialed";
        return -1;
    }
    // auto nb_convert = 0;
    // while (nb_convert > 0) {
    //     nb_convert += nb_convert;
    //     auto buffer_size = swresample_proxy->avSamplesGetBufferSize(nb_convert);
    //     nb_convert = swresample_proxy->resample(&audio_buf_, nb_samples, nullptr, 0);
    //     if (nb_convert < 0) {
    //         LOG(ERROR) << "swr_convert fialed";
    //         return -1;
    //     }
    // }
    auto convert_buffer_size = swresample_proxy->avSamplesGetBufferSize(nb_convert);
    audio_buf_ = new uint8_t[convert_buffer_size];
    memset(audio_buf_, 0x00, convert_buffer_size);
    if (av_sample_fmt_is_planar(out_swr_context.format))
        copyPlanar(audio_buf_, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                   out_swr_context.channel);
    else
        copyPacked(audio_buf_, audio_buf.get()->getBuffer(), out_swr_context.format, nb_convert,
                   out_swr_context.channel);
    // writePcm(audio_buf_, convert_buffer_size);

    return convert_buffer_size;
}

// int SDL2Audio::getPcmFromFile() {
//     std::ifstream in("./out.pcm", std::ios::in | std::ios::binary);
//     if (!in.is_open()) {
//         LOG(ERROR) << "open pcm file failed";
//         return -1;
//     }
//     static uint64_t index = 0;
//     static bool end = false;
//     if (end) return -1;
//     in.seekg(index);
//     auto len = 1024;
//     audio_buf_ = new uint8_t[len];
//     memset(audio_buf_, 0x00, len);
//     in.read(reinterpret_cast<char *>(audio_buf_), len);
//     if (in.bad()) {
//         end = true;
//         delete[] audio_buf_;
//         return -1;
//     }
//     auto read_size = in.gcount();
//     index = in.tellg();
//     in.close();
//     return read_size;
// }

// Planar: L L L L R R R R
void SDL2Audio::copyPlanar(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels) {
    auto per_sample_size = av_get_bytes_per_sample(format);
    size_t offset = 0;
    for (auto i = 0; i < nb_samples; ++i)
        for (auto channel = 0; channel < channels; ++channel) {
            // out_.write(reinterpret_cast<char*>(buff[channel] + per_sample_size * i), per_sample_size);
            memcpy(des + offset, reinterpret_cast<char *>(src[channel] + per_sample_size * i), per_sample_size);
            offset += per_sample_size;
        }
}

// Packed: L R L R L R L R
void SDL2Audio::copyPacked(uint8_t *des, uint8_t **src, AVSampleFormat format, int64_t nb_samples, int channels) {
    auto per_sample_size = av_get_bytes_per_sample(format);
    memcpy(des, reinterpret_cast<char *>(src[0]), per_sample_size * nb_samples * channels);
}

// void SDL2Audio::writePcm(uint8_t *data, size_t len) {
//     std::ofstream out("./out1.pcm", std::ios::binary | std::ios::app);
//     out.write(reinterpret_cast<char *>(data), len);
// }

SDL2Video::SDL2Video() : window_(nullptr), render_(nullptr), texture_(nullptr), rect_({0, 0, 0, 0}) {}

SDL2Video::~SDL2Video() { uninit(); }

SDL2Video *SDL2Video::instance() {
    static SDL2Video self;
    return &self;
}

bool SDL2Video::init(int width, int height, std::shared_ptr<FrameQueue> &video_frame_queue) {
    video_frame_queue_ = video_frame_queue;

    window_ = SDL_CreateWindow("Multimedia-Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        LOG(ERROR) << "failed to create window by sdl";
        return false;
    }
    render_ = SDL_CreateRenderer(window_, -1, 0);
    if (!render_) {
        LOG(ERROR) << "failed to create render";
        return false;
    }
    texture_ = SDL_CreateTexture(render_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture_) {
        LOG(ERROR) << "failed to create texture";
        return false;
    }
    return true;
}

bool SDL2Video::refresh(VideoRefreshCallbacks cb) {
    Frame *af = nullptr;
    if (getFrame(&af)) {
        cb(af->duration);

        SDL_UpdateYUVTexture(texture_, nullptr, af->frame->data[0], af->frame->linesize[0], af->frame->data[1],
                             af->frame->linesize[1], af->frame->data[2], af->frame->linesize[2]);
        rect_.x = 0;
        rect_.y = 0;
        rect_.w = af->width;
        rect_.h = af->height;

        SDL_RenderClear(render_);
        SDL_RenderCopy(render_, texture_, nullptr, &rect_);
        SDL_RenderPresent(render_);
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
}
bool SDL2Video::getFrame(Frame **frame) {
    if (video_frame_queue_->frameRemaining() == 0) return false;
    *frame = video_frame_queue_->peekReadable();
    if (!*frame) return false;
    video_frame_queue_->frameNext();
    return true;
}

SDL2Proxy *SDL2Proxy::instance() {
    static SDL2Proxy self;
    return &self;
}

SDL2Proxy::SDL2Proxy() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        LOG(ERROR) << "failed SDL_Init";
        return;
    }
}

SDL2Proxy::~SDL2Proxy() { SDL_Quit(); }

bool SDL2Proxy::initAudio(SDL_AudioSpec *spec, std::shared_ptr<FrameQueue> &audio_frame_queue) {
    return SDL2Audio::instance()->init(spec, audio_frame_queue);
}

bool SDL2Proxy::initVideo(int width, int height, std::shared_ptr<FrameQueue> &video_frame_queue) {
    return SDL2Video::instance()->init(width, height, video_frame_queue);
}

void SDL2Proxy::refreshAudio(void *udata, Uint8 *stream, int len) {
    SDL2Audio::instance()->refresh(udata, stream, len);
}

Uint32 SDL2Proxy::refreshVideo(Uint32 interval, void *opaque) {
    SDL2Video::instance()->refresh([&opaque](int millisecond) {
        scheduleRefreshVideo(millisecond);
        // SDL_Event event;
        // event.type = FF_REFRESH_EVENT;
        // event.user.data1 = opaque;
        // SDL_PushEvent(&event);
    });
    return 0;
}

void SDL2Proxy::scheduleRefreshVideo(int millisecond_time) { SDL_AddTimer(millisecond_time, refreshVideo, nullptr); }
