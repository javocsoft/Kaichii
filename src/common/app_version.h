// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include <fmt/format.h>

namespace Common {

/// What the window title shows: "Kaichii v1.0.49", or "Kaichii v1.0 (dev)" for a build that wasn't
/// released (no build number).
[[nodiscard]] inline std::string AppTitle(std::string_view name, std::string_view version,
                                          std::string_view build_number) {
    if (build_number.empty()) {
        return fmt::format("{} v{} (dev)", name, version);
    }
    return fmt::format("{} v{}", name, version);
}

/// The details About shows after the title: "Kaichii v1.0.49 | build 49 | Clang 23.1.6". A build
/// that wasn't released names its commit and branch instead of a build number.
[[nodiscard]] inline std::string AppDetails(std::string_view name, std::string_view version,
                                            std::string_view build_number,
                                            std::string_view commit_and_branch,
                                            std::string_view compiler) {
    const std::string build = build_number.empty() ? std::string{commit_and_branch}
                                                   : fmt::format("build {}", build_number);
    return fmt::format("{} | {} | {}", AppTitle(name, version, build_number), build, compiler);
}

} // namespace Common
