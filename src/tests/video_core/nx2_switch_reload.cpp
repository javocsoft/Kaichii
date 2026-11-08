// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <optional>
#include <string>
#include <catch2/catch_test_macros.hpp>

#include "video_core/nx2_cmdpage_diag.h"
#include "video_core/nx2_download_trace.h"
#include "video_core/nx2_profile.h"

namespace {

void SetSwitch(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value ? value : "");
#else
    if (value) {
        setenv(name, value, 1);
    } else {
        unsetenv(name);
    }
#endif
}

/// Sets a switch for the test and puts back what the environment had.
struct ScopedSwitch {
    const char* name;
    std::optional<std::string> previous;

    ScopedSwitch(const char* name_, const char* value) : name{name_} {
        if (const char* old = std::getenv(name)) {
            previous = old;
        }
        SetSwitch(name, value);
    }

    ~ScopedSwitch() {
        SetSwitch(name, previous ? previous->c_str() : nullptr);
    }
};

void ReadSwitches() {
    VideoCommon::Nx2Profile::ReadSwitch();
    Tegra::Nx2CmdPageDiag::ReadSwitch();
    VideoCommon::Nx2DownloadTrace::ReadSwitch();
}

} // namespace

// The frontend loads nx2_switches.txt into the environment after static initialization, and
// VideoCore::CreateGPU reads these switches again before any GPU thread starts.
TEST_CASE("Diagnostic switches set after startup take effect when they are read again",
          "[nx2_switches]") {
    {
        const ScopedSwitch profile{"EDEN_NX2_GPU_PROFILE", "1"};
        const ScopedSwitch diag{"EDEN_NX2_CMDPAGE_DIAG", "1"};
        const ScopedSwitch trace{"EDEN_NX2_DOWNLOAD_TRACE", "1"};
        ReadSwitches();
        CHECK(VideoCommon::Nx2Profile::Enabled());
        CHECK(Tegra::Nx2CmdPageDiag::g_enabled);
        CHECK(VideoCommon::Nx2DownloadTrace::g_enabled);
    }
    {
        const ScopedSwitch profile{"EDEN_NX2_GPU_PROFILE", nullptr};
        const ScopedSwitch diag{"EDEN_NX2_CMDPAGE_DIAG", nullptr};
        const ScopedSwitch trace{"EDEN_NX2_DOWNLOAD_TRACE", nullptr};
        ReadSwitches();
        CHECK_FALSE(VideoCommon::Nx2Profile::Enabled());
        CHECK_FALSE(Tegra::Nx2CmdPageDiag::g_enabled);
        CHECK_FALSE(VideoCommon::Nx2DownloadTrace::g_enabled);
    }
    // Leave them as the test run's environment sets them.
    ReadSwitches();
}
