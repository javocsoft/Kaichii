# Installing Kaichii

Builds are on the [releases page](https://github.com/javocsoft/Kaichii/releases). Each release comes
in a standard and a **PGO** build: the same program, compiled with profiles recorded while playing.
Take the PGO one when your system has it.

## Linux

Download `kaichii-linux-x86_64-pgo.AppImage` (or `kaichii-linux-aarch64-pgo.AppImage` on ARM), make
it executable and run it:

```sh
chmod +x kaichii-linux-x86_64-pgo.AppImage
./kaichii-linux-x86_64-pgo.AppImage
```

The AppImage carries everything it needs. It runs on systems with glibc 2.39 or newer: Ubuntu
24.04, Fedora 40, Debian 13, Arch and the distributions based on them.

- **It says it needs FUSE.** Install your distribution's FUSE 2 package (`libfuse2t64` on Ubuntu
  24.04, `fuse2` on Arch), or start it with `APPIMAGE_EXTRACT_AND_RUN=1 ./kaichii-…AppImage`.
- **The applications menu and the taskbar icon.** Desktops show an app's icon from its entry in
  the applications menu, which an AppImage doesn't install by itself. The first time it starts,
  Kaichii offers to add itself there; say yes and the taskbar shows its icon. **Add Kaichii to
  the applications menu** in *Configure > Interface* changes it later.
- **You'd rather use your system's libraries.** The `.tar.gz` holds the same programs without
  their libraries; Qt 6 and the usual libraries must be installed.

## Windows

Download `kaichii-windows-x86_64-pgo.zip`, extract **the whole folder** wherever you like, and run
`kaichii.exe`.

- **Windows protected your PC.** SmartScreen doesn't know the program yet: choose *More info* and
  *Run anyway*.
- **A missing `VCRUNTIME140.dll` or `MSVCP140.dll`.** Install Microsoft's
  [Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)
  for x64.

## macOS

Download `kaichii-macos-arm64.zip` (Apple Silicon, macOS 15 or newer) and unzip it. The app isn't
notarized, so the first time right-click it and choose *Open*, or run
`xattr -dr com.apple.quarantine kaichii.app`. macOS support is experimental.

## Updates

Kaichii looks for a newer build each time it starts and offers to install it. The AppImage and the
Windows folder update themselves; your saves and settings stay. Turn the check off with **Check for
updates** in *Configure > Interface* (or type its name in the settings search).

## Where your data lives

Kaichii keeps saves, keys, firmware, settings and shader caches in a folder named `eden` (the name
of an earlier project, kept so nothing gets lost):

| System | Folder |
|---|---|
| Linux | `~/.local/share/eden` (data), `~/.config/eden` (settings), `~/.cache/eden` (caches) |
| Windows | `%APPDATA%\eden` |

*File > Open Kaichii Folders* opens them. On Windows, a folder named `user` next to `kaichii.exe`
makes Kaichii portable: everything is kept there instead.

### Coming from another emulator

Lichii and Eden use the same `eden` folder, so Kaichii finds your saves, settings and caches as they
are. From yuzu and other emulators based on it, Kaichii offers to import their data on its first
start. A build still named Lichii won't update itself to Kaichii: download Kaichii once by hand.

Next: [First launch](FirstLaunch.md).
