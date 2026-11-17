// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/mesh_pipeline_input_aliases.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"

namespace {

Vulkan::GraphicsPipelineCacheKey MeshKey(bool dynamic_input) {
    Vulkan::GraphicsPipelineCacheKey key{};
    key.unique_hashes = {0, 0x46cc6aaba42f700f, 0, 0xa7a82b69c7715831, 0,
                         0x9451c0e980b1cc56};
    key.state.dynamic_vertex_input.Assign(dynamic_input ? 1 : 0);
    key.state.color_formats[0] = 0xca;
    return key;
}

} // namespace

TEST_CASE("Mesh pipeline lookup reuses a key with different leftover attribute types",
          "[video_core][mesh_pipeline_input]") {
    Vulkan::MeshPipelineInputAliases<Vulkan::GraphicsPipelineCacheKey> aliases;
    auto cached = MeshKey(true);
    cached.state.attribute_types = 0x5;
    auto drawn = MeshKey(true);
    drawn.state.attribute_types = 0xa0;
    REQUIRE(cached != drawn);

    aliases.Remember(cached);
    REQUIRE(aliases.Canonicalize(drawn));
    REQUIRE(drawn == cached);
    REQUIRE(drawn.Hash() == cached.Hash());
}

TEST_CASE("Mesh pipeline lookup reuses static vertex input left by another draw",
          "[video_core][mesh_pipeline_input]") {
    Vulkan::MeshPipelineInputAliases<Vulkan::GraphicsPipelineCacheKey> aliases;
    auto first = MeshKey(false);
    first.state.attributes[3].enabled.Assign(1);
    first.state.attributes[3].offset.Assign(12);
    first.state.vertex_strides[0] = 32;
    auto next = MeshKey(false);
    next.state.attributes[0].enabled.Assign(1);
    next.state.binding_divisors[1] = 2;
    REQUIRE(first != next);

    REQUIRE_FALSE(aliases.Canonicalize(first));
    REQUIRE(aliases.Canonicalize(next));
    REQUIRE(next == first);
}

TEST_CASE("Mesh pipeline aliases keep shaders and render targets distinct",
          "[video_core][mesh_pipeline_input]") {
    Vulkan::MeshPipelineInputAliases<Vulkan::GraphicsPipelineCacheKey> aliases;
    auto first = MeshKey(false);
    first.state.attributes[3].enabled.Assign(1);
    aliases.Remember(first);

    auto other_shader = MeshKey(false);
    other_shader.unique_hashes[3] ^= 1;
    auto other_target = MeshKey(false);
    other_target.state.color_formats[0] = 0xc6;
    REQUIRE_FALSE(aliases.Canonicalize(other_shader));
    REQUIRE_FALSE(aliases.Canonicalize(other_target));
    REQUIRE(other_shader.state.attributes[3].enabled.Value() == 0);
    REQUIRE(other_target.state.attributes[3].enabled.Value() == 0);
}

TEST_CASE("Mesh pipeline aliases ignore fields excluded by dynamic state",
          "[video_core][mesh_pipeline_input]") {
    Vulkan::MeshPipelineInputAliases<Vulkan::GraphicsPipelineCacheKey> aliases;
    auto first = MeshKey(false);
    first.state.extended_dynamic_state.Assign(1);
    first.state.vertex_strides[0] = 16;
    auto next = first;
    next.state.vertex_strides[0] = 32;
    REQUIRE(first == next);

    aliases.Remember(first);
    REQUIRE_FALSE(aliases.Canonicalize(next));
    REQUIRE(next.state.vertex_strides[0] == 32);
}

TEST_CASE("Mesh pipeline aliases stay stable across alternating vertex formats",
          "[video_core][mesh_pipeline_input]") {
    Vulkan::MeshPipelineInputAliases<Vulkan::GraphicsPipelineCacheKey> aliases;
    auto first = MeshKey(true);
    first.state.attribute_types = 0x5;
    REQUIRE_FALSE(aliases.Canonicalize(first));
    for (const u64 input : {u64{0xa0}, u64{0x5}, u64{0x3}, u64{0xa0}}) {
        auto drawn = MeshKey(true);
        drawn.state.attribute_types = input;
        REQUIRE(aliases.Canonicalize(drawn) == (input != 0x5));
        REQUIRE(drawn == first);
    }
}
