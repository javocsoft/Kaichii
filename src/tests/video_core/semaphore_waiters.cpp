// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/engines/puller.h"
#include "video_core/nx2.h"

using Tegra::Nx2SemaphoreWaiters;

TEST_CASE("A puller torn down while it waits leaves the semaphore waiters", "[video_core][nx2_waiters]") {
    // The waiters outlive an emulation session. A channel stopped mid-wait must not leave its
    // slot behind, latched or not, for a puller the next session makes at the same address.
    auto& waiters = Nx2SemaphoreWaiters::Get();
    const void* owner = nullptr;
    {
        Tegra::Engines::Puller puller;
        owner = &puller;
        waiters.Add(&puller, 0x1000, 5, Nx2SemaphoreWaiters::AcquireGequal);
        waiters.OnWrite(0x1000, 5);
        REQUIRE(waiters.Latched(&puller));
    }
    CHECK_FALSE(waiters.Latched(owner));
}
