// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/native_qmd.h"

namespace {

using Tegra::NativeQmdScheduler;
using Descriptor = NativeQmdScheduler::Descriptor;
using Address = NativeQmdScheduler::Address;
using Word = NativeQmdScheduler::Word;

constexpr Word Schedule = 3;
constexpr Word ScheduleOnce = 2;
constexpr Word Decrement = 7;
constexpr Word Increment = 12;
constexpr Word EnableRelease = 1U << 23;
constexpr Word ReleaseFormat = 1U << 30;
constexpr Word ReductionRelease = 1U << 26;
constexpr std::string_view NoError{};

/// Dependency counts are per QMD and shared by every scheduler, so each test takes its own.
Address NextAddress() {
    static Address next = 0x1000'0000;
    return next += 0x100;
}

Word WithCount(Word action, Word count) {
    return action | ((count - 1) << 14);
}

Descriptor MakeQmd(Word dependencies = 0) {
    Descriptor qmd{};
    qmd[18] = 0x30;
    qmd[4] = dependencies << 14;
    return qmd;
}

void AddRelease(Descriptor& qmd, Word slot, Address destination, Word payload) {
    qmd[slot] = static_cast<Word>(destination);
    qmd[slot + 1] = EnableRelease | ReleaseFormat | static_cast<Word>(destination >> 32);
    qmd[slot + 2] = payload;
}

void SetDependentQmd(Descriptor& qmd, Address next, Word action) {
    qmd[15] = static_cast<Word>(next >> 8);
    qmd[16] = 1 | (action << 1);
}

/// Guest memory holding QMDs, and the releases that reached it.
struct Guest {
    std::map<Address, Descriptor> qmds;
    std::vector<std::pair<Address, Word>> releases;

    auto Read() {
        return [this](Address address, Descriptor& out) {
            const auto it = qmds.find(address);
            if (it == qmds.end()) {
                return false;
            }
            out = it->second;
            return true;
        };
    }

    auto Release() {
        return [this](Address address, Word payload) {
            releases.emplace_back(address, payload);
            return true;
        };
    }
};

} // namespace

TEST_CASE("QMD without dependencies runs when scheduled", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    AddRelease(qmd, 24, 0x12'3456'7890, 0xAB);
    AddRelease(qmd, 28, 0x20, 0xCD);
    AddRelease(qmd, 52, 0x30, 0xEF);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases ==
          std::vector<std::pair<Address, Word>>{{0x12'3456'7890, 0xAB}, {0x20, 0xCD}, {0x30, 0xEF}});
}

TEST_CASE("QMD waits for every dependency", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd(3);
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    REQUIRE(scheduler.Signal(address, Decrement, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.empty());
    // The count field says how many at once.
    REQUIRE(scheduler.Signal(address, WithCount(Decrement, 2), guest.Read(), guest.Release()) ==
            NoError);
    CHECK(guest.releases.size() == 1);
}

TEST_CASE("QMD dependency count can rise again", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd(1);
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    REQUIRE(scheduler.Signal(address, Increment, guest.Read(), guest.Release()) == NoError);
    REQUIRE(scheduler.Signal(address, Decrement, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.empty());
    REQUIRE(scheduler.Signal(address, Decrement, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.size() == 1);
}

TEST_CASE("QMD decrement from another channel may arrive before the schedule",
          "[video_core][native_qmd]") {
    // Mario Kart World does this. Keeping the count per channel lost the decrement, and the QMD
    // never launched.
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd(1);
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler other_channel;
    NativeQmdScheduler scheduling_channel;
    REQUIRE(other_channel.Signal(address, Decrement, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.empty());
    REQUIRE(scheduling_channel.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.size() == 1);
}

TEST_CASE("QMD runs once until it is scheduled afresh", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    REQUIRE(scheduler.Signal(address, ScheduleOnce, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.size() == 1);
    REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.size() == 2);
}

TEST_CASE("QMD release payload is read when the QMD runs", "[video_core][native_qmd]") {
    // The SDK patches the payload while the QMD waits for its dependencies.
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd(1);
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
    guest.qmds[address][26] = 2;
    REQUIRE(scheduler.Signal(address, Decrement, guest.Read(), guest.Release()) == NoError);
    REQUIRE(guest.releases.size() == 1);
    CHECK(guest.releases[0].second == 2);
}

TEST_CASE("QMD that fails to dispatch still completes", "[video_core][native_qmd]") {
    // Other queues and the CPU wait on its releases, and the console never fails a launch.
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    qmd[12] = 1;
    qmd[13] = 1;
    qmd[14] = 1;
    AddRelease(qmd, 24, 0x10, 1);
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    int dispatches{};
    const auto error = scheduler.Signal(address, Schedule, guest.Read(), guest.Release(),
                                        [&](const Descriptor&) {
                                            ++dispatches;
                                            return false;
                                        });
    CHECK(error == "native shader dispatch required");
    CHECK(dispatches == 1);
    CHECK(guest.releases.size() == 1);
}

TEST_CASE("QMD with an empty grid needs no dispatch", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    qmd[12] = 1;
    qmd[13] = 1;
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    bool dispatched{};
    CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release(),
                           [&](const Descriptor&) {
                               dispatched = true;
                               return true;
                           }) == NoError);
    CHECK_FALSE(dispatched);
}

TEST_CASE("QMD with work needs a dispatch callback", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    qmd[12] = 1;
    qmd[13] = 1;
    qmd[14] = 1;
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
          "native shader dispatch required");
}

TEST_CASE("One bad QMD release makes no release visible", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    AddRelease(qmd, 24, 0x10, 1);
    AddRelease(qmd, 28, 0x20, 2);
    qmd[29] &= ~ReleaseFormat;
    guest.qmds[address] = qmd;

    NativeQmdScheduler scheduler;
    CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
          "unsupported QMD release format");
    CHECK(guest.releases.empty());
}

