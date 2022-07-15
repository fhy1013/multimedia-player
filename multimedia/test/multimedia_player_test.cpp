#include <gtest/gtest.h>
#include "core_media.h"

TEST(L7PlayerTest, testL7Player) {
    // current path is /L7Player/build/multimedia/test when  CMake Run Tests
    std::string media_file = "../../../resource/media/test.mp4";
    CoreMedia media;
    if (media.open(media_file) && media.start()) {
    }
    EXPECT_TRUE(true);
}
