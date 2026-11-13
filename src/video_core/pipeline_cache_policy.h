// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "common/common_types.h"
#include "common/settings_enums.h"

namespace VideoCommon {

/// Driver memory a built graphics pipeline holds (measured on NVIDIA).
constexpr u64 GraphicsPipelineBytes = 450'000;

/// Graphics pipelines kept before any is evicted: 1/16 of host RAM.
/// @param total_physical_memory Host RAM in bytes.
constexpr size_t GraphicsPipelineBudget(u64 total_physical_memory) {
    return static_cast<size_t>(total_physical_memory / 16 / GraphicsPipelineBytes);
}

/// How many graphics pipelines to evict, least recently used first.
/// @param cached Graphics pipelines in the cache.
/// @param stale  Pipelines among them unused for long enough to be evicted.
/// @param budget GraphicsPipelineBudget of the host.
/// @return Zero within the budget, else the excess, at most `stale`.
constexpr size_t GraphicsPipelinesToEvict(size_t cached, size_t stale, size_t budget) {
    if (cached <= budget) {
        return 0;
    }
    return std::min(stale, cached - budget);
}

/// Pipelines built from the disk cache at boot that no draw has used yet stay out of eviction up to
/// this many (~0.9 GB of driver memory at ~0.45 MB each). Evicting one throws its prebuild away, and
/// the game translates it again on the GPU thread when it reaches that content (Ocarina of Time:
/// 1174 of 1346 evicted on the title screen, 684 rebuilt in 10 s on entering the field).
inline constexpr size_t MaxUnusedPrebuiltPipelines = 2000;

/// Whether a stale pipeline stays as one of the unused prebuilt ones; `kept` counts those so far.
constexpr bool KeepsUnusedPrebuiltPipeline(bool was_used, size_t& kept) {
    if (was_used || kept >= MaxUnusedPrebuiltPipelines) {
        return false;
    }
    ++kept;
    return true;
}

/// Attribute mask of a native vertex program, two bits per attribute as in
/// FixedPipelineState::attribute_types.
/// @param input_generic Components of each generic attribute in the program header's input map.
constexpr u64 VertexAttributeMask(std::span<const std::array<bool, 4>> input_generic) {
    u64 mask = 0;
    for (size_t index = 0; index < input_generic.size(); ++index) {
        if (std::ranges::any_of(input_generic[index], [](bool component) { return component; })) {
            mask |= u64{3} << (index * 2);
        }
    }
    return mask;
}

/// Vertex attribute types a graphics pipeline key keeps.
/// @param attribute_types Types of every attribute register.
/// @param unique_hashes   Program hash of each stage, zero when the stage is off.
/// @param attribute_masks ShaderInfo::vertex_attribute_mask of each stage.
/// @return The types masked by every active stage, or all of them when VertexA is on.
constexpr u64 KeyedAttributeTypes(u64 attribute_types, std::span<const u64> unique_hashes,
                                  std::span<const u64> attribute_masks) {
    if (unique_hashes.empty() || unique_hashes[0] != 0) {
        return attribute_types;
    }
    for (size_t stage = 1; stage < unique_hashes.size(); ++stage) {
        if (unique_hashes[stage] != 0) {
            attribute_types &= attribute_masks[stage];
        }
    }
    return attribute_types;
}

/// Sessions a graphics pipeline stays in the used list after its last use (the file stores a u8).
/// The pipelines of the last few are built at boot (the boot window); the older ones are what the
/// shader preload builds in the background.
constexpr u8 UsedPipelinesKeptSessions = 254;

/// Host memory a graphics pipeline built ahead of use holds: building 17,162 cached Ocarina of
/// Time pipelines took 17.9 GB of resident memory on an RTX 3050 Laptop.
constexpr u64 PreloadedPipelineBytes = 1'050'000;

/// Memory the shader_preload_memory setting gives pipelines built ahead of use. Auto takes 1/16
/// of the RAM, from 512 MiB to 4 GiB.
constexpr u64 ShaderPreloadBytes(Settings::ShaderPreloadMemory mode, u64 host_ram) {
    constexpr u64 GiB = u64{1} << 30;
    switch (mode) {
    case Settings::ShaderPreloadMemory::Off:
        return 0;
    case Settings::ShaderPreloadMemory::Auto:
        return std::clamp(host_ram / 16, GiB / 2, 4 * GiB);
    case Settings::ShaderPreloadMemory::Gib1:
        return GiB;
    case Settings::ShaderPreloadMemory::Gib2:
        return 2 * GiB;
    case Settings::ShaderPreloadMemory::Gib4:
        return 4 * GiB;
    case Settings::ShaderPreloadMemory::Gib8:
        return 8 * GiB;
    }
    return 0;
}

/// The preload takes used pipelines whose sessions since their last use are at least
/// `boot_window` (younger ones are built at boot anyway) and under the returned limit: the
/// smallest limit that reaches `count` pipelines, so the most recently used come first. 256 when
/// the list holds fewer.
/// @param ages Sessions since each used pipeline's last use.
constexpr u32 PreloadAgeLimit(std::span<const u8> ages, u8 boot_window, size_t count) {
    std::array<size_t, 256> per_age{};
    for (const u8 age : ages) {
        ++per_age[age];
    }
    size_t taken = 0;
    for (u32 age = boot_window; age < per_age.size(); ++age) {
        taken += per_age[age];
        if (taken >= count) {
            return age + 1;
        }
    }
    return 256;
}

} // namespace VideoCommon
