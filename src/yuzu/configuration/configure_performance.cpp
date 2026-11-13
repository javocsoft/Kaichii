// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <thread>

#include <QButtonGroup>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSysInfo>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "common/hardware_test.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "qt_common/qt_common.h"
#include "video_core/vulkan_common/hardware_test.h"
#include "qt_common/config/setting_help.h"
#include "qt_common/config/shared_translation.h"
#include "yuzu/configuration/configure_performance.h"
#include "yuzu/configuration/shared_widget.h"

namespace {

QString GigabytesText(u64 bytes) {
    return QString::number(static_cast<double>(bytes) / static_cast<double>(u64{1} << 30), 'f', 1);
}

} // Anonymous namespace

ConfigurePerformance::ConfigurePerformance(
    const Settings::HostProfile& host_, const ConfigurationShared::Builder& builder_,
    std::shared_ptr<std::vector<ConfigurationShared::Tab*>> group, QWidget* parent)
    : Tab(group, parent), host{host_}, builder{builder_},
      setting_names{ConfigurationShared::InitializeTranslations(this)} {
    setAccessibleName(tr("Performance"));
    // The descriptions wrap: in a scroll area they get the height they need however small the
    // dialog is.
    auto* const outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* const scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);
    auto* const content = new QWidget(scroll);
    scroll->setWidget(content);
    auto* const layout = new QVBoxLayout(content);

    auto* const machine_box = new QGroupBox(tr("This computer"), content);
    auto* const machine = new QFormLayout(machine_box);
    machine->addRow(tr("Graphics:"), new QLabel(host.gpu_name.empty()
                                                     ? tr("Not detected")
                                                     : QString::fromStdString(host.gpu_name)));
    machine->addRow(tr("Video memory:"),
                    new QLabel(tr("%1 GB, %2")
                                   .arg(GigabytesText(host.gpu_memory_bytes))
                                   .arg(host.gpu_integrated ? tr("shared with the system")
                                                            : tr("dedicated"))));
    machine->addRow(tr("Processor:"), new QLabel(tr("%1 threads").arg(host.cpu_threads)));
    machine->addRow(tr("Memory:"), new QLabel(tr("%1 GB").arg(GigabytesText(host.ram_bytes))));
    machine->addRow(tr("System:"), new QLabel(QSysInfo::prettyProductName()));
    class_label = new QLabel(machine_box);
    machine->addRow(tr("Class:"), class_label);
    measured_label = new QLabel(machine_box);
    measured_label->setWordWrap(true);
    machine->addRow(tr("Measured:"), measured_label);
    test_button = new QPushButton(tr("Test this computer"), machine_box);
    test_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    test_button->setToolTip(
        tr("Measures the graphics card's speed, the video memory free now and the processor's "
           "speed, in about ten seconds, and recommends a preset from them. Close any game "
           "first."));
    connect(test_button, &QPushButton::clicked, this, &ConfigurePerformance::StartHardwareTest);
    machine->addRow(QString{}, test_button);
    layout->addWidget(machine_box);

    auto* const preset_box = new QGroupBox(tr("Presets"), content);
    auto* const presets = new QVBoxLayout(preset_box);
    preset_names = {tr("Performance"), tr("Balanced"), tr("Quality"), tr("Ultra")};
    const std::array descriptions{
        tr("Half resolution with FSR upscaling. For integrated graphics, laptops and cards with "
           "4 GB or less. In Ocarina of Time it also skips the game's sharpening pass."),
        tr("Three-quarter resolution with FSR upscaling. For cards with 6 to 8 GB."),
        tr("Native resolution. For cards with 10 to 12 GB."),
        tr("One and a half times native resolution. For cards with 16 GB or more."),
    };
    preset_buttons = new QButtonGroup(this);
    for (size_t i = 0; i < Settings::AllPerformancePresets.size(); ++i) {
        auto* const radio = new QRadioButton(preset_box);
        auto* const description = new QLabel(descriptions[i], preset_box);
        description->setWordWrap(true);
        description->setContentsMargins(24, 0, 0, 6);
        presets->addWidget(radio);
        presets->addWidget(description);
        preset_buttons->addButton(radio, static_cast<int>(i));
        preset_radios[i] = radio;
    }
    auto* const common_note =
        new QLabel(tr("Every preset uses fast GPU accuracy and asynchronous shaders, sizes "
                      "texture memory to the card, decodes ASTC textures on the processor when it "
                      "has 8 threads or more, and gives the GPU threads performance cores of their "
                      "own when it has enough. Other settings are left as they are."),
                   preset_box);
    common_note->setWordWrap(true);
    presets->addWidget(common_note);

    auto* const apply_button = new QPushButton(tr("Use this preset"), preset_box);
    apply_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(apply_button, &QPushButton::clicked, this, &ConfigurePerformance::ApplySelectedPreset);
    presets->addWidget(apply_button);
    status_label = new QLabel(preset_box);
    status_label->setWordWrap(true);
    presets->addWidget(status_label);
    layout->addWidget(preset_box);

    current_label = new QLabel(content);
    current_label->setWordWrap(true);
    layout->addWidget(current_label);
    auto* const scope = new QLabel(Settings::IsConfiguringGlobal()
                                       ? tr("Presets set here apply to every game without its "
                                            "own settings.")
                                       : tr("Presets set here apply to this game only."),
                                   content);
    scope->setWordWrap(true);
    layout->addWidget(scope);
    layout->addStretch();

    UpdateRecommendation();
    ShowMeasured();
    SetConfiguration();
}

