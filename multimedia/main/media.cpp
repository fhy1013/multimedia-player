#include "glog_proxy.h"
int main(int argc, char **argv) {
    InitLog(argv[0]);
    LOG(INFO) << "multimedia_player init";
    UninitLog();
    return 0;
}