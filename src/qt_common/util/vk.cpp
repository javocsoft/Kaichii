// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <utility>
#include <vector>

#include "qt_common/qt_common.h"

#include "common/dynamic_library.h"
#include "common/logging.h"
#include "qt_common/util/vk.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "video_core/vulkan_common/vulkan_surface.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"
#include "vulkan/vulkan_core.h"

class QWindow;

namespace VkDeviceInfo {
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
        record.integrated = SharesSystemMemory(properties.properties.deviceType);
        record.software = properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
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

} // namespace VkDeviceInfo
