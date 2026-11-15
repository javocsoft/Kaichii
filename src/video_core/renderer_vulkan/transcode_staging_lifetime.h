// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>

namespace Vulkan {

template <typename Pool, typename Ref, typename Upload>
void UploadAndReleaseTranscodeOutput(Pool& pool, Ref& output, Upload&& upload) {
    std::forward<Upload>(upload)();
    pool.FreeDeferred(output);
}

} // namespace Vulkan
