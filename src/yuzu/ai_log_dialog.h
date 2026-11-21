// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QStringList>

#include "common/log_diagnosis.h"

class AiProviderPanel;
class AiRequest;
class ButtonSpinner;
class QCheckBox;
class QComboBox;
class QLabel;
class QNetworkAccessManager;
class QPlainTextEdit;
class QPushButton;
struct AiReply;

/// Help > Explain the last problem: reads Kaichii's log for the causes a player can act on, with
/// no AI and nothing sent. When it finds none, or the player wants more, it can ask the AI with
/// the lines it shows (the user's paths taken out), and draft a bug report for GitHub when the
/// answer is an emulator bug.
class AiLogDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AiLogDialog(QWidget* parent);

private:
    void LoadLog();
    void Ask();
    void HandleReply(const AiReply& reply);
    void OpenBugReport();
    [[nodiscard]] QString LogPath(bool previous) const;

    Common::LogDiagnosis::Analysis analysis;
    QJsonObject answer;
    QNetworkAccessManager* network;
    QComboBox* log_choice;
    QLabel* findings;
    QPlainTextEdit* description;
    QCheckBox* show_sent;
    QPlainTextEdit* sent_preview;
    AiProviderPanel* provider_panel;
    AiRequest* request;
    QPushButton* ask_button;
    ButtonSpinner* ask_spinner;
    QLabel* status;
    QLabel* answer_text;
    QPushButton* bug_button;
};
