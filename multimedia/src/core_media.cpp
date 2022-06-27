#include "core_media.h"
#include "glog_proxy.h"
#include "swscale_proxy.h"

#include <functional>

CoreMedia::CoreMedia()
    : media_(""),
      format_context_(nullptr),
      video_(nullptr),
      audio_(nullptr),
      video_stream_index_(-1),
      audio_stream_index_(-1),
      tbn_{0, 0},
      media_time_(0),
      thread_video_(nullptr),
      thread_audio_(nullptr),
      thread_demux_(nullptr),
      status_(STOP),
      sdl_proxy_(nullptr) {
    LOG(INFO) << "CoreMedia() ";
    init();
}

CoreMedia::~CoreMedia() {
    unInit();
    LOG(INFO) << "~CoreMedia() ";
}

bool CoreMedia::init() {
    format_context_ = avformat_alloc_context();
    if (!format_context_) {
        LOG(ERROR) << "allocate memory for format context error";
        return false;
    }

    video_stream_index_ = -1;
    audio_stream_index_ = -1;
    tbn_ = {0, 0};
    media_time_ = 0;

    video_ = new CoreDecoderVideo();
    audio_ = new CoreDecoderAudio();

    thread_video_ = new CoreThreadVideo(this);
    if (!thread_video_) {
        LOG(ERROR) << "new CoreThreadVideo error";
        return false;
    }

    thread_audio_ = new CoreThreadAudio(this);
    if (!thread_audio_) {
        LOG(ERROR) << "new CoreThreadAudio error";
        return false;
    }

    thread_demux_ = new CoreThreadDemux(this);
    if (!thread_demux_) {
        LOG(INFO) << "new CoreThreadDemux error";
        return false;
    }

    status_cb_ = std::bind(&CoreMedia::statusCallback, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3);

    status_ = STOP;
    sdl_proxy_ = std::make_shared<SDL2Proxy>();

    audio_frame_queue_ = std::make_shared<FrameQueue>();
    video_frame_queue_ = std::make_shared<FrameQueue>();

    return true;
}

void CoreMedia::unInit() {
    status_ = STOP;

    if (audio_) {
        delete audio_;
        audio_ = nullptr;
    }
    if (video_) {
        delete video_;
        video_ = nullptr;
    }

    if (thread_demux_) {
        delete thread_demux_;
        thread_demux_ = nullptr;
    }
    if (thread_audio_) {
        delete thread_audio_;
        thread_audio_ = nullptr;
    }
    if (thread_video_) {
        delete thread_video_;
        thread_video_ = nullptr;
    }
    status_cb_ = nullptr;

    close();
}

bool CoreMedia::start() {
    if (video_stream_index_ == -1 && audio_stream_index_ == -1) {
        LOG(ERROR) << "can't find video/audio stream";
        unInit();
        return false;
    }
    status_ = START;
    videoFrameQueue()->setStatus(START);
    audioFrameQueue()->setStatus(START);

    if (!initVideo() || !initAudio() || !startThreads()) {
        unInit();
        return false;
    }

    return true;
}

bool CoreMedia::stop() {
    status_ = STOP;
    stopThreads();
    unInit();
    return true;
}

bool CoreMedia::pause() {
    if (status() == PAUSE)
        status_ = START;
    else if (status() == START)
        status_ = PAUSE;
    // LOG(INFO) << "pause";
    // videoFrameQueue()->setStatus(status());
    // audioFrameQueue()->setStatus(status());

    return true;
}

