// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Apart from vk.cpp, which enumerates the devices through Vulkan: what reads the records links
// without the renderer.

#include <algorithm>
#include <thread>

#include "common/memory_detect.h"
#include "common/settings.h"
#include "common/thread.h"
#include "qt_common/util/vk.h"

namespace VkDeviceInfo {

Record::Record(std::string_view name_, const std::vector<VkPresentModeKHR>& vsync_modes_,
               bool has_broken_compute_)
    : name{name_}, vsync_support{vsync_modes_}, has_broken_compute{has_broken_compute_} {}

Record::~Record() = default;

Settings::HostProfile DetectHostProfile(const std::vector<Record>& records, int device_index) {
    Settings::HostProfile host{
        .cpu_threads = std::thread::hardware_concurrency(),
        .ram_bytes = Common::GetMemInfo().TotalPhysicalMemory,
        .gpu_thread_pinning = Common::CanPinGpuThreads(),
    };
    if (device_index >= 0 && static_cast<size_t>(device_index) < records.size()) {
        const Record& record = records[static_cast<size_t>(device_index)];
        host.gpu_name = record.device_name;
        host.gpu_vendor_id = record.vendor_id;
        host.gpu_integrated = record.integrated;
        host.gpu_software = record.software;
        host.gpu_memory_bytes = record.device_local_bytes;
        if (record.integrated) {
            const auto dedicated = std::ranges::find_if(
                records, [](const Record& other) { return !other.integrated; });
            if (dedicated != records.end()) {
                host.other_dedicated_gpu_name = dedicated->device_name;
            }
        }
        // The hardware test's figures count only for the GPU they were measured on.
        if (!host.gpu_name.empty() &&
            Settings::values.hardware_test_gpu.GetValue() == host.gpu_name) {
            host.measured_gpu_ratio =
                Settings::values.hardware_test_gpu_permille.GetValue() / 1000.0;
            host.measured_free_memory_bytes =
                u64{Settings::values.hardware_test_free_memory_mib.GetValue()} << 20;
        }
    }
    return host;
}

} // namespace VkDeviceInfo
