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
constexpr u64 BreathOfTheWildProgramId = 0x01007EF00011E000ULL;
constexpr u64 TearsOfTheKingdomProgramId = 0x0100F2C0115B6000ULL;

enum class Kind { All, Place, Character, Enemy, Boss, Dungeon, Item, Remake, Timeline, Ability };

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

/// A game with a companion: its cards, the topics it offers and how loaded scenes map to cards.
struct Game {
    u64 program_id;
    Text title;
    Text intro;
    Text items_topic;
    std::span<const Entry> (*entries)();
    std::span<const Kind> kinds;
    const Entry* (*entry_for_scene)(std::string_view scene);
};

[[nodiscard]] const Game& Ocarina();
[[nodiscard]] const Game& BreathOfTheWild();
[[nodiscard]] const Game& TearsOfTheKingdom();
[[nodiscard]] const Game* FindGame(u64 program_id);
[[nodiscard]] inline bool SupportsGame(u64 program_id) {
    return FindGame(program_id) != nullptr;
}

[[nodiscard]] std::span<const Entry> OcarinaEntries();
[[nodiscard]] std::span<const Entry> BestiaryEntries();
[[nodiscard]] std::span<const Entry> WorldEntries();
[[nodiscard]] std::span<const Entry> ItemEntries();
[[nodiscard]] std::span<const Entry> BreathOfTheWildEntries();
[[nodiscard]] std::span<const Entry> TearsOfTheKingdomEntries();
[[nodiscard]] const Entry* OcarinaEntryForScene(std::string_view scene);
[[nodiscard]] const Entry* BreathOfTheWildEntryForScene(std::string_view scene);
[[nodiscard]] const Entry* TearsOfTheKingdomEntryForScene(std::string_view scene);

[[nodiscard]] const Entry* FindEntry(const Game& game, std::string_view id);
[[nodiscard]] const Entry* EntryForScene(const Game& game, std::string_view scene);
[[nodiscard]] QString Localize(Text text, bool spanish);
[[nodiscard]] std::vector<const Entry*> Search(const Game& game, QString query, Kind kind,
                                               bool spoilers);

} // namespace GameCompanion
