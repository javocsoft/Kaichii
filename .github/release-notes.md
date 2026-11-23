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
- **A Game Companion for Ocarina of Time.** An optional, offline reference panel for the Switch 2 edition: places, characters, enemies (the 77 names of the Hall of Fallen Foes, plus Gohma Eggs), bosses, dungeons, items and songs, what the remake changed, and where the game sits in the Zelda timeline, in English and Spanish. Open it from *View > Show Game Companion* or the game's right-click menu; it is off by default. Search, filter by topic, and open a card for context, tips and a link to its source; the ✕ on a card closes it and gives the list its space back. Story spoilers (later bosses, dungeons, the timeline split) stay hidden until you tick *Show story spoilers*, and switch off again when the panel closes. While you play from a `.nup` file, it can suggest the card for the area or boss arena the game last loaded; it does not read Link's position or what is on screen, and it never goes online by itself. Unit tests cover the catalog, search, spoilers, scene suggestions and the card; played in Ocarina of Time on Linux. Guide: *Game Companion* in the player's guide.

### For developers
- The debug server's `status` reports `companion_scene`, the last Ocarina of Time scene the loader saw read from the game's files.
