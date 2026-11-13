// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/auto_optimize_dialog.h"

#include <algorithm>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "common/frame_pacing.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/nx2_present_intervals.h"
#include "qt_common/config/qt_config.h"
#include "video_core/gpu.h"
#include "video_core/pipeline_build_stats.h"
#include "video_core/renderer_base.h"
#include "video_core/video_memory_status.h"

namespace {
using namespace Common::AutoOptimize;

constexpr int BaselineSeconds = 15;
/// Frame generation needs a few frames to build its chain and warm up.
constexpr int SettleSeconds = 4;
constexpr int TrialSeconds = 15;
constexpr int TotalSeconds = BaselineSeconds + SettleSeconds + TrialSeconds;
} // Anonymous namespace

AutoOptimizeDialog::AutoOptimizeDialog(QWidget* parent, Core::System& system_,
                                       std::string config_name_)
    : QDialog(parent), system{system_}, config_name{std::move(config_name_)} {
    setWindowTitle(tr("Optimize for This Game"));
    setMinimumWidth(520);

    text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(
        tr("<p>Lichii measures the game as it runs now, tries frame generation for a few "
           "seconds, and then proposes changes. Nothing is changed until you accept.</p>"
           "<p><b>Before you start:</b> stand somewhere demanding (outdoors, with the camera "
           "still) and don't pause or open menus for about %1 seconds.</p>")
            .arg(TotalSeconds));
    progress = new QProgressBar(this);
    progress->setRange(0, TotalSeconds);
    progress->setVisible(false);
    choices_layout = new QVBoxLayout;

    start_button = new QPushButton(tr("Start"), this);
    apply_button = new QPushButton(tr("Apply"), this);
    apply_button->setVisible(false);
    close_button = new QPushButton(tr("Close"), this);
    auto* const buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(start_button);
    buttons->addWidget(apply_button);
    buttons->addWidget(close_button);

    auto* const layout = new QVBoxLayout(this);
    layout->addWidget(text);
    layout->addWidget(progress);
    layout->addLayout(choices_layout);
    layout->addLayout(buttons);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &AutoOptimizeDialog::Tick);
    connect(start_button, &QPushButton::clicked, this, &AutoOptimizeDialog::Start);
    connect(apply_button, &QPushButton::clicked, this, &AutoOptimizeDialog::Apply);
    connect(close_button, &QPushButton::clicked, this, &QDialog::reject);
}

AutoOptimizeDialog::~AutoOptimizeDialog() {
    // Closed during the trial: leave frame generation as it was.
    if (trial_switched_on) {
        Settings::values.frame_gen.SetValue(false);
    }
}

Sample AutoOptimizeDialog::TakeSample(double seconds) const {
    const auto intervals = Core::RecentPresentIntervals(seconds * 1000.0);
    if (intervals.size() < 10) {
        return {};
    }
    const Common::FramePacing pacing = Common::ComputeFramePacing(intervals, seconds * 1000.0);
    return {.seconds = seconds,
            .fps = pacing.average_fps,
            .low_1pct_fps = pacing.low_1pct_fps,
            .stutters = pacing.stutters};
}

u64 AutoOptimizeDialog::GeneratedFrames() const {
    return system.IsPoweredOn() ? system.GPU().Renderer().GetFrameGenStatus().generated_frames : 0;
}

bool AutoOptimizeDialog::FrameGenerationRuns() const {
    return system.IsPoweredOn() && system.GPU().Renderer().GetFrameGenStatus().state ==
                                       VideoCore::FrameGenState::Running;
}

void AutoOptimizeDialog::Start() {
    start_button->setEnabled(false);
    progress->setVisible(true);
    progress->setValue(0);
    measurements = {};
    measurements.frame_gen_enabled = Settings::values.frame_gen.GetValue();
    measurements.resolution = static_cast<u32>(Settings::values.resolution_setup.GetValue());
    measurements.lowest_resolution = static_cast<u32>(Settings::ResolutionSetup::Res1_2X);
    measurements.skips_unbuilt_draws =
        Settings::values.native_async_draws.GetValue() != Settings::NativeAsyncDraws::Wait;
    measurements.preloads_shaders =
        Settings::values.shader_preload_memory.GetValue() != Settings::ShaderPreloadMemory::Off;
    built_late_at_start = VideoCommon::g_cached_pipelines_built_late.load();
    spilled_at_start = VideoCommon::g_video_memory_status.host_used.load();
    EnterPhase(Phase::Baseline);
    timer->start();
}

