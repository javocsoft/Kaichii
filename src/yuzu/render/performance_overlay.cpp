// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <vector>

#include "common/frame_pacing.h"
#include "core/core.h"
#include "core/perf_stats.h"
#include "performance_overlay.h"
#include "ui_performance_overlay.h"

#include "main_window.h"

#include <QChart>
#include <QChartView>
#include <QGraphicsLayout>
#include <QGuiApplication>
#include <QLineSeries>
#include <QMouseEvent>
#include <QPainter>
#include <QValueAxis>
#include <QWindow>

namespace {

/// How far back the frame pacing view looks.
constexpr double PacingWindowMs = 10000.0;

/// A Switch 2 game's present-to-present intervals (ms) over the last `window_ms`, oldest first.
/// Empty for other games, which don't fill the present history, and when the newest present is
/// more than two seconds old (paused or loading): the overlay then shows its average view.
std::vector<double> RecentPresentIntervals(double window_ms) {
    std::vector<double> intervals;
    u64 last = Core::g_nx2_present_count.load(std::memory_order_acquire);
    if (last >= 1 && Core::Nx2PresentTime(last).load(std::memory_order_relaxed) == 0) {
        --last; // the newest present may not have stored its time yet
    }
    if (last < 2) {
        return intervals;
    }
    u64 newer = Core::Nx2PresentTime(last).load(std::memory_order_relaxed);
    const u64 now = static_cast<u64>(
        std::chrono::steady_clock::now().time_since_epoch() / std::chrono::nanoseconds{1});
    if (newer == 0 || now - newer > 2'000'000'000ULL) {
        return intervals;
    }
    const u64 first = last > Core::Nx2PresentHistory - 1 ? last - (Core::Nx2PresentHistory - 1) : 1;
    double total = 0.0;
    for (u64 number = last - 1; number >= first && total < window_ms; --number) {
        const u64 older = Core::Nx2PresentTime(number).load(std::memory_order_relaxed);
        if (older == 0 || older >= newer) {
            break;
        }
        const double ms = static_cast<double>(newer - older) / 1e6;
        intervals.push_back(ms);
        total += ms;
        newer = older;
        if (number == first) {
            break;
        }
    }
    std::ranges::reverse(intervals);
    return intervals;
}

} // Anonymous namespace

// TODO(crueter): Reset samples when user changes turbo, slow, etc.
PerformanceOverlay::PerformanceOverlay(MainWindow* parent)
    : QWidget(parent), m_mainWindow{parent}, ui(new Ui::PerformanceOverlay) {
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    raise();

    // chart setup
    m_fpsSeries = new QLineSeries(this);

    QPen pen(Qt::red);
    pen.setWidth(2);
    m_fpsSeries->setPen(pen);

    m_fpsChart = new QChart;
    m_fpsChart->addSeries(m_fpsSeries);
    // The 30 fps frame time, under the per-frame view.
    m_targetSeries = new QLineSeries(this);
    QPen target_pen(QColor(200, 200, 200));
    target_pen.setWidth(1);
    target_pen.setStyle(Qt::DashLine);
    m_targetSeries->setPen(target_pen);
    m_fpsChart->addSeries(m_targetSeries);
    m_fpsChart->legend()->hide();
    m_fpsChart->setBackgroundBrush(Qt::black);
    m_fpsChart->setBackgroundVisible(true);
    m_fpsChart->layout()->setContentsMargins(2, 2, 2, 2);
    m_fpsChart->setMargins(QMargins{4, 4, 4, 4});

    // axes
    m_fpsX = new QValueAxis(this);
    m_fpsX->setRange(0, NUM_FPS_SAMPLES);
    m_fpsX->setVisible(false);

    m_fpsY = new QValueAxis(this);
    m_fpsY->setRange(0, 60);
    m_fpsY->setLabelFormat(QStringLiteral("%d"));
    m_fpsY->setLabelsColor(Qt::white);

    QFont axisFont = m_fpsY->labelsFont();
    axisFont.setPixelSize(10);
    m_fpsY->setLabelsFont(axisFont);
    m_fpsY->setTickCount(3);

    // gray-ish label w/ white lines
    m_fpsY->setLabelsVisible(true);
    m_fpsY->setGridLineColor(QColor(50, 50, 50));
    m_fpsY->setLinePenColor(Qt::white);

    m_fpsChart->addAxis(m_fpsX, Qt::AlignBottom);
    m_fpsChart->addAxis(m_fpsY, Qt::AlignLeft);
    m_fpsSeries->attachAxis(m_fpsX);
    m_fpsSeries->attachAxis(m_fpsY);
    m_targetSeries->attachAxis(m_fpsX);
    m_targetSeries->attachAxis(m_fpsY);

    // chart view
    m_fpsChartView = new QChartView(m_fpsChart, this);
    m_fpsChartView->setRenderHint(QPainter::Antialiasing);
    m_fpsChartView->setMinimumHeight(100);

    ui->verticalLayout->addWidget(m_fpsChartView, 1);

    // thanks Debian.
    QFont font = ui->fps->font();
    font.setWeight(QFont::DemiBold);

    ui->fps->setFont(font);
    ui->frametime->setFont(font);

    // pos/stats
    resetPosition(m_mainWindow->pos());
    connect(parent, &MainWindow::positionChanged, this, &PerformanceOverlay::resetPosition);
    connect(m_mainWindow, &MainWindow::statsUpdated, this, &PerformanceOverlay::updateStats);
}

