// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/texture_cache/types.h"

TEST_CASE("An unavailable render target is distinct from a valid null texture descriptor",
          "[video_core][texture_cache]") {
    const auto missing = VideoCommon::MissingRenderTargetView();
    CHECK_FALSE(missing);
    CHECK(missing != VideoCommon::NULL_IMAGE_VIEW_ID);
    CHECK(static_cast<bool>(VideoCommon::NULL_IMAGE_VIEW_ID));
}
