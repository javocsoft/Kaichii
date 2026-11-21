// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>

namespace Yuzu {

/// The picture filling a square of `side` pixels, cropped rather than letterboxed, with corners of
/// `radius` cut away. Transparent parts of the picture stay transparent.
[[nodiscard]] QImage FitCover(const QImage& picture, int side, qreal radius);

/// A shadow with the cover's own shape: black, its alpha the cover's scaled by `alpha` / 255.
[[nodiscard]] QImage CoverShadow(const QImage& cover, int alpha);

} // namespace Yuzu