PerformanceOverlay::~PerformanceOverlay() {
    delete ui;
}

void PerformanceOverlay::resetPosition(const QPoint& _) {
    auto pos = m_mainWindow->pos();
    move(pos.x() + m_offset.x(), pos.y() + m_offset.y());
}

void PerformanceOverlay::updateStats(const Core::PerfStatsResults& results,
                                     const VideoCore::ShaderNotify& shaders) {
    if (ShowFramePacing()) {
        return;
    }
    m_targetSeries->clear();
    auto fps = results.average_game_fps;
    if (!std::isnan(fps)) {
        // don't sample measurements < 3 fps because they are probably outliers or freezes
        static constexpr double FPS_SAMPLE_THRESHOLD = 3.0;

        QString fpsText = tr("%1 fps").arg(std::round(fps), 0, 'f', 0);
        // if (!m_fpsSuffix.isEmpty()) fpsText = fpsText % QStringLiteral(" (%1)").arg(m_fpsSuffix);
        ui->fps->setText(fpsText);

        // sampling
        if (fps > FPS_SAMPLE_THRESHOLD) {
            m_fpsSamples.push_back(fps);
            m_fpsPoints.push_back(QPointF{m_xPos++, fps});
        }

        if (m_fpsSamples.size() > NUM_FPS_SAMPLES) {
            m_fpsSamples.pop_front();
            m_fpsPoints.pop_front();
        }

        // For the average only go back 10 samples max
        if (m_fpsSamples.size() >= 2) {
            const int back_search = std::min(size_t(10), m_fpsSamples.size() - 1);
            double sum = std::accumulate(m_fpsSamples.end() - back_search, m_fpsSamples.end(), 0.0);
            double avg = sum / back_search;

            ui->fps_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 0));
        }

        // chart it :)
        if (!m_fpsPoints.empty()) {
            auto [min_it, max_it] = std::minmax_element(m_fpsSamples.begin(), m_fpsSamples.end());
            double min_fps = *min_it;
            double max_fps = *max_it;

            ui->fps_min->setText(tr("Min: %1").arg(min_fps, 0, 'f', 0));
            ui->fps_max->setText(tr("Max: %1").arg(max_fps, 0, 'f', 0));

            m_fpsSeries->replace(QList<QPointF>(m_fpsPoints.begin(), m_fpsPoints.end()));

            qreal x_min = std::max(0.0, m_xPos - NUM_FPS_SAMPLES);
            qreal x_max = std::max(qreal(10), m_xPos);
            m_fpsX->setRange(x_min, x_max);
            m_fpsY->setRange(0.0, max_fps);
            m_fpsY->setLabelFormat(QStringLiteral("%d"));
            ui->fps_max->setStyleSheet(QString{});
        }
    }

    auto ft = results.frametime;
    if (!std::isnan(ft)) {
        // don't sample measurements > 500 ms because they are probably outliers
        static constexpr double FT_SAMPLE_THRESHOLD = 500.0;

        double ft_ms = results.frametime * 1000.0;
        ui->frametime->setText(tr("%1 ms").arg(ft_ms, 0, 'f', 2));

        // sampling
        if (ft_ms <= FT_SAMPLE_THRESHOLD)
            m_frametimeSamples.push_back(ft_ms);

        if (m_frametimeSamples.size() > NUM_FRAMETIME_SAMPLES)
            m_frametimeSamples.pop_front();

        if (!m_frametimeSamples.empty()) {
            auto [min_it, max_it] =
                std::minmax_element(m_frametimeSamples.begin(), m_frametimeSamples.end());
            ui->ft_min->setText(tr("Min: %1").arg(*min_it, 0, 'f', 1));
            ui->ft_max->setText(tr("Max: %1").arg(*max_it, 0, 'f', 1));
        }

        // For the average only go back 10 samples max
        if (m_frametimeSamples.size() >= 2) {
            const int back_search = std::min(size_t(10), m_frametimeSamples.size() - 1);
            double sum = std::accumulate(m_frametimeSamples.end() - back_search,
                                         m_frametimeSamples.end(), 0.0);
            double avg = sum / back_search;

            ui->ft_avg->setText(tr("Avg: %1").arg(avg, 0, 'f', 1));
        }
    }
}

