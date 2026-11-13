// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QString>

#include "common/common_types.h"
#include "qt_common/config/shared_translation.h"

namespace Settings {
class BasicSetting;
struct HostProfile;
} // namespace Settings

namespace ConfigurationShared {

/// Who an option is for.
enum class SettingLevel : u8 {
    Basic,     ///< What most players change
    Advanced,  ///< Fine tuning that is safe to try
    Expert,    ///< Emulation accuracy and workarounds: for a known problem in a game
    Developer, ///< For debugging the emulator
};

/// What turning an option on, or raising it, does. Options whose values have no order (an API, a
/// decoding method) leave this empty and say it in their description.
enum class SettingEffect : u8 {
    Faster,
    Slower,
    BetterImage,
    WorseImage,
    LessStutter,
    MoreFrames,
    MoreLag,
    MoreVideoMemory,
    LessVideoMemory,
    MayCauseGlitches,
    MayFreeze,
};

struct SettingHelp {
    SettingLevel level = SettingLevel::Basic;
    std::vector<SettingEffect> effects;
    /// Takes effect when a game starts, although it can be changed while one runs. Settings that
    /// can't change while a game runs say so without this.
    bool applies_at_boot = false;
    /// The games it acts on; empty when it acts on every game.
    std::vector<u64> program_ids;
};

using SettingHelpMap = std::map<u32, SettingHelp>;

/// The level, effects and games of every setting the configuration pages show, by setting id.
std::unique_ptr<SettingHelpMap> InitializeSettingHelp();

/// The name of a game the help table names by program id.
QString GameName(u64 program_id);

/// The setting with this id, in Settings::values or UISettings::values; null when there is none.
Settings::BasicSetting* FindSetting(u32 id);

/// How `value` (as the setting serializes it) reads in the interface: an enum's combobox text,
/// "on" or "off" for a switch, the number otherwise.
QString FormatSettingValue(const Settings::BasicSetting& setting, const std::string& value,
                           const ComboboxTranslationMap& enums);

struct SettingHelpContext {
    const TranslationMap& names;
    const SettingHelpMap& help;
    const ComboboxTranslationMap& enums;
    /// The machine, for the value of the preset recommended for it; null leaves that out.
    const Settings::HostProfile* host = nullptr;
};

/// The help panel's text for a setting: its name, description, effects, when it applies, which
/// games it acts on, its default and the recommended preset's value.
QString SettingHelpHtml(const Settings::BasicSetting& setting, const SettingHelpContext& context);

/// The help panel's text for a control that is not a setting, from its name and tooltip.
QString ControlHelpHtml(const QString& name, const QString& description);

} // namespace ConfigurationShared
