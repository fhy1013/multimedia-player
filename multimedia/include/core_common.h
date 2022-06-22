#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "libavutil/error.h"
#include "libswresample/swresample.h"

#include <SDL2/SDL.h>

#ifdef __cplusplus
}

// C++中使用av_err2str宏
extern char av_error[AV_ERROR_MAX_STRING_SIZE];
#define err2str(errnum) av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)
#endif

#define FF_QUIT_EVENT (SDL_USEREVENT + 1)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 2)