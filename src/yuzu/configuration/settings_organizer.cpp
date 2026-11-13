// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/settings_organizer.h"

#include <algorithm>

#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

#include "common/settings.h"

namespace ConfigurationShared {

namespace {

/// Set on a row once it is wrapped in a gate, so a page gated twice isn't wrapped twice.
constexpr char GatedProperty[] = "lichiiSettingGated";

QString Translate(const char* text) {
    return QCoreApplication::translate("ConfigurationShared", text);
}

std::optional<u32> SettingIdOf(const QWidget& widget) {
    const QVariant id = widget.property(SettingIdProperty);
    if (!id.isValid()) {
        return std::nullopt;
    }
    return id.toUInt();
}

/// Takes `widget` out of the layout it is in, so another layout can take it.
void Detach(QWidget* widget) {
    QWidget* const parent = widget->parentWidget();
    if (parent != nullptr && parent->layout() != nullptr) {
        parent->layout()->removeWidget(widget);
    }
}

/// The setting rows under `root`, outermost first in tree order.
std::vector<QWidget*> SettingRows(QWidget* root) {
    std::vector<QWidget*> rows;
    for (QWidget* widget : root->findChildren<QWidget*>()) {
        if (const auto id = SettingIdOf(*widget); id && FindSettingRow(root, *id) == widget) {
            rows.push_back(widget);
        }
    }
    return rows;
}

} // Anonymous namespace

QWidget* FindSettingRow(QWidget* root, u32 id) {
    for (QWidget* widget : root->findChildren<QWidget*>()) {
        if (SettingIdOf(*widget) != id) {
            continue;
        }
        bool outermost = true;
        for (QWidget* parent = widget->parentWidget(); parent != nullptr && parent != root;
             parent = parent->parentWidget()) {
            if (SettingIdOf(*parent) == id) {
                outermost = false;
                break;
            }
        }
        if (outermost) {
            return widget;
        }
    }
    return nullptr;
}

SettingsOrganizer::SettingsOrganizer(const SettingHelpMap& help_, u64 program_id_, QObject* parent)
    : QObject(parent), help{help_}, program_id{program_id_} {}

SettingsOrganizer::~SettingsOrganizer() = default;

QWidget* SettingsOrganizer::BuildPage(std::span<QWidget* const> sources,
                                      std::vector<PageSection> page_sections) {
    auto* const page = new QWidget();
    auto* const layout = new QVBoxLayout(page);
    for (PageSection& spec : page_sections) {
        if (spec.replacement != nullptr) {
            Detach(spec.replacement);
            if (auto* box = qobject_cast<QGroupBox*>(spec.replacement);
                box != nullptr && spec.title != nullptr) {
                box->setTitle(Translate(spec.title));
                sections.push_back({box, spec.title, {}});
            }
            layout->addWidget(spec.replacement);
            continue;
        }
        auto* const box = new QGroupBox(Translate(spec.title), page);
        auto* const box_layout = new QVBoxLayout(box);
        Section section{box, spec.title, {}};
        for (const u32 id : spec.ids) {
            for (QWidget* source : sources) {
                if (QWidget* row = FindSettingRow(source, id)) {
                    Detach(row);
                    box_layout->addWidget(row);
                    section.rows.emplace_back(row);
                    break;
                }
            }
        }
        for (QWidget* extra : spec.extra_rows) {
            Detach(extra);
            box_layout->addWidget(extra);
            section.rows.emplace_back(extra);
        }
        if (section.rows.empty()) {
            // Settings this build doesn't show (NVIDIA DLSS without Streamline...).
            delete box;
            continue;
        }
        layout->addWidget(box);
        sections.push_back(std::move(section));
    }
    layout->addStretch(1);
    Update();
    return page;
}

std::array<QWidget*, 3> SettingsOrganizer::BuildGraphicsPages(std::span<QWidget* const> sources,
                                                              QWidget* graphics_card_group,
                                                              QWidget* background_row) {
    std::array<std::vector<PageSection>, 3> pages;
    for (const GoalSection& goal : GraphicsSections()) {
        PageSection section{.title = goal.title, .ids = goal.ids};
        const auto has = [&goal](const Settings::BasicSetting& setting) {
            return std::ranges::find(goal.ids, setting.Id()) != goal.ids.end();
        };
        if (has(Settings::values.renderer_backend)) {
            // The Graphics page's own group, with its API and device logic.
            section = {.replacement = graphics_card_group};
        } else if (has(Settings::values.anti_aliasing)) {
            section.extra_rows.push_back(background_row);
        }
        pages[static_cast<size_t>(goal.page)].push_back(std::move(section));
    }
    std::array<QWidget*, 3> built{};
    for (size_t page = 0; page < pages.size(); ++page) {
        built[page] = BuildPage(sources, std::move(pages[page]));
    }
    // Their rows live in the new pages now; the source pages still apply them.
    for (QWidget* const source : sources) {
        source->setVisible(false);
    }
    return built;
}

void SettingsOrganizer::Gate(QWidget* page) {
    for (QWidget* row : SettingRows(page)) {
        if (row->property(GatedProperty).toBool()) {
            continue;
        }
        const auto level = LevelOf(row);
        if (level == SettingLevel::Basic) {
            continue;
        }
        QWidget* const parent = row->parentWidget();
        QLayout* const layout = parent != nullptr ? parent->layout() : nullptr;
        if (layout == nullptr) {
            continue;
        }
        // A wrapper takes the row's place: the page that built the row keeps showing and hiding
        // the row itself, and the level shows or hides the wrapper.
        auto* const gate = new QWidget(parent);
        auto* const gate_layout = new QVBoxLayout(gate);
        gate_layout->setContentsMargins(0, 0, 0, 0);
        QLayoutItem* const replaced = layout->replaceWidget(row, gate);
        if (replaced == nullptr) {
            delete gate;
            continue;
        }
        delete replaced;
        gate_layout->addWidget(row);
        row->setProperty(GatedProperty, true);
        row->installEventFilter(this);
        gates.push_back({gate, row, level});
    }
    Update();
}

void SettingsOrganizer::SetShowAdvanced(bool show) {
    show_advanced = show;
    Update();
}

void SettingsOrganizer::SetDeveloperMode(bool on) {
    developer_mode = on;
    Update();
}

bool SettingsOrganizer::Shows(SettingLevel level) const {
    switch (level) {
    case SettingLevel::Basic:
        return true;
    case SettingLevel::Advanced:
    case SettingLevel::Expert:
        return show_advanced;
    case SettingLevel::Developer:
        return show_advanced && developer_mode;
    }
    return false;
}

std::optional<SettingLevel> SettingsOrganizer::LevelOf(QWidget* row) const {
    const auto id = SettingIdOf(*row);
    if (!id) {
        return SettingLevel::Basic;
    }
    const auto it = help.find(*id);
    if (it == help.end()) {
        return SettingLevel::Basic;
    }
    const auto& games = it->second.program_ids;
    if (!games.empty() && std::ranges::find(games, program_id) == games.end()) {
        return std::nullopt;
    }
    return it->second.level;
}

void SettingsOrganizer::Retranslate() {
    for (Section& section : sections) {
        if (section.box && section.title != nullptr) {
            section.box->setTitle(Translate(section.title));
        }
    }
}

bool SettingsOrganizer::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent) {
        for (const Gated& gated : gates) {
            if (gated.row == watched) {
                UpdateGate(gated);
                break;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void SettingsOrganizer::UpdateGate(const Gated& gated) const {
    if (!gated.gate) {
        return;
    }
    // The row's own page may hide it (a frame generation option while it is off): an empty
    // wrapper would still take a row's spacing.
    const bool row_shown = gated.row && gated.row->isVisibleTo(gated.gate);
    gated.gate->setVisible(gated.level && Shows(*gated.level) && row_shown);
}

void SettingsOrganizer::Update() {
    for (const Gated& gated : gates) {
        UpdateGate(gated);
    }
    // A section whose every row is above the level shown would be an empty box.
    for (const Section& section : sections) {
        if (!section.box) {
            continue;
        }
        const bool shown =
            section.rows.empty() ||
            std::ranges::any_of(section.rows, [this](const QPointer<QWidget>& row) {
                if (!row) {
                    return false;
                }
                const auto level = LevelOf(row);
                return level && Shows(*level);
            });
        section.box->setVisible(shown);
    }
}

} // namespace ConfigurationShared
