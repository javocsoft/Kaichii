// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QButtonGroup>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSysInfo>
#include <QVBoxLayout>

#include "common/settings.h"
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
    : Tab(group, parent), host{host_}, builder{builder_} {
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
    const std::array tier_names{tr("Entry level"), tr("Mid range"), tr("High end"),
                                tr("Enthusiast")};
    machine->addRow(tr("Class:"),
                    new QLabel(tier_names[static_cast<size_t>(Settings::ClassifyHost(host))]));
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
    const Settings::PerformancePreset recommended = Settings::RecommendedPreset(host);
    preset_buttons = new QButtonGroup(this);
    for (size_t i = 0; i < Settings::AllPerformancePresets.size(); ++i) {
        const auto preset = Settings::AllPerformancePresets[i];
        QString name = preset_names[i];
        if (preset == recommended) {
            name = tr("%1 (recommended for this computer)").arg(name);
        }
        auto* const radio = new QRadioButton(name, preset_box);
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

    SetConfiguration();
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
    current_label->setText(
        current ? tr("The saved settings match the %1 preset.")
                      .arg(preset_names[static_cast<size_t>(*current)])
                : tr("The saved settings match no preset."));
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
