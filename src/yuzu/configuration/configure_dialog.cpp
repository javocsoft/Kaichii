// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <memory>

#include <QAbstractButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QStyle>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

#include "common/logging.h"
#include "common/settings.h"
#include "common/settings_enums.h"
#include "core/core.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
#include "ui_configure.h"
#include "yuzu/configuration/configure_applets.h"
#include "yuzu/configuration/configure_audio.h"
#include "yuzu/configuration/configure_cpu.h"
#include "yuzu/configuration/configure_debug_tab.h"
#include "yuzu/configuration/configure_dialog.h"
#include "yuzu/configuration/configure_filesystem.h"
#include "yuzu/configuration/configure_general.h"
#include "yuzu/configuration/configure_graphics.h"
#include "yuzu/configuration/configure_performance.h"
#include "yuzu/configuration/configure_graphics_advanced.h"
#include "yuzu/configuration/configure_graphics_extensions.h"
#include "yuzu/configuration/configure_hotkeys.h"
#include "yuzu/configuration/configure_input.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/configuration/configure_network.h"
#include "yuzu/configuration/configure_profile_manager.h"
#include "yuzu/configuration/configure_system.h"
#include "yuzu/configuration/configure_ui.h"
#include "yuzu/configuration/configure_web.h"
#include "yuzu/configuration/settings_organizer.h"
#include "yuzu/configuration/settings_style.h"
#include "yuzu/configuration/setting_help_panel.h"
#include "yuzu/configuration/settings_page_scroll.h"
#include "yuzu/hotkeys.h"

