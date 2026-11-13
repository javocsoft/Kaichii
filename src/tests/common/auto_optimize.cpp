// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/auto_optimize.h"

namespace {
using namespace Common::AutoOptimize;

/// Ocarina of Time on an RTX 3050 Laptop at half resolution: 31 fps, frame generation costs under
/// one frame and generates as many as the game presents.
Measurements Laptop() {
    Measurements m;
    m.base = {.seconds = 15.0, .fps = 31.3, .low_1pct_fps = 26.0, .stutters = 0};
    m.frame_gen_trial = Sample{.seconds = 15.0, .fps = 30.5, .low_1pct_fps = 25.0, .stutters = 0};
    m.generated_fps = 30.5;
    m.memory = VideoMemory{.used = 2'819ULL << 20, .budget = 3'547ULL << 20, .spilled = 741ULL << 20};
    m.resolution = 1;
    return m;
}

bool Has(const std::vector<Recommendation>& list, Action action) {
    for (const auto& item : list) {
        if (item.action == action) {
            return true;
        }
    }
    return false;
}
} // Anonymous namespace

TEST_CASE("Frame generation is proposed when the trial keeps the real frames", "[common][optimize]") {
    const auto list = Recommend(Laptop());
    REQUIRE(list.size() == 1);
    REQUIRE(list[0].action == Action::EnableFrameGeneration);
    REQUIRE_FALSE(list[0].needs_restart);
}

TEST_CASE("Frame generation is not proposed when it would hurt", "[common][optimize]") {
    auto m = Laptop();
    m.frame_gen_trial->fps = 27.0; // a saturated GPU: the trial lost 14 % of the real frames
    REQUIRE(JudgeFrameGeneration(m) == FrameGenerationVerdict::CostsTooMuch);
    REQUIRE(Recommend(m).empty());

    m = Laptop();
    m.base.fps = 18.0;
    m.frame_gen_trial->fps = 18.0;
    m.generated_fps = 18.0;
    REQUIRE(JudgeFrameGeneration(m) == FrameGenerationVerdict::TooFewFrames);

    m = Laptop();
    m.generated_fps = 2.0; // running, but the surface gives it nothing to do
    REQUIRE(JudgeFrameGeneration(m) == FrameGenerationVerdict::NotGenerating);

    m = Laptop();
    m.frame_gen_trial.reset();
    REQUIRE(JudgeFrameGeneration(m) == FrameGenerationVerdict::NotTried);

    m = Laptop();
    m.frame_gen_enabled = true;
    REQUIRE(JudgeFrameGeneration(m) == FrameGenerationVerdict::AlreadyOn);
    REQUIRE(Recommend(m).empty());
}

TEST_CASE("Full video memory proposes a lower resolution first, at the next boot",
          "[common][optimize]") {
    auto m = Laptop();
    m.resolution = 5; // 1.5x on a 4 GB card: 2.1 GiB of images in system memory, 7 fps
    m.base.fps = 7.2;
    m.frame_gen_trial.reset();
    m.memory =
        VideoMemory{.used = 3'215ULL << 20, .budget = 3'577ULL << 20, .spilled = 2'076ULL << 20};
    const auto list = Recommend(m);
    REQUIRE(list.size() == 1);
    REQUIRE(list[0].action == Action::LowerResolution);
    REQUIRE(list[0].reason == Reason::VideoMemoryFull);
    REQUIRE(list[0].needs_restart);

    // Nothing lower to offer, or no memory figures: no resolution change.
    m.resolution = m.lowest_resolution;
    REQUIRE_FALSE(Has(Recommend(m), Action::LowerResolution));
    m.resolution = 5;
    m.memory.reset();
    REQUIRE_FALSE(Has(Recommend(m), Action::LowerResolution));
    // Native resolution fits: 0.4 GiB in system memory, 30 fps.
    m.memory =
        VideoMemory{.used = 2'961ULL << 20, .budget = 3'532ULL << 20, .spilled = 394ULL << 20};
    REQUIRE_FALSE(Has(Recommend(m), Action::LowerResolution));
}

TEST_CASE("Many stutters propose skipping draws whose shaders still build", "[common][optimize]") {
    auto m = Laptop();
    m.base.stutters = 6; // in 15 s: 24 a minute
    REQUIRE(Has(Recommend(m), Action::SkipUnbuiltDraws));
    m.skips_unbuilt_draws = true;
    REQUIRE_FALSE(Has(Recommend(m), Action::SkipUnbuiltDraws));
    m.skips_unbuilt_draws = false;
    m.base.stutters = 2;
    REQUIRE_FALSE(Has(Recommend(m), Action::SkipUnbuiltDraws));
}

TEST_CASE("Shaders built again while playing propose preloading them", "[common][optimize]") {
    auto m = Laptop();
    m.cached_pipelines_built_late = 9; // in 15 s: 36 a minute, as walking in Kokiri Forest
    const auto list = Recommend(m);
    REQUIRE(Has(list, Action::PreloadShaders));
    for (const auto& item : list) {
        if (item.action == Action::PreloadShaders) {
            REQUIRE(item.reason == Reason::ShadersBuiltAgain);
            REQUIRE(item.needs_restart);
        }
    }
    m.preloads_shaders = true;
    REQUIRE_FALSE(Has(Recommend(m), Action::PreloadShaders));
    m.preloads_shaders = false;
    m.cached_pipelines_built_late = 2; // standing still: a few
    REQUIRE_FALSE(Has(Recommend(m), Action::PreloadShaders));
}
