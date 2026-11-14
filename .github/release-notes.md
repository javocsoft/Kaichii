<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->
### Big fixes
- **Windows: games start again.** On Windows, build 36 stopped every game about a second after it started: the loading screen stayed on "Iniciando..."/"Starting...", the log showed `Break 0xE401`, and closing Kaichii hung on "Emulation thread failed to stop". Linux was not affected. The cause was in the emulated kernel: a game thread that waits inside a system call can wake up on another emulated core, and the Windows build (clang-cl with ThinLTO) kept using the core it had left, so the call's results went to another thread's registers. That thread then made calls the game never asked for and the game aborted. The kernel now reads which core and thread it runs on afresh each time, on every compiler. Played on Windows (i7-12700H, RTX 3050 Laptop) with Super Mario Bros. Wonder and Ocarina of Time, which both failed before; a build without the fix still failed on the same PC (#70).

### Crashes and stability
- **Tip if a game closes by itself on Windows:** programs that draw overlays (FPS counters, recorders) install Vulkan layers that load into Kaichii too. While investigating the fix above, Ocarina of Time closed at its first movie because of FPS Monitor's layer (`fpsmonvk64.dll`), which crashed inside its own code; uninstalling FPS Monitor fixed it. If a game closes without an error, try closing or uninstalling overlay programs first.

### For developers
- **The C++ tests run to the end on Windows.** A page table test wrote to memory that Windows had only reserved, which crashed the tests binary there; CI runs the tests only on Linux, so it was never seen (#71).
- **Release notes like these.** Each release now starts with what changed since the previous one: these written notes when there are new ones, and always the full list of what was merged (#72).
