// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <unordered_map>

#include "common/common_types.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"

namespace Vulkan {

// Mesh pipelines have no vertex input, but the key can retain the previous vertex draw's input.
template <typename Key>
class MeshPipelineInputAliases {
public:
    bool Canonicalize(Key& key) {
        const VertexInput input = Read(key.state);
        const auto [it, inserted] = first_inputs.try_emplace(WithoutInput(key), input);
        if (inserted || it->second == input) {
            return false;
        }
        Key canonical = key;
        Write(canonical.state, it->second);
        if (canonical == key) {
            return false;
        }
        key = canonical;
        return true;
    }

    void Remember(const Key& key) {
        first_inputs.try_emplace(WithoutInput(key), Read(key.state));
    }

private:
    struct VertexInput {
        u64 attribute_types;
        std::array<FixedPipelineState::VertexAttribute, Maxwell::NumVertexAttributes> attributes;
        std::array<u32, Maxwell::NumVertexArrays> binding_divisors;
        std::array<u16, Maxwell::NumVertexArrays> vertex_strides;

        bool operator==(const VertexInput& other) const noexcept {
            return attribute_types == other.attribute_types &&
                   binding_divisors == other.binding_divisors &&
                   vertex_strides == other.vertex_strides &&
                   std::ranges::equal(attributes, other.attributes,
                                      [](const auto& lhs, const auto& rhs) {
                                          return lhs.raw == rhs.raw;
                                      });
        }
    };

    static VertexInput Read(const FixedPipelineState& state) {
        return {state.attribute_types, state.attributes, state.binding_divisors,
                state.vertex_strides};
    }

    static void Write(FixedPipelineState& state, const VertexInput& input) {
        state.attribute_types = input.attribute_types;
        state.attributes = input.attributes;
        state.binding_divisors = input.binding_divisors;
        state.vertex_strides = input.vertex_strides;
    }

    static Key WithoutInput(Key key) {
        Write(key.state, VertexInput{});
        return key;
    }

    std::unordered_map<Key, VertexInput> first_inputs;
};

} // namespace Vulkan
