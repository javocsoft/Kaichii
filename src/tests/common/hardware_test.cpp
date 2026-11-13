// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "common/hardware_test.h"

TEST_CASE("The CPU workload is the same work on every machine", "[common][hardware_test]") {
    using Common::HardwareTest::CpuWorkload;
    // Same rounds, same checksum: the score compares the same work.
    static_assert(CpuWorkload(1000) == CpuWorkload(1000));
    REQUIRE(CpuWorkload(100000) == CpuWorkload(100000));
    REQUIRE(CpuWorkload(100000) != CpuWorkload(100001));
    REQUIRE(CpuWorkload(100000, 1) != CpuWorkload(100000, 2));
}

TEST_CASE("The CPU rate is measured over the time asked", "[common][hardware_test]") {
    const auto begin = std::chrono::steady_clock::now();
    const double rate = Common::HardwareTest::MeasureCpuRate(std::chrono::milliseconds{50});
    REQUIRE(std::chrono::steady_clock::now() - begin >= std::chrono::milliseconds{50});
    REQUIRE(rate > 0.0);
}
