#include <gtest/gtest.h>
#include "core_media.h"

TEST(multimediaPlayerTest, testMultimedia) {
    // current path is /multimedia player/build/multimedia/test when  CMake Run Tests
    std::string media_file = "../../../resource/media/test.mp4";
    CoreMedia media;
    if (media.open(media_file) && media.start()) {
    }
    EXPECT_TRUE(true);
}
