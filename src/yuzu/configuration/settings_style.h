// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

#include "common/common_types.h"

class QWidget;

namespace ConfigurationShared {

/// The pictures of the settings categories, drawn in code so they take the theme's colors.
enum class CategoryGlyph : u8 {
    Performance, ///< A gauge
    Image,       ///< A framed landscape
    Smoothness,  ///< A wave
    Audio,       ///< A speaker
    Controls,    ///< A controller
    System,      ///< A chip
    Interface,   ///< A window
    Advanced,    ///< Sliders
};

/// A category's icon, in `color` and, while selected, in `selected` (for 1x and 2x screens).
[[nodiscard]] QIcon CategoryIcon(CategoryGlyph glyph, const QColor& color, const QColor& selected,
                                 int size = 20);

/// Whether the interface uses the system's palette (the Default and Colorful themes, light or
/// dark with the system) rather than a theme style sheet that draws its own boxes.
[[nodiscard]] bool UsesSystemPalette();

/// The settings dialogs' look on the system palette: sections as rounded cards, the category list
/// as a navigation list, and the help panel set apart. Colors come from the palette.
[[nodiscard]] QString SettingsDialogStyleSheet();

/// Applies SettingsDialogStyleSheet to a settings dialog when the system palette is in use.
void StyleSettingsDialog(QWidget& dialog);

} // namespace ConfigurationShared
