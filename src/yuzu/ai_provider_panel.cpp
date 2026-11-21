// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/ai_provider_panel.h"

#include <algorithm>
#include <functional>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>

#include "common/ai_optimize_policy.h"
#include "qt_common/config/uisettings.h"
#include "yuzu/ai_credentials.h"
#include "yuzu/button_spinner.h"

namespace {

// The catalog's model notes, translated.
QString ModelNote(std::string_view note) {
    if (note == "recommended") {
        return QCoreApplication::translate("AiOptimizeDialog", "Recommended");
    }
    if (note == "most capable") {
        return QCoreApplication::translate("AiOptimizeDialog", "Most capable");
    }
    if (note == "newest") {
        return QCoreApplication::translate("AiOptimizeDialog", "Newest");
    }
    if (note == "balance of cost and capability") {
        return QCoreApplication::translate("AiOptimizeDialog", "Balance of cost and capability");
    }
    if (note == "lowest cost") {
        return QCoreApplication::translate("AiOptimizeDialog", "Lowest cost");
    }
    if (note == "faster, lower cost") {
        return QCoreApplication::translate("AiOptimizeDialog", "Faster, lower cost");
    }
    if (note == "most capable, highest cost") {
        return QCoreApplication::translate("AiOptimizeDialog", "Most capable, highest cost");
    }
    return QString::fromLatin1(note.data(), note.size());
}

} // Anonymous namespace

AiProviderPanel::AiProviderPanel(QNetworkAccessManager* network_, QWidget* parent)
    : QWidget(parent), network{network_} {
    auto* const form = new QFormLayout(this);
    form->setContentsMargins(0, 0, 0, 0);
    provider = new QComboBox(this);
    provider->addItem(QStringLiteral("OpenAI"));
    provider->addItem(QStringLiteral("Anthropic"));
    model = new QComboBox(this);
    model->setEditable(true);
    model->setInsertPolicy(QComboBox::NoInsert);
    model->setToolTip(tr("Pick one of the provider's main models, or type another model name."));
    list_models_button = new QPushButton(tr("Load from provider"), this);
    list_models_button->setToolTip(
        tr("Ask the provider which models your key can use. Needs your API key."));
    list_spinner = new ButtonSpinner(list_models_button);
    auto* const model_row = new QHBoxLayout;
    model_row->addWidget(model, 1);
    model_row->addWidget(list_models_button);
    model_note = new QLabel(this);
    model_note->setEnabled(false); // Greyed: a hint, not a control
    api_key = new QLineEdit(this);
    api_key->setEchoMode(QLineEdit::Password);
    api_key->setPlaceholderText(tr("Your API key"));
    key_link = new QLabel(this);
    key_link->setTextFormat(Qt::RichText);
    key_link->setTextInteractionFlags(Qt::TextBrowserInteraction);
    key_link->setOpenExternalLinks(true);
    remember_key = new QCheckBox(tr("Remember this key in the system's credential store"), this);
    forget_key_button = new QPushButton(tr("Forget saved key"), this);
    forget_key_button->setEnabled(false);
    if (!AiCredentials::Available()) {
        remember_key->setEnabled(false);
        remember_key->setToolTip(tr("This system has no credential store Kaichii can use, so the "
                                    "key stays in memory only while this window is open."));
    } else {
        remember_key->setToolTip(tr("Saved by the operating system (Keychain, Credential Manager "
                                    "or Secret Service), never in Kaichii's settings files. "
                                    "It is saved once a request succeeds."));
    }
    auto* const remember_row = new QHBoxLayout;
    remember_row->addWidget(remember_key, 1);
    remember_row->addWidget(forget_key_button);
    form->addRow(tr("Provider:"), provider);
    form->addRow(tr("Model:"), model_row);
    form->addRow(QString{}, model_note);
    form->addRow(tr("API key:"), api_key);
    form->addRow(QString{}, key_link);
    form->addRow(QString{}, remember_row);

    connect(provider, &QComboBox::currentIndexChanged, this, &AiProviderPanel::UpdateProvider);
    connect(list_models_button, &QPushButton::clicked, this, &AiProviderPanel::ListModels);
    connect(model, &QComboBox::currentTextChanged, this, &AiProviderPanel::UpdateModelNote);
    connect(forget_key_button, &QPushButton::clicked, this, &AiProviderPanel::ForgetKey);
    {
        const QSignalBlocker blocker(provider);
        provider->setCurrentIndex(UISettings::values.ai_assistant_provider.GetValue() == 1 ? 1 : 0);
    }
    UpdateProvider();
}

