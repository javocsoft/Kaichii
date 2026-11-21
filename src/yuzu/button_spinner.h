// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QElapsedTimer>
#include <QObject>
#include <QString>

class QPushButton;
class QTimer;

/// Shows a turning arc and the seconds spent on a button while something it started runs, and
/// restores the button's text afterwards. The arc keeps its colour on a disabled button.
class ButtonSpinner final : public QObject {
    Q_OBJECT

public:
    explicit ButtonSpinner(QPushButton* button);

    /// `text` gives the button's label for the seconds spent so far.
    void Start(std::function<QString(qint64)> text);
    void Stop();
    [[nodiscard]] bool Running() const;

private:
    void Tick();

    QPushButton* button;
    QTimer* timer;
    QElapsedTimer clock;
    QString restore_text;
    std::function<QString(qint64)> text;
    int frame = 0;
};