void AutoOptimizeDialog::EnterPhase(Phase next) {
    phase = next;
    elapsed = 0;
    switch (phase) {
    case Phase::Baseline:
        text->setText(tr("Measuring the game as it is…"));
        break;
    case Phase::Settle:
        Settings::values.frame_gen.SetValue(true);
        trial_switched_on = true;
        text->setText(tr("Trying frame generation…"));
        break;
    case Phase::Trial:
        generated_at_trial_start = GeneratedFrames();
        break;
    default:
        break;
    }
}

void AutoOptimizeDialog::Tick() {
    if (!system.IsPoweredOn()) {
        timer->stop();
        text->setText(tr("The game stopped. Start it again to optimize."));
        return;
    }
    ++elapsed;
    progress->setValue(progress->value() + 1);
    switch (phase) {
    case Phase::Baseline:
        if (elapsed < BaselineSeconds) {
            return;
        }
        measurements.base = TakeSample(BaselineSeconds);
        measurements.cached_pipelines_built_late =
            VideoCommon::g_cached_pipelines_built_late.load() - built_late_at_start;
        if (const u64 budget = VideoCommon::g_video_memory_status.budget.load(); budget != 0) {
            measurements.memory =
                // The lower of two readings: loading peaks for a moment.
                VideoMemory{VideoCommon::g_video_memory_status.used.load(), budget,
                            (std::min)(spilled_at_start,
                                       VideoCommon::g_video_memory_status.host_used.load())};
        }
        if (measurements.base.fps == 0.0) {
            timer->stop();
            text->setText(tr("No frames could be measured: this works with Switch 2 games "
                             "while they are running and not paused."));
            start_button->setEnabled(true);
            return;
        }
        if (measurements.frame_gen_enabled) {
            ShowResults();
        } else {
            EnterPhase(Phase::Settle);
        }
        return;
    case Phase::Settle:
        if (elapsed < SettleSeconds) {
            return;
        }
        if (FrameGenerationRuns()) {
            EnterPhase(Phase::Trial);
        } else {
            EndTrial();
            ShowResults();
        }
        return;
    case Phase::Trial:
        if (elapsed < TrialSeconds) {
            return;
        }
        measurements.frame_gen_trial = TakeSample(TrialSeconds);
        measurements.generated_fps =
            static_cast<double>(GeneratedFrames() - generated_at_trial_start) / TrialSeconds;
        EndTrial();
        ShowResults();
        return;
    default:
        return;
    }
}

void AutoOptimizeDialog::EndTrial() {
    if (trial_switched_on) {
        Settings::values.frame_gen.SetValue(false);
        trial_switched_on = false;
    }
}