bool CoreMedia::open(const std::string& media) {
    media_ = media;
    if (avformat_open_input(&format_context_, media_.c_str(), NULL, NULL) != 0) {
        LOG(ERROR) << "ERROR could not open the file " << media_;
        return false;
    }

    if (avformat_find_stream_info(format_context_, NULL) < 0) {
        LOG(ERROR) << "ERROR could not get the stream info";
        return false;
    }
    // av_dump_format(format_context_, 0, media_.c_str(), 0);

    video_stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audio_stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index_ < 0 && video_stream_index_ < 0) {
        LOG(ERROR) << "can't find best video and audio stream";
        return false;
    }
    if (video_stream_index_ >= 0) {
        tbn_.num = format_context_->streams[video_stream_index_]->time_base.num;
        tbn_.den = format_context_->streams[video_stream_index_]->time_base.den;
    } else if (audio_stream_index_ >= 0) {
        tbn_.num = format_context_->streams[audio_stream_index_]->time_base.num;
        tbn_.den = format_context_->streams[audio_stream_index_]->time_base.den;
    }

    return true;
}

bool CoreMedia::close() {
    // is_open_.store(false);
    if (format_context_) {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }
    return true;
}

void CoreMedia::statusCallback(AVMediaType type, MediaStatus status, long pts) {
    double timestamp = pts * av_q2d(tbn_);
    if (type == AVMEDIA_TYPE_VIDEO) media_time_ = timestamp;
    // LOG(INFO) << av_get_media_type_string(type) << " " << statusTostr(status) << " " << timestamp;
}

void CoreMedia::loop() {
    bool quit = false;
    while (!quit) {
        SDL_PollEvent(&event_);
        switch (event_.type) {
            case SDL_KEYDOWN:
                switch (event_.key.keysym.sym) {
                    case SDLK_p:
                    case SDLK_SPACE:
                        pause();
                        break;
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        quit = true;
                        stop();
                        break;
                    default:
                        break;
                }
                break;
            case FF_REFRESH_EVENT:
                // sdl_proxy_->refreshVideo(0, event_.user.data1);
                break;
            // case FF_QUIT_EVENT:
            case SDL_QUIT:
                quit = true;
                stop();
                break;

            default:
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    LOG(INFO) << "CoreMedia::loop() end, program quit";
}

std::shared_ptr<FrameQueue>& CoreMedia::videoFrameQueue() { return video_frame_queue_; }
std::shared_ptr<FrameQueue>& CoreMedia::audioFrameQueue() { return audio_frame_queue_; }

bool CoreMedia::initVideo() {
    if (video_stream_index_ >= 0)
        return video_->init(
            format_context_, video_stream_index_,
            [this](VideoParams in) {
                VideoParams out{1280, 720, in.format};
                if (!SwscaleProxy::instance()->init(in, out)) return false;
                // sdl_proxy_->initVideo(in.width, in.height, this);
                sdl_proxy_->initVideo(out, this);
                sdl_proxy_->scheduleRefreshVideo(1);
            },
            videoFrameQueue());
    return true;
}

bool CoreMedia::initAudio() {
    if (audio_stream_index_ >= 0)
        return audio_->init(
            format_context_, audio_stream_index_,
            [this](AudioParams in) {
                AudioParams out{in.channel, AV_SAMPLE_FMT_S16, 48000};
                if (!SwresampleProxy::instance()->init(in, out)) return false;

                SDL_AudioSpec spec = {0};
                spec.freq = out.sample_rate;
                spec.channels = static_cast<uint8_t>(out.channel);
                spec.format = AUDIO_S16SYS;
                spec.silence = 0;
                spec.samples = SDL_AUDIO_BUFFER_SIZE;
                spec.callback = sdl_proxy_->refreshAudio;
                spec.userdata = this;
                return sdl_proxy_->initAudio(&spec, this);
            },
            audioFrameQueue());
    return true;
}

bool CoreMedia::startThreads() {
    thread_video_->start();
    thread_audio_->start();
    thread_demux_->start();
    return true;
}
void CoreMedia::stopThreads() {
    WaitForSingleObject(thread_demux_->waitCloseHandle(), INFINITE);

    videoFrameQueue()->setStatus(status());
    videoFrameQueue()->notify();
    WaitForSingleObject(thread_video_->waitCloseHandle(), 3);

    audioFrameQueue()->setStatus(status());
    audioFrameQueue()->notify();
    WaitForSingleObject(thread_audio_->waitCloseHandle(), 3);
}