// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>
#include <QDialog>
#include <QPointer>
#include "configuration/shared_widget.h"
#include "qt_common/config/setting_help.h"
#include "qt_common/config/shared_translation.h"
#include "qt_common/util/vk.h"
#include "yuzu/configuration/configuration_shared.h"

namespace Core {
class System;
}

class ConfigureApplets;
class ConfigureAudio;
class ConfigureCpu;
class ConfigureDebugTab;
class ConfigureFilesystem;
class ConfigureGeneral;
class ConfigureGraphics;
class ConfigurePerformance;
class ConfigureGraphicsAdvanced;
class ConfigureGraphicsExtensions;
class ConfigureHotkeys;
class ConfigureInput;
class ConfigureProfileManager;
class ConfigureSystem;
class ConfigureNetwork;
class ConfigureUi;
class ConfigureWeb;

class HotkeyRegistry;
class QCheckBox;
class QLineEdit;
class QListWidget;

namespace ConfigurationShared {
class SettingHelpPanel;
class SettingsOrganizer;
} // namespace ConfigurationShared

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class ConfigureDialog;
}

class ConfigureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureDialog(QWidget* parent, HotkeyRegistry& registry_,
                             InputCommon::InputSubsystem* input_subsystem,
                             std::vector<VkDeviceInfo::Record>& vk_device_records,
                             Core::System& system_, bool enable_web_config = true);
    ~ConfigureDialog() override;

    void ApplyConfiguration();

private slots:
    void OnLanguageChanged(const QString& locale);

signals:
    void LanguageChanged(const QString& locale);
    void ExternalContentDirsChanged();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void HandleApplyButtonClicked();

    /// A category of the list on the left: its pages and who each is for.
    struct Category {
        const char* name; ///< Untranslated, in the ConfigureDialog context
        ConfigurationShared::SettingLevel level;
        std::vector<std::pair<QWidget*, ConfigurationShared::SettingLevel>> pages;
        bool search_controls = true; ///< Whether search looks at its controls besides settings
    };
    /// A control the search found, and where.
    struct SearchHit {
        size_t category;
        QWidget* page;
        QPointer<QWidget> control;
        ConfigurationShared::SettingLevel level;
    };

    void SetConfiguration();
    void UpdateVisibleTabs();
    void PopulateSelectionList();
    void BuildGoalPages();
    void BuildSidebar();
    void RetranslateGoalPages();
    /// Shows the options of the levels chosen, and saves the choice.
    void SetLevels(bool show_advanced, bool developer_mode);
    void Search(const QString& text);
    void OpenSearchHit(int index);
    /// The scroll area a settings page is shown in, made on first use (see SettingsPageScroll).
    QWidget* ScrollPage(QWidget* page);

    std::unique_ptr<Ui::ConfigureDialog> ui;
    HotkeyRegistry& registry;

    Core::System& system;
    std::unique_ptr<ConfigurationShared::Builder> builder;
    std::vector<ConfigurationShared::Tab*> tab_group;

    std::unique_ptr<ConfigureApplets> applets_tab;
    std::unique_ptr<ConfigureAudio> audio_tab;
    std::unique_ptr<ConfigureCpu> cpu_tab;
    std::unique_ptr<ConfigureDebugTab> debug_tab_tab;
    std::unique_ptr<ConfigureFilesystem> filesystem_tab;
    std::unique_ptr<ConfigureGeneral> general_tab;
    std::unique_ptr<ConfigureGraphicsAdvanced> graphics_advanced_tab;
    std::unique_ptr<ConfigureGraphicsExtensions> graphics_extensions_tab;
    std::unique_ptr<ConfigureUi> ui_tab;
    std::unique_ptr<ConfigureGraphics> graphics_tab;
    std::unique_ptr<ConfigurePerformance> performance_tab;
    std::unique_ptr<ConfigureHotkeys> hotkeys_tab;
    std::unique_ptr<ConfigureInput> input_tab;
    std::unique_ptr<ConfigureNetwork> network_tab;
    std::unique_ptr<ConfigureProfileManager> profile_tab;
    std::unique_ptr<ConfigureSystem> system_tab;
    std::unique_ptr<ConfigureWeb> web_tab;

    std::unordered_map<QWidget*, QWidget*> scroll_pages;

    std::unique_ptr<ConfigurationShared::SettingsOrganizer> organizer;
    ConfigurationShared::SettingHelpPanel* help_panel = nullptr;
    QWidget* image_page = nullptr;
    QWidget* smoothness_page = nullptr;
    QWidget* gpu_emulation_page = nullptr;
    std::vector<Category> categories;
    std::vector<SearchHit> search_hits;
    QWidget* sidebar = nullptr;
    QLineEdit* search_box = nullptr;
    QListWidget* search_results = nullptr;
    QCheckBox* show_advanced_box = nullptr;
    QCheckBox* developer_mode_box = nullptr;
};
