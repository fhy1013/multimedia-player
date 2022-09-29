#include "core_decoder.h"
#include "glog_proxy.h"

CoreDecoder::CoreDecoder()
    : stream_index_(-1),
      format_context_(nullptr),
      codec_parameters_(nullptr),
      codec_context_(nullptr),
      codec_(nullptr),
      stream_(nullptr),
      frame_(nullptr),
      packet_queue_(nullptr),
      frame_queue_(nullptr) {
    // LOG(INFO) << "CoreDecoder() ";
}

CoreDecoder::~CoreDecoder() {
    unInit();
    // LOG(INFO) << "~CoreDecoder() ";
}

bool CoreDecoder::init(AVFormatContext* format_context, int stream_index, std::shared_ptr<FrameQueue>& frame_queue) {
    unInit();
    format_context_ = format_context;
    if (stream_index < 0) return false;
    stream_index_ = stream_index;

    stream_ = format_context_->streams[stream_index];
    codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);
    codec_context_ = avcodec_alloc_context3(codec_);
    if (!codec_context_) {
        LOG(ERROR) << "avcodec_alloc_context3 failed!";
        return false;
    }

    avcodec_parameters_to_context(codec_context_, stream_->codecpar);
    auto ret = avcodec_open2(codec_context_, codec_, nullptr);
    if (ret < 0) {
        codec_context_ = nullptr;
        LOG(ERROR) << "avcodec_open2 failed err " << ret;
        return false;
    }

    frame_ = av_frame_alloc();

    packet_queue_ = std::make_shared<PacketQueue>();

    frame_queue_ = frame_queue;

    return true;
}

void CoreDecoder::unInit() {
    if (codec_context_) {
        avcodec_flush_buffers(codec_context_);
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }

    stream_ = nullptr;
    codec_ = nullptr;
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
}

// bool CoreDecoder::openOutFile(const std::string& file) {
//     out_.open(file, std::ios::trunc | std::ios::out | std::ios::binary);
//     if (!out_.is_open()) {
//         LOG(ERROR) << "Failed to open file " << file;
//         return false;
//     }
//     return true;
// }

// bool CoreDecoder::closeOutFile() {
//     out_.close();
//     return true;
// }

bool CoreDecoder::avcodecFlushBuffer(AVPacket* pack) {
    if (strcmp((char*)pack->data, FLUSH_DATA) == 0) {
        avcodec_flush_buffers(codec_context_);
        av_packet_unref(pack);
        return true;
    }
    return false;
}