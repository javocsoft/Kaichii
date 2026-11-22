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
- **Less video memory held for nothing on small cards.** The buffer cache's small buffers live in pool blocks that stay allocated while any buffer in them lives. On discrete cards with under 9 GB, the blocks are now 4 MB instead of 16 MB. On an RTX 3050 Laptop (4 GB) in Ocarina of Time, memory held this way fell from 15 MB to 3 MB, with no change in frame rate. Port of Lichii #2316, with a unit test (#101).
- **The GPU thread no longer keeps a core busy while it waits.** While waiting for the second GPU thread (full command ring, or the end of its idle work), the first one spun or yielded in a loop; it now sleeps until woken. Standing on Link's porch in Ocarina of Time, Kaichii used about 0.15 fewer CPU cores (6.04-6.15 against 6.22-6.27, three rounds each), at the same frame rate. Port of Lichii #2237, with a unit test (#101).