ConfigureDialog::ConfigureDialog(QWidget* parent, HotkeyRegistry& registry_,
                                 InputCommon::InputSubsystem* input_subsystem,
                                 std::vector<VkDeviceInfo::Record>& vk_device_records,
                                 Core::System& system_, bool enable_web_config)
    : QDialog(parent), ui{std::make_unique<Ui::ConfigureDialog>()}, registry(registry_),
      system{system_},
      builder{std::make_unique<ConfigurationShared::Builder>(this, !system_.IsPoweredOn())},
      applets_tab{std::make_unique<ConfigureApplets>(system_, nullptr, *builder, this)},
      audio_tab{std::make_unique<ConfigureAudio>(system_, nullptr, *builder, this)},
      cpu_tab{std::make_unique<ConfigureCpu>(system_, nullptr, *builder, this)},
      debug_tab_tab{std::make_unique<ConfigureDebugTab>(system_, this)},
      filesystem_tab{std::make_unique<ConfigureFilesystem>(this)},
      general_tab{std::make_unique<ConfigureGeneral>(system_, nullptr, *builder, this)},
      graphics_advanced_tab{
          std::make_unique<ConfigureGraphicsAdvanced>(system_, nullptr, *builder, this)},
      graphics_extensions_tab{
          std::make_unique<ConfigureGraphicsExtensions>(system_, nullptr, *builder, this)},
      ui_tab{std::make_unique<ConfigureUi>(system_, this)},
      graphics_tab{std::make_unique<ConfigureGraphics>(
          system_, vk_device_records, [&]() { graphics_advanced_tab->ExposeComputeOption(); },
          [this](Settings::AspectRatio ratio, Settings::ResolutionSetup setup) {
              ui_tab->UpdateScreenshotInfo(ratio, setup);
          },
          nullptr, *builder, this)},
      performance_tab{std::make_unique<ConfigurePerformance>(
          VkDeviceInfo::DetectHostProfile(vk_device_records, Settings::values.vulkan_device.GetValue()),
          *builder, nullptr, this)},
      hotkeys_tab{std::make_unique<ConfigureHotkeys>(system_.HIDCore(), this)},
      input_tab{std::make_unique<ConfigureInput>(system_, this)},
      network_tab{std::make_unique<ConfigureNetwork>(system_, this)},
      profile_tab{std::make_unique<ConfigureProfileManager>(system_, this)},
      system_tab{std::make_unique<ConfigureSystem>(system_, nullptr, *builder, this)},
      web_tab{std::make_unique<ConfigureWeb>(this)} {
    Settings::SetConfiguringGlobal(true);

    ui->setupUi(this);
    ConfigurationShared::StyleSettingsDialog(*this);

    organizer = std::make_unique<ConfigurationShared::SettingsOrganizer>(builder->Help(), 0);
    BuildGoalPages();

    web_tab->SetWebServiceConfigEnabled(enable_web_config);
    hotkeys_tab->Populate(registry);

    input_tab->Initialize(input_subsystem);

    // After Initialize: the controller pages exist only then.
    using enum ConfigurationShared::SettingLevel;
    std::vector<std::pair<QWidget*, ConfigurationShared::SettingLevel>> controls;
    for (QWidget* const page : input_tab->GetSubTabs()) {
        controls.emplace_back(page, Basic);
    }
    controls.emplace_back(hotkeys_tab.get(), Basic);
    // By what the options are for. Pages above the basic level show with advanced options on.
    categories = {
        {QT_TR_NOOP("Performance"), Basic, {{performance_tab.get(), Basic}}},
        {QT_TR_NOOP("Image"), Basic, {{image_page, Basic}}},
        {QT_TR_NOOP("Smoothness and shaders"), Basic, {{smoothness_page, Basic}}},
        {QT_TR_NOOP("Audio"), Basic, {{audio_tab.get(), Basic}}},
        // Not searched for controls: the controller pages' buttons are key names.
        {QT_TR_NOOP("Controls"), Basic, std::move(controls), false},
        {QT_TR_NOOP("System"),
         Basic,
         {{system_tab.get(), Basic},
          {profile_tab.get(), Basic},
          {network_tab.get(), Advanced},
          {filesystem_tab.get(), Advanced},
          {applets_tab.get(), Advanced}}},
        {QT_TR_NOOP("Interface"),
         Basic,
         {{general_tab.get(), Basic}, {ui_tab.get(), Basic}, {web_tab.get(), Advanced}}},
        {QT_TR_NOOP("Advanced"),
         Advanced,
         {{gpu_emulation_page, Advanced},
          {cpu_tab.get(), Advanced},
          {debug_tab_tab.get(), Developer}}},
    };
    // Every page gets its scroll area in the tab widget once; the list then shows a category's.
    ui->tabWidget->addTab(ScrollPage(input_tab.get()), input_tab->accessibleName());
    for (const Category& category : categories) {
        for (const auto& [page, level] : category.pages) {
            organizer->Gate(page);
            ui->tabWidget->addTab(ScrollPage(page), page->accessibleName());
        }
    }

    // What the option under the pointer or the focus does, under the pages.
    help_panel = new ConfigurationShared::SettingHelpPanel(
        builder->Translations(), builder->Help(), builder->ComboboxTranslations(),
        VkDeviceInfo::DetectHostProfile(vk_device_records,
                                        Settings::values.vulkan_device.GetValue()),
        this);
    ui->verticalLayout->insertWidget(1, help_panel);
    help_panel->Watch(ui->tabWidget);

    BuildSidebar();
    RetranslateGoalPages();


    general_tab->SetResetCallback([&] { this->close(); });

    SetConfiguration();
    PopulateSelectionList();
    SetLevels(UISettings::values.settings_show_advanced.GetValue(),
              UISettings::values.settings_developer_mode.GetValue());

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index != -1) {
            debug_tab_tab->SetCurrentIndex(0);
        }
    });
    connect(ui_tab.get(), &ConfigureUi::LanguageChanged, this, &ConfigureDialog::OnLanguageChanged);
    connect(general_tab.get(), &ConfigureGeneral::ExternalContentDirsChanged, this,
            &ConfigureDialog::ExternalContentDirsChanged);
    connect(ui->selectorList, &QListWidget::itemSelectionChanged, this,
            &ConfigureDialog::UpdateVisibleTabs);

    if (system.IsPoweredOn()) {
        QPushButton* apply_button = ui->buttonBox->addButton(QDialogButtonBox::Apply);
        connect(apply_button, &QAbstractButton::clicked, this,
                &ConfigureDialog::HandleApplyButtonClicked);
    }

    // Room for the pages and the help panel: what the pages ask for leaves out their advanced
    // rows while those are hidden.
    ConfigurationShared::FitDialogToScreen(*this, 0.9, QSize{1100, 820});
    ui->selectorList->setCurrentRow(0);

    // Selects the leftmost button on the bottom bar (Cancel as of writing)
    ui->buttonBox->setFocus();
}

