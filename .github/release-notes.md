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
- **The Game Companion now covers Breath of the Wild.** The offline reference panel that came with Ocarina of Time in v1.0.52 now has a catalog for *The Legend of Zelda: Breath of the Wild* on Switch, with 88 cards in English and Spanish: regions and settlements, characters, enemies, bosses, the Divine Beasts and shrines, equipment, runes and the Champions' abilities, the Expansion Pass, the Nintendo Switch 2 Edition and the story of the Great Calamity, each linked to the Zelda Wiki article it was checked against. It has its own colors and topics, and the panel switches when you change game. Entering a shrine or a Divine Beast suggests its card, with `.nsp` and `.xci` files as well as `.nup`; regions are not suggested, because the open world streams around Link wherever he goes. Played on Linux with the game's base version: the scene detection followed the game into the open world from a saved game; entering a shrine or a Divine Beast was not played. Unit tests cover the catalog, the scene suggestions and the panel.

### For developers
- Scene detection for the Game Companion moved out of the NUP loader into `src/core/loader/companion_scenes.cpp`, and packed games (`.nsp`, `.xci`) are watched through `FileSys::WatchRomFSReads`, which maps the game's reads of its RomFS image back to file paths. The debug server's scripted input (`input`, `input_sequence`) now works in Switch games too, counting 60 Hz frames of wall time, since their presents are not counted.
