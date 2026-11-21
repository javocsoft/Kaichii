// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "common/common_types.h"

/// Reads a Kaichii log for the causes a player can act on, without any AI: the first layer of
/// "Explain the last problem". It looks at the last game session in the log (or the whole log when
/// no game booted) and keeps the lines worth showing, with the user's paths taken out.
namespace Common::LogDiagnosis {

enum class Finding : u8 {
    MissingKeys,       ///< Decryption keys absent, or older than the game
    LoadFailed,        ///< The game file didn't load for another reason
    NoFirmware,        ///< No firmware installed, and the game didn't load
    VulkanUnavailable, ///< Vulkan didn't start: usually the graphics driver
    DeviceLost,        ///< The graphics driver lost the device while playing
    OutOfVideoMemory,  ///< Video memory ran out
    ShadersRejected,   ///< Shaders Kaichii can't translate yet: an emulator limitation
    GuestCrashed,      ///< The game stopped on a memory access: an emulator bug
};

struct Analysis {
    std::vector<Finding> findings;  ///< In the order above, each at most once
    std::vector<std::string> lines; ///< The lines to show and send, redacted and capped
    std::string program_id;         ///< Of the last game booted, if any
    std::string build;              ///< From the version line, if any
    bool game_booted = false;
};

/// The most lines `lines` holds, and the longest a line is kept.
inline constexpr size_t MaxLines = 150;
inline constexpr size_t MaxLineLength = 300;

[[nodiscard]] Analysis Analyze(std::string_view log);

/// Replaces the user's home folder and user name in paths with "~": "/home/ana/x", "/Users/ana/x"
/// and "C:\Users\ana\x" all become "~/x" or "~\x".
[[nodiscard]] std::string Redact(std::string_view line);

} // namespace Common::LogDiagnosis