AiProviderPanel::~AiProviderPanel() {
    if (models_reply) {
        models_reply->disconnect(this);
        models_reply->abort();
    }
    api_key->clear();
}

bool AiProviderPanel::Anthropic() const {
    return provider->currentIndex() == 1;
}

QString AiProviderPanel::ProviderName() const {
    return provider->currentText();
}

QString AiProviderPanel::Model() const {
    return model->currentText().trimmed();
}

QString AiProviderPanel::Key() const {
    return api_key->text().trimmed();
}

bool AiProviderPanel::Ready() const {
    return !Model().isEmpty() && !Key().isEmpty();
}

QString AiProviderPanel::CheckMissing() {
    const bool no_model = Model().isEmpty();
    const bool no_key = Key().isEmpty();
    if (no_model) {
        model->setFocus();
    } else if (no_key) {
        api_key->setFocus();
    }
    if (no_model && no_key) {
        return tr("Choose a model and enter your API key.");
    }
    if (no_model) {
        return tr("Choose a model.");
    }
    if (no_key) {
        return tr("Enter your API key.");
    }
    return {};
}

bool AiProviderPanel::Busy() const {
    return models_reply != nullptr;
}

std::string_view AiProviderPanel::ProviderId() const {
    return Anthropic() ? "anthropic" : "openai";
}

void AiProviderPanel::SetLocked(bool locked) {
    provider->setEnabled(!locked && !Busy());
    model->setEnabled(!locked);
    list_models_button->setEnabled(!locked && !Busy());
    api_key->setReadOnly(locked);
}

void AiProviderPanel::UpdateModelNote() {
    const int index = model->findText(model->currentText());
    const QString note = index < 0 ? QString{} : model->itemData(index, Qt::ToolTipRole).toString();
    model_note->setText(note.isEmpty() ? tr("Model chosen by name or listed by the provider.")
                                       : note);
}

void AiProviderPanel::UpdateProvider() {
    using Common::AiOptimize::Provider;
    const auto which = Anthropic() ? Provider::Anthropic : Provider::OpenAI;
    UISettings::values.ai_assistant_provider = Anthropic() ? 1U : 0U;
    const std::string saved = Anthropic() ? UISettings::values.ai_assistant_anthropic_model.GetValue()
                                          : UISettings::values.ai_assistant_openai_model.GetValue();
    model->clear();
    const auto add = [this](const auto& models) {
        for (const auto& entry : models) {
            const QString id = QString::fromLatin1(entry.id.data(), entry.id.size());
            model->addItem(id, id);
            model->setItemData(model->count() - 1, ModelNote(entry.note), Qt::ToolTipRole);
        }
    };
    if (Anthropic()) {
        add(Common::AiOptimize::AnthropicModels);
    } else {
        add(Common::AiOptimize::OpenAIModels);
    }
    const std::string_view url = Common::AiOptimize::KeyPageUrl(which);
    key_link->setText(tr("Don't have a key? <a href=\"%1\">Create one on the %2 website</a>.")
                          .arg(QString::fromLatin1(url.data(), url.size()), provider->currentText()));
    key_link->setToolTip(QString::fromLatin1(url.data(), url.size()));
    const std::string_view fallback = Common::AiOptimize::DefaultModel(which);
    model->setCurrentText(QString::fromStdString(saved.empty() ? std::string{fallback} : saved));
    UpdateModelNote();
    LoadSavedKey();
}

void AiProviderPanel::LoadSavedKey() {
    api_key->clear();
    key_saved = false;
    remember_key->setChecked(false);
    forget_key_button->setEnabled(false);
    if (!AiCredentials::Available()) {
        return;
    }
    AiCredentials::Result result{};
    const auto key = AiCredentials::Load(ProviderId(), &result);
    if (key) {
        api_key->setText(*key);
        key_saved = true;
        remember_key->setChecked(true);
        forget_key_button->setEnabled(true);
        emit StatusChanged(tr("Using the API key saved for %1.").arg(provider->currentText()));
    } else if (result == AiCredentials::Result::Failed) {
        emit StatusChanged(tr("The system's credential store didn't give the saved key. Enter it "
                              "again, or check that the store is unlocked."));
    } else {
        emit StatusChanged({});
    }
}

