<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->
### Graphics and performance
- **Switch 2 DMA copies and clears keep texture-list pages current.** Some destination pages were missed when the source had no GPU-modified range, which could leave later GPU reads with stale data. Synthetic copy and clear tests pass; no game image has been compared for this change (#81).
- **Vulkan avoids a mid-draw wait when descriptor memory fills.** Descriptor storage can grow within a bounded frame budget and submits a pending command buffer before reusing its memory. This addresses a potential stall and reuse hazard; no frame-rate gain has been measured (#81).
- **Large ASTC arrays use smaller temporary decode images on integrated GPUs.** GPU ASTC-to-BC transcoding now works in layer batches when needed and keeps its output buffer reserved through upload recording. Batch and lifetime tests pass; no integrated-GPU game run has measured the visual result or speed (#81).
- **Mario Kart World receives its expected GPU timestamp path.** Its Switch 2 program ID selects the legacy timestamp calculation while other games keep the Ampere calculation. A program-ID test passes; Mario Kart World was not run for this change (#81).

### Crashes and stability
- **Short or unmapped data is handled more carefully.** Guest memory spans check every page and their exclusive end, short hexadecimal keys no longer read beyond their value, CMIF short reads preserve the untouched guest buffer, and unsigned bitfield folding handles word-boundary cases. Synthetic regression tests pass; no particular game crash was reproduced for this batch (#81).
- **Missing render targets and staging-buffer wraps stay distinguishable and safe.** A failed render-target lookup no longer looks like a valid null texture descriptor, and a busy first streaming region does not lose the current staging position. Both cases have regression tests; no game-specific visual comparison was made (#81).

### Controls, audio and networking
- **A game no longer overwrites your saved volume and mute choices.** Its audio-target commands still change its own state and may affect the current sound, but they do not rewrite the user's audio settings. The service was tested through IPC, without a game-specific audio comparison (#81).
- **Motion timing resets when a controller reconnects.** SDL restarts the sensor clock and its error backoff, avoiding a stale timestamp after reconnect. The clock test passes; a physical controller reconnect has not been tested for this release (#81).
- **HTTPS requests use a usable CA bundle.** Kaichii and its updater select a system certificate file when available, honor explicit OpenSSL overrides and retain the bundled fallback. Certificate selection tests pass; this is not a claim that every network endpoint was exercised (#81).

The user launched the updated Linux build successfully. Linux Clang and GCC builds and their test suites passed; Windows and macOS game runs remain unverified.
