// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "tests/core/system_test_context.h"
#include "video_core/control/channel_state.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/macro.h"
#include "video_core/memory_manager.h"
#include "video_core/nx2.h"

namespace Tegra {

struct DmaPusherTestAccess {
    static bool IsMemoryWriter(const Engines::EngineInterface& engine, Engines::EngineTypes type,
                               u32 method) {
        return DmaPusher::IsMemoryWriter(engine, type, method);
    }

    static bool Step(DmaPusher& pusher) {
        return pusher.Step();
    }

    static bool HasParkedMethods(const DmaPusher& pusher) {
        return !pusher.parked_methods.empty();
    }

    static u32 MethodCount(const DmaPusher& pusher) {
        return pusher.dma_state.method_count;
    }
};

} // namespace Tegra

TEST_CASE("Command writer classification excludes only plain compute and DMA state",
          "[video_core][command_writer]") {
    ScopedSystemTestContext context;
    Core::System system;
    Core::DeviceMemory physical;
    Tegra::MaxwellDeviceMemoryManager device_memory{physical};
    Tegra::MemoryManager memory{system, device_memory, 32, 1ULL << 31};
    Tegra::Engines::KeplerCompute compute{memory};
    Tegra::Engines::MaxwellDMA dma{memory};
    Tegra::MacroEngine macros{true};
    using Access = Tegra::DmaPusherTestAccess;
    using Type = Tegra::Engines::EngineTypes;
    for (const bool native : {false, true}) {
        CAPTURE(native);
        compute.SetNativeMacroMode(native ? &macros : nullptr, {});
        for (u32 method = 0; method < 0xe00; ++method) {
            CAPTURE(method);
            const bool writer = Access::IsMemoryWriter(compute, Type::KeplerCompute, method);
            if (!native) {
                REQUIRE(writer == compute.execution_mask[method]);
            } else {
                const bool traced = std::getenv("EDEN_NATIVE_DRAW_DIAGNOSTICS") != nullptr &&
                                    (method == 0x5a || (method >= 0xc6 && method <= 0x108));
                const bool effect = traced || method == 0x45 || method == 0x46 || method == 0x47 ||
                                    method == 0x48 || method == 0xae || method == 0xaf ||
                                    method == 0xb0 || method == 0x107 || method == 0x6c3 ||
                                    method == KEPLER_COMPUTE_REG_INDEX(exec_upload) ||
                                    method == KEPLER_COMPUTE_REG_INDEX(data_upload);
                REQUIRE(writer == effect);
            }
        }
    }
    for (u32 method = 0; method < 0xe00; ++method) {
        CAPTURE(method);
        REQUIRE(Access::IsMemoryWriter(dma, Type::MaxwellDMA, method) ==
                dma.execution_mask[method]);
    }
    for (const u32 method : {0xe00U, 0xfffU, 0xffffU, std::numeric_limits<u32>::max()}) {
        CAPTURE(method);
        REQUIRE(Access::IsMemoryWriter(compute, Type::KeplerCompute, method));
        REQUIRE(Access::IsMemoryWriter(dma, Type::MaxwellDMA, method));
    }
}

namespace {

class RecordingEngine final : public Tegra::Engines::EngineInterface {
public:
    std::vector<std::pair<u32, u32>> received;

    void CallMethod(Core::System&, u32 method, u32 value, bool) override {
        received.emplace_back(method, value);
    }
    void CallMultiMethod(Core::System& system, u32 method, const u32* words, u32 amount,
                         u32) override {
        for (u32 i = 0; i < amount; ++i) {
            CallMethod(system, method, words[i], true);
        }
    }
};

} // namespace

// Ocarina of Time hung when a six-word call arrived between the two halves of a split register
// write that still owed four words: the pusher took the call as the write's data and later waited
// on a bogus semaphore (zortos293/Lichii#2415, from Eglantine #924).
TEST_CASE("GPFIFO returns to a parked register write even when word counts match",
          "[video_core][dma_pusher]") {
    ScopedSystemTestContext context;
    Core::System system;
    Core::DeviceMemory physical;
    Tegra::MaxwellDeviceMemoryManager device_memory{physical};
    Tegra::MemoryManager memory{system, device_memory, 32, 1ULL << 31};
    Tegra::Control::ChannelState channel{0};
    Tegra::DmaPusher pusher{system, memory, channel};
    const bool ampere = Tegra::g_nx2_ampere.exchange(true);
    SCOPE_EXIT {
        Tegra::g_nx2_ampere.store(ampere);
    };
    RecordingEngine engine;
    engine.execution_mask.set();
    pusher.BindSubchannel(&engine, 0, Tegra::Engines::EngineTypes::Fermi2D);

    const auto write = [](u32 method, u32 count) {
        return Tegra::BuildCommandHeader(static_cast<Tegra::BufferMethods>(method), count,
                                         Tegra::SubmissionMode::Increasing);
    };
    const auto data = [](u32 value) { return Tegra::CommandHeader{.argument = value}; };
    Tegra::CommandList list{4};
    list.submitted_words = {write(0x4c7, 2), write(0x5c7, 4), data(21), data(22),
                            data(11),        data(12),        data(23), data(24)};
    const std::array<GPUVAddr, 4> addresses{0x1004, 0x2000, 0x1008, 0x200c};
    const std::array<u32, 5> offsets{0, 1, 4, 6, 8};
    for (size_t i = 0; i < addresses.size(); ++i) {
        list.command_lists[i].addr.Assign(addresses[i]);
        list.command_lists[i].size.Assign(offsets[i + 1] - offsets[i]);
        list.submitted_offsets.push_back(offsets[i]);
    }
    pusher.Push(std::move(list));

    for (size_t i = 0; i < addresses.size(); ++i) {
        REQUIRE(Tegra::DmaPusherTestAccess::Step(pusher));
    }
    CHECK(engine.received == std::vector<std::pair<u32, u32>>{
                                 {0x5c7, 21}, {0x5c8, 22}, {0x4c7, 11},
                                 {0x4c8, 12}, {0x5c9, 23}, {0x5ca, 24}});
    CHECK_FALSE(Tegra::DmaPusherTestAccess::HasParkedMethods(pusher));
    CHECK(Tegra::DmaPusherTestAccess::MethodCount(pusher) == 0);
}
