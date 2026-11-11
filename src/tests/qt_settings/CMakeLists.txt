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
