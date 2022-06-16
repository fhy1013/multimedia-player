#pragma once

#include "glog_proxy.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}

// C++中使用av_err2str宏
extern char av_error[AV_ERROR_MAX_STRING_SIZE];
#define err2str(errnum) av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)
#endif