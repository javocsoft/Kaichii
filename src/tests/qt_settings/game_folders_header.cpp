// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// game_folders.h is included next to the Objective-C runtime (qt_common.cpp on macOS), which
// declares `Category` in the global namespace. This file stops compiling if the header brings in
// a global name like uisettings.h's `using Settings::Category;` again.

#include "qt_common/util/game_folders.h"

typedef struct objc_category* Category;
