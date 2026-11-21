// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <QDialog>
#include <QElapsedTimer>
#include <QJsonObject>

#include "common/ai_optimize_policy.h"
#include "common/common_types.h"
#include "common/performance_presets.h"
#include "yuzu/ai_request.h"

class AiProviderPanel;
class ButtonSpinner;
class QCheckBox;
class QGroupBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QVBoxLayout;

namespace Core {
class System;
}

class AiOptimizeDialog final : public QDialog {
    Q_OBJECT

public:
    AiOptimizeDialog(QWidget* parent, Core::System& system, Settings::HostProfile host,
                     std::string config_name, u64 program_id, QString game_name,
                     std::function<void()> stop_game, bool global_scope = false,
                     QString global_undo_path = {}, std::function<void()> save_global = {});
    ~AiOptimizeDialog() override;

private:
    struct ProposedChange {
        std::string key;
        std::string value;
        QString reason;
        std::string previous;
        bool previous_global = true;
        QCheckBox* checkbox = nullptr;
    };

    void PrepareRequest();
    void Analyze();
    void HandleReply(const AiReply& reply);
    void ClearProposal();
    void ShowPlan(const QJsonObject& plan);
    void Apply();
    void Undo();
    QJsonObject Diagnostic() const;
    bool GameRunning() const;
    QString UndoPath() const;

    /// The last 10 s of play, if the game runs and presented enough frames.
    std::optional<Common::AiOptimize::Measurement> MeasureNow() const;
    /// Measures now and compares with what the last AI change recorded before it was applied.
    void MeasureResult();
    void KeepResult();
    void UpdateMeasurementPanel();
    QJsonObject ReadUndoRecord() const;
    bool WriteUndoRecord(const QJsonObject& record) const;

    Core::System& system;
    Settings::HostProfile host;
    std::string config_name;
    u64 program_id;
    QString game_name;
    std::function<void()> stop_game;
    bool global_scope = false;
    QString global_undo_path;
    std::function<void()> save_global;
    std::vector<ProposedChange> changes;
    QJsonObject prepared_diagnostic;
    bool developer_mode = false;
    QNetworkAccessManager* network;
    AiProviderPanel* provider_panel;
    AiRequest* request;
    ButtonSpinner* analyze_spinner;
    QPlainTextEdit* complaint;
    QPlainTextEdit* data_preview;
    QLabel* status;
    QLabel* explanation;
    QVBoxLayout* proposals;
    QPushButton* analyze_button;
    QPushButton* refresh_button;
    QPushButton* apply_button;
    QPushButton* undo_button;
    QGroupBox* measurement_box;
    QLabel* measurement_text;
    QPushButton* measure_button;
    QPushButton* keep_button;
};
