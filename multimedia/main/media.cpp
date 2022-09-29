#include "glog_proxy.h"
#include "core_media.h"

#undef main
int main(int argc, char **argv) {
    InitLog(argv[0]);
    LOG(INFO) << "L7Player init";
    std::string media_file = "../../resource/media/nature.mkv";
    if (argc >= 2) {
        media_file = argv[1];
    }

    {
        CoreMedia media;
        if (media.open(media_file) && media.start()) {
            media.loop();
        }
    }

    UninitLog();
    return 0;
}