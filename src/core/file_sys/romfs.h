// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <string_view>

#include "core/file_sys/vfs/vfs.h"

namespace FileSys {

struct RomFSReadObserver {
    std::function<bool(std::string_view)> accepts;
    std::function<void(std::string_view)> on_read;
};

// Converts a RomFS binary blob to VFS Filesystem
// Returns nullptr on failure
VirtualDir ExtractRomFS(const VirtualFile& file);

// Converts a VFS filesystem into a RomFS binary
// Returns nullptr on failure
VirtualFile CreateRomFS(const VirtualDir& dir, VirtualDir ext = nullptr,
                        RomFSReadObserver observer = {});

// Wraps a RomFS image so that a read touching a file `observer` accepts calls its on_read with that
// file's path ("/dir/name"). Returns `romfs` itself when no file is accepted.
VirtualFile WatchRomFSReads(VirtualFile romfs, const RomFSReadObserver& observer);

} // namespace FileSys
