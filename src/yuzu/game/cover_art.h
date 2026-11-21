// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QSize>

namespace Yuzu {

/// The picture filling `size`, cropped rather than letterboxed, with corners of `radius` cut away.
/// Transparent parts of the picture stay transparent.
[[nodiscard]] QImage FitCover(const QImage& picture, QSize size, qreal radius);

/// A stand-in box for a game without box art: its icon, centred, over a blurred and darkened
/// enlargement of the same icon that fills `size`.
[[nodiscard]] QImage IconBox(const QImage& icon, QSize size, qreal radius);

/// A shadow with the cover's own shape: black, its alpha the cover's scaled by `alpha` / 255.
[[nodiscard]] QImage CoverShadow(const QImage& cover, int alpha);

} // namespace Yuzu
