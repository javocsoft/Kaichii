// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "core/hle/service/nvdrv/devices/nx2_stub_device.h"

TEST_CASE("NX2 presentation resolves parent handles and image offsets", "[video_core][nx2_mapping]") {
    Service::Nvidia::Devices::Nx2GpuMemoryRegistry registry;
    registry.MapAt(7, 0x1000, 0x8000, 0x4000, 0x3000);
    registry.RecordDeviceRange(0x4000, 0x3000, 0x10000);
    REQUIRE(registry.GpuAddressOf(7, 0x1800) == 0x8800U);
    REQUIRE(registry.DeviceAddressOfGpu(0x8800) == 0x10800U);
    REQUIRE_FALSE(registry.GpuAddressOf(7, 0xfff));
    REQUIRE_FALSE(registry.GpuAddressOf(7, 0x4000));
    REQUIRE_FALSE(registry.GpuAddressOf(8, 0x1800));
    REQUIRE_FALSE(registry.DeviceAddressOfGpu(0xb000));
}

TEST_CASE("NX2 partial unmaps preserve remaining parent offsets", "[video_core][nx2_mapping]") {
    Service::Nvidia::Devices::Nx2GpuMemoryRegistry registry;
    registry.MapAt(7, 0x1000, 0x8000, 0x4000, 0x3000);
    registry.RecordDeviceRange(0x4000, 0x3000, 0x10000);
    registry.Unmap(0x9000, 0x1000);
    REQUIRE(registry.GpuAddressOf(7, 0x1000) == 0x8000U);
    REQUIRE_FALSE(registry.GpuAddressOf(7, 0x2000));
    REQUIRE_FALSE(registry.DeviceAddressOfGpu(0x9000));
    REQUIRE(registry.GpuAddressOf(7, 0x3000) == 0xa000U);
    REQUIRE(registry.DeviceAddressOfGpu(0xa000) == 0x12000U);
}

TEST_CASE("NX2 remaps and resets discard stale parent handles", "[video_core][nx2_mapping]") {
    Service::Nvidia::Devices::Nx2GpuMemoryRegistry registry;
    registry.MapAt(7, 0, 0x8000, 0x4000, 0x1000);
    registry.MapAt(8, 0x1000, 0x8000, 0x6000, 0x1000);
    registry.RecordDeviceRange(0x6000, 0x1000, 0x20000);
    REQUIRE_FALSE(registry.GpuAddressOf(7, 0));
    REQUIRE(registry.GpuAddressOf(8, 0x1000) == 0x8000U);
    REQUIRE(registry.DeviceAddressOfGpu(0x8000) == 0x20000U);
    registry.Reset();
    REQUIRE_FALSE(registry.GpuAddressOf(8, 0x1000));
    REQUIRE_FALSE(registry.DeviceAddressOfGpu(0x8000));
}

namespace {
using Service::Nvidia::Devices::Nx2GpuMemoryRegistry;

/// Runs `call` on another thread, as the GPU thread reaching the registry while a guest thread
/// waits for it, and reports whether it finished within two seconds. On a timeout the thread is
/// left blocked and `registry` is leaked, so the test fails instead of hanging.
bool OtherThreadReaches(Nx2GpuMemoryRegistry*& registry, void (*call)(Nx2GpuMemoryRegistry&)) {
    auto done = std::make_shared<std::promise<void>>();
    auto finished = done->get_future();
    std::thread([registry, call, done] {
        call(*registry);
        done->set_value();
    }).detach();
    if (finished.wait_for(std::chrono::seconds{2}) == std::future_status::ready) {
        return true;
    }
    registry = nullptr; // leaked: the blocked thread still uses it
    return false;
}

void BackSparse(Nx2GpuMemoryRegistry& registry) {
    registry.BackSparse(0x8000, 0x4000, 0x1000);
}
} // Anonymous namespace

TEST_CASE("NX2 registry listeners may wait for a thread that reaches the registry",
          "[video_core][nx2_mapping]") {
    // Zortos 1218: a guest thread mapping memory waits in the listener for the GPU thread, whose
    // back backs a sparse block through the registry. With the listener called under the registry
    // mutex, that froze Ocarina of Time on its warps.
    auto* registry = new Nx2GpuMemoryRegistry;
    bool reached = false;
    int backs = 0;
    registry->AddListener(
        &backs,
        [&](u64, u64, u64) { reached = OtherThreadReaches(registry, &BackSparse); },
        [](u64, u64) {}, {}, {}, [&](u64, u64, u64) { ++backs; });
    registry->MapAt(0x8000, 0x4000, 0x1000);
    REQUIRE(reached);
    REQUIRE(backs == 1);

    // A new address space replays the existing mappings the same way.
    reached = false;
    int other = 0;
    registry->AddListener(
        &other, [&](u64, u64, u64) { reached = OtherThreadReaches(registry, &BackSparse); },
        [](u64, u64) {});
    REQUIRE(reached);
    registry->RemoveListener(&other);
    registry->RemoveListener(&backs);
    delete registry;
}

TEST_CASE("NX2 registry RemoveListener waits for a call in flight", "[video_core][nx2_mapping]") {
    Nx2GpuMemoryRegistry registry;
    std::promise<void> entered;
    std::promise<void> release;
    auto released = release.get_future().share();
    std::atomic<bool> call_done{false};
    int owner = 0;
    registry.AddListener(
        &owner,
        [&](u64, u64, u64) {
            entered.set_value();
            released.wait();
            call_done = true;
        },
        [](u64, u64) {});
    std::thread mapper([&] { registry.MapAt(0x8000, 0x4000, 0x1000); });
    entered.get_future().wait();
    auto removed = std::async(std::launch::async, [&] { registry.RemoveListener(&owner); });
    // The owner's destructor calls RemoveListener: it must not return while its callback runs.
    REQUIRE(removed.wait_for(std::chrono::milliseconds{100}) == std::future_status::timeout);
    release.set_value();
    removed.wait();
    REQUIRE(call_done);
    mapper.join();
}
