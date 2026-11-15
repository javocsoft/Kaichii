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
- **A cover view for the game list.** View → Game List Mode → Cover View shows your games as their covers alone, large and without names, like a shelf. The covers fill the window's width in whole columns and resize with it; each is the game's own artwork at full resolution, with rounded corners and a soft shadow. Point at a cover to see the game's name, double-click to play, right-click for the usual menu; searching and controllers work as in the other views. Seen on Windows (#79).
- **Compatibility reports say how smoothly a game ran.** The report now asks two separate things: **Compatibility**, whether the game works (graphics, sound, crashes, progress), and the new **Smoothness**, how fluid it felt on your computer with your settings: Perfect, Smooth, Occasional stutters, Choppy or Very poor. You can also give the usual frame rate, which the performance overlay shows. A game that only loads or doesn't start has no smoothness to give, so the choice turns off. The compatibility descriptions now speak only of whether the game works (#77).

### Compatibility list
- **See at a glance how a game plays, not only whether it works.** The [compatibility list](https://javocsoft.github.io/Kaichii/Compatibility/) shows each report's smoothness and frame rate under its compatibility, in the comparison by operating system and in a Smoothness column of each system's table. The first such report: Ocarina of Time on Windows 11, Perfect, with occasional stutters at 25-33 fps. Reports sent before this release show a dash (#77).
