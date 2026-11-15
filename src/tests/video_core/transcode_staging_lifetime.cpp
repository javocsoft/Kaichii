// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/transcode_staging_lifetime.h"

TEST_CASE("ASTC transcode output remains reserved until its upload is recorded",
          "[video_core][transcode]") {
    struct Output {
        bool deferred{true};
        bool upload_recorded{};
    } output;
    struct Pool {
        bool released{};
        void FreeDeferred(Output& ref) {
            REQUIRE(ref.deferred);
            REQUIRE(ref.upload_recorded);
            ref.deferred = false;
            released = true;
        }
    } pool;

    Vulkan::UploadAndReleaseTranscodeOutput(pool, output, [&] {
        CHECK_FALSE(pool.released);
        output.upload_recorded = true;
    });
    CHECK(pool.released);
    CHECK_FALSE(output.deferred);
}