ConfigureDialog::~ConfigureDialog() = default;

void ConfigureDialog::SetConfiguration() {}

void ConfigureDialog::ApplyConfiguration() {
    general_tab->ApplyConfiguration();
    ui_tab->ApplyConfiguration();
    system_tab->ApplyConfiguration();
    profile_tab->ApplyConfiguration();
    filesystem_tab->ApplyConfiguration();
    input_tab->ApplyConfiguration();
    hotkeys_tab->ApplyConfiguration(registry);
    cpu_tab->ApplyConfiguration();
    graphics_tab->ApplyConfiguration();
    graphics_advanced_tab->ApplyConfiguration();
    graphics_extensions_tab->ApplyConfiguration();
    audio_tab->ApplyConfiguration();
    debug_tab_tab->ApplyConfiguration();
    web_tab->ApplyConfiguration();
    network_tab->ApplyConfiguration();
    applets_tab->ApplyConfiguration();
    system.ApplySettings();
    Settings::LogSettings();
}

void ConfigureDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureDialog::RetranslateUI() {
    const int old_row = ui->selectorList->currentRow();
    const int old_index = ui->tabWidget->currentIndex();

    ui->retranslateUi(this);
    RetranslateGoalPages();

    PopulateSelectionList();
    ui->selectorList->setCurrentRow(old_row);

    UpdateVisibleTabs();
    ui->tabWidget->setCurrentIndex(old_index);
    if (!search_box->text().isEmpty()) {
        Search(search_box->text());
    }
}

void ConfigureDialog::HandleApplyButtonClicked() {
    UISettings::values.configuration_applied = true;
    ApplyConfiguration();
}

void ConfigureDialog::PopulateSelectionList() {
    [[maybe_unused]] const QSignalBlocker blocker(ui->selectorList);

    // In the order of `categories`.
    using enum ConfigurationShared::CategoryGlyph;
    constexpr std::array glyphs{Performance, Image,  Smoothness, Audio,
                                Controls,    System, Interface,  Advanced};
    // Drawn in the list's own colors, which a theme's style sheet may set.
    ui->selectorList->ensurePolished();
    const QPalette& palette = ui->selectorList->palette();
    const QColor color = palette.color(QPalette::Text);
    const QColor selected = palette.color(QPalette::HighlightedText);

    ui->selectorList->clear();
    for (size_t index = 0; index < categories.size(); ++index) {
        auto* const item = new QListWidgetItem(tr(categories[index].name));
        if (index < glyphs.size()) {
            item->setIcon(ConfigurationShared::CategoryIcon(glyphs[index], color, selected));
        }
        item->setData(Qt::UserRole, static_cast<int>(index));
        ui->selectorList->addItem(item);
        item->setHidden(!organizer->Shows(categories[index].level));
    }
    if (sidebar != nullptr) {
        UpdateSidebarWidth();
    }
}

