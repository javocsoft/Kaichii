// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <QDialog>

#include "common/auto_optimize.h"

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
class QVBoxLayout;

namespace Core {
class System;
}

/// "Optimize for this game": measures the running game, tries frame generation for a few seconds,
/// and proposes changes the user accepts or discards. Accepted ones are saved as the game's own
/// settings. The decisions are Common::AutoOptimize's; this only measures and shows them.
class AutoOptimizeDialog final : public QDialog {
    Q_OBJECT

public:
    /// @param config_name The game's per-game configuration name (its title id, or file name).
    explicit AutoOptimizeDialog(QWidget* parent, Core::System& system, std::string config_name);
    ~AutoOptimizeDialog() override;

private:
    enum class Phase { Intro, Baseline, Settle, Trial, Results };

    void Start();
    void Tick();
    void EnterPhase(Phase next);
    void EndTrial();
    void ShowResults();
    void Apply();
    [[nodiscard]] Common::AutoOptimize::Sample TakeSample(double seconds) const;
    [[nodiscard]] u64 GeneratedFrames() const;
    [[nodiscard]] bool FrameGenerationRuns() const;

    Core::System& system;
    std::string config_name;

    Phase phase = Phase::Intro;
    int elapsed = 0; ///< Seconds in the current phase
    bool trial_switched_on = false;
    u64 generated_at_trial_start = 0;
    u64 spilled_at_start = 0;
    u64 built_late_at_start = 0;
    Common::AutoOptimize::Measurements measurements;
    std::vector<Common::AutoOptimize::Recommendation> recommendations;
    std::vector<QCheckBox*> choices;

    QLabel* text;
    QProgressBar* progress;
    QVBoxLayout* choices_layout;
    QPushButton* start_button;
    QPushButton* apply_button;
    QPushButton* close_button;
    QTimer* timer;
};