void AiProviderPanel::ForgetKey() {
    const auto result = AiCredentials::Forget(ProviderId());
    if (result == AiCredentials::Result::Ok || result == AiCredentials::Result::NotFound) {
        key_saved = false;
        api_key->clear();
        remember_key->setChecked(false);
        forget_key_button->setEnabled(false);
        emit StatusChanged(tr("The saved %1 key was removed from this computer.")
                               .arg(provider->currentText()));
    } else {
        emit StatusChanged(tr("The system's credential store didn't remove the key."));
    }
}

void AiProviderPanel::RequestSucceeded() {
    const std::string used_model = Model().toStdString();
    if (Anthropic()) {
        UISettings::values.ai_assistant_anthropic_model = used_model;
    } else {
        UISettings::values.ai_assistant_openai_model = used_model;
    }
    if (!AiCredentials::Available()) {
        return;
    }
    if (remember_key->isChecked()) {
        if (AiCredentials::Save(ProviderId(), Key()) == AiCredentials::Result::Ok) {
            key_saved = true;
            forget_key_button->setEnabled(true);
        } else {
            emit StatusChanged(tr("The answer is below, but the system's credential store didn't "
                                  "save the key."));
        }
    } else if (key_saved) {
        // Unticking the box means "don't keep it": forget the copy saved earlier.
        ForgetKey();
    }
}

void AiProviderPanel::ListModels() {
    if (models_reply) {
        return;
    }
    if (Key().isEmpty()) {
        emit StatusChanged(tr("Enter your API key to load the provider's models."));
        return;
    }
    QNetworkRequest request(Anthropic()
                                ? QUrl{QStringLiteral("https://api.anthropic.com/v1/models?limit=100")}
                                : QUrl{QStringLiteral("https://api.openai.com/v1/models")});
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    if (Anthropic()) {
        request.setRawHeader("x-api-key", Key().toUtf8());
        request.setRawHeader("anthropic-version", "2026-10-21");
    } else {
        request.setRawHeader("Authorization", "Bearer " + Key().toUtf8());
    }
    emit StatusChanged({});
    models_reply = network->get(request);
    list_models_button->setEnabled(false);
    provider->setEnabled(false);
    list_spinner->Start([](qint64 seconds) { return tr("Loading… %1 s").arg(seconds); });
    emit BusyChanged(true);
    connect(models_reply, &QNetworkReply::finished, this,
            [this, reply = models_reply] { HandleModelList(reply); });
}

void AiProviderPanel::HandleModelList(QNetworkReply* reply) {
    models_reply = nullptr;
    list_spinner->Stop();
    list_models_button->setEnabled(true);
    provider->setEnabled(true);
    emit BusyChanged(false);
    const QByteArray bytes = reply->readAll();
    const auto error = reply->error();
    const QString error_text = reply->errorString();
    reply->deleteLater();
    const QJsonObject response = QJsonDocument::fromJson(bytes).object();
    if (error != QNetworkReply::NoError) {
        const QString message =
            response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        emit StatusChanged(tr("The provider didn't list its models: %1")
                               .arg(message.isEmpty() ? error_text : message.left(300)));
        return;
    }
    using Common::AiOptimize::Provider;
    const auto which = Anthropic() ? Provider::Anthropic : Provider::OpenAI;
    QStringList ids;
    for (const QJsonValue item : response.value(QStringLiteral("data")).toArray()) {
        const QString id = item.toObject().value(QStringLiteral("id")).toString();
        if (Common::AiOptimize::IsTextModel(which, id.toStdString()) && !ids.contains(id)) {
            ids.append(id);
        }
    }
    if (ids.isEmpty()) {
        emit StatusChanged(tr("The provider listed no models this assistant can use."));
        return;
    }
    if (!Anthropic()) {
        // OpenAI lists in no particular order; newer versions sort higher by name.
        std::ranges::sort(ids, std::greater<>{});
    }
    const QString current = model->currentText();
    for (const QString& id : ids) {
        if (model->findText(id) < 0) {
            model->addItem(id, id);
        }
    }
    model->setCurrentText(current);
    emit StatusChanged(
        tr("%n model(s) available for your key.", nullptr, static_cast<int>(ids.size())));
}