void ConfigureDialog::BuildGoalPages() {
    const std::array<QWidget*, 3> sources{graphics_tab.get(), graphics_advanced_tab.get(),
                                          graphics_extensions_tab.get()};
    const auto pages = organizer->BuildGraphicsPages(sources, graphics_tab->GraphicsCardGroup(),
                                                     graphics_tab->BackgroundRow());
    using ConfigurationShared::GoalPage;
    image_page = pages[static_cast<size_t>(GoalPage::Image)];
    smoothness_page = pages[static_cast<size_t>(GoalPage::Smoothness)];
    gpu_emulation_page = pages[static_cast<size_t>(GoalPage::GraphicsEmulation)];
}

void ConfigureDialog::BuildSidebar() {
    sidebar = new QWidget(this);
    auto* const layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    search_box = new QLineEdit(sidebar);
    search_box->setClearButtonEnabled(true);
    search_results = new QListWidget(sidebar);
    search_results->setVisible(false);
    search_results->setWordWrap(true);
    search_results->setAlternatingRowColors(true);
    show_advanced_box = new QCheckBox(sidebar);
    developer_mode_box = new QCheckBox(sidebar);
    delete ui->horizontalLayout->replaceWidget(ui->selectorList, sidebar);
    ui->selectorList->setMinimumWidth(0);
    ui->selectorList->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->selectorList->setIconSize(QSize{20, 20});
    ui->selectorList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(search_box);
    layout->addWidget(ui->selectorList, 1);
    layout->addWidget(search_results, 1);
    layout->addWidget(show_advanced_box);
    layout->addWidget(developer_mode_box);

    connect(search_box, &QLineEdit::textChanged, this, &ConfigureDialog::Search);
    connect(search_box, &QLineEdit::returnPressed, this, [this] {
        if (!search_hits.empty()) {
            OpenSearchHit(0);
        }
    });
    connect(search_results, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        OpenSearchHit(item->data(Qt::UserRole).toInt());
    });
    connect(search_results, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        OpenSearchHit(item->data(Qt::UserRole).toInt());
    });
    connect(show_advanced_box, &QCheckBox::toggled, this,
            [this](bool on) { SetLevels(on, developer_mode_box->isChecked()); });
    connect(developer_mode_box, &QCheckBox::toggled, this,
            [this](bool on) { SetLevels(show_advanced_box->isChecked(), on); });
}

void ConfigureDialog::RetranslateGoalPages() {
    image_page->setAccessibleName(tr("Image"));
    smoothness_page->setAccessibleName(tr("Smoothness and shaders"));
    gpu_emulation_page->setAccessibleName(tr("GPU emulation"));
    organizer->Retranslate();
    search_box->setPlaceholderText(tr("Search settings"));
    show_advanced_box->setText(tr("Show advanced options"));
    show_advanced_box->setToolTip(
        tr("Shows the options for fine tuning and for fixing problems in games, and the "
           "Advanced category."));
    developer_mode_box->setText(tr("Developer mode"));
    developer_mode_box->setToolTip(tr("Also shows the options for debugging Kaichii."));
    UpdateSidebarWidth();
}

void ConfigureDialog::UpdateSidebarWidth() {
    // Wide enough for its own texts, and no wider: the pages need the room. The categories'
    // icons and padding count once they are in the list.
    const int list_width = ui->selectorList->sizeHintForColumn(0) +
                           2 * ui->selectorList->frameWidth() +
                           ui->selectorList->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int width = std::max({list_width, show_advanced_box->sizeHint().width(),
                                developer_mode_box->sizeHint().width(), 160});
    sidebar->setFixedWidth(width);
}

void ConfigureDialog::SetLevels(bool show_advanced, bool developer_mode) {
    UISettings::values.settings_show_advanced = show_advanced;
    UISettings::values.settings_developer_mode = developer_mode;
    {
        const QSignalBlocker advanced_blocker(show_advanced_box);
        const QSignalBlocker developer_blocker(developer_mode_box);
        show_advanced_box->setChecked(show_advanced);
        developer_mode_box->setChecked(developer_mode);
        developer_mode_box->setEnabled(show_advanced);
    }
    organizer->SetShowAdvanced(show_advanced);
    organizer->SetDeveloperMode(developer_mode);
    for (int row = 0; row < ui->selectorList->count(); ++row) {
        QListWidgetItem* const item = ui->selectorList->item(row);
        item->setHidden(!organizer->Shows(categories[item->data(Qt::UserRole).toInt()].level));
    }
    if (const QListWidgetItem* current = ui->selectorList->currentItem();
        current == nullptr || current->isHidden()) {
        ui->selectorList->setCurrentRow(0);
    }
    UpdateVisibleTabs();
}

