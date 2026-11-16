// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "core/device_memory_hint_range.h"

TEST_CASE("Cold memory hints stay inside the device address space", "[core][cold_hint]") {
    constexpr DAddr end = DAddr{1} << 36;
    CHECK(Core::ClipColdHintSize(0x1000, 0x2000, end) == 0x2000);
    CHECK(Core::ClipColdHintSize(end - 0x1000, 0x2000, end) == 0x1000);
    CHECK(Core::ClipColdHintSize(end - 0x1000, std::numeric_limits<size_t>::max(), end) ==
          0x1000);
    CHECK(Core::ClipColdHintSize(end, 0x1000, end) == 0);
    CHECK(Core::ClipColdHintSize(std::numeric_limits<DAddr>::max(), 0x1000, end) == 0);
    CHECK(Core::ClipColdHintSize(0x1000, 0, end) == 0);
}
