# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
add_executable(settings_page_scroll_tests
    settings_page_scroll.cpp
    ../../yuzu/configuration/settings_page_scroll.cpp
    ../../yuzu/configuration/settings_page_scroll.h
)
target_compile_features(settings_page_scroll_tests PRIVATE cxx_std_20)
target_include_directories(settings_page_scroll_tests PRIVATE ../..)
target_link_libraries(settings_page_scroll_tests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME settings_page_scroll COMMAND settings_page_scroll_tests)
set_tests_properties(settings_page_scroll PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The settings help and layout: every shown setting is explained and placed, the panel follows the
# pointer and focus, and the organizer moves rows and shows them by level.
add_executable(setting_help_tests
    setting_help.cpp
    ../../yuzu/configuration/setting_help_panel.cpp
    ../../yuzu/configuration/setting_help_panel.h
    ../../yuzu/configuration/per_game_summary.cpp
    ../../yuzu/configuration/per_game_summary.h
    ../../yuzu/configuration/settings_organizer.cpp
    ../../yuzu/configuration/settings_organizer.h
)
target_compile_features(setting_help_tests PRIVATE cxx_std_20)
target_include_directories(setting_help_tests PRIVATE ../..)
target_link_libraries(setting_help_tests PRIVATE qt_common frontend_common common SimpleIni::SimpleIni
                      Qt6::Widgets Qt6::Test)
add_test(NAME setting_help COMMAND setting_help_tests)
set_tests_properties(setting_help PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Game list icons for games without one.
add_executable(game_list_icon_tests game_list_icon.cpp)
target_compile_features(game_list_icon_tests PRIVATE cxx_std_20)
target_include_directories(game_list_icon_tests PRIVATE ../..)
target_link_libraries(game_list_icon_tests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME game_list_icon COMMAND game_list_icon_tests)
set_tests_properties(game_list_icon PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The first-run assistant: the preset it proposes and what it asks the main window to do.
add_executable(first_run_assistant_tests
    first_run_assistant.cpp
    ../../yuzu/first_run_assistant.cpp
    ../../yuzu/first_run_assistant.h
)
target_compile_features(first_run_assistant_tests PRIVATE cxx_std_20)
target_include_directories(first_run_assistant_tests PRIVATE ../..)
target_link_libraries(first_run_assistant_tests PRIVATE qt_common frontend_common common
                      SimpleIni::SimpleIni Qt6::Widgets Qt6::Test)
add_test(NAME first_run_assistant COMMAND first_run_assistant_tests)
set_tests_properties(first_run_assistant PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
