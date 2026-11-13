// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#if defined(__unix__) && !defined(__APPLE__) && !defined(__ANDROID__)

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "common/nvidia_shader_cache.h"

namespace {
/// Sets or clears an environment variable for one test and restores it afterwards.
struct ScopedEnv {
    explicit ScopedEnv(const char* name_, const char* value) : name{name_} {
        if (const char* old = std::getenv(name)) {
            previous = old;
        }
        if (value != nullptr) {
            setenv(name, value, 1);
        } else {
            unsetenv(name);
        }
    }
    ~ScopedEnv() {
        if (previous) {
            setenv(name, previous->c_str(), 1);
        } else {
            unsetenv(name);
        }
    }
    const char* name;
    std::optional<std::string> previous;
};

std::string Env(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}
} // namespace

TEST_CASE("The NVIDIA shader cache keeps its own lichii folder and a 10 GiB limit",
          "[common][nvidia_shader_cache]") {
    const auto temp = std::filesystem::temp_directory_path() /
                      ("nvidia-cache-test-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ScopedEnv xdg{"XDG_CACHE_HOME", temp.c_str()};
    ScopedEnv path{"__GL_SHADER_DISK_CACHE_PATH", nullptr};
    ScopedEnv size{"__GL_SHADER_DISK_CACHE_SIZE", nullptr};
    Common::SetUpNvidiaShaderDiskCache();
    CHECK(Env("__GL_SHADER_DISK_CACHE_PATH") == (temp / "lichii" / "nvidia").string());
    CHECK(std::filesystem::is_directory(temp / "lichii" / "nvidia"));
    CHECK(Env("__GL_SHADER_DISK_CACHE_SIZE") == "10737418240");
    std::error_code error;
    std::filesystem::remove_all(temp, error);
}

TEST_CASE("The NVIDIA shader cache keeps what the user set", "[common][nvidia_shader_cache]") {
    ScopedEnv path{"__GL_SHADER_DISK_CACHE_PATH", "/somewhere/else"};
    ScopedEnv size{"__GL_SHADER_DISK_CACHE_SIZE", "123"};
    Common::SetUpNvidiaShaderDiskCache();
    CHECK(Env("__GL_SHADER_DISK_CACHE_PATH") == "/somewhere/else");
    CHECK(Env("__GL_SHADER_DISK_CACHE_SIZE") == "123");
}

TEST_CASE("Without an absolute cache folder only the limit is raised",
          "[common][nvidia_shader_cache]") {
    ScopedEnv xdg{"XDG_CACHE_HOME", "relative/cache"};
    ScopedEnv home{"HOME", nullptr};
    ScopedEnv path{"__GL_SHADER_DISK_CACHE_PATH", nullptr};
    ScopedEnv size{"__GL_SHADER_DISK_CACHE_SIZE", nullptr};
    Common::SetUpNvidiaShaderDiskCache();
    CHECK(std::getenv("__GL_SHADER_DISK_CACHE_PATH") == nullptr);
    CHECK(Env("__GL_SHADER_DISK_CACHE_SIZE") == "10737418240");
}

#endif
