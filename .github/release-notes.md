<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->
### Performance
- **Frame pacing targets game frames when frame generation is on.** Target 30/60/90/120 now count each game's frame once and space its generated presents within that interval. Generation also leaves a free presentation frame for the next game frame, avoiding a wait when the queue fills. Unit tests cover target rates and queue capacity. Donkey Kong Bananza ran with FSR3 2x and Target 60 and produced generated frames, but no before-and-after pacing or FPS gain was measured (#83).

### Crashes and stability
- **Texture cleanup uses the image's own GPU address space.** Unregistering an image after a channel switch no longer looks in the newly bound channel's page table. A synthetic two-address-space test passes; the reported multi-channel game case has not been replayed (#83).
- **VP9 header encoding no longer reads past its stream while propagating a carry.** The regression test reproduced the old stream exception and passes with the fix. The affected video scene has not been replayed (#83).
- **Windows sparse-vector pages stay writable after another thread commits them.** This avoids a protection change that could race with a GPU cache update. Both Windows binary variants compile; the Windows-only regression test and a game run have not been performed (#83).

The Linux build ran Donkey Kong Bananza with FSR3 frame generation. No Switch 1 game was run. Windows game runs have not been verified, and GitHub did not assign a macOS runner for the test build.
