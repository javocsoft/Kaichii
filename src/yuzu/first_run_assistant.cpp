// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/first_run_assistant.h"

#include <QButtonGroup>
#include <QCoreApplication>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "qt_common/config/setting_help.h"

namespace {

/// The "keep my current settings" choice on the preset page.
constexpr int KeepCurrent = -2;

/// The Performance page's words for the same things, already translated there.
QString PerformancePageText(const char* text) {
    return QCoreApplication::translate("ConfigurePerformance", text);
}

QLabel* WrappedLabel(const QString& text, QWidget* parent) {
    auto* const label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}

} // Anonymous namespace

FirstRunAssistant::FirstRunAssistant(QWidget* parent, std::optional<Settings::HostProfile> host_,
                                     QStringList controllers, bool docked, Hooks hooks_)
    : QWizard(parent), host{std::move(host_)}, hooks{std::move(hooks_)} {
    setWindowTitle(tr("Welcome to Kaichii"));
    // The same look on every desktop.
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage);
    addPage(GamesPage());
    addPage(ComputerPage());
    addPage(ControlsPage(controllers, docked));
    setMinimumSize(640, 520);
}

FirstRunAssistant::~FirstRunAssistant() = default;

QWizardPage* FirstRunAssistant::GamesPage() {
    auto* const page = new QWizardPage(this);
    page->setTitle(tr("Your games"));
    page->setSubTitle(tr("Kaichii lists the games in the folders you add, and looks in them again "
                         "each time it starts."));
    auto* const layout = new QVBoxLayout(page);
    folders = new QListWidget(page);
    folders->setMaximumHeight(folders->fontMetrics().lineSpacing() * 6);
    layout->addWidget(folders);
    auto* const add = new QPushButton(tr("Add a games folder..."), page);
    add->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(add, &QPushButton::clicked, this, [this] {
        if (const QString folder = hooks.choose_folder(); !folder.isEmpty()) {
            hooks.add_game_folder(folder);
            UpdateGames();
        }
    });
    layout->addWidget(add);
    keys_label = WrappedLabel({}, page);
    layout->addWidget(keys_label);
    keys_button = new QPushButton(tr("Install decryption keys..."), page);
    keys_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(keys_button, &QPushButton::clicked, this, [this] {
        hooks.install_keys();
        UpdateGames();
    });
    layout->addWidget(keys_button);
    layout->addStretch(1);
    UpdateGames();
    return page;
}

void FirstRunAssistant::UpdateGames() {
    folders->clear();
    const QStringList list = hooks.game_folders();
    folders->addItems(list);
    if (list.isEmpty()) {
        auto* const none = new QListWidgetItem(tr("No folder yet."));
        none->setFlags(Qt::NoItemFlags);
        folders->addItem(none);
    }
    const bool keys = hooks.keys_present();
    keys_label->setText(keys ? tr("Decryption keys are installed, so NSP and XCI files can be "
                                  "read too.")
                             : tr("Switch 2 games packed as NUP files need nothing else. NSP and "
                                  "XCI files also need the decryption keys of a console you "
                                  "own."));
    keys_button->setVisible(!keys);
}

QWizardPage* FirstRunAssistant::ComputerPage() {
    auto* const page = new QWizardPage(this);
    page->setTitle(tr("This computer"));
    auto* const layout = new QVBoxLayout(page);
    presets = new QButtonGroup(page);
    if (!host) {
        page->setSubTitle(tr("Kaichii couldn't read the graphics card, so it keeps the current "
                             "settings."));
    } else {
        // The header cuts a long subtitle, so the explanation goes on the page.
        page->setSubTitle(
            tr("Kaichii found %1 with %2 GB of video memory, a %3-thread processor and %4 GB of "
               "memory.")
                .arg(QString::fromStdString(host->gpu_name))
                .arg(static_cast<double>(host->gpu_memory_bytes) / (1ULL << 30), 0, 'f', 1)
                .arg(host->cpu_threads)
                .arg(static_cast<double>(host->ram_bytes) / (1ULL << 30), 0, 'f', 0));
        layout->addWidget(WrappedLabel(
            tr("A preset sets the resolution and the options that matter most for speed."), page));
        const std::array descriptions{
            PerformancePageText("Half resolution with FSR upscaling. For integrated graphics, "
                                "laptops and cards with 4 GB or less. In Ocarina of Time it also "
                                "skips the game's sharpening pass."),
            PerformancePageText(
                "Three-quarter resolution with FSR upscaling. For cards with 6 to 8 GB."),
            PerformancePageText("Native resolution. For cards with 10 to 12 GB."),
            PerformancePageText(
                "One and a half times native resolution. For cards with 16 GB or more."),
        };
        const auto recommended = Settings::RecommendedPreset(*host);
        for (size_t index = 0; index < Settings::AllPerformancePresets.size(); ++index) {
            const auto preset = Settings::AllPerformancePresets[index];
            const QString name = ConfigurationShared::PresetName(preset);
            auto* const radio = new QRadioButton(
                preset == recommended
                    ? PerformancePageText("%1 (recommended for this computer)").arg(name)
                    : name,
                page);
            presets->addButton(radio, static_cast<int>(index));
            radio->setChecked(preset == recommended);
            layout->addWidget(radio);
            auto* const description = WrappedLabel(descriptions[index], page);
            description->setContentsMargins(24, 0, 0, 4);
            layout->addWidget(description);
        }
    }
    auto* const keep = new QRadioButton(tr("Keep my current settings"), page);
    presets->addButton(keep, KeepCurrent);
    keep->setChecked(!host.has_value());
    layout->addWidget(keep);
    layout->addWidget(WrappedLabel(tr("You can change it, or measure this computer, at any time "
                                      "under Configure > Performance."),
                                   page));
    layout->addStretch(1);
    return page;
}

QWizardPage* FirstRunAssistant::ControlsPage(const QStringList& controllers, bool docked) {
    auto* const page = new QWizardPage(this);
    page->setTitle(tr("Console mode and controllers"));
    page->setSubTitle(tr("Games draw for the mode the console is in."));
    auto* const layout = new QVBoxLayout(page);
    docked_radio = new QRadioButton(
        tr("Docked, as on a TV: games draw more detail, at a higher resolution."), page);
    auto* const handheld = new QRadioButton(
        tr("Handheld, as in your hands: lighter, which suits slower computers."), page);
    auto* const modes = new QButtonGroup(page);
    modes->addButton(docked_radio);
    modes->addButton(handheld);
    (docked ? docked_radio : handheld)->setChecked(true);
    layout->addWidget(docked_radio);
    layout->addWidget(handheld);
    layout->addSpacing(12);
    layout->addWidget(WrappedLabel(
        controllers.isEmpty()
            ? tr("No controller was found. Player 1 is set up for the keyboard.")
            : tr("Controllers found: %1. Player 1 is set up for the keyboard; choose a "
                 "controller for it in Configure controls.")
                  .arg(controllers.join(QStringLiteral(", "))),
        page));
    auto* const configure = new QPushButton(tr("Configure controls..."), page);
    configure->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(configure, &QPushButton::clicked, this, [this] { hooks.configure_controls(); });
    layout->addWidget(configure);
    layout->addStretch(1);
    return page;
}

std::optional<Settings::PerformancePreset> FirstRunAssistant::ChosenPreset() const {
    const int id = presets->checkedId();
    if (!host || id < 0) {
        return std::nullopt;
    }
    return Settings::AllPerformancePresets[static_cast<size_t>(id)];
}

bool FirstRunAssistant::Docked() const {
    return docked_radio->isChecked();
}
