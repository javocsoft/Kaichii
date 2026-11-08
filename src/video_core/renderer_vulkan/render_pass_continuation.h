// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Vulkan {

/// Whether a draw into a framebuffer can stay in the render pass already open on it.
///
/// A deferred clear opens the framebuffer with a variant of its render pass, which differs only in
/// load and store ops, so the two are compatible. On a tile-based GPU the draws after the clear
/// stay in that variant: restarting with the framebuffer's own pass stored the attachments and
/// loaded them back, and lost the multisampled contents a variant discards (only tilers discard
/// them). An immediate-mode GPU gains nothing from that, and Ocarina of Time ran slightly slower
/// on an RTX 3050 when it continued (35.0 against 35.5 fps over nine alternating runs), so there
/// the open pass must be the framebuffer's own.
template <typename RenderPassHandle, typename FramebufferHandle, typename Extent>
[[nodiscard]] constexpr bool ContinuesRenderPass(bool tiler, RenderPassHandle active_pass,
                                                 FramebufferHandle active_framebuffer,
                                                 const Extent& active_area,
                                                 RenderPassHandle pass,
                                                 FramebufferHandle framebuffer,
                                                 const Extent& area) {
    if (!active_pass || framebuffer != active_framebuffer || area.width != active_area.width ||
        area.height != active_area.height) {
        return false;
    }
    return tiler || pass == active_pass;
}

} // namespace Vulkan
