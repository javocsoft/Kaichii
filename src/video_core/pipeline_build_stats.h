// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>

#include "common/common_types.h"

namespace VideoCommon {

/// Graphics pipelines the disk cache holds that a draw needed before any was built (not at boot,
/// not by the background prebuild), counted since the emulator started: each was translated again
/// while playing. The frontend reads it to tell whether preloading shaders would help.
inline std::atomic<u64> g_cached_pipelines_built_late{};

} // namespace VideoCommon
