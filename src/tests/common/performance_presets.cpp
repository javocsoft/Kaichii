// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "common/performance_presets.h"

using namespace Settings;

namespace {

constexpr u64 GiB = u64{1} << 30;

HostProfile Host(std::string name, u64 gpu_gib, bool integrated = false, u32 threads = 16,
                 u64 ram_gib = 32) {
    return {.gpu_name = std::move(name),
            .gpu_vendor_id = 0x10DE,
            .gpu_integrated = integrated,
            .gpu_memory_bytes = gpu_gib * GiB,
            .cpu_threads = threads,
            .ram_bytes = ram_gib * GiB};
}

} // Anonymous namespace

TEST_CASE("Hosts are classed by GPU memory, laptops and CPU", "[common][performance_presets]") {
    // The machine Ocarina of Time was measured on.
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4, false, 20, 31)) ==
          HardwareTier::Entry);
    CHECK(ClassifyHost(Host("Intel(R) Iris(R) Xe Graphics", 8, true)) == HardwareTier::Entry);
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 4060", 8)) == HardwareTier::Mid);
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 3060", 12)) == HardwareTier::High);
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 4080", 16)) == HardwareTier::Enthusiast);
    // A 16 GB laptop card stays below a desktop one, and so does any card in a weak machine.
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 4090 Laptop GPU", 16)) == HardwareTier::High);
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 4080", 16, false, 6)) == HardwareTier::Mid);
    CHECK(ClassifyHost(Host("NVIDIA GeForce RTX 4080", 16, false, 16, 8)) == HardwareTier::Mid);
    // Nothing detected is the weakest case, not the strongest.
    CHECK(ClassifyHost(HostProfile{}) == HardwareTier::Entry);
}

TEST_CASE("Each tier recommends its own preset", "[common][performance_presets]") {
    CHECK(RecommendedPreset(Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4)) ==
          PerformancePreset::Performance);
    CHECK(RecommendedPreset(Host("AMD Radeon RX 7600", 8)) == PerformancePreset::Balanced);
    CHECK(RecommendedPreset(Host("AMD Radeon RX 6700 XT", 12)) == PerformancePreset::Quality);
    CHECK(RecommendedPreset(Host("NVIDIA GeForce RTX 5090", 32)) == PerformancePreset::Ultra);
}

TEST_CASE("A preset sets its values, per game or globally, and is recognised afterwards",
          "[common][performance_presets]") {
    const HostProfile laptop = Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4);
    for (const bool per_game : {false, true}) {
        Values values;
        values.anti_aliasing.SetValue(AntiAliasing::Smaa);
        values.gpu_accuracy.SetValue(GpuAccuracy::High);
        values.use_docked_mode.SetValue(ConsoleMode::Handheld);
        ApplyPerformancePreset(values, PerformancePreset::Performance, laptop, per_game);
        CHECK(values.resolution_setup.UsingGlobal() == !per_game);
        CHECK(values.resolution_setup.GetValue() == ResolutionSetup::Res1_2X);
        CHECK(values.anti_aliasing.GetValue() == AntiAliasing::None);
        CHECK(values.oot_anti_aliasing.GetValue() == OotAntiAliasing::NoSharpening);
        CHECK(values.gpu_accuracy.GetValue() == GpuAccuracy::Low);
        CHECK(values.use_asynchronous_shaders.GetValue());
        CHECK(values.vram_usage_mode.GetValue() == VramUsageMode::Conservative);
        CHECK(values.accelerate_astc.GetValue() == AstcDecodeMode::CpuAsynchronous);
        // What the preset does not name stays the player's.
        CHECK(values.use_docked_mode.GetValue() == ConsoleMode::Handheld);
        CHECK(MatchingPerformancePreset(values, laptop) == PerformancePreset::Performance);
    }
}

TEST_CASE("ASTC decodes on the CPU only where there are threads to spare",
          "[common][performance_presets]") {
    Values values;
    ApplyPerformancePreset(values, PerformancePreset::Performance,
                           Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4, false, 4), false);
    CHECK(values.accelerate_astc.GetValue() == AstcDecodeMode::Gpu);
    ApplyPerformancePreset(values, PerformancePreset::Performance,
                           Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4, false, 20), false);
    CHECK(values.accelerate_astc.GetValue() == AstcDecodeMode::CpuAsynchronous);
}

TEST_CASE("Presets pin the GPU threads only where the processor has the cores",
          "[common][performance_presets]") {
    Values values;
    HostProfile host = Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4, false, 20);
    for (const bool can_pin : {true, false, true}) {
        host.gpu_thread_pinning = can_pin;
        ApplyPerformancePreset(values, PerformancePreset::Performance, host, false);
        CHECK(values.pin_gpu_threads.GetValue() == can_pin);
        CHECK(MatchingPerformancePreset(values, host) == PerformancePreset::Performance);
    }
}

TEST_CASE("Preset entries are what the settings write and read back",
          "[common][performance_presets]") {
    const HostProfile laptop = Host("NVIDIA GeForce RTX 3050 Laptop GPU", 4);
    Values values;
    for (const PerformancePreset preset : AllPerformancePresets) {
        for (const PresetEntry& entry : PerformancePresetEntries(values, preset, laptop)) {
            entry.setting->LoadString(entry.value);
            CHECK(entry.setting->ToString() == entry.value);
        }
        CHECK(MatchingPerformancePreset(values, laptop) == preset);
    }
}

TEST_CASE("Changing a preset's setting by hand leaves no preset matched",
          "[common][performance_presets]") {
    const HostProfile desktop = Host("NVIDIA GeForce RTX 3060", 12);
    Values values;
    for (const PerformancePreset preset : AllPerformancePresets) {
        ApplyPerformancePreset(values, preset, desktop, false);
        CHECK(MatchingPerformancePreset(values, desktop) == preset);
    }
    values.resolution_setup.SetValue(ResolutionSetup::Res2X);
    CHECK_FALSE(MatchingPerformancePreset(values, desktop).has_value());
}
