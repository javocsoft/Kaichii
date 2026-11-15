// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QIcon>

/// The icon `name` from dist/icons/menu (Lichii's line icons, zortos293/Lichii#1365) in the
/// current theme's tone: dark grey on light themes, light grey on dark ones and white on a dark
/// theme's highlighted row. Empty when the icon or Qt's SVG image plugin is missing.
[[nodiscard]] QIcon MenuIcon(const char* name);