TEST_CASE("QMD reduction release needs the flags", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    auto qmd = MakeQmd();
    AddRelease(qmd, 24, 0x10, 1);
    qmd[25] |= ReductionRelease;
    guest.qmds[address] = qmd;

    NativeQmdScheduler plain;
    CHECK(plain.Signal(address, Schedule, guest.Read(), guest.Release()) ==
          "unsupported QMD reduction callback");

    const Address reduced = NextAddress();
    guest.qmds[reduced] = qmd;
    Word seen_flags{};
    NativeQmdScheduler scheduler;
    CHECK(scheduler.Signal(reduced, Schedule, guest.Read(),
                           [&](Address, Word, Word flags) {
                               seen_flags = flags;
                               return true;
                           }) == NoError);
    CHECK(seen_flags == qmd[25]);
}

TEST_CASE("QMD chains into its dependent QMD", "[video_core][native_qmd]") {
    Guest guest;
    const Address first = NextAddress();
    const Address second = NextAddress();
    auto waiting = MakeQmd(1);
    AddRelease(waiting, 24, 0x20, 2);
    guest.qmds[second] = waiting;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(second, Schedule, guest.Read(), guest.Release()) == NoError);

    auto qmd = MakeQmd();
    AddRelease(qmd, 24, 0x10, 1);
    SetDependentQmd(qmd, second, 4);
    guest.qmds[first] = qmd;
    REQUIRE(scheduler.Signal(first, Schedule, guest.Read(), guest.Release()) == NoError);

    CHECK(guest.releases == std::vector<std::pair<Address, Word>>{{0x10, 1}, {0x20, 2}});
}

TEST_CASE("QMD can schedule its dependent QMD", "[video_core][native_qmd]") {
    Guest guest;
    const Address first = NextAddress();
    const Address second = NextAddress();
    auto next = MakeQmd();
    AddRelease(next, 24, 0x20, 2);
    guest.qmds[second] = next;

    auto qmd = MakeQmd();
    SetDependentQmd(qmd, second, 3);
    guest.qmds[first] = qmd;

    NativeQmdScheduler scheduler;
    REQUIRE(scheduler.Signal(first, Schedule, guest.Read(), guest.Release()) == NoError);
    CHECK(guest.releases.size() == 1);
}

TEST_CASE("Invalid QMD signals report why", "[video_core][native_qmd]") {
    Guest guest;
    const Address address = NextAddress();
    NativeQmdScheduler scheduler;

    SECTION("unmapped") {
        CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == "unmapped QMD");
    }
    SECTION("other QMD version") {
        auto qmd = MakeQmd();
        qmd[18] = 0x26;
        guest.qmds[address] = qmd;
        CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
              "unsupported QMD version or queue");
    }
    SECTION("queue bit") {
        auto qmd = MakeQmd();
        qmd[4] |= 0x100;
        guest.qmds[address] = qmd;
        CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
              "unsupported QMD version or queue");
    }
    SECTION("unknown action") {
        CHECK(scheduler.Signal(address, 5, guest.Read(), guest.Release()) ==
              "unsupported PCAS action");
    }
    SECTION("more decrements than dependencies") {
        guest.qmds[address] = MakeQmd(1);
        REQUIRE(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) == NoError);
        CHECK(scheduler.Signal(address, WithCount(Decrement, 2), guest.Read(), guest.Release()) ==
              "QMD dependency underflow");
    }
    SECTION("unknown dependent action") {
        auto qmd = MakeQmd();
        SetDependentQmd(qmd, NextAddress(), 2);
        guest.qmds[address] = qmd;
        CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
              "unsupported dependent QMD action");
    }
    SECTION("second dependent QMD") {
        auto qmd = MakeQmd();
        qmd[16] = 1U << 5;
        guest.qmds[address] = qmd;
        CHECK(scheduler.Signal(address, Schedule, guest.Read(), guest.Release()) ==
              "second dependent QMD unsupported");
    }
    SECTION("release to unmapped memory") {
        auto qmd = MakeQmd();
        AddRelease(qmd, 24, 0x10, 1);
        guest.qmds[address] = qmd;
        CHECK(scheduler.Signal(address, Schedule, guest.Read(),
                               [](Address, Word) { return false; }) == "unmapped QMD release");
    }
}

TEST_CASE("QMD actions 0 and 1 do nothing", "[video_core][native_qmd]") {
    Guest guest;
    NativeQmdScheduler scheduler;
    CHECK(scheduler.Signal(NextAddress(), 0, guest.Read(), guest.Release()) == NoError);
    CHECK(scheduler.Signal(NextAddress(), 1, guest.Read(), guest.Release()) == NoError);
}
