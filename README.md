## multimedia player

# 1.0.0.0
# Prepare
You have to go to https://ffmpeg.org/download.html to download the latest version of ffmpeg.
Extract it to /multimedia-player/external_dependencies/ directory and rename it to ffmpeg.
The current version of ffmpeg is 4.4

You have to create before running the test case /multimedia-player/resource/media directory and prepare test.mp4 file.

The dir is:
multimedia-player
    bin
    external_dependencies
        ffmpeg # download and extract by youself
        glad
        glfw
        glog
        googletest
        sdl
    inc
    lib
    multimedia
    resource
        media
            test.mp4 # prepare by youself