// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/pipeline_cache_policy.h"

using namespace VideoCommon;

TEST_CASE("Graphics pipeline budget is a sixteenth of host RAM", "[video_core][pipeline_cache]") {
    constexpr u64 GiB = u64{1} << 30;
    STATIC_REQUIRE(GraphicsPipelineBudget(0) == 0);
    STATIC_REQUIRE(GraphicsPipelineBudget(16 * GraphicsPipelineBytes - 1) == 0);
    STATIC_REQUIRE(GraphicsPipelineBudget(16 * GraphicsPipelineBytes) == 1);
    CHECK(GraphicsPipelineBudget(16 * GiB) == 2386);
    CHECK(GraphicsPipelineBudget(32 * GiB) == 4772);
}

TEST_CASE("Graphics pipelines are evicted only past the budget", "[video_core][pipeline_cache]") {
    SECTION("within the budget nothing is evicted, however stale") {
        CHECK(GraphicsPipelinesToEvict(0, 0, 100) == 0);
        CHECK(GraphicsPipelinesToEvict(100, 100, 100) == 0);
        CHECK(GraphicsPipelinesToEvict(50, 50, 100) == 0);
    }
    SECTION("past the budget only the excess is evicted") {
        CHECK(GraphicsPipelinesToEvict(101, 101, 100) == 1);
        CHECK(GraphicsPipelinesToEvict(150, 80, 100) == 50);
    }
    SECTION("pipelines used recently are kept even past the budget") {
        CHECK(GraphicsPipelinesToEvict(150, 20, 100) == 20);
        CHECK(GraphicsPipelinesToEvict(150, 0, 100) == 0);
    }
    SECTION("a zero budget evicts every stale pipeline") {
        CHECK(GraphicsPipelinesToEvict(10, 7, 0) == 7);
    }
}

TEST_CASE("Unused prebuilt pipelines stay out of eviction up to a budget",
          "[video_core][pipeline_cache]") {
    size_t kept = 0;
    CHECK_FALSE(KeepsUnusedPrebuiltPipeline(true, kept));
    CHECK(kept == 0);
    for (size_t i = 0; i < MaxUnusedPrebuiltPipelines; ++i) {
        REQUIRE(KeepsUnusedPrebuiltPipeline(false, kept));
    }
    CHECK(kept == MaxUnusedPrebuiltPipelines);
    // Past the budget they are candidates like the rest.
    CHECK_FALSE(KeepsUnusedPrebuiltPipeline(false, kept));
    CHECK_FALSE(KeepsUnusedPrebuiltPipeline(true, kept));
    CHECK(kept == MaxUnusedPrebuiltPipelines);
}

TEST_CASE("Vertex attribute mask follows the header's input map", "[video_core][pipeline_cache]") {
    std::array<std::array<bool, 4>, 32> inputs{};
    CHECK(VertexAttributeMask(inputs) == 0);

    inputs[0] = {true, false, false, false};
    inputs[3] = {false, false, false, true};
    inputs[31] = {false, true, true, false};
    CHECK(VertexAttributeMask(inputs) == ((u64{3} << 0) | (u64{3} << 6) | (u64{3} << 62)));

    inputs.fill({true, true, true, true});
    CHECK(VertexAttributeMask(inputs) == ~u64{0});
}

TEST_CASE("Pipeline keys keep only the attribute types active stages read",
          "[video_core][pipeline_cache]") {
    constexpr u64 types = 0x1234'5678'9abc'def0;
    constexpr u64 first_four = 0xff;

    SECTION("a VertexA program keeps every type") {
        const std::array<u64, 6> hashes{1, 2, 0, 0, 0, 3};
        const std::array<u64, 6> masks{0, 0, 0, 0, 0, 0};
        CHECK(KeyedAttributeTypes(types, hashes, masks) == types);
    }
    SECTION("the vertex program's input map masks the types") {
        const std::array<u64, 6> hashes{0, 2, 0, 0, 0, 3};
        const std::array<u64, 6> masks{0, first_four, 0, 0, 0, ~u64{0}};
        CHECK(KeyedAttributeTypes(types, hashes, masks) == (types & first_four));
    }
    SECTION("inactive stages do not mask") {
        const std::array<u64, 6> hashes{0, 2, 0, 0, 0, 0};
        const std::array<u64, 6> masks{0, ~u64{0}, 0, 0, 0, 0};
        CHECK(KeyedAttributeTypes(types, hashes, masks) == types);
    }
    SECTION("task and mesh programs drop every type") {
        const std::array<u64, 6> hashes{0, 0, 7, 8, 0, 3};
        const std::array<u64, 6> masks{0, 0, 0, 0, 0, ~u64{0}};
        CHECK(KeyedAttributeTypes(types, hashes, masks) == 0);
    }
    SECTION("garbage in unread registers no longer changes the key") {
        const std::array<u64, 6> hashes{0, 2, 0, 0, 0, 3};
        const std::array<u64, 6> masks{0, first_four, 0, 0, 0, ~u64{0}};
        const u64 other = (types & first_four) | 0xabcd'0000'0000'0000;
        CHECK(KeyedAttributeTypes(types, hashes, masks) ==
              KeyedAttributeTypes(other, hashes, masks));
    }
}

TEST_CASE("Shader preload memory follows the setting", "[video_core][pipeline_cache]") {
    using Settings::ShaderPreloadMemory;
    constexpr u64 GiB = u64{1} << 30;
    STATIC_REQUIRE(ShaderPreloadBytes(ShaderPreloadMemory::Off, 32 * GiB) == 0);
    STATIC_REQUIRE(ShaderPreloadBytes(ShaderPreloadMemory::Gib2, 8 * GiB) == 2 * GiB);
    // Auto: 1/16 of the RAM, from 512 MiB to 4 GiB.
    STATIC_REQUIRE(ShaderPreloadBytes(ShaderPreloadMemory::Auto, 32 * GiB) == 2 * GiB);
    STATIC_REQUIRE(ShaderPreloadBytes(ShaderPreloadMemory::Auto, 4 * GiB) == GiB / 2);
    STATIC_REQUIRE(ShaderPreloadBytes(ShaderPreloadMemory::Auto, 128 * GiB) == 4 * GiB);
    // 2 GiB holds about 2,000 built pipelines.
    CHECK(ShaderPreloadBytes(ShaderPreloadMemory::Gib2, 0) / PreloadedPipelineBytes == 2045);
}

TEST_CASE("The shader preload takes the most recently used pipelines first",
          "[video_core][pipeline_cache]") {
    // Built at boot: ages 0-15. Older: 10 at age 16, 10 at age 20, 10 at age 100.
    std::vector<u8> ages(40, 3);
    ages.insert(ages.end(), 10, 16);
    ages.insert(ages.end(), 10, 20);
    ages.insert(ages.end(), 10, 100);
    CHECK(PreloadAgeLimit(ages, 16, 5) == 17);
    CHECK(PreloadAgeLimit(ages, 16, 10) == 17);
    CHECK(PreloadAgeLimit(ages, 16, 11) == 21);
    CHECK(PreloadAgeLimit(ages, 16, 30) == 101);
    // Fewer than asked: every older one.
    CHECK(PreloadAgeLimit(ages, 16, 1000) == 256);
    CHECK(PreloadAgeLimit({}, 16, 10) == 256);
}
