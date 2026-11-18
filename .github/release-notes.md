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
- **Fewer duplicate mesh pipeline builds with static vertex input.** A mesh draw could inherit vertex attributes left by an earlier draw and compile another Vulkan pipeline for the same shaders. An isolated Ocarina of Time save built 31 mesh pipeline keys instead of 72 for the same 26 shader sets with static vertex input. With the usual dynamic input setting, both builds produced 31 keys. This measures pipeline builds in one scene, not an FPS gain (#85).

### Switch Sports Resort
- **The Power Cruising shader can read an output attribute it wrote.** Kaichii previously rejected the reported `ISBERD.ATTR.32.O` instruction. It now supports that exact 32-bit, unskewed read in task and mesh programs while continuing to reject unmodelled modes. Synthetic translation tests and four SPIR-V validation cases pass. The affected game was not available for a visual check, so the water is not yet confirmed fixed (#86).

An isolated Linux build containing both changes loaded an Ocarina of Time save and rendered Link's house. A non-publishing workflow built Linux, Windows and macOS binaries successfully; Windows and macOS games were not run. No Switch 1 game was run.
