// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <thread>
#include <utility>
#include <vector>

#include "qt_common/qt_common.h"

#include "common/dynamic_library.h"
#include "common/logging.h"
#include "common/memory_detect.h"
#include "common/thread.h"
#include "qt_common/util/vk.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "video_core/vulkan_common/vulkan_surface.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"
#include "vulkan/vulkan_core.h"

class QWindow;

namespace VkDeviceInfo {
Record::Record(std::string_view name_, const std::vector<VkPresentModeKHR>& vsync_modes_,
               bool has_broken_compute_)
    : name{name_}, vsync_support{vsync_modes_}, has_broken_compute{has_broken_compute_} {}

Record::~Record() = default;

void PopulateRecords(std::vector<Record>& records, QWindow* window) try {
    using namespace Vulkan;

    // Create a test window with a Vulkan surface type for checking present modes.
    QWindow test_window(window);
    test_window.setSurfaceType(QWindow::VulkanSurface);
    test_window.create();
    auto wsi = QtCommon::GetWindowSystemInfo(&test_window);

    vk::InstanceDispatch dld;
    const auto library = OpenLibrary();
    const vk::Instance instance = CreateInstance(*library, dld, VK_API_VERSION_1_1, wsi.type);
    const std::vector<VkPhysicalDevice> physical_devices = instance.EnumeratePhysicalDevices();
    // Without a surface (a platform that can't present, such as VNC) the devices are still
    // listed, with the default present modes: performance presets class the machine from them.
    vk::SurfaceKHR surface;
    try {
        surface = CreateSurface(instance, wsi);
    } catch (const Vulkan::vk::Exception& exception) {
        LOG_WARNING(Frontend, "Listing Vulkan devices without a surface: {}", exception.what());
    }

    records.clear();
    records.reserve(physical_devices.size());
    for (const VkPhysicalDevice device : physical_devices) {
        const auto physical_device = vk::PhysicalDevice(device, dld);
        std::string name = physical_device.GetProperties().deviceName;

        const std::vector<VkPresentModeKHR> present_modes =
            surface ? physical_device.GetSurfacePresentModesKHR(*surface) : default_present_modes;

        VkPhysicalDeviceDriverProperties driver_properties{};
        driver_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        driver_properties.pNext = nullptr;
        VkPhysicalDeviceProperties2 properties{};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        properties.pNext = &driver_properties;
        dld.vkGetPhysicalDeviceProperties2(physical_device, &properties);

        const auto driverID = driver_properties.driverID;

        bool has_broken_compute{
            Vulkan::Device::CheckBrokenCompute(driverID, properties.properties.driverVersion)};

        std::string driver_string = Vulkan::vk::GetDriverName(driver_properties);

        if (driver_string.empty())
            driver_string = "Unknown";

        name = fmt::format("{} ({})", name, driver_string);

        Record& record = records.emplace_back(name, present_modes, has_broken_compute);
        record.device_name = properties.properties.deviceName;
        record.vendor_id = properties.properties.vendorID;
        record.integrated =
            properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        const VkPhysicalDeviceMemoryProperties memory =
            physical_device.GetMemoryProperties().memoryProperties;
        for (u32 heap = 0; heap < memory.memoryHeapCount; ++heap) {
            if ((memory.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
                record.device_local_bytes =
                    (std::max)(record.device_local_bytes, u64{memory.memoryHeaps[heap].size});
            }
        }
    }
} catch (const Vulkan::vk::Exception& exception) {
    LOG_ERROR(Frontend, "Failed to enumerate devices with error: {}", exception.what());
}

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
        host.gpu_memory_bytes = record.device_local_bytes;
    }
    return host;
}

} // namespace VkDeviceInfo
