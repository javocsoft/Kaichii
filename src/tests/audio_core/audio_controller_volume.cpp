// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <optional>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "common/scope_exit.h"
#include "common/settings.h"
#include "core/hle/service/audio/audio_controller.h"
#include "core/hle/service/server_manager.h"
#include "tests/audio_core/sysmix_test_context.h"

TEST_CASE("Guest audio targets do not overwrite the user's host volume", "[audio][audctl]") {
    SysmixTests::Context context;
    auto& kernel = context.system.Kernel();
    Kernel::KThread client{kernel};
    Kernel::KAutoObject::Create(&client);
    REQUIRE(Kernel::KThread::InitializeDummyThread(context.system, &client, &*context.process)
                .IsSuccess());
    SCOPE_EXIT {
        context.process->UnregisterThread(kernel, &client);
        context.process->Close(kernel);
        kernel.UnregisterKernelObject(&client);
    };
    const u8 previous_volume = Settings::values.volume.GetValue();
    const bool previous_mute = Settings::values.audio_muted.GetValue();
    SCOPE_EXIT {
        Settings::values.volume.SetValue(previous_volume);
        Settings::values.audio_muted.SetValue(previous_mute);
    };
    Settings::values.volume.SetValue(0);
    Settings::values.audio_muted.SetValue(false);
    std::optional<std::jthread> server_thread;
    Service::ServerManager server{context.system};
    server_thread.emplace(kernel.RunOnHostCoreProcess("audctl-volume-test", [&] { server.LoopProcess(); }));
    auto manager = std::make_shared<Service::SessionRequestManager>(kernel, server);
    Service::Audio::IAudioController service{context.system, nullptr};

    const auto send = [&](u32 command, u32 first, u32 second) {
        std::array<u32_le, IPC::COMMAND_BUFFER_LENGTH> words{};
        IPC::CommandHeader header{};
        header.type.Assign(IPC::CommandType::Request);
        header.data_size.Assign(12);
        std::memcpy(words.data(), &header, sizeof(header));
        words[4] = 0x49434653;
        words[6] = command;
        words[8] = first;
        words[9] = second;
        Service::HLERequestContext request{kernel, context.Memory(), nullptr, &client};
        request.SetSessionRequestManager(manager);
        REQUIRE(request.PopulateFromIncomingCommandBuffer(words.data()).IsSuccess());
        service.InvokeRequest(request);
        REQUIRE(request.CommandBuffer()[6] == ResultSuccess.raw);
        return static_cast<u32>(request.CommandBuffer()[8]);
    };
    constexpr u32 headphones = static_cast<u32>(Service::Set::AudioOutputModeTarget::Headphone);
    CHECK(send(0, headphones, 0) == 15);
    CHECK(Settings::values.volume.GetValue() == 0);
    send(1, headphones, 8);
    CHECK(send(0, headphones, 0) == 8);
    CHECK(Settings::values.volume.GetValue() == 0);
    send(5, 1, headphones);
    CHECK_FALSE(Settings::values.audio_muted.GetValue());
}
