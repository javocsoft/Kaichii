// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string_view>
#include <utility>

namespace Yuzu {

/// Who a menu entry is for. Basic entries are always shown; the others follow the settings'
/// "Show advanced options" and "Developer mode", as the settings themselves do.
enum class MenuLevel {
    Basic,
    /// Fine tuning, maintenance and console tools a player rarely needs.
    Advanced,
    /// Debugging and testing Kaichii itself.
    Developer,
};

[[nodiscard]] constexpr bool MenuShows(MenuLevel level, bool show_advanced, bool developer_mode) {
    switch (level) {
    case MenuLevel::Basic:
        return true;
    case MenuLevel::Advanced:
        return show_advanced;
    case MenuLevel::Developer:
        // As in the settings, developer mode only counts with the advanced options shown.
        return show_advanced && developer_mode;
    }
    return false;
}

/// The menu bar's entries above the basic level, by their object name in main.ui (an action, or a
/// submenu). Everything else is basic: playing, setting up, the game list and help.
inline constexpr std::array<std::pair<std::string_view, MenuLevel>, 15> MainMenuLevels{{
    // File
    {"action_Load_Folder", MenuLevel::Advanced},
    {"action_NAND_Folder", MenuLevel::Advanced},
    {"action_SDMC_Folder", MenuLevel::Advanced},
    // View
    {"action_Single_Window_Mode", MenuLevel::Advanced},
    {"action_Enable_Overlay_Applet", MenuLevel::Advanced},
    {"action_Post_Processing_Shaders", MenuLevel::Advanced},
    {"menu_Reset_Window_Size", MenuLevel::Advanced},
    {"menu_View_Debugging", MenuLevel::Developer},
    // Tools
    {"action_Verify_installed_contents", MenuLevel::Advanced},
    {"action_Data_Manager", MenuLevel::Advanced},
    {"menu_cabinet_applet", MenuLevel::Advanced},
    {"menu_Applets", MenuLevel::Advanced},
    {"menu_Create_Shortcuts", MenuLevel::Advanced},
    {"menuTAS", MenuLevel::Developer},
    // Help
    {"action_Eden_Dependencies", MenuLevel::Developer},
}};

} // namespace Yuzu
