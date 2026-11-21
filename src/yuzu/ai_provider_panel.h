// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include <QString>
#include <QWidget>

class ButtonSpinner;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;

/// The AI provider, model and API key every AI feature asks for: the model list with its notes
/// and "Load from provider", the link to the provider's key page, and the key remembered in the
/// system's credential store. The last provider and model are kept in the UI settings.
class AiProviderPanel final : public QWidget {
    Q_OBJECT

public:
    AiProviderPanel(QNetworkAccessManager* network, QWidget* parent);
    ~AiProviderPanel() override;

    [[nodiscard]] bool Anthropic() const;
    [[nodiscard]] QString ProviderName() const;
    [[nodiscard]] QString Model() const;
    [[nodiscard]] QString Key() const;
    /// Whether a model and a key are filled in.
    [[nodiscard]] bool Ready() const;
    /// Whether the model list is loading.
    [[nodiscard]] bool Busy() const;

    /// Locks the controls while a request runs.
    void SetLocked(bool locked);
    /// Keeps the model, and the key if the box is ticked (or forgets it if not), after a request
    /// that succeeded.
    void RequestSucceeded();

signals:
    void StatusChanged(const QString& text);
    /// The model list started or stopped loading.
    void BusyChanged(bool busy);

private:
    void UpdateProvider();
    void UpdateModelNote();
    void LoadSavedKey();
    void ForgetKey();
    void ListModels();
    void HandleModelList(QNetworkReply* reply);
    [[nodiscard]] std::string_view ProviderId() const;

    QNetworkAccessManager* network;
    QNetworkReply* models_reply = nullptr;
    QComboBox* provider;
    QComboBox* model;
    QLabel* model_note;
    QPushButton* list_models_button;
    ButtonSpinner* list_spinner;
    QLineEdit* api_key;
    QLabel* key_link;
    QCheckBox* remember_key;
    QPushButton* forget_key_button;
    bool key_saved = false;
};
