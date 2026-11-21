// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QImage>
#include <QWidget>

#include "common/common_types.h"

class QLabel;
class QPushButton;

/// A game's box art in its properties: the picture the full cover view shows, chosen from a file
/// or removed. Changes wait for Save, which OK and Apply call.
class BoxArtEditor : public QWidget {
    Q_OBJECT

public:
    /// Asks the player for an image file; empty when they cancel.
    using FilePicker = std::function<QString(QWidget* parent)>;
    /// Reads an image file the player picked; a null image when it isn't a usable picture.
    using ImageReader = std::function<QImage(const QString& path)>;

    BoxArtEditor(u64 program_id, FilePicker pick_file, ImageReader read_image,
                 QWidget* parent = nullptr);

    /// Shows the picture as the box art, to be stored by Save.
    void SetPicture(const QImage& picture);
    /// Shows no box art, to be removed by Save.
    void ClearPicture();
    [[nodiscard]] bool HasChanges() const;
    /// Stores or removes the box art. False when it couldn't be written.
    bool Save();

    /// The preview's size: a case's front, as high as the game's icon preview.
    static constexpr int PreviewHeight = 256;

private:
    void Choose();
    void ShowPreview();

    u64 program_id;
    FilePicker pick_file;
    ImageReader read_image;
    QImage picture;
    bool changed = false;

    QLabel* preview = nullptr;
    QPushButton* choose_button = nullptr;
    QPushButton* remove_button = nullptr;
};