void ConfigureDialog::Search(const QString& text) {
    using ConfigurationShared::SettingLevel;
    const bool searching = text.trimmed().size() >= 2;
    ui->selectorList->setVisible(!searching);
    search_results->setVisible(searching);
    search_results->clear();
    search_hits.clear();
    if (!searching) {
        return;
    }
    constexpr size_t MaxHits = 60;
    const auto& names = builder->Translations();
    const auto clean = [](QString name) {
        name.remove(QLatin1Char('&'));
        name = name.trimmed();
        if (name.endsWith(QLatin1Char(':'))) {
            name.chop(1);
        }
        return name;
    };
    std::vector<QString> hit_names;
    const auto add = [&](size_t category, QWidget* page, QWidget* control, const QString& name,
                         SettingLevel level) {
        if (search_hits.size() < MaxHits) {
            search_hits.push_back({category, page, control, level});
            hit_names.push_back(name);
        }
    };
    for (size_t category = 0; category < categories.size(); ++category) {
        const Category& entry = categories[category];
        for (const auto& [page, page_level] : entry.pages) {
            const SettingLevel base = std::max(entry.level, page_level);
            if (ConfigurationShared::MatchesSearch(page->accessibleName(), text)) {
                add(category, page, nullptr, page->accessibleName(), base);
            }
            for (QWidget* const widget : page->findChildren<QWidget*>()) {
                if (const QVariant id = widget->property(ConfigurationShared::SettingIdProperty);
                    id.isValid()) {
                    if (ConfigurationShared::FindSettingRow(page, id.toUInt()) != widget) {
                        continue;
                    }
                    const auto level = organizer->LevelOf(widget);
                    const auto it = names.find(id.toUInt());
                    if (!level || it == names.end() || it->second.first.trimmed().isEmpty()) {
                        continue;
                    }
                    const QString& setting_name = it->second.first;
                    if (ConfigurationShared::MatchesSearch(
                            setting_name + QLatin1Char(' ') + it->second.second, text)) {
                        add(category, page, widget, clean(setting_name), std::max(base, *level));
                    }
                    continue;
                }
                // Controls that are not settings, outside setting rows.
                if (!entry.search_controls) {
                    continue;
                }
                bool in_row = false;
                for (QWidget* parent = widget->parentWidget(); parent != nullptr && parent != page;
                     parent = parent->parentWidget()) {
                    if (parent->property(ConfigurationShared::SettingIdProperty).isValid()) {
                        in_row = true;
                        break;
                    }
                }
                if (in_row) {
                    continue;
                }
                QString name;
                if (const auto* button = qobject_cast<QAbstractButton*>(widget)) {
                    name = button->text();
                } else if (const auto* group = qobject_cast<QGroupBox*>(widget)) {
                    name = group->title();
                } else if (const auto* label = qobject_cast<QLabel*>(widget);
                           label != nullptr && label->text().trimmed().endsWith(QLatin1Char(':'))) {
                    name = label->text();
                }
                if (name.trimmed().isEmpty() || Qt::mightBeRichText(name)) {
                    continue;
                }
                if (ConfigurationShared::MatchesSearch(
                        name + QLatin1Char(' ') + widget->toolTip(), text)) {
                    add(category, page, widget, clean(name), base);
                }
            }
        }
    }
    if (search_hits.empty()) {
        auto* const item = new QListWidgetItem(tr("No settings match"));
        item->setFlags(Qt::NoItemFlags);
        search_results->addItem(item);
        return;
    }
    for (size_t index = 0; index < search_hits.size(); ++index) {
        const SearchHit& hit = search_hits[index];
        const QString category = tr(categories[hit.category].name);
        const QString tab = hit.page->accessibleName();
        const QString where = hit.control == nullptr || tab == category
                                  ? category
                                  : tr("%1 › %2").arg(category, tab);
        auto* const item = new QListWidgetItem(hit_names[index] + QLatin1Char('\n') + where);
        item->setData(Qt::UserRole, static_cast<int>(index));
        search_results->addItem(item);
    }
}