bool PerformanceOverlay::ShowFramePacing() {
    const std::vector<double> intervals = RecentPresentIntervals(PacingWindowMs);
    if (intervals.size() < 10) {
        return false;
    }
    const Common::FramePacing pacing = Common::ComputeFramePacing(intervals);
    ui->fps->setText(tr("%1 fps").arg(pacing.fps, 0, 'f', 0));
    ui->fps_avg->setText(tr("Avg: %1").arg(pacing.average_fps, 0, 'f', 0));
    ui->fps_min->setText(tr("1% low: %1").arg(pacing.low_1pct_fps, 0, 'f', 0));
    ui->fps_max->setText(tr("Stutters: %1").arg(pacing.stutters));
    ui->fps_max->setStyleSheet(pacing.stutters > 0 ? QStringLiteral("color: #ff8080;")
                                                   : QString{});
    ui->frametime->setText(tr("%1 ms").arg(pacing.median_ms, 0, 'f', 1));
    ui->ft_min->setText(tr("Min: %1").arg(pacing.min_ms, 0, 'f', 1));
    ui->ft_avg->setText(tr("p99: %1").arg(pacing.p99_ms, 0, 'f', 1));
    ui->ft_max->setText(tr("Max: %1").arg(pacing.max_ms, 0, 'f', 1));

    // Each frame's time over the window, against the 30 fps line: a stall stands out as a spike.
    QList<QPointF> points;
    points.reserve(static_cast<qsizetype>(intervals.size()));
    for (size_t index = 0; index < intervals.size(); ++index) {
        points.append(QPointF{static_cast<qreal>(index), intervals[index]});
    }
    m_fpsSeries->replace(points);
    const qreal last_x = static_cast<qreal>(intervals.size() - 1);
    m_targetSeries->replace(QList<QPointF>{{0.0, 1000.0 / 30.0}, {last_x, 1000.0 / 30.0}});
    m_fpsX->setRange(0.0, last_x);
    m_fpsY->setRange(0.0, std::clamp(pacing.max_ms * 1.1, 50.0, 250.0));
    m_fpsY->setLabelFormat(QStringLiteral("%d ms"));
    return true;
}

void PerformanceOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(m_background);
    painter.setPen(Qt::NoPen);

    painter.drawRoundedRect(rect(), 10.0, 10.0);
}

void PerformanceOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    // Wayland ignores move() on top-level windows, so the overlay sat where the compositor put it:
    // let the compositor drag it instead.
    if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland")) && windowHandle() &&
        windowHandle()->startSystemMove()) {
        return;
    }
    m_drag_start_pos = event->pos();
}

void PerformanceOverlay::mouseMoveEvent(QMouseEvent* event) {
    // drag
    if (event->buttons() & Qt::LeftButton) {
        QPoint new_global_pos = event->globalPosition().toPoint() - m_drag_start_pos;
        m_offset = new_global_pos - m_mainWindow->pos();
        move(new_global_pos);
    }
}

void PerformanceOverlay::closeEvent(QCloseEvent* event) {
    emit closed();
}
