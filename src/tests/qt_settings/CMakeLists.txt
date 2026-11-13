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

# The settings help: every shown setting is explained, and the panel follows the pointer and focus.
add_executable(setting_help_tests
    setting_help.cpp
    ../../yuzu/configuration/setting_help_panel.cpp
    ../../yuzu/configuration/setting_help_panel.h
)
target_compile_features(setting_help_tests PRIVATE cxx_std_20)
target_include_directories(setting_help_tests PRIVATE ../..)
target_link_libraries(setting_help_tests PRIVATE qt_common frontend_common common SimpleIni::SimpleIni
                      Qt6::Widgets Qt6::Test)
add_test(NAME setting_help COMMAND setting_help_tests)
set_tests_properties(setting_help PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
