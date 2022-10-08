#include "glog_proxy.h"
#include "core_media.h"

#undef main
int main(int argc, char **argv) {
    InitLog(argv[0]);
    LOG(INFO) << "L7Player init";
    // std::string media_url = "../../resource/media/nature.mkv";
    std::string media_url = "https://media.w3.org/2010/05/sintel/trailer.mp4";
    if (argc >= 2) {
        media_url = argv[1];
    }

    {
        CoreMedia media;
        if (media.open(media_url) && media.start()) {
            media.loop();
        }
    }

    UninitLog();
    return 0;
}