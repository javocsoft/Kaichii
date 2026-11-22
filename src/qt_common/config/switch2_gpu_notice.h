// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

#include "common/performance_presets.h"

namespace QtCommon {

/// Tells players whose graphics will likely run Switch 2 games too slowly to play
/// (Settings::Switch2GpuConcernFor) that Switch games are fine, or, on a software renderer, that
/// every game is slow; and points them to a dedicated GPU in the machine when it has one. Empty
/// when there is no concern.
[[nodiscard]] QString Switch2GpuNotice(const Settings::HostProfile& host);

} // namespace QtCommon
