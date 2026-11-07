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
};

PresetValues ValuesOf(PerformancePreset preset) {
    switch (preset) {
    case PerformancePreset::Performance:
        // Measured in Ocarina of Time on an RTX 3050 Laptop: 14 fps at native resolution with the
        // game's own anti-aliasing network, 29 fps at half resolution without its sharpening.
        return {ResolutionSetup::Res1_2X, ScalingFilter::Fsr, AntiAliasing::None,
                OotAntiAliasing::NoSharpening};
    case PerformancePreset::Balanced:
        return {ResolutionSetup::Res3_4X, ScalingFilter::Fsr, AntiAliasing::None,
                OotAntiAliasing::NoSharpening};
    case PerformancePreset::Quality:
        return {ResolutionSetup::Res1X, ScalingFilter::Fsr3, AntiAliasing::None,
                OotAntiAliasing::Game};
    case PerformancePreset::Ultra:
        return {ResolutionSetup::Res3_2X, ScalingFilter::Fsr3, AntiAliasing::None,
                OotAntiAliasing::Game};
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

HardwareTier ClassifyHost(const HostProfile& host) {
    HardwareTier tier = HardwareTier::Entry;
    if (!host.gpu_integrated) {
        if (host.gpu_memory_bytes >= 15 * GiB) {
            tier = HardwareTier::Enthusiast;
        } else if (host.gpu_memory_bytes >= 10 * GiB) {
            tier = HardwareTier::High;
        } else if (host.gpu_memory_bytes >= 6 * GiB) {
            tier = HardwareTier::Mid;
        }
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

std::vector<PresetEntry> PerformancePresetEntries(Values& values, PerformancePreset preset,
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
        entry(values.resolution_setup, preset_values.resolution),
        entry(values.scaling_filter, preset_values.scaling_filter),
        entry(values.anti_aliasing, preset_values.anti_aliasing),
        entry(values.oot_anti_aliasing, preset_values.oot_anti_aliasing),
        // Every preset: what these settings change costs frames and buys nothing a player sees.
        entry(values.gpu_accuracy, GpuAccuracy::Low),
        entry(values.use_asynchronous_shaders, true),
        entry(values.vram_usage_mode, VramModeFor(host)),
        entry(values.accelerate_astc, AstcModeFor(host)),
    };
}

void ApplyPerformancePreset(Values& values, PerformancePreset preset, const HostProfile& host,
                            bool per_game) {
    for (const PresetEntry& entry : PerformancePresetEntries(values, preset, host)) {
        entry.setting->SetGlobal(!per_game);
        entry.setting->LoadString(entry.value);
    }
}

std::optional<PerformancePreset> MatchingPerformancePreset(Values& values,
                                                           const HostProfile& host) {
    for (const PerformancePreset preset : AllPerformancePresets) {
        const auto entries = PerformancePresetEntries(values, preset, host);
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
