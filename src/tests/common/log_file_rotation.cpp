// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "common/log_file_rotation.h"

namespace {
namespace fs = std::filesystem;

struct TempDir {
    TempDir() {
        path = fs::temp_directory_path() /
               ("lichii-log-rotation-" + std::to_string(std::random_device{}()));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    fs::path path;
};

void Write(const fs::path& path, const std::string& text) {
    std::ofstream{path} << text;
}

std::string Read(const fs::path& path) {
    std::ifstream file{path};
    return {std::istreambuf_iterator<char>{file}, {}};
}

/// A file can't be renamed over a directory that holds something.
void Block(const fs::path& path) {
    fs::create_directories(path);
    Write(path / "keep", "x");
}
} // Anonymous namespace

TEST_CASE("A new session's log replaces the previous session's copy", "[common][log]") {
    TempDir dir;
    const auto log = dir.path / "eden_log.txt";
    Write(log, "previous");
    Write(dir.path / "eden_log.txt.old.txt", "older");
    const auto choice = Common::Log::ChooseLogFile(log);
    REQUIRE(choice.path == log);
    REQUIRE_FALSE(choice.append);
    REQUIRE_FALSE(fs::exists(log));
    REQUIRE(Read(dir.path / "eden_log.txt.old.txt") == "previous");

    // No log yet: nothing to move.
    const auto fresh = Common::Log::ChooseLogFile(dir.path / "other_log.txt");
    REQUIRE(fresh.path == dir.path / "other_log.txt");
    REQUIRE_FALSE(fresh.append);
}

TEST_CASE("A log that can't be moved is kept, and the session logs to a second file",
          "[common][log]") {
    TempDir dir;
    const auto log = dir.path / "eden_log.txt";
    Write(log, "previous");
    Block(dir.path / "eden_log.txt.old.txt");
    const auto choice = Common::Log::ChooseLogFile(log);
    REQUIRE(choice.path == dir.path / "eden_log_2.txt");
    REQUIRE_FALSE(choice.append);
    REQUIRE(Read(log) == "previous");

    // Neither can be moved: append to the second one rather than truncating it.
    Write(dir.path / "eden_log_2.txt", "second");
    Block(dir.path / "eden_log_2.txt.old.txt");
    const auto blocked = Common::Log::ChooseLogFile(log);
    REQUIRE(blocked.path == dir.path / "eden_log_2.txt");
    REQUIRE(blocked.append);
    REQUIRE(Read(dir.path / "eden_log_2.txt") == "second");
}
