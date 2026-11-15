<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->
### New features
- **A redesigned performance overlay.** View → Show Performance Overlay now shows the frame rate in large type, the frame time, and how busy the CPU and the GPU are: the CPU figure is the busiest emulated core or GPU emulation thread, the GPU figure is the host GPU's busy time per frame, and the higher one is highlighted because it is what limits the frame rate. Below it, a 5-second graph of frame times marks the min, median and max and draws slow frames in red, and a footer shows the upscaler, the resolution scale and how many frames frame generation adds. For Switch 2 games the numbers come from the game's real frames and the footer shows the 1% low and the stutters. The overlay still stays where you drag it and remembers the spot. Its background is solid rather than see-through, so it keeps working on COSMIC and Wayland. Ported from Zortos's redesign in Lichii (zortos293/Lichii#1345), and used on Windows with Ocarina of Time (#74).
- **Icons in the menus.** The File, Emulation, View, Tools, Multiplayer and Help menus show line icons, as Lichii's do. In Lichii they come only with its own dark theme; here they work in every theme, darker on light themes and lighter on dark ones, and they follow a theme change. Options with a check box have none, so their check mark stays visible. The icons are Lichii's, from zortos293/Lichii#1365 (#75).

### Compatibility list
- **Reports are compared by operating system.** The same game can run better on Linux than on Windows on the same PC, and every report already records its operating system. The [compatibility list](https://javocsoft.github.io/Kaichii/Compatibility/) now opens with a table of games against Windows, Linux and macOS, showing the newest report on each and how many there are, then lists every report in a section for its system with the system's full name. Reports of the same game under different titles count as one game. Existing reports are included as they are (#73).

### For developers
- **Qt Charts is no longer needed.** The performance overlay was its only user (#74).
- **The GPU busy time is measured without `EDEN_DKB_FRAME_TRACE`** when the overlay asks for it, one frame at a time; nothing is recorded while the overlay is hidden (#74).
- **`tools/ci/test_menu_icons.py`** checks that every menu icon the frontend asks for exists and is in its resource file (#75).
