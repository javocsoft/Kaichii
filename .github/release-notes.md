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
- **Revisiting Switch 2 textures can cause fewer memory stalls.** Kaichii now keeps uploaded game-memory pages in RAM while the PC has enough free memory. When RAM is low, it still gives the OS a cold-page hint; Linux uses a gentler hint instead of requesting immediate pageout. This avoids paging out data that the game may soon need again. Synthetic policy and range tests pass. A user played Ocarina of Time on Linux and reported noticeably smoother gameplay, but no controlled FPS gain has been measured (#82).

The Linux Qt build opened the user's existing game library and launched Ocarina of Time. Windows and macOS game runs have not been verified for this change.
