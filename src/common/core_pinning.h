// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "common/common_types.h"

namespace Common {

/// One logical processor as the host reports it.
struct LogicalProcessor {
    /// What the host's affinity call takes (a CPU set id on Windows).
    u32 id;
    /// The physical core: equal for SMT siblings.
    u32 core;
    /// Higher is faster. Equal on every processor of a CPU that is not hybrid.
    u8 efficiency_class;
};

/// Physical cores of the fastest class needed before the GPU threads are pinned.
constexpr std::size_t MinCoresForGpuThreadPinning = 6;

/// Performance logical processors a hybrid CPU needs before its time-critical threads (six guest
/// cores, the GPU front and back, the Vulkan worker, timing and fencing) are kept on them with
/// Windows CPU sets. With fewer they crowd each other: an i5-1235U (2 P-cores, 4 logical
/// processors) ran Ocarina of Time at 9.3 fps that way and at 11.5 fps with Windows placing them.
constexpr std::size_t MinPerformanceProcessorsForHybridPlacement = 8;

/// Whether the time-critical threads are kept on the performance processors of a hybrid CPU.
[[nodiscard]] constexpr bool UsesHybridPlacement(std::size_t performance_processors,
                                                 std::size_t efficiency_processors) {
    return efficiency_processors != 0 &&
           performance_processors >= MinPerformanceProcessorsForHybridPlacement;
}

/// Logical processor ids for each pinned role. All three are empty when the host has fewer than
/// MinCoresForGpuThreadPinning performance cores.
struct GpuThreadPinning {
    std::size_t performance_cores{};
    std::vector<u32> front;
    std::vector<u32> back;
    std::vector<u32> guest;
};

/// The split GPU thread's front and back each get a whole physical performance core, so no SMT
/// sibling shares it, and the guest cores get the remaining performance cores. Core 0 takes most
/// interrupts, so the GPU threads get the next two.
[[nodiscard]] GpuThreadPinning PlanGpuThreadPinning(std::span<const LogicalProcessor> processors);

/// The logical processor ids of a Linux CPU list such as "0-3,8,10-11" (the format of
/// /sys/bus/event_source/devices/cpu_core/cpus), in order. Parsing stops at a malformed entry.
[[nodiscard]] std::vector<u32> ParseCpuList(std::string_view list);

} // namespace Common