void ConfigurePerformance::UpdateRecommendation() {
    const std::array tier_names{tr("Entry level"), tr("Mid range"), tr("High end"),
                                tr("Enthusiast")};
    class_label->setText(tier_names[static_cast<size_t>(Settings::ClassifyHost(host))]);
    const Settings::PerformancePreset recommended = Settings::RecommendedPreset(host);
    for (size_t i = 0; i < Settings::AllPerformancePresets.size(); ++i) {
        preset_radios[i]->setText(Settings::AllPerformancePresets[i] == recommended
                                      ? tr("%1 (recommended for this computer)")
                                            .arg(preset_names[i])
                                      : preset_names[i]);
    }
}

void ConfigurePerformance::ShowMeasured() {
    if (host.measured_gpu_ratio <= 0.0) {
        measured_label->setText(tr("Not yet: the class comes from the video memory alone."));
        return;
    }
    QString text = tr("Graphics %1 % of the reference computer's speed, %2 GB of video memory "
                      "free")
                       .arg(host.measured_gpu_ratio * 100.0, 0, 'f', 0)
                       .arg(GigabytesText(host.measured_free_memory_bytes));
    if (const u32 cpu = Settings::values.hardware_test_cpu_permille.GetValue(); cpu != 0) {
        text += tr(", processor %1 %").arg(cpu / 10.0, 0, 'f', 0);
    }
    text += tr(".\nReference: %1, where Ocarina of Time runs at about 30 fps outdoors.")
                .arg(QString::fromLatin1(Common::HardwareTest::ReferenceMachine));
    measured_label->setText(text);
}

void ConfigurePerformance::StartHardwareTest() {
    if (QtCommon::system && QtCommon::system->IsPoweredOn()) {
        measured_label->setText(tr("Close the game first: it would slow the test down."));
        return;
    }
    test_button->setEnabled(false);
    measured_label->setText(tr("Testing… about ten seconds."));
    struct Outcome {
        std::optional<Vulkan::GpuTestResult> gpu;
        std::string error;
        double cpu_rate = 0.0;
    };
    const auto device_index = static_cast<size_t>(Settings::values.vulkan_device.GetValue());
    auto* const watcher = new QFutureWatcher<Outcome>(this);
    connect(watcher, &QFutureWatcher<Outcome>::finished, this, [this, watcher] {
        const Outcome outcome = watcher->result();
        watcher->deleteLater();
        test_button->setEnabled(true);
        if (!outcome.gpu) {
            measured_label->setText(
                tr("The test could not run: %1").arg(QString::fromStdString(outcome.error)));
            return;
        }
        const auto permille = [](double ratio) {
            return static_cast<u32>(std::clamp(ratio * 1000.0, 1.0, 1e6));
        };
        Settings::values.hardware_test_gpu.SetValue(outcome.gpu->device_name);
        Settings::values.hardware_test_gpu_permille.SetValue(
            permille(outcome.gpu->tflops / Common::HardwareTest::ReferenceGpuTflops));
        Settings::values.hardware_test_free_memory_mib.SetValue(
            static_cast<u32>(outcome.gpu->budget_bytes >> 20));
        Settings::values.hardware_test_cpu_permille.SetValue(
            permille(outcome.cpu_rate / Common::HardwareTest::ReferenceCpuRate));
        if (outcome.gpu->device_name == host.gpu_name) {
            host.measured_gpu_ratio =
                Settings::values.hardware_test_gpu_permille.GetValue() / 1000.0;
            host.measured_free_memory_bytes = outcome.gpu->budget_bytes;
        }
        ShowMeasured();
        UpdateRecommendation();
        UpdateCurrent();
    });
    watcher->setFuture(QtConcurrent::run([device_index] {
        Outcome outcome;
        try {
            outcome.gpu = Vulkan::RunGpuTest(device_index);
        } catch (const std::exception& error) {
            outcome.error = error.what();
            return outcome;
        }
        // On a thread of its own: the pool thread's affinity stays as it was.
        std::thread cpu_thread([&outcome] {
            Common::SetCurrentThreadToPerformanceCores();
            outcome.cpu_rate =
                Common::HardwareTest::MeasureCpuRate(std::chrono::milliseconds{1500});
        });
        cpu_thread.join();
        return outcome;
    }));
}

