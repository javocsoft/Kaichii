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
- **A full cover view, with each game's box art.** *View > Game List Mode > Full Cover View* shows the games as cases on a shelf, in a Switch case's shape and without names. A game's properties have a new *Box Art* tab, next to its icon, to choose the picture or remove it; it works for every game format and is kept in Kaichii's `box_art` data folder, never in the game file, so pictures can also be copied there by hand under the game's title id. A game without box art shows its icon on a case. Unit tests cover storing, replacing and removing box art, the editor and the view drawing it; checked on screen with synthetic pictures, not with real box art (#92).

### Interface
- **WebP pictures work as icons and box art.** Choosing a `.webp` file said the image wasn't valid, although the file dialog offered WebP: Kaichii's builds didn't include Qt's WebP reader. Every build now installs and ships it (Linux AppImage, Windows; macOS already had it). With the `.tar.gz`, install your system's Qt image formats plugins. A unit test fails if the reader is missing, and a reported WebP box art (393 by 640, with transparency) was decoded with it (#93).
- **Covers with transparent parts show the shelf behind them.** In *Cover View*, a game whose icon has transparent areas (Donkey Kong Bananza's cut-out art, the rounded corners of Super Mario Bros. Wonder's) showed a grey square there: the shelf's shadow was drawn as a full square under every cover. The shadow now follows the picture's own shape. A unit test paints a cover with a transparent cut-out and checks that the shelf shows through; the real covers were not checked on screen (#91).
