// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QImage>
#include <QWidget>

#include "common/common_types.h"

class QPushButton;

/// The icon buttons in a game's properties for games other than NUP: an icon of the player's own
/// in place of the game file's, chosen from a file or dropped. Changes wait for Save, which OK and
/// Apply call. NUP games keep their own buttons, which write the icon into the file.
class CustomIconEditor : public QWidget {
    Q_OBJECT

public:
    /// Asks the player for an image file; empty when they cancel.
    using FilePicker = std::function<QString(QWidget* parent)>;
    /// Reads an image file the player picked; a null image when it isn't a usable picture.
    using ImageReader = std::function<QImage(const QString& path)>;

    CustomIconEditor(u64 program_id, FilePicker pick_file, ImageReader read_image,
                     QWidget* parent = nullptr);

    /// The custom icon as it will be stored; null when the game file's icon is shown.
    [[nodiscard]] QImage Icon() const;
    /// Takes the picture as the game's icon, to be stored by Save.
    void SetPicture(const QImage& picture);
    /// Goes back to the game file's icon, to be stored by Save.
    void Restore();
    [[nodiscard]] bool HasChanges() const;
    /// Stores or removes the custom icon. False when it couldn't be written.
    bool Save();

signals:
    /// The icon to preview changed: the custom one, or null for the game file's.
    void IconChanged(const QImage& icon);

private:
    void Choose();
    void UpdateButtons();

    u64 program_id;
    FilePicker pick_file;
    ImageReader read_image;
    QImage icon;
    bool changed = false;

    QPushButton* upload_button = nullptr;
    QPushButton* restore_button = nullptr;
};
