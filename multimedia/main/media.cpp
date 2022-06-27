#include "glog_proxy.h"
#include "core_media.h"

#undef main
int main(int argc, char **argv) {
    InitLog(argv[0]);
    LOG(INFO) << "multimedia_player init";
    // std::string media_file = "../../resource/media/test.mp4";
    std::string media_file = "../../resource/media/nature.mkv";
    {
        CoreMedia media;
        if (media.open(media_file) && media.start()) {
            media.loop();
        }
    }

    UninitLog();
    return 0;
}