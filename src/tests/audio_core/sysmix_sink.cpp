// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include "audio_core/sink/null_sink.h"
#include "common/settings.h"
#include "tests/audio_core/sysmix_test_context.h"

TEST_CASE("sysmix sink admission is bounded and preserves samples on rejection", "[sysmix_sink]") {
    SysmixTests::Context context;
    auto owned_stream = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 2, true);
    auto& stream = *owned_stream;
    stream.SetSystemVolume(1.0f / Settings::Volume());
    AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
    std::array<s16, 480> samples{};
    for (s16 value = 1; value <= 3; ++value) {
        samples.fill(value);
        REQUIRE(stream.TryAppendOutputBlock(block, samples));
    }
    REQUIRE(stream.GetQueueSize() == 3);
    samples.fill(99);
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, samples));
    REQUIRE(samples.front() == 99);
    std::array<s16, 480> output{};
    for (s16 value = 1; value <= 3; ++value) {
        stream.ProcessAudioOutAndRender(output, 240);
        REQUIRE(std::ranges::all_of(output, [value](s16 sample) { return sample == value; }));
    }
    REQUIRE(stream.GetQueueSize() == 0);
    stream.ProcessAudioOutAndRender(output, 240);
    REQUIRE(std::ranges::all_of(output, [](s16 sample) { return sample == 0; }));
    REQUIRE(stream.TryAppendOutputBlock(block, samples));
    stream.MarkFaulted();
    stream.ProcessAudioOutAndRender(output, 240);
    REQUIRE(std::ranges::all_of(output, [](s16 sample) { return sample == 0; }));
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, samples));
}

TEST_CASE("sysmix sink queues enough blocks for the host's largest callback", "[sysmix_sink]") {
    SysmixTests::Context context;
    auto owned_stream = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 2, true);
    auto& stream = *owned_stream;
    stream.SetSystemVolume(1.0f / Settings::Volume());
    REQUIRE(stream.SysmixQueueLimit() == 3);
    // PipeWire's default quantum: three 240-frame blocks left 304 frames of every callback silent.
    std::vector<s16> output(1024 * 2);
    stream.ProcessAudioOutAndRender(output, 1024);
    REQUIRE(stream.SysmixQueueLimit() == 6);
    AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
    std::array<s16, 480> samples{};
    samples.fill(7);
    for (int i = 0; i < 6; ++i) {
        REQUIRE(stream.TryAppendOutputBlock(block, samples));
    }
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, samples));
    stream.ProcessAudioOutAndRender(output, 1024);
    REQUIRE(std::ranges::all_of(output, [](s16 sample) { return sample == 7; }));
    // A smaller request later keeps the largest one's limit.
    stream.ProcessAudioOutAndRender(std::span{output}.first(480), 240);
    REQUIRE(stream.SysmixQueueLimit() == 6);

    // Six device channels: the sample ring holds 45 blocks, one of them playing.
    auto surround = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 6, true);
    std::vector<s16> large(48000 * 6);
    surround->ProcessAudioOutAndRender(large, 48000);
    REQUIRE(surround->SysmixQueueLimit() == 44);
}

TEST_CASE("sysmix sink rejects invalid blocks and unavailable backends", "[sysmix_sink]") {
    SysmixTests::Context context;
    auto owned_stream = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 2, false);
    auto& stream = *owned_stream;
    AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
    std::array<s16, 480> samples{};
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, samples));
    stream.ready = true;
    block.frames = 241;
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, samples));
    block.frames = 240;
    REQUIRE_FALSE(stream.TryAppendOutputBlock(block, std::span{samples}.first(479)));
    REQUIRE(stream.GetQueueSize() == 0);
    AudioCore::Sink::NullSink null{"auto"};
    auto* discard = null.AcquireSinkStream(context.system, 2, "discard", AudioCore::Sink::StreamType::Out);
    REQUIRE(discard->IsReady());
    for (u32 i = 0; i < 20; ++i) {
        REQUIRE(discard->TryAppendOutputBlock(block, samples));
    }
    REQUIRE(discard->GetQueueSize() == 0);
}

TEST_CASE("sysmix sink converts stereo to host channels without losing block alignment", "[sysmix_sink]") {
    SysmixTests::Context context;
    auto owned_stream = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 6, true);
    auto& stream = *owned_stream;
    stream.SetSystemVolume(1.0f / Settings::Volume());
    AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
    std::array<s16, 480> samples{};
    for (std::size_t i = 0; i < 240; ++i) {
        samples[i * 2] = 100;
        samples[i * 2 + 1] = -100;
    }
    REQUIRE(stream.TryAppendOutputBlock(block, samples));
    std::array<s16, 1440> output{};
    stream.ProcessAudioOutAndRender(output, 240);
    for (std::size_t i = 0; i < 240; ++i) {
        REQUIRE(output[i * 6] == 100);
        REQUIRE(output[i * 6 + 1] == -100);
        for (std::size_t channel = 2; channel < 6; ++channel) {
            REQUIRE(output[i * 6 + channel] == 0);
        }
    }
}

