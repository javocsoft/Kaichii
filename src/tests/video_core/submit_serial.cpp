// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/submit_serial.h"

TEST_CASE("A flush right after a submit has nothing to send", "[video_core][submit_serial]") {
    Vulkan::SubmitSerial serial;
    REQUIRE_FALSE(serial.RecordedSinceSubmit(false));
    serial.Recorded(); // a draw
    REQUIRE(serial.RecordedSinceSubmit(false));
    serial.Recorded(); // the submit's own command, recorded by SubmitExecution
    serial.Submitted();
    // SignalFence's FlushCommands after InnerFence::Queue submitted.
    REQUIRE_FALSE(serial.RecordedSinceSubmit(false));
    // A deferred clear is work although no command was recorded yet.
    REQUIRE(serial.RecordedSinceSubmit(true));
    serial.Recorded();
    REQUIRE(serial.RecordedSinceSubmit(false));
}
