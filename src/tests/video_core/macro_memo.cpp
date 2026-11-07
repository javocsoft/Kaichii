// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/macro_memo.h"

namespace {

using Memo = Tegra::BasicMacroMemo<std::unordered_map<std::uint64_t, std::uint32_t>>;
using Tegra::MacroMemoPairs;
using Words = std::vector<std::uint32_t>;

constexpr std::uint32_t Start = 0x40;
const Words Parameters{1, 2};
const Words DataRam{10, 20, 30, 40};

Words ToVector(std::span<const std::uint32_t> words) {
    return {words.begin(), words.end()};
}

} // namespace

TEST_CASE("Macro memo misses a call it has not seen", "[video_core][macro_memo]") {
    Memo memo;
    Memo::Effects effects;
    CHECK_FALSE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(memo.GetStatistics().misses == 1);
}

TEST_CASE("Macro memo replays a call's effects in order", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {{1, 20}}, {{2, 99}}, {{0x100, 1}, {0x104, 2}, {0x100, 3}});

    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(ToVector(effects.data_ram_writes) == Words{2, 99});
    CHECK(ToVector(effects.writes) == Words{0x100, 1, 0x104, 2, 0x100, 3});
}

TEST_CASE("Macro memo tells calls apart", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}});

    Memo::Effects effects;
    SECTION("other parameter value") {
        CHECK_FALSE(memo.Find(Start, Words{1, 3}, DataRam, effects));
    }
    SECTION("other parameter count") {
        CHECK_FALSE(memo.Find(Start, Words{1}, DataRam, effects));
        CHECK_FALSE(memo.Find(Start, Words{1, 2, 0}, DataRam, effects));
    }
    SECTION("other macro") {
        CHECK_FALSE(memo.Find(Start + 1, Parameters, DataRam, effects));
    }
}

TEST_CASE("Macro memo only replays while its data RAM reads hold", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {{1, 20}, {3, 40}}, {}, {{0x100, 1}});

    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));

    auto changed = DataRam;
    changed[3] = 41;
    CHECK_FALSE(memo.Find(Start, Parameters, changed, effects));
    CHECK(memo.GetStatistics().guard_failures == 1);
    // A word it did not read changing is fine.
    changed = DataRam;
    changed[0] = 11;
    CHECK(memo.Find(Start, Parameters, changed, effects));
}

TEST_CASE("Macro memo stores a call again in place", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {{1, 20}}, {}, {{0x100, 1}});
    const auto words = memo.ArenaWords();
    memo.Store(Start, Parameters, {{1, 21}}, {}, {{0x100, 2}});

    CHECK(memo.GetStatistics().overwrites == 1);
    CHECK(memo.ArenaWords() == words);

    auto changed = DataRam;
    changed[1] = 21;
    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, changed, effects));
    CHECK(ToVector(effects.writes) == Words{0x100, 2});
}

TEST_CASE("Macro memo takes the newest record of a call with a new shape",
          "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}});
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}, {0x104, 2}});
    CHECK(memo.Size() == 1);

    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(ToVector(effects.writes) == Words{0x100, 1, 0x104, 2});
}

TEST_CASE("Macro memo tries the last call of a macro first", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}});

    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(memo.GetStatistics().slot_hits == 1);

    // A different call of the same macro falls through to the hash table.
    memo.Store(Start, Words{7}, {}, {}, {{0x100, 2}});
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(memo.GetStatistics().hits == 2);
    CHECK(memo.GetStatistics().slot_hits == 1);
    CHECK(ToVector(effects.writes) == Words{0x100, 1});
}

TEST_CASE("Macro memo forgets everything when macro code changes", "[video_core][macro_memo]") {
    Memo memo;
    memo.Synchronize(1);
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}});

    memo.Synchronize(1);
    CHECK(memo.Size() == 1);

    memo.Synchronize(2);
    CHECK(memo.Size() == 0);
    CHECK(memo.ArenaWords() == 0);
    Memo::Effects effects;
    CHECK_FALSE(memo.Find(Start, Parameters, DataRam, effects));
}

TEST_CASE("Macro memo keeps live calls when its arena compacts", "[video_core][macro_memo]") {
    Memo memo;
    memo.Store(Start, Parameters, {}, {}, {{0x100, 1}});

    // Storing one call under ever new shapes abandons each old record. 1 << 22 words is the arena's
    // limit; this crosses it without keeping more than one live record per call.
    MacroMemoPairs writes;
    writes.resize(1000, {0x200, 1});
    const Words other{9};
    std::uint32_t value{};
    while (memo.GetStatistics().compactions == 0) {
        writes.emplace_back(0x204, value++);
        memo.Store(Start + 1, other, {}, {}, writes);
    }

    CHECK(memo.GetStatistics().clears == 0);
    Memo::Effects effects;
    REQUIRE(memo.Find(Start, Parameters, DataRam, effects));
    CHECK(ToVector(effects.writes) == Words{0x100, 1});
    REQUIRE(memo.Find(Start + 1, other, DataRam, effects));
    CHECK(effects.writes.size() == writes.size() * 2);
}
