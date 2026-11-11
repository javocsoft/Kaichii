// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QRect>
#include <QScrollArea>
#include <QSize>

class QDialog;

namespace ConfigurationShared {

/// A settings page in a frameless scroll area. A page taller or wider than its dialog scrolls,
/// instead of squeezing its controls or pushing the dialog past the screen (Configure was 1223x720
/// at 100 % scale and twice that at 200 %, with no way to shrink it).
class SettingsPageScroll final : public QScrollArea {
public:
    /// Takes `page` (and its accessible name, which is the tab title). Combo boxes, spin boxes and
    /// sliders on it change on the wheel only once clicked or tabbed into: an unfocused one passes
    /// the wheel to the page, so scrolling over a page never changes a setting.
    explicit SettingsPageScroll(QWidget* page, QWidget* parent = nullptr);

    /// The page's full size plus room for a scroll bar, so a dialog fitting its content still
    /// opens without one when the screen allows.
    QSize sizeHint() const override;
};

/// Sizes a dialog that holds settings pages: the size its content asks for, at most `fraction` of
/// the available area of its parent's screen. QDialog centres it on its parent when shown.
void FitDialogToScreen(QDialog& dialog, double fraction = 0.9);

/// The size FitDialogToScreen gives a dialog wanting `wanted` on a screen whose free area is
/// `available`.
QSize FitSizeToScreen(QSize wanted, QSize available, double fraction = 0.9);

} // namespace ConfigurationShared
