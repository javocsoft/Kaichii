// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <span>
#include <string_view>
#include <vector>

#include <QString>

#include "common/common_types.h"

namespace GameCompanion {

constexpr u64 OcarinaSwitch2ProgramId = 0x0400486024DEC000ULL;

enum class Kind { All, Place, Character, Enemy, Boss, Dungeon, Item, Remake, Timeline };

struct Text {
    std::string_view en;
    std::string_view es;
};

struct Entry {
    std::string_view id;
    Kind kind;
    Text title;
    Text summary;
    Text detail;
    std::string_view source_url;
    bool spoiler;
};

[[nodiscard]] constexpr bool SupportsGame(u64 program_id) {
    return program_id == OcarinaSwitch2ProgramId;
}

[[nodiscard]] std::span<const Entry> OcarinaEntries();
[[nodiscard]] std::span<const Entry> BestiaryEntries();
[[nodiscard]] std::span<const Entry> WorldEntries();
[[nodiscard]] std::span<const Entry> ItemEntries();
[[nodiscard]] const Entry* FindEntry(std::string_view id);
[[nodiscard]] const Entry* EntryForScene(std::string_view scene);
[[nodiscard]] QString Localize(Text text, bool spanish);
[[nodiscard]] std::vector<const Entry*> Search(QString query, Kind kind, bool spoilers);

} // namespace GameCompanion
