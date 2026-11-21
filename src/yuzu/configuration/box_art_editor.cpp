// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include "qt_common/game_list/box_art.h"
#include "yuzu/configuration/box_art_editor.h"
#include "yuzu/game/cover_art.h"
#include "yuzu/game/cover_layout.h"

BoxArtEditor::BoxArtEditor(u64 program_id_, FilePicker pick_file_, ImageReader read_image_,
                           QWidget* parent)
    : QWidget{parent}, program_id{program_id_}, pick_file{std::move(pick_file_)},
      read_image{std::move(read_image_)} {
    setObjectName(QStringLiteral("boxArtEditor"));

    preview = new QLabel(this);
    preview->setObjectName(QStringLiteral("boxArtPreview"));
    preview->setFixedSize(Yuzu::BoxAspectWidth * PreviewHeight / Yuzu::BoxAspectHeight,
                          PreviewHeight);
    preview->setAlignment(Qt::AlignCenter);
    preview->setWordWrap(true);

    choose_button = new QPushButton(tr("Choose Box Art..."), this);
    choose_button->setObjectName(QStringLiteral("chooseBoxArt"));
    choose_button->setIcon(QIcon::fromTheme(QStringLiteral("document-open"),
                                            QIcon::fromTheme(QStringLiteral("insert-image"))));
    choose_button->setToolTip(tr("Preview an image as the game's box art, shown by View > Game "
                                 "List Mode > Full Cover View. OK or Apply saves it."));
    remove_button = new QPushButton(tr("Remove"), this);
    remove_button->setObjectName(QStringLiteral("removeBoxArt"));
    remove_button->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    remove_button->setToolTip(tr("Show the game's icon on its case again. OK or Apply saves it."));

    auto* const buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(choose_button);
    buttons->addWidget(remove_button);

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(preview, 0, Qt::AlignHCenter);
    layout->addLayout(buttons);

    connect(choose_button, &QPushButton::clicked, this, &BoxArtEditor::Choose);
    connect(remove_button, &QPushButton::clicked, this, &BoxArtEditor::ClearPicture);

    if (program_id == 0) {
        // Box art is kept by program id; a game without one can't have it.
        choose_button->setEnabled(false);
        choose_button->setToolTip(tr("This game has no program id, so it can't have box art."));
    } else {
        picture = QtCommon::BoxArt::Load(program_id);
    }
    ShowPreview();
}

void BoxArtEditor::SetPicture(const QImage& picture_) {
    picture = picture_;
    changed = true;
    ShowPreview();
}

void BoxArtEditor::ClearPicture() {
    if (picture.isNull()) {
        return;
    }
    picture = {};
    changed = true;
    ShowPreview();
}

bool BoxArtEditor::HasChanges() const {
    return changed;
}

bool BoxArtEditor::Save() {
    if (!changed) {
        return true;
    }
    const bool saved = picture.isNull() ? QtCommon::BoxArt::Remove(program_id)
                                        : QtCommon::BoxArt::Save(program_id, picture);
    if (saved) {
        changed = false;
    }
    return saved;
}

void BoxArtEditor::Choose() {
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

void BoxArtEditor::ShowPreview() {
    remove_button->setEnabled(!picture.isNull());
    if (picture.isNull()) {
        preview->setPixmap({});
        preview->setText(tr("No box art.\nThe full cover view shows the game's icon on a case."));
        preview->setStyleSheet(
            QStringLiteral("QLabel { border: 1px dashed palette(mid); border-radius: 6px; }"));
        return;
    }
    const qreal ratio = devicePixelRatioF();
    QPixmap shown = QPixmap::fromImage(
        Yuzu::FitCover(picture, (QSizeF(preview->size()) * ratio).toSize(), 6 * ratio));
    shown.setDevicePixelRatio(ratio);
    preview->setStyleSheet({});
    preview->setText({});
    preview->setPixmap(shown);
}
