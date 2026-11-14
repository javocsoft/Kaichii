// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/texture_cache/game_upscale.h"

using VideoCommon::GameUpscale::HeldNative;
using VideoCommon::GameUpscale::HoldFrames;
using VideoCommon::GameUpscale::Holding;
using VideoCommon::GameUpscale::IsUpscale;
using VideoCommon::GameUpscale::Source;

TEST_CASE("A compute dispatch upscales a smaller render target", "[video_core][rescale]") {
    // Donkey Kong Bananza: its dynamic resolution scene, upscaled to the 1920x1080 frame.
    const Source scene{
        .rescaleable = true, .colour = true, .is_2d = true, .width = 1397, .height = 786};
    CHECK(IsUpscale(scene, 1920, 1080));

    // Same size, smaller, or larger in one direction only.
    CHECK_FALSE(IsUpscale(scene, 1397, 786));
    CHECK_FALSE(IsUpscale(scene, 698, 393));
    CHECK_FALSE(IsUpscale(scene, 1920, 786));

    // A texture from the game, depth or stencil, or not a 2D image.
    Source texture = scene;
    texture.rescaleable = false;
    CHECK_FALSE(IsUpscale(texture, 1920, 1080));
    Source depth = scene;
    depth.colour = false;
    CHECK_FALSE(IsUpscale(depth, 1920, 1080));
    Source volume = scene;
    volume.is_2d = false;
    CHECK_FALSE(IsUpscale(volume, 1920, 1080));
}

TEST_CASE("A game upscale holds the images below its output native", "[video_core][rescale]") {
    // The reduced scene and half-size passes stay native; the full-size frame, the 2880x1620
    // output and square shadow maps taller than the frame keep the scale.
    CHECK(HeldNative(1397, 786, 1920, 1080));
    CHECK(HeldNative(960, 540, 1920, 1080));
    CHECK_FALSE(HeldNative(1920, 1080, 1920, 1080));
    CHECK_FALSE(HeldNative(2880, 1620, 1920, 1080));
    CHECK_FALSE(HeldNative(1376, 1368, 1920, 1080));

    // It holds for a while after the last upscale, then lets them follow the scale again.
    CHECK(Holding(100, 100));
    CHECK(Holding(100, 100 + HoldFrames));
    CHECK_FALSE(Holding(100, 101 + HoldFrames));
}