void AutoOptimizeDialog::ShowResults() {
    timer->stop();
    phase = Phase::Results;
    progress->setVisible(false);
    start_button->setVisible(false);
    recommendations = Recommend(measurements);

    QString summary = tr("<p><b>Measured:</b> %1 fps (slowest 1 %: %2 fps), stutters in %4 s: %3.")
                          .arg(measurements.base.fps, 0, 'f', 1)
                          .arg(measurements.base.low_1pct_fps, 0, 'f', 1)
                          .arg(measurements.base.stutters)
                          .arg(measurements.base.seconds, 0, 'f', 0);
    if (measurements.memory) {
        summary += tr("<br>Video memory: %1 of %2 MiB, and %3 MiB in system memory.")
                       .arg(measurements.memory->used >> 20)
                       .arg(measurements.memory->budget >> 20)
                       .arg(measurements.memory->spilled >> 20);
    }
    if (measurements.frame_gen_trial) {
        summary += tr("<br>With frame generation: %1 fps from the game plus %2 generated, "
                      "%3 shown.")
                       .arg(measurements.frame_gen_trial->fps, 0, 'f', 1)
                       .arg(measurements.generated_fps, 0, 'f', 1)
                       .arg(measurements.frame_gen_trial->fps + measurements.generated_fps, 0,
                            'f', 0);
    }
    summary += QStringLiteral("</p>");
    switch (JudgeFrameGeneration(measurements)) {
    case FrameGenerationVerdict::TooFewFrames:
        summary += tr("<p>Frame generation is left off: under %1 fps it shows artefacts and "
                      "the controls feel slow.</p>")
                       .arg(MinFpsForFrameGeneration, 0, 'f', 0);
        break;
    case FrameGenerationVerdict::CostsTooMuch:
        summary += tr("<p>Frame generation is left off: it took too many of the game's own "
                      "frames, so the graphics card has no room for it here.</p>");
        break;
    case FrameGenerationVerdict::NotTried:
    case FrameGenerationVerdict::NotGenerating:
        summary += tr("<p>Frame generation is left off: it could not run on this system.</p>");
        break;
    default:
        break;
    }
    if (recommendations.empty()) {
        summary += tr("<p>The current settings already fit this game here: nothing to change.</p>");
        text->setText(summary);
        return;
    }
    summary += tr("<p><b>Proposed changes</b> (saved for this game only):</p>");
    text->setText(summary);
    for (const Recommendation& item : recommendations) {
        QString label;
        switch (item.action) {
        case Action::EnableFrameGeneration:
            label = tr("Turn on frame generation: smoother motion at almost no cost here");
            break;
        case Action::LowerResolution:
            label = tr("Lower the resolution one step: the game's images don't fit in video memory "
                       "(takes effect the next time the game starts)");
            break;
        case Action::SkipUnbuiltDraws:
            label = tr("Skip objects while their shaders build: fewer stutters, objects may "
                       "appear a moment late");
            break;
        case Action::PreloadShaders:
            label = tr("Preload shaders from earlier sessions in system memory: fewer stutters "
                       "in areas you come back to (takes effect the next time the game starts)");
            break;
        }
        auto* const box = new QCheckBox(label, this);
        box->setChecked(true);
        choices_layout->addWidget(box);
        choices.push_back(box);
    }
    apply_button->setVisible(true);
}

void AutoOptimizeDialog::Apply() {
    // Reads the game's settings file as the Properties dialog does, then saves it with the changes.
    QtConfig game_config(config_name, Config::ConfigType::PerGameConfig);
    for (size_t index = 0; index < recommendations.size(); ++index) {
        if (!choices[index]->isChecked()) {
            continue;
        }
        switch (recommendations[index].action) {
        case Action::EnableFrameGeneration:
            Settings::values.frame_gen.SetGlobal(false);
            Settings::values.frame_gen.SetValue(true);
            Settings::values.frame_gen_type.SetGlobal(false);
            Settings::values.frame_gen_type.SetValue(Settings::FrameGenType::Fsr3);
            break;
        case Action::LowerResolution: {
            // Only read when a game boots, so the running game keeps its resolution.
            const auto current = Settings::values.resolution_setup.GetValue();
            Settings::values.resolution_setup.SetGlobal(false);
            Settings::values.resolution_setup.SetValue(
                static_cast<Settings::ResolutionSetup>(static_cast<u32>(current) - 1));
            break;
        }
        case Action::SkipUnbuiltDraws:
            Settings::values.native_async_draws.SetGlobal(false);
            Settings::values.native_async_draws.SetValue(Settings::NativeAsyncDraws::Bounded);
            break;
        case Action::PreloadShaders:
            Settings::values.shader_preload_memory.SetGlobal(false);
            Settings::values.shader_preload_memory.SetValue(Settings::ShaderPreloadMemory::Auto);
            break;
        }
    }
    game_config.SaveAllValues();
    accept();
}
