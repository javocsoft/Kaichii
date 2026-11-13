// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cctype>
#include <string>
#include <type_traits>

#include "common/performance_presets.h"

namespace Settings {
namespace {

constexpr u64 GiB = u64{1} << 30;

bool ContainsInsensitive(std::string_view text, std::string_view word) {
    return std::ranges::search(text, word, [](char a, char b) {
               return std::tolower(static_cast<unsigned char>(a)) ==
                      std::tolower(static_cast<unsigned char>(b));
           }).begin() != text.end();
}

bool IsLaptopGpu(std::string_view name) {
    return ContainsInsensitive(name, "laptop") || ContainsInsensitive(name, "mobile") ||
           ContainsInsensitive(name, "max-q");
}

/// What a preset sets. Everything else is the player's.
struct PresetValues {
    ResolutionSetup resolution;
    ScalingFilter scaling_filter;
    AntiAliasing anti_aliasing;
    OotAntiAliasing oot_anti_aliasing;
    NativeAsyncDraws native_async_draws;
};

PresetValues ValuesOf(PerformancePreset preset) {
    switch (preset) {
    case PerformancePreset::Performance:
        // Measured in Ocarina of Time on an RTX 3050 Laptop: 14 fps at native resolution with the
        // game's own anti-aliasing network, 29 fps at half resolution without its sharpening.
        // A Switch 2 draw whose new pipeline is still building is skipped instead of stalling
        // (its pipeline is translated on a worker meanwhile): walking in Kokiri Forest from the
        // shader pack lost 2.2 s to slow frames instead of 3.2 s, over three rounds.
        return {ResolutionSetup::Res1_2X, ScalingFilter::Fsr, AntiAliasing::None,
                OotAntiAliasing::NoSharpening, NativeAsyncDraws::Bounded};
    case PerformancePreset::Balanced:
        return {ResolutionSetup::Res3_4X, ScalingFilter::Fsr, AntiAliasing::None,
                OotAntiAliasing::NoSharpening, NativeAsyncDraws::Wait};
    case PerformancePreset::Quality:
        return {ResolutionSetup::Res1X, ScalingFilter::Fsr3, AntiAliasing::None,
                OotAntiAliasing::Game, NativeAsyncDraws::Wait};
    case PerformancePreset::Ultra:
        return {ResolutionSetup::Res3_2X, ScalingFilter::Fsr3, AntiAliasing::None,
                OotAntiAliasing::Game, NativeAsyncDraws::Wait};
    }
    return ValuesOf(PerformancePreset::Quality);
}

/// Cards with little memory collect textures early instead of failing allocations.
VramUsageMode VramModeFor(const HostProfile& host) {
    return host.gpu_memory_bytes > 6 * GiB ? VramUsageMode::Aggressive
                                           : VramUsageMode::Conservative;
}

/// Where ASTC textures are decoded when the GPU can't sample them. On the GPU the decode competes
/// with the game: in Ocarina of Time on an RTX 3050 Laptop it took 19 of every 35 ms at half
/// resolution, and decoding on the CPU in the background took it from 29 to 40 fps.
AstcDecodeMode AstcModeFor(const HostProfile& host) {
    return host.cpu_threads >= 8 ? AstcDecodeMode::CpuAsynchronous : AstcDecodeMode::Gpu;
}

} // Anonymous namespace

HardwareTier ComputeTier(double gpu_ratio) {
    if (gpu_ratio >= 4.7) {
        return HardwareTier::Enthusiast;
    }
    if (gpu_ratio >= 2.1) {
        return HardwareTier::High;
    }
    if (gpu_ratio >= 0.6) {
        return HardwareTier::Mid;
    }
    return HardwareTier::Entry;
}

HardwareTier ClassifyHost(const HostProfile& host) {
    // Other programs holding a quarter of the video memory or more: class by what is left.
    u64 memory = host.gpu_memory_bytes;
    if (host.measured_free_memory_bytes != 0 &&
        host.measured_free_memory_bytes < memory / 4 * 3) {
        memory = host.measured_free_memory_bytes;
    }
    HardwareTier tier = HardwareTier::Entry;
    if (!host.gpu_integrated) {
        if (memory >= 15 * GiB) {
            tier = HardwareTier::Enthusiast;
        } else if (memory >= 10 * GiB) {
            tier = HardwareTier::High;
        } else if (memory >= 6 * GiB) {
            tier = HardwareTier::Mid;
        }
    }
    if (host.measured_gpu_ratio > 0.0) {
        tier = (std::min)(tier, ComputeTier(host.measured_gpu_ratio));
    }
    if (IsLaptopGpu(host.gpu_name)) {
        tier = (std::min)(tier, HardwareTier::High);
    }
    if (host.cpu_threads < 8 || host.ram_bytes < 12 * GiB) {
        tier = (std::min)(tier, HardwareTier::Mid);
    }
    return tier;
}

PerformancePreset RecommendedPreset(const HostProfile& host) {
    switch (ClassifyHost(host)) {
    case HardwareTier::Entry:
        return PerformancePreset::Performance;
    case HardwareTier::Mid:
        return PerformancePreset::Balanced;
    case HardwareTier::High:
        return PerformancePreset::Quality;
    case HardwareTier::Enthusiast:
        return PerformancePreset::Ultra;
    }
    return PerformancePreset::Balanced;
}

std::vector<PresetEntry> PerformancePresetEntries(Values& target, PerformancePreset preset,
                                                  const HostProfile& host) {
    const PresetValues preset_values = ValuesOf(preset);
    // As Setting::ToString writes them, which LoadString reads back.
    const auto entry = [](auto& setting, auto value) {
        if constexpr (std::is_same_v<decltype(value), bool>) {
            return PresetEntry{&setting, value ? "true" : "false"};
        } else {
            using Underlying = std::underlying_type_t<decltype(value)>;
            return PresetEntry{&setting, std::to_string(Underlying(value))};
        }
    };
    return {
        entry(target.resolution_setup, preset_values.resolution),
        entry(target.scaling_filter, preset_values.scaling_filter),
        entry(target.anti_aliasing, preset_values.anti_aliasing),
        entry(target.oot_anti_aliasing, preset_values.oot_anti_aliasing),
        entry(target.native_async_draws, preset_values.native_async_draws),
        // Every preset: what these settings change costs frames and buys nothing a player sees.
        entry(target.gpu_accuracy, GpuAccuracy::Low),
        entry(target.use_asynchronous_shaders, true),
        entry(target.vram_usage_mode, VramModeFor(host)),
        entry(target.accelerate_astc, AstcModeFor(host)),
        // Ocarina of Time on an i7-12700H: 24.4-25.3 to 26.1-26.9 fps outside, 34.8 to 36.8 inside.
        entry(target.pin_gpu_threads, host.gpu_thread_pinning),
    };
}

void ApplyPerformancePreset(Values& target, PerformancePreset preset, const HostProfile& host,
                            bool per_game) {
    for (const PresetEntry& entry : PerformancePresetEntries(target, preset, host)) {
        entry.setting->SetGlobal(!per_game);
        entry.setting->LoadString(entry.value);
    }
}

std::vector<PresetDifference> PresetDifferences(Values& target, PerformancePreset preset,
                                               const HostProfile& host) {
    std::vector<PresetDifference> differences;
    for (const PresetEntry& entry : PerformancePresetEntries(target, preset, host)) {
        if (std::string current = entry.setting->ToString(); current != entry.value) {
            differences.push_back({entry.setting, std::move(current), entry.value});
        }
    }
    return differences;
}

std::optional<PerformancePreset> MatchingPerformancePreset(Values& target,
                                                           const HostProfile& host) {
    for (const PerformancePreset preset : AllPerformancePresets) {
        const auto entries = PerformancePresetEntries(target, preset, host);
        if (std::ranges::all_of(entries, [](const PresetEntry& entry) {
                return entry.setting->ToString() == entry.value;
            })) {
            return preset;
        }
    }
    return std::nullopt;
}

std::string_view PerformancePresetKey(PerformancePreset preset) {
    switch (preset) {
    case PerformancePreset::Performance:
        return "performance";
    case PerformancePreset::Balanced:
        return "balanced";
    case PerformancePreset::Quality:
        return "quality";
    case PerformancePreset::Ultra:
        return "ultra";
    }
    return "balanced";
}

} // namespace Settings
