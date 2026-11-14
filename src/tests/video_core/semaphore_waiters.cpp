// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "video_core/engines/puller.h"
#include "video_core/nx2.h"

using Tegra::Nx2SemaphoreWaiters;

TEST_CASE("A puller torn down while it waits leaves the semaphore waiters", "[video_core][nx2_waiters]") {
    // The waiters outlive an emulation session. A channel stopped mid-wait must not leave its
    // slot behind, latched or not, for a puller the next session makes at the same address.
    auto& waiters = Nx2SemaphoreWaiters::Get();
    // Kept as a number: the waiters compare owners by address only, and GCC rejects a pointer to
    // the puller once it is gone (-Werror=dangling-pointer).
    std::uintptr_t owner = 0;
    {
        Tegra::Engines::Puller puller;
        owner = reinterpret_cast<std::uintptr_t>(&puller);
        waiters.Add(&puller, 0x1000, 5, Nx2SemaphoreWaiters::AcquireGequal);
        waiters.OnWrite(0x1000, 5);
        REQUIRE(waiters.Latched(&puller));
    }
    CHECK_FALSE(waiters.Latched(reinterpret_cast<const void*>(owner)));
}
