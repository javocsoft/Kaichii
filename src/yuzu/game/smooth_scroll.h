// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QObject>

class QAbstractScrollArea;
class QScrollBar;
class QVariantAnimation;
class QWheelEvent;

/// The game list's animated wheel scrolling. It moves the scroll bars of whichever view is shown
/// when the animation runs, so every view scrolls, not only the ones it was first set up for.
class SmoothScroll : public QObject {
    Q_OBJECT

public:
    /// `view` returns the view the wheel scrolls now.
    SmoothScroll(std::function<QAbstractScrollArea*()> view, QObject* parent = nullptr);

    /// Starts scrolling for a wheel turn; Shift turns a vertical wheel into a horizontal scroll.
    void Wheel(const QWheelEvent& event);

    /// How long a scroll animates.
    static constexpr int DurationMs = 200;

private:
    void Scroll(QVariantAnimation* animation, int& target, QScrollBar* bar, int delta);

    std::function<QAbstractScrollArea*()> view;
    QVariantAnimation* vertical = nullptr;
    QVariantAnimation* horizontal = nullptr;
    int vertical_target = 0;
    int horizontal_target = 0;
};
