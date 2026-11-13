// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"

namespace Vulkan {

/// What the hardware test measured on one Vulkan device.
struct GpuTestResult {
    std::string device_name;
    /// Single precision fused multiply-add throughput, in trillions of operations a second.
    double tflops = 0.0;
    /// The largest device-local heap, and what VK_EXT_memory_budget says this process may use of
    /// it right now (0 without the extension).
    u64 heap_bytes = 0;
    u64 budget_bytes = 0;
};

/// Runs the hardware test's compute workload on the Vulkan device at `device_index` (the order
/// vkEnumeratePhysicalDevices gives, as the vulkan_device setting counts) for about a second.
/// Needs no window and no game. Throws vk::Exception, or std::runtime_error with a reason.
[[nodiscard]] GpuTestResult RunGpuTest(size_t device_index);

} // namespace Vulkan
