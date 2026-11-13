// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/common_types.h"
#include "common/settings.h"

namespace Settings {

/// What the frontend knows about the machine: the Vulkan device the emulator renders with, and the
/// CPU and RAM. Unknown values stay zero and count as the weakest choice.
struct HostProfile {
    std::string gpu_name;
    u32 gpu_vendor_id = 0;    ///< PCI vendor id: 0x10DE NVIDIA, 0x1002 AMD, 0x8086 Intel
    bool gpu_integrated = false;
    u64 gpu_memory_bytes = 0; ///< The largest device-local heap
    u32 cpu_threads = 0;
    u64 ram_bytes = 0;
    /// Enough physical performance cores to pin the split GPU threads (Common::CanPinGpuThreads)
    bool gpu_thread_pinning = false;
    /// From the hardware test (Configure > Performance) on this GPU, when the player ran it:
    /// measured compute throughput as a share of the reference machine's (1.0 = the same), and
    /// the video memory free for the emulator then. Zero when not measured.
    double measured_gpu_ratio = 0.0;
    u64 measured_free_memory_bytes = 0;
};

enum class HardwareTier : u8 {
    Entry,      ///< Integrated GPUs and cards up to 4 GB: laptops and older desktops
    Mid,        ///< 6 to 8 GB
    High,       ///< 10 to 12 GB
    Enthusiast, ///< 16 GB and more
};

enum class PerformancePreset : u8 {
    Performance, ///< Half resolution: for Entry hardware in Switch 2 games
    Balanced,    ///< Three quarters resolution
    Quality,     ///< Native resolution
    Ultra,       ///< One and a half times native
};

inline constexpr std::array AllPerformancePresets{
    PerformancePreset::Performance, PerformancePreset::Balanced, PerformancePreset::Quality,
    PerformancePreset::Ultra};

/// The hardware's class. Laptop GPUs are capped at High: with the same memory they run well
/// behind their desktop namesakes. A CPU with fewer than 8 threads or less than 12 GB of RAM caps
/// it at Mid, since Switch 2 games then wait on the CPU or on paging, not on the GPU. With the
/// hardware test's figures, the class is also capped by the measured compute (ComputeTier), and by
/// the free video memory when other programs hold much of it.
[[nodiscard]] HardwareTier ClassifyHost(const HostProfile& host);

/// The class a GPU's measured compute supports, as a share of the reference machine's. Measured in
/// Ocarina of Time on the reference (an RTX 3050 Laptop): half resolution without the game's
/// sharpening runs at 30 fps (Performance), native with its anti-aliasing network at 14 fps. So
/// Quality needs about 2.1 times the reference, Ultra (2.25 times the pixels) about 4.7 times,
/// and Balanced (0.56 times the pixels of native without the network) about 0.6 times.
[[nodiscard]] HardwareTier ComputeTier(double gpu_ratio);

[[nodiscard]] PerformancePreset RecommendedPreset(const HostProfile& host);

/// One setting a preset sets, and the value it sets, as the setting serializes it.
struct PresetEntry {
    BasicSetting* setting;
    std::string value;
};

/// What a preset sets in `target`. A frontend can show these in its widgets before saving.
[[nodiscard]] std::vector<PresetEntry> PerformancePresetEntries(Values& target,
                                                                PerformancePreset preset,
                                                                const HostProfile& host);

/// A setting whose value differs from what a preset sets: both as Setting::ToString writes them.
struct PresetDifference {
    BasicSetting* setting;
    std::string current;
    std::string preset;
};

/// The settings in `target` that differ from `preset`, in the order the preset lists them. Empty
/// when `target` matches it.
[[nodiscard]] std::vector<PresetDifference> PresetDifferences(Values& target,
                                                             PerformancePreset preset,
                                                             const HostProfile& host);

/// Writes a preset's settings, as per-game values when `per_game`. Settings a preset does not
/// name keep their values, so a player's other choices survive.
void ApplyPerformancePreset(Values& target, PerformancePreset preset, const HostProfile& host,
                            bool per_game);

/// The preset whose settings `target` holds, if any.
[[nodiscard]] std::optional<PerformancePreset> MatchingPerformancePreset(Values& target,
                                                                         const HostProfile& host);

[[nodiscard]] std::string_view PerformancePresetKey(PerformancePreset preset);

} // namespace Settings
