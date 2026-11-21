// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>

#include "qt_common/game_list/game_pictures.h"
#include "yuzu/configuration/custom_icon_editor.h"

CustomIconEditor::CustomIconEditor(u64 program_id_, FilePicker pick_file_, ImageReader read_image_,
                                   QWidget* parent)
    : QWidget{parent}, program_id{program_id_}, pick_file{std::move(pick_file_)},
      read_image{std::move(read_image_)} {
    setObjectName(QStringLiteral("customIconEditor"));

    upload_button = new QPushButton(tr("Upload Icon..."), this);
    upload_button->setObjectName(QStringLiteral("uploadCustomIcon"));
    upload_button->setIcon(QIcon::fromTheme(QStringLiteral("document-open"),
                                            QIcon::fromTheme(QStringLiteral("insert-image"))));
    upload_button->setToolTip(tr("Show an image of your own as the game's icon in the game list. "
                                 "The game file isn't changed. OK or Apply saves it."));
    restore_button = new QPushButton(tr("Restore Game Icon"), this);
    restore_button->setObjectName(QStringLiteral("restoreGameIcon"));
    restore_button->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    restore_button->setToolTip(tr("Show the icon from the game file again. OK or Apply saves it."));

    auto* const layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(upload_button);
    layout->addWidget(restore_button);

    connect(upload_button, &QPushButton::clicked, this, &CustomIconEditor::Choose);
    connect(restore_button, &QPushButton::clicked, this, &CustomIconEditor::Restore);

    if (program_id == 0) {
        // Custom icons are kept by program id; a game without one can't have one.
        upload_button->setEnabled(false);
        upload_button->setToolTip(
            tr("This game has no program id, so it can't have an icon of its own."));
    } else {
        icon = QtCommon::CustomIcon::Load(program_id);
    }
    UpdateButtons();
}

QImage CustomIconEditor::Icon() const {
    return icon;
}

void CustomIconEditor::SetPicture(const QImage& picture) {
    icon = QtCommon::CustomIcon::Squared(picture);
    changed = true;
    UpdateButtons();
    emit IconChanged(icon);
}

void CustomIconEditor::Restore() {
    if (icon.isNull()) {
        return;
    }
    icon = {};
    changed = true;
    UpdateButtons();
    emit IconChanged(icon);
}

bool CustomIconEditor::HasChanges() const {
    return changed;
}

bool CustomIconEditor::Save() {
    if (!changed) {
        return true;
    }
    const bool saved = icon.isNull() ? QtCommon::CustomIcon::Remove(program_id)
                                     : QtCommon::CustomIcon::Save(program_id, icon);
    if (saved) {
        changed = false;
    }
    return saved;
}

void CustomIconEditor::Choose() {
    const QString selected = pick_file(this);
    if (selected.isEmpty()) {
        return;
    }
    const QImage image = read_image(selected);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Invalid Image"),
                             tr("Select a PNG, JPEG, BMP or WebP image up to 8 MiB, "
                                "8192 pixels per side and 16 megapixels."));
        return;
    }
    SetPicture(image);
}

void CustomIconEditor::UpdateButtons() {
    restore_button->setEnabled(!icon.isNull());
}
