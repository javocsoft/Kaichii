// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/hle/kernel/guest_profile.h"

namespace {
using Kernel::GuestProfile::Table;
constexpr u64 Ms = 1'000'000;

size_t Count(const std::string& text, const std::string& what) {
    size_t n = 0;
    for (size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + 1)) {
        ++n;
    }
    return n;
}
} // namespace

TEST_CASE("Guest profile ranks threads by their time outside SVCs", "[core][guest_profile]") {
    Table table;
    REQUIRE(table.Empty());
    table.AddOutside(7, 100 * Ms);
    table.AddSvc(7, 44, 0x18, "vsync", 400 * Ms);
    table.AddOutside(3, 900 * Ms);
    table.AddSvc(3, 28, 0x21, "nvdrv", 20 * Ms);
    table.AddSvc(3, 28, 0x21, "nvdrv", 30 * Ms);
    table.AddSvc(3, 28, 0x1A, "0x1000", 70 * Ms);
    table.AddOutside(9, 10 * Ms);

    // Over 10 s as rates: thread 3 first, its lock wait before its IPC.
    const std::string rates = table.Format(10.0, 2, 4, true);
    REQUIRE(rates.find("  t3 p28: outside SVCs 90.0 ms/s, in SVCs 12.0 ms/s (0/s)") == 0);
    REQUIRE(rates.find("; ArbitrateLock[0x1000] 7.0 ms/s") != std::string::npos);
    REQUIRE(rates.find(", SendSyncRequest[nvdrv] 5.0 ms/s") != std::string::npos);
    REQUIRE(rates.find("ArbitrateLock") < rates.find("SendSyncRequest"));
    REQUIRE(rates.find("t7 p44") != std::string::npos);
    REQUIRE(rates.find("t9") == std::string::npos); // only two threads asked for
    REQUIRE(Count(rates, "\n") == 2);

    // As totals, with one wait per thread.
    const std::string totals = table.Format(0.05, 1, 1, false);
    REQUIRE(totals == "  t3 p28: outside SVCs 900.0 ms, in SVCs 120.0 ms (3x); "
                      "ArbitrateLock[0x1000] 70.0 ms (1x)\n");

    table.Clear();
    REQUIRE(table.Empty());
    REQUIRE(table.Format(1.0, 8, 4, true).empty());
}

TEST_CASE("Guest profile names the SVCs it knows", "[core][guest_profile]") {
    using Kernel::GuestProfile::SvcName;
    REQUIRE(SvcName(0x18) == "WaitSynchronization");
    REQUIRE(SvcName(0x21) == "SendSyncRequest");
    REQUIRE(SvcName(0x34) == "WaitForAddress");
    REQUIRE(SvcName(0x7f) == "svc");
}
