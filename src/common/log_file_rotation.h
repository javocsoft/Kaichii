// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <system_error>

namespace Common::Log {

/// Moves a log over its ".old.txt" copy. The rename replaces the copy in one step, and fails if
/// the log is still open elsewhere (a Kaichii process that hasn't exited yet, on Windows). A log
/// that can't even be checked counts as a failure, so it is never truncated.
/// @returns Whether the log is out of the way (moved, or there was none).
inline bool RotateLogFile(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return !ec;
    }
    auto old_path = path;
    old_path += ".old.txt";
    std::filesystem::rename(path, old_path, ec);
    return !ec;
}

struct LogFileChoice {
    std::filesystem::path path;
    bool append; ///< Append to `path` (it couldn't be moved), counting its size toward the limit
};

/// The file a new session logs to: `path` once rotated. If it can't be moved, a second log
/// (eden_log_2.txt), so the previous session is kept and this one still gets the full size limit;
/// if that one can't be moved either, the session is appended to it.
inline LogFileChoice ChooseLogFile(const std::filesystem::path& path) {
    if (RotateLogFile(path)) {
        return {path, false};
    }
    auto second = path.parent_path() / path.stem();
    second += "_2";
    second += path.extension();
    return {second, !RotateLogFile(second)};
}

} // namespace Common::Log
