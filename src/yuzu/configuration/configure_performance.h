// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// The Performance page: what the emulator detected about the machine, and presets that set the
// graphics settings for it in one step.

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <QString>

#include "common/performance_presets.h"
#include "yuzu/configuration/configuration_shared.h"

class QButtonGroup;
class QLabel;
class QPushButton;
class QRadioButton;

namespace ConfigurationShared {
class Builder;
}

class ConfigurePerformance final : public ConfigurationShared::Tab {
    Q_OBJECT

public:
    explicit ConfigurePerformance(const Settings::HostProfile& host,
                                  const ConfigurationShared::Builder& builder,
                                  std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group,
                                  QWidget* parent = nullptr);
    ~ConfigurePerformance() override;

    // The other pages save what a preset shows in their widgets.
    void ApplyConfiguration() override {}
    void SetConfiguration() override;

private:
    void ApplySelectedPreset();
    void UpdateCurrent();
    /// The class and the recommended preset, after the hardware test changed what is known.
    void UpdateRecommendation();
    void StartHardwareTest();
    void ShowMeasured();
    /// A setting's name as the settings pages show it.
    QString SettingName(const Settings::BasicSetting& setting) const;
    /// A value of a setting (as Setting::ToString writes it) as the settings pages show it.
    QString SettingValue(const Settings::BasicSetting& setting, const std::string& value) const;

    Settings::HostProfile host;
    const ConfigurationShared::Builder& builder;
    QButtonGroup* preset_buttons{};
    std::array<QRadioButton*, Settings::AllPerformancePresets.size()> preset_radios{};
    std::array<QString, Settings::AllPerformancePresets.size()> preset_names;
    std::unique_ptr<ConfigurationShared::TranslationMap> setting_names;
    QLabel* current_label{};
    QLabel* status_label{};
    QLabel* class_label{};
    QLabel* measured_label{};
    QPushButton* test_button{};
};