void ConfigureDialog::OpenSearchHit(int index) {
    if (index < 0 || static_cast<size_t>(index) >= search_hits.size()) {
        return;
    }
    const SearchHit hit = search_hits[static_cast<size_t>(index)];
    if (!organizer->Shows(hit.level)) {
        // What was found shows only with advanced options: show them rather than hide it.
        SetLevels(true, hit.level == ConfigurationShared::SettingLevel::Developer ||
                            developer_mode_box->isChecked());
    }
    for (int row = 0; row < ui->selectorList->count(); ++row) {
        if (ui->selectorList->item(row)->data(Qt::UserRole).toInt() ==
            static_cast<int>(hit.category)) {
            ui->selectorList->setCurrentRow(row);
            break;
        }
    }
    UpdateVisibleTabs();
    if (const int tab = ui->tabWidget->indexOf(ScrollPage(hit.page)); tab >= 0) {
        ui->tabWidget->setCurrentIndex(tab);
    }
    QPointer<QWidget> control = hit.control;
    if (!control) {
        return;
    }
    // After the tab's layout has run, so the scroll position is right.
    QTimer::singleShot(0, this, [this, control] {
        if (!control) {
            return;
        }
        for (QWidget* parent = control->parentWidget(); parent != nullptr;
             parent = parent->parentWidget()) {
            if (auto* const area = qobject_cast<QScrollArea*>(parent)) {
                area->ensureWidgetVisible(control);
                break;
            }
        }
        QWidget* target = control;
        if (target->focusPolicy() == Qt::NoFocus) {
            for (QWidget* const child : control->findChildren<QWidget*>()) {
                if (child->focusPolicy() != Qt::NoFocus && child->isVisibleTo(control)) {
                    target = child;
                    break;
                }
            }
        }
        target->setFocus(Qt::OtherFocusReason);
        help_panel->Explain(control);
    });
}

void ConfigureDialog::OnLanguageChanged(const QString& locale) {
    emit LanguageChanged(locale);
    //  Reloading the game list is needed to force retranslation.
    UISettings::values.is_game_list_reload_pending = true;
    // first apply the configuration, and then restore the display
    ApplyConfiguration();
    RetranslateUI();
    SetConfiguration();
}

void ConfigureDialog::UpdateVisibleTabs() {
    const auto items = ui->selectorList->selectedItems();
    if (items.isEmpty()) {
        return;
    }

    [[maybe_unused]] const QSignalBlocker blocker(ui->tabWidget);

    QWidget* const current = ui->tabWidget->currentWidget();
    ui->tabWidget->clear();

    const auto index = static_cast<size_t>(items[0]->data(Qt::UserRole).toInt());
    const Category& category = categories[index];
    for (const auto& [page, level] : category.pages) {
        if (organizer->Shows(level)) {
            ui->tabWidget->addTab(ScrollPage(page), page->accessibleName());
        }
    }
    if (const int tab = ui->tabWidget->indexOf(current); tab >= 0) {
        ui->tabWidget->setCurrentIndex(tab);
    }
}

QWidget* ConfigureDialog::ScrollPage(QWidget* page) {
    auto*& scroll = scroll_pages[page];
    if (scroll == nullptr) {
        scroll = new ConfigurationShared::SettingsPageScroll(page, this);
    }
    return scroll;
}
