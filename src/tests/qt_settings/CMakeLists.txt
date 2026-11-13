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
    ../../yuzu/configuration/settings_style.cpp
    ../../yuzu/configuration/settings_style.h
)
target_compile_features(setting_help_tests PRIVATE cxx_std_20)
target_include_directories(setting_help_tests PRIVATE ../..)
target_link_libraries(setting_help_tests PRIVATE qt_common frontend_common common SimpleIni::SimpleIni
                      Qt6::Widgets Qt6::Test)
add_test(NAME setting_help COMMAND setting_help_tests)
set_tests_properties(setting_help PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The game list: icons for games without one, and when it counts as empty.
add_executable(game_list_tests game_list.cpp)
target_compile_features(game_list_tests PRIVATE cxx_std_20)
target_include_directories(game_list_tests PRIVATE ../..)
target_link_libraries(game_list_tests PRIVATE qt_common frontend_common common
                      SimpleIni::SimpleIni Qt6::Widgets Qt6::Test)
add_test(NAME game_list COMMAND game_list_tests)
set_tests_properties(game_list PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

# The Spanish catalog built from es.ts and Qt's own next to it: the screens find their strings
# under the context they ask for, and Qt's buttons follow the interface language.
if (ENABLE_QT_TRANSLATION)
    set(SPANISH_TS ${PROJECT_SOURCE_DIR}/dist/languages/es.ts)
    set_source_files_properties(${SPANISH_TS} PROPERTIES
                                OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
    qt_add_translation(SPANISH_QM ${SPANISH_TS})
    include(QtBaseTranslations)
    copy_qtbase_translations(SPANISH_QM ${CMAKE_CURRENT_BINARY_DIR}/translations ${SPANISH_TS})
    add_executable(translation_tests
        translation.cpp
        ../../yuzu/configuration/per_game_summary.cpp
        ../../yuzu/configuration/per_game_summary.h
        ../../yuzu/configuration/settings_organizer.cpp
        ../../yuzu/configuration/settings_organizer.h
        ../../yuzu/first_run_assistant.cpp
        ../../yuzu/first_run_assistant.h
        ${SPANISH_QM}
    )
    target_compile_features(translation_tests PRIVATE cxx_std_20)
    target_compile_definitions(translation_tests PRIVATE
                               TRANSLATIONS_DIR="${CMAKE_CURRENT_BINARY_DIR}/translations")
    target_include_directories(translation_tests PRIVATE ../..)
    target_link_libraries(translation_tests PRIVATE qt_common frontend_common common
                          SimpleIni::SimpleIni Qt6::Widgets Qt6::Test)
    add_test(NAME translation COMMAND translation_tests)
    set_tests_properties(translation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Adding Kaichii to the applications menu when it runs as an AppImage.
add_executable(desktop_integration_tests desktop_integration.cpp)
target_compile_features(desktop_integration_tests PRIVATE cxx_std_20)
target_compile_definitions(desktop_integration_tests PRIVATE
    SHIPPED_DESKTOP_ENTRY="${PROJECT_SOURCE_DIR}/dist/io.github.javocsoft.kaichii.desktop")
target_include_directories(desktop_integration_tests PRIVATE ../..)
target_link_libraries(desktop_integration_tests PRIVATE qt_common frontend_common common
                      SimpleIni::SimpleIni Qt6::Widgets Qt6::Test)
add_test(NAME desktop_integration COMMAND desktop_integration_tests)
