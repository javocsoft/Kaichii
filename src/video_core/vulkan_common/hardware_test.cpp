// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "video_core/vulkan_common/hardware_test.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include "common/scope_exit.h"
#include "video_core/host_shaders/hardware_test_fma_comp_spv.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {
namespace {

constexpr u32 WorkgroupSize = 256;
constexpr u32 Workgroups = 4096;
constexpr u64 Invocations = u64{WorkgroupSize} * Workgroups;
/// Floating point operations per loop iteration and invocation (hardware_test_fma.comp).
constexpr u64 FlopsPerIteration = 256;
/// Each measured dispatch lasts about this long: well under any driver's watchdog.
constexpr double TargetSeconds = 0.1;
/// GPUs raise their clocks under load: dispatches run this long before any is measured.
constexpr double WarmUpSeconds = 0.6;
constexpr int MeasuredRuns = 5;

struct PushConstants {
    u32 iterations;
    float seed;
};

u32 FindMemoryType(const VkPhysicalDeviceMemoryProperties& properties, u32 type_bits) {
    // Device-local first: the result buffer is only written, never read back.
    for (const VkMemoryPropertyFlags wanted : {VkMemoryPropertyFlags{
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
                                               VkMemoryPropertyFlags{0}}) {
        for (u32 index = 0; index < properties.memoryTypeCount; ++index) {
            if ((type_bits & (1U << index)) != 0 &&
                (properties.memoryTypes[index].propertyFlags & wanted) == wanted) {
                return index;
            }
        }
    }
    throw std::runtime_error("no memory type for the test buffer");
}

} // Anonymous namespace

GpuTestResult RunGpuTest(size_t device_index) {
    vk::DeviceDispatch dld;
    const auto library = OpenLibrary();
    const vk::Instance instance = CreateInstance(*library, dld, VK_API_VERSION_1_1);
    const std::vector<VkPhysicalDevice> devices = instance.EnumeratePhysicalDevices();
    if (device_index >= devices.size()) {
        throw std::runtime_error(fmt::format("no Vulkan device {}", device_index));
    }
    const vk::PhysicalDevice physical(devices[device_index], dld);
    const VkPhysicalDeviceProperties properties = physical.GetProperties();

    GpuTestResult result;
    result.device_name = properties.deviceName;

    // The memory this process may use now: other programs' use comes off the budget.
    const auto extensions = physical.EnumerateDeviceExtensionProperties();
    const bool has_budget = std::ranges::any_of(extensions, [](const VkExtensionProperties& ext) {
        return std::strcmp(ext.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0;
    });
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{};
    budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    const VkPhysicalDeviceMemoryProperties memory =
        physical.GetMemoryProperties(has_budget ? &budget : nullptr).memoryProperties;
    for (u32 heap = 0; heap < memory.memoryHeapCount; ++heap) {
        const VkMemoryHeap& entry = memory.memoryHeaps[heap];
        if ((entry.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 &&
            entry.size > result.heap_bytes) {
            result.heap_bytes = entry.size;
            result.budget_bytes = has_budget ? budget.heapBudget[heap] : 0;
        }
    }

    const std::vector<VkQueueFamilyProperties> families = physical.GetQueueFamilyProperties();
    u32 family = static_cast<u32>(families.size());
    for (u32 index = 0; index < families.size(); ++index) {
        if ((families[index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
            families[index].timestampValidBits != 0) {
            family = index;
            break;
        }
    }
    if (family == families.size() || properties.limits.timestampPeriod <= 0.0f) {
        throw std::runtime_error("the device cannot time compute work");
    }
    const u32 valid_bits = families[family].timestampValidBits;
    const u64 timestamp_mask = valid_bits >= 64 ? ~u64{0} : (u64{1} << valid_bits) - 1;

    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_ci{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    const vk::Device device = vk::Device::Create(devices[device_index], queue_ci, {}, nullptr, dld);
    const vk::Queue queue = device.GetQueue(family);

    const VkBufferCreateInfo buffer_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = Invocations * sizeof(float),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    VkBuffer buffer{};
    vk::Check(dld.vkCreateBuffer(*device, &buffer_ci, nullptr, &buffer));
    SCOPE_EXIT {
        dld.vkDestroyBuffer(*device, buffer, nullptr);
    };
    const VkMemoryRequirements requirements = device.GetBufferMemoryRequirements(buffer);
    const vk::DeviceMemory buffer_memory = device.AllocateMemory(VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = FindMemoryType(memory, requirements.memoryTypeBits),
    });
    vk::Check(dld.vkBindBufferMemory(*device, buffer, *buffer_memory, 0));

    const VkDescriptorSetLayoutBinding binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr,
    };
    const vk::DescriptorSetLayout set_layout =
        device.CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = 1,
            .pBindings = &binding,
        });
    const VkPushConstantRange push_range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    const VkDescriptorSetLayout set_layout_handle = *set_layout;
    const vk::PipelineLayout layout = device.CreatePipelineLayout(VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &set_layout_handle,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range,
    });
    const vk::ShaderModule shader = device.CreateShaderModule(VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = sizeof(HARDWARE_TEST_FMA_COMP_SPV),
        .pCode = HARDWARE_TEST_FMA_COMP_SPV,
    });
    const vk::Pipeline pipeline = device.CreateComputePipeline(VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage =
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = *shader,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            },
        .layout = *layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    });

    const VkDescriptorPoolSize pool_size{
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
    };
    const vk::DescriptorPool descriptor_pool =
        device.CreateDescriptorPool(VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = 1,
            .poolSizeCount = 1,
            .pPoolSizes = &pool_size,
        });
    const vk::DescriptorSets sets = descriptor_pool.Allocate(VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &set_layout_handle,
    });
    const VkDescriptorSet set = sets[0];
    const VkDescriptorBufferInfo buffer_info{
        .buffer = buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    const VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr,
    };
    dld.vkUpdateDescriptorSets(*device, 1, &write, 0, nullptr);

    const vk::QueryPool query_pool = device.CreateQueryPool(VkQueryPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = 2,
        .pipelineStatistics = 0,
    });
    const vk::CommandPool command_pool = device.CreateCommandPool(VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = family,
    });
    const vk::CommandBuffers command_buffers = command_pool.Allocate(1);
    const vk::CommandBuffer cmdbuf(command_buffers[0], dld);
    const vk::Fence fence = device.CreateFence(VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    });

    // Seconds the GPU took for one dispatch of `iterations` loops per invocation.
    const auto run = [&](u32 iterations) {
        cmdbuf.Begin(VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        });
        cmdbuf.ResetQueryPool(*query_pool, 0, 2);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *layout, 0, set, {});
        cmdbuf.PushConstants(*layout, VK_SHADER_STAGE_COMPUTE_BIT,
                             PushConstants{.iterations = iterations, .seed = 0.5f});
        cmdbuf.WriteTimestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, *query_pool, 0);
        cmdbuf.Dispatch(Workgroups, 1, 1);
        cmdbuf.WriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, *query_pool, 1);
        cmdbuf.End();
        const VkSubmitInfo submit{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = cmdbuf.address(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        vk::Check(queue.Submit(submit, *fence));
        vk::Check(fence.Wait());
        fence.Reset();
        std::array<u64, 2> stamps{};
        vk::Check(dld.vkGetQueryPoolResults(*device, *query_pool, 0, 2, sizeof(stamps),
                                            stamps.data(), sizeof(u64),
                                            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
        const u64 ticks = ((stamps[1] & timestamp_mask) - (stamps[0] & timestamp_mask)) &
                          timestamp_mask;
        return static_cast<double>(ticks) * properties.limits.timestampPeriod * 1e-9;
    };

    u32 iterations = 64;
    const auto warm_up_end =
        std::chrono::steady_clock::now() + std::chrono::duration<double>(WarmUpSeconds);
    while (std::chrono::steady_clock::now() < warm_up_end) {
        const double seconds = run(iterations);
        if (seconds > 0.0) {
            iterations = static_cast<u32>(std::clamp(
                static_cast<double>(iterations) * TargetSeconds / seconds, 64.0, 1048576.0));
        }
    }
    for (int index = 0; index < MeasuredRuns; ++index) {
        const double seconds = run(iterations);
        if (seconds <= 0.0) {
            continue;
        }
        const double flops =
            static_cast<double>(Invocations) * iterations * FlopsPerIteration / seconds;
        result.tflops = (std::max)(result.tflops, flops * 1e-12);
    }
    if (result.tflops == 0.0) {
        throw std::runtime_error("the device's timestamps did not advance");
    }
    return result;
}

} // namespace Vulkan