TEST_CASE("sysmix sink publishes samples before concurrently consumed metadata", "[sysmix_sink]") {
    SysmixTests::Context context;
    auto owned_stream = std::make_unique<SysmixTests::Stream>(
        context.system, AudioCore::Sink::StreamType::Sysmix, 2, true);
    auto& stream = *owned_stream;
    stream.SetSystemVolume(1.0f / Settings::Volume());
    constexpr u32 Blocks = 1000;
    std::atomic<bool> done{}, mismatch{};
    std::atomic<u32> consumed{};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    std::jthread consumer{[&] {
        std::array<s16, 480> output{};
        u32 expected = 1;
        while ((!done || stream.GetQueueSize() != 0) &&
               std::chrono::steady_clock::now() < deadline) {
            stream.ProcessAudioOutAndRender(output, 240);
            if (output.front() == 0) {
                if (!std::ranges::all_of(output, [](s16 value) { return value == 0; })) {
                    mismatch = true;
                }
                continue;
            }
            if (!std::ranges::all_of(output, [expected](s16 value) {
                    return static_cast<u32>(value) == expected;
                })) {
                mismatch = true;
            }
            ++expected;
            ++consumed;
        }
    }};
    AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
    for (u32 index = 1; index <= Blocks; ++index) {
        std::array<s16, 480> samples{};
        samples.fill(static_cast<s16>(index));
        bool accepted{};
        while (!(accepted = stream.TryAppendOutputBlock(block, samples)) &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        if (!accepted) {
            mismatch = true;
            break;
        }
    }
    done = true;
    consumer.join();
    REQUIRE_FALSE(mismatch.load());
    REQUIRE(consumed.load() == Blocks);
    REQUIRE(stream.GetQueueSize() == 0);
}

TEST_CASE("legacy output and renderer sinks preserve FIFO volume and repeated-frame underrun", "[sysmix_sink]") {
    SysmixTests::Context context;
    for (const auto type : {AudioCore::Sink::StreamType::Out, AudioCore::Sink::StreamType::Render}) {
        auto owned_stream = std::make_unique<SysmixTests::Stream>(context.system, type, 2, true);
        auto& stream = *owned_stream;
        stream.SetSystemVolume(0.5f);
        AudioCore::Sink::SinkBuffer block{240, 0, 0, false};
        std::array<s16, 480> samples{};
        samples.fill(2000);
        stream.AppendBuffer(block, samples);
        samples.fill(-4000);
        stream.AppendBuffer(block, samples);
        REQUIRE(stream.GetQueueSize() == 2);
        std::array<s16, 480> output{};
        stream.ProcessAudioOutAndRender(output, 240);
        REQUIRE(std::ranges::all_of(output, [](s16 value) { return value == 1000; }));
        stream.ProcessAudioOutAndRender(output, 240);
        REQUIRE(std::ranges::all_of(output, [](s16 value) { return value == -2000; }));
        REQUIRE(stream.GetQueueSize() == 0);
        stream.ProcessAudioOutAndRender(output, 240);
        REQUIRE(std::ranges::all_of(output, [](s16 value) { return value == -2000; }));
    }
}

TEST_CASE("The null sink paces the audio renderer at the output's sample rate", "[sysmix_sink]") {
    SysmixTests::Context context;
    AudioCore::Sink::NullSink null{"auto"};
    auto* stream =
        null.AcquireSinkStream(context.system, 2, "render", AudioCore::Sink::StreamType::Render);
    stream->Start();
    // Each block is 240 frames at 48 kHz, 5 ms: 21 waits span at least 20 blocks.
    const auto begin = std::chrono::steady_clock::now();
    for (u32 i = 0; i < 21; ++i) {
        stream->WaitFreeSpace({});
    }
    CHECK(std::chrono::steady_clock::now() - begin >= std::chrono::milliseconds{95});
    // A paused stream doesn't hold the renderer.
    stream->Stop();
    const auto paused = std::chrono::steady_clock::now();
    for (u32 i = 0; i < 100; ++i) {
        stream->WaitFreeSpace({});
    }
    CHECK(std::chrono::steady_clock::now() - paused < std::chrono::milliseconds{100});
}
