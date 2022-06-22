#include "core_decoder_video.h"
#include "glog_proxy.h"

CoreDecoderVideo::CoreDecoderVideo() {}

CoreDecoderVideo::~CoreDecoderVideo() { unInit(); }

bool CoreDecoderVideo::init(AVFormatContext *format_context, int stream_index, VideoCallback cb,
                            std::shared_ptr<FrameQueue> &video_frame_queue) {
    auto ret = CoreDecoder::init(format_context, stream_index, video_frame_queue);
    if (!ret) return ret;

    cb(codec_context_->width, codec_context_->height);
    return ret;
}

void CoreDecoderVideo::unInit() {}

bool CoreDecoderVideo::decode(AVPacket *pack, DecodeCallback cb) {
    // static size_t count = 0;
    // LOG(INFO) << "Video count " << ++count;
    auto ret = avcodec_send_packet(codec_context_, pack);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOG(ERROR) << "Error in avcodec_send_packet: " << err2str(ret);
        av_packet_unref(pack);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_context_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG(ERROR) << "Error avcodec_receive_frame: " << err2str(ret);
            return false;
        }

        // LOG(INFO) << "Frame " << codec_context_->frame_number
        //           << " (type=" << av_get_picture_type_char(frame_->pict_type) << ", size=" << frame_->pkt_size
        //           << " bytes, format=" << frame_->format << ") pts " << frame_->pts << " key_frame "
        //           << frame_->key_frame << " [DTS " << frame_->coded_picture_number;
        // Check if the frame is a planar YUV 4:2:0, 12bpp
        // if (frame_->format != AV_PIX_FMT_YUV420P) {
        //     LOG(WARNING) << "Warning: the generated file may not be a grayscale image, but could e.g. be just the "
        //                     "R component if the video format is RGB";
        // } else
        //     saveFramAsYUV(frame_);
        cb(frame_->pts);
    }
}

// void CoreDecoderVideo::saveFramAsYUV(AVFrame *pFrame) {
//     // static std::map<int, unsigned char *> buff;
//     static int current_format_number = 0;
//     int width = pFrame->width;
//     int height = pFrame->height;
//     int len = width * height * 1.5;
//     //可以放在while循环外面，这样就不用每次都申请，释放了
//     unsigned char *yuv_buf = new unsigned char[len];
//     if (!yuv_buf) {
//         LOG(ERROR) << "malloc failed";
//         return;
//     }

//     // //把解码出来的数据存成YUV数据，方便验证解码是否正确
//     for (auto i = 0; i < height; i++) {
//         auto index = width * i;
//         memcpy(yuv_buf + width * i, pFrame->data[0] + pFrame->linesize[0] * i, width);
//         if (i < height / 2) {
//             index = width * height + width * i / 2;
//             memcpy(yuv_buf + width * height + width * i / 2, pFrame->data[1] + pFrame->linesize[1] * i, width / 2);
//             index = width * height * 5 / 4 + width * i / 2;
//             memcpy(yuv_buf + width * height * 5 / 4 + width * i / 2, pFrame->data[2] + pFrame->linesize[2] * i,
//                    width / 2);
//         }
//     }

//     out_.write(reinterpret_cast<char *>(yuv_buf), len);

//     delete[] yuv_buf;
// }