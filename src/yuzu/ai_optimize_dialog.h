// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <QDialog>
#include <QElapsedTimer>
#include <QJsonObject>

#include "common/common_types.h"
#include "common/performance_presets.h"

class QCheckBox;
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

    void UpdateProvider();
    void UpdateModelNote();
    void LoadSavedKey();
    void ForgetKey();
    void ListModels();
    void HandleModelList(QNetworkReply* reply);
    void RememberKeyAfterSuccess();
    bool Anthropic() const;
    std::string_view ProviderId() const;
    void PrepareRequest();
    /// Shows a spinner and the time spent on `button` while a request runs; `text` gives its label
    /// for the elapsed seconds.
    void StartBusy(QPushButton* button, std::function<QString(qint64)> text);
    void StopBusy();
    void UpdateBusy();
    void ClearProposal();
    void Analyze();
    void HandleReply(QNetworkReply* reply);
    void ShowPlan(const QJsonObject& plan);
    void Apply();
    void Undo();
    QJsonObject Diagnostic() const;
    bool GameRunning() const;
    QString UndoPath() const;

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
    QNetworkAccessManager* network;
    QNetworkReply* active_reply = nullptr;
    QNetworkReply* models_reply = nullptr;
    QComboBox* provider;
    QComboBox* model;
    QPushButton* list_models_button;
    QLabel* model_note;
    QLineEdit* api_key;
    QCheckBox* remember_key;
    QPushButton* forget_key_button;
    bool key_saved = false;
    QTimer* busy_timer;
    QElapsedTimer busy_clock;
    QPushButton* busy_button = nullptr;
    QString busy_restore_text;
    std::function<QString(qint64)> busy_text;
    int busy_frame = 0;
    bool developer_mode = false;
    QPlainTextEdit* complaint;
    QPlainTextEdit* data_preview;
    QLabel* status;
    QLabel* explanation;
    QVBoxLayout* proposals;
    QPushButton* analyze_button;
    QPushButton* refresh_button;
    QPushButton* apply_button;
    QPushButton* undo_button;
};