ConfigurePerformance::~ConfigurePerformance() = default;

void ConfigurePerformance::SetConfiguration() {
    const auto current = Settings::MatchingPerformancePreset(Settings::values, host);
    const auto selected = current.value_or(Settings::RecommendedPreset(host));
    preset_radios[static_cast<size_t>(selected)]->setChecked(true);
    UpdateCurrent();
}

void ConfigurePerformance::UpdateCurrent() {
    const auto current = Settings::MatchingPerformancePreset(Settings::values, host);
    QString text = current ? tr("The saved settings match the %1 preset.")
                                 .arg(preset_names[static_cast<size_t>(*current)])
                           : tr("The saved settings match no preset.");
    // What keeps them from the recommended preset, so a slow setting is easy to spot.
    const auto recommended = Settings::RecommendedPreset(host);
    if (current != recommended) {
        const auto differences =
            Settings::PresetDifferences(Settings::values, recommended, host);
        if (!differences.empty()) {
            text += QStringLiteral("\n\n") +
                    tr("Compared with the %1 preset recommended for this computer:")
                        .arg(preset_names[static_cast<size_t>(recommended)]);
            for (const Settings::PresetDifference& difference : differences) {
                text += QStringLiteral("\n• ") +
                        tr("%1 %2 (the preset uses %3)")
                            .arg(SettingName(*difference.setting),
                                 SettingValue(*difference.setting, difference.current),
                                 SettingValue(*difference.setting, difference.preset));
            }
        }
    }
    current_label->setText(text);
}

QString ConfigurePerformance::SettingName(const Settings::BasicSetting& setting) const {
    if (setting_names) {
        if (const auto it = setting_names->find(setting.Id());
            it != setting_names->end() && !it->second.first.isEmpty()) {
            QString name = it->second.first.trimmed();
            return name.endsWith(QLatin1Char(':')) ? name : name + QLatin1Char(':');
        }
    }
    return QString::fromStdString(setting.GetLabel()) + QLatin1Char(':');
}

QString ConfigurePerformance::SettingValue(const Settings::BasicSetting& setting,
                                           const std::string& value) const {
    return ConfigurationShared::FormatSettingValue(setting, value, builder.ComboboxTranslations());
}

void ConfigurePerformance::ApplySelectedPreset() {
    const int index = preset_buttons->checkedId();
    if (index < 0) {
        return;
    }
    const auto preset = Settings::AllPerformancePresets[static_cast<size_t>(index)];
    size_t shown = 0;
    const auto entries = Settings::PerformancePresetEntries(Settings::values, preset, host);
    for (const Settings::PresetEntry& entry : entries) {
        if (auto* const widget = builder.FindWidget(*entry.setting);
            widget != nullptr && widget->ShowValue(entry.value)) {
            ++shown;
        }
    }
    status_label->setText(
        shown == entries.size()
            ? tr("The preset is now set in the Graphics pages. Press OK to save it.")
            : tr("%1 of %2 settings were set in the Graphics pages; the others can't change "
                 "while a game runs. Press OK to save.")
                  .arg(shown)
                  .arg(entries.size()));
}
