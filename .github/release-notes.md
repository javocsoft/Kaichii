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
- **A warning when the graphics will run Switch 2 games too slowly.** On integrated graphics (an Intel Iris Xe laptop was reported at 6 to 7 fps in Switch 2 games, while Switch games ran fine) Kaichii now says that Switch games run well but Switch 2 games need a dedicated graphics card: on *Configure > Performance*, in the first start's assistant, and once per graphics card when a Switch 2 game starts. After *Test this computer*, the measurement decides instead: below half the reference computer's graphics speed any card gets the warning, and a fast integrated GPU that measures above it doesn't. On a laptop that renders with its integrated graphics but also has a dedicated card, the warning names that card and where to choose it. With a software renderer such as llvmpipe (a missing or broken graphics driver) it says instead that every game will be very slow and to install the driver. Unit tests cover when it shows, the texts and the assistant; not checked on a machine with integrated graphics (#99).
