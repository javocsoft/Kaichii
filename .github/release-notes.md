<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->

### Interface
- **Covers with transparent parts show the shelf behind them.** In *Cover View*, a game whose icon has transparent areas (Donkey Kong Bananza's cut-out art, the rounded corners of Super Mario Bros. Wonder's) showed a grey square there: the shelf's shadow was drawn as a full square under every cover. The shadow now follows the picture's own shape. A unit test paints a cover with a transparent cut-out and checks that the shelf shows through; the real covers were not checked on screen.
