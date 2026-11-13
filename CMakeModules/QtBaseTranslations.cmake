# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# Copies Qt's own catalog (qtbase_<language>.qm: its standard buttons, wizard buttons and dialogs)
# for each interface catalog in ARGN to dest_dir, named after the interface catalog
# (qtbase_ja_JP.qm for ja_JP.ts), and appends the copies to out_var. The packages don't all
# deploy Qt's translations, so the frontend carries them. A language Qt has no catalog for is left
# out, and its Qt strings stay in English.
function(copy_qtbase_translations out_var dest_dir)
    set(qt_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
    set(copies ${${out_var}})
    foreach (ts IN LISTS ARGN)
        get_filename_component(language "${ts}" NAME_WE)
        string(REGEX REPLACE "_.*" "" base "${language}")
        foreach (candidate IN ITEMS "${language}" "${base}")
            set(source "${qt_dir}/qtbase_${candidate}.qm")
            if (EXISTS "${source}")
                set(copy "${dest_dir}/qtbase_${language}.qm")
                configure_file("${source}" "${copy}" COPYONLY)
                list(APPEND copies "${copy}")
                break()
            endif()
        endforeach()
    endforeach()
    set(${out_var} ${copies} PARENT_SCOPE)
endfunction()
