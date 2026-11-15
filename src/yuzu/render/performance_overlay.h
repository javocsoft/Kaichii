// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <deque>
#include <optional>
#include <vector>
#include <QWidget>

#include "common/common_types.h"
#include "common/dkb_cpu_trace.h"
#include "common/frame_pacing.h"
#include "core/perf_stats.h"

namespace VideoCore {
class ShaderNotify;
}

class MainWindow;

/// Frame stats over the game: big fps, frame time, CPU and GPU %, a 5 s frame-time graph and,
/// for Switch 2 games, the 1% low and stutters of their real presents. The layout is Lichii's
/// redesign (zortos293/Lichii#1345); it stays a child of the game window here.
class PerformanceOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceOverlay(MainWindow* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void resetPosition(const QPoint& pos);
    void updateStats(const Core::PerfStatsResults& results, const VideoCore::ShaderNotify& shaders);
    /// The frames shown on screen per second that frame generation adds, 0 without it.
    double GeneratedFps();
    void paintGraph(QPainter& painter, const QRectF& area);

    MainWindow* m_mainWindow = nullptr;

    QPoint m_offset{25, 75};
    QPoint m_drag_start_pos;

    std::optional<double> m_fps;
    /// The last 5 s of 500 ms fps readings, for min/median/max
    std::deque<float> m_fpsSamples;
    std::optional<double> m_frameMs;
    std::optional<double> m_cpuPercent;
    std::optional<double> m_gpuPercent;
    std::optional<Common::DkbCpuTrace::Snapshot> m_cpuSnapshot;
    /// The graph's frame times, oldest first.
    std::vector<float> m_frames;
    /// A Switch 2 game's pacing from its real presents; nullopt for other games.
    std::optional<Common::FramePacing> m_pacing;
    QString m_tags;

    /// The renderer's generated-frame counter at its last reading, for the rate between readings.
    u64 m_generatedFrames = 0;
    std::chrono::steady_clock::time_point m_generatedTime{};
    double m_generatedFps = 0.0;

signals:
    void closed();
};
