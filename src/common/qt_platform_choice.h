// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

namespace Common {

/// Whether the Qt frontend runs through X11 unless the player chose Wayland ("Force X11 as Graphics
/// Backend", on by default). Under Qt's native Wayland platform, message boxes came out shorter
/// than their text and buttons on COSMIC (Qt 6.8.3, with either client-side decoration); through
/// XWayland they fit.
inline constexpr bool DefaultForceX11 = true;

/// The QT_QPA_PLATFORM value the Qt frontend sets before it starts, or empty to leave Qt's choice.
/// `user_value` is the variable as the player left it, which always wins. X11 comes first and
/// Wayland second, so a session without an X server still starts.
[[nodiscard]] constexpr std::string_view QtPlatformOverride(bool force_x11,
                                                            std::string_view user_value) {
    if (!force_x11 || !user_value.empty()) {
        return {};
    }
    return "xcb;wayland";
}

} // namespace Common
