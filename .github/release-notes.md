<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->

### Saves and game list
- **A save folder copied to the wrong place is no longer deleted.** A game's save folder copied one level too high in the NAND (straight into `user/save/0000000000000000`) showed the *Orphaned Profiles Detected* message at every start, and an empty folder there was deleted. Only real profile folders count now. Port of Lichii #2265; a unit test covers title folders, other folders and real orphaned profiles (#100).
- **Play time survives a crash when stopping a game.** When the services closed after a stop, a second copy of the play time list saved the times read at boot over the session just played; if Kaichii ended before saving again, the session was lost. Port of Lichii #1971, with a unit test (#100).
- **Restart keeps the game's update.** Starting a game cancels the game list's rescan, and a cancelled rescan dropped the updates and add-ons it had registered, so *Restart* could boot the base version. Port of Lichii #2298 (as Fritchii ships it), with a unit test (#100).

### Crashes and stability
- **Correct rounding in the CPU emulation.** Some rounding instructions (half precision, round-half-away and exact rounding, and every one on processors without SSE4.1) rounded the wrong way on x86-64 PCs, in any game. Port of Lichii #2266, with a test of each instruction (#100).
- **Shadows at other resolutions.** A game's slope depth bias was halved at 2x and doubled at 0.5x (the *Performance* preset): fine stripes on lit surfaces above 1x, shadows drawn further from objects below. It now keeps the game's value at any scale. Port of Lichii #2057, with a unit test; not checked in a game (#100).
- **An Ocarina of Time hang in the GPU command stream.** A command buffer called between the two halves of a split register write was taken as the write's data. Port of Lichii #2415, with a unit test of the reported pattern; upstream asks for long play tests before trusting it (#100).
- **Smaller fixes from Lichii:** the emulated timer could spin forever on an expired event (#2252, with a test); code invalidated from another thread could still run from a stale copy (#2053); textures with unmapped sparse parts could crash (#2360); a linear texture copy could read past its buffer (#2414); long shader address chains could overflow the stack (#2301); Xenoblade Chronicles DE crashed switching between docked and handheld (#2299); the configuration file is flushed to disk before it replaces the old one (#1972); and Switch 2 syncpoint waits armed again are woken when their value moves (#2052) (#100).

### Interface
- **A warning when the graphics will run Switch 2 games too slowly.** On integrated graphics (an Intel Iris Xe laptop was reported at 6 to 7 fps in Switch 2 games, while Switch games ran fine) Kaichii now says that Switch games run well but Switch 2 games need a dedicated graphics card: on *Configure > Performance*, in the first start's assistant, and once per graphics card when a Switch 2 game starts. After *Test this computer*, the measurement decides instead: below half the reference computer's graphics speed any card gets the warning, and a fast integrated GPU that measures above it doesn't. On a laptop that renders with its integrated graphics but also has a dedicated card, the warning names that card and where to choose it. With a software renderer such as llvmpipe (a missing or broken graphics driver) it says instead that every game will be very slow and to install the driver. Unit tests cover when it shows, the texts and the assistant; not checked on a machine with integrated graphics (#99).
