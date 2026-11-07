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

    const Settings::HostProfile host;
    const ConfigurationShared::Builder& builder;
    QButtonGroup* preset_buttons{};
    std::array<QRadioButton*, Settings::AllPerformancePresets.size()> preset_radios{};
    std::array<QString, Settings::AllPerformancePresets.size()> preset_names;
    QLabel* current_label{};
    QLabel* status_label{};
};
