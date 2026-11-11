// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Common {

/// On NVIDIA's Linux driver, pipelines are only kept in the driver's own disk cache (Vulkan's
/// DriverCachesPipelines). Its default limit is 1 GiB for the whole cache folder: a large Ocarina of
/// Time cache (about 20,000 pipelines) fills it, and every boot then compiles again the pipelines
/// that no longer fit. Before anything loads the driver, this points __GL_SHADER_DISK_CACHE_PATH at
/// a folder of Lichii's own ($XDG_CACHE_HOME/lichii/nvidia, or ~/.cache/lichii/nvidia), so another
/// application with the default limit can't trim it, and sets __GL_SHADER_DISK_CACHE_SIZE to 10 GiB.
/// A value the user set for either variable is kept; other drivers ignore both. Does nothing outside
/// Linux. From Zortos PR 1301, which set it up for the Qt frontend only.
void SetUpNvidiaShaderDiskCache();

} // namespace Common
