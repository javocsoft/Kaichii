// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <deque>
#include "common/common_types.h"
#include <QWidget>

namespace VideoCore {
class ShaderNotify;
}
namespace Core {
struct PerfStatsResults;
}
namespace Ui {
class PerformanceOverlay;
}

class QLineSeries;
class QChart;
class QChartView;
class QValueAxis;
class MainWindow;

class PerformanceOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceOverlay(MainWindow* parent = nullptr);
    ~PerformanceOverlay();

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void resetPosition(const QPoint& pos);
    void updateStats(const Core::PerfStatsResults& results, const VideoCore::ShaderNotify& shaders);
    /// Shows a Switch 2 game's per-frame pacing (frame times, 1% low, stutters). False when there
    /// are no recent presents to show: the overlay then shows its averages.
    bool ShowFramePacing();
    /// Widens the overlay when its text no longer fits.
    void FitWidth();

    MainWindow* m_mainWindow = nullptr;
    /// The renderer's generated-frame counter at its last reading, for the rate between readings.
    u64 m_generatedFrames = 0;
    std::chrono::steady_clock::time_point m_generatedTime{};
    double m_generatedFps = 0.0;
    Ui::PerformanceOverlay* ui;

    // colors
    QColor m_background{72, 72, 72};

    QPoint m_offset{25, 75};

    // frametime
    const size_t NUM_FRAMETIME_SAMPLES = 300;
    std::deque<double> m_frametimeSamples;

    // fps
    const size_t NUM_FPS_SAMPLES = 120;
    qreal m_xPos = 0;
    std::deque<double> m_fpsSamples;
    std::deque<QPointF> m_fpsPoints;

    // drag
    QPoint m_drag_start_pos;

    // fps chart
    QLineSeries* m_fpsSeries = nullptr;
    QLineSeries* m_targetSeries = nullptr;
    QChart* m_fpsChart = nullptr;
    QChartView* m_fpsChartView = nullptr;
    QValueAxis* m_fpsX = nullptr;
    QValueAxis* m_fpsY = nullptr;

signals:
    void closed();
};
