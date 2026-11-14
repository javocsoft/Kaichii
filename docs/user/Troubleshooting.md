# Troubleshooting

## The log

Kaichii writes what happens in a log. *File > Open Kaichii Folders > Log Folder* opens its folder:
`eden_log.txt` is the current or last session, and `eden_log.txt.old` the one before. Its first line
names the build. Attach it when you report a problem, after checking it for personal information.

## Kaichii doesn't start

- **Linux:** make the AppImage executable (`chmod +x`); if it mentions FUSE, see
  [Installing Kaichii](Install.md#linux).
- **Windows:** extract the whole folder, not only `kaichii.exe`. A missing `VCRUNTIME140.dll` or
  `MSVCP140.dll` means the Visual C++ Redistributable is needed (see
  [Installing Kaichii](Install.md#windows)).
- **macOS:** the app isn't notarized: right-click it and choose *Open* the first time.
- **Any system:** update the graphics driver. Kaichii draws with Vulkan, and old drivers are the most
  common reason a game doesn't start or shows a black screen.

## A game doesn't load: "bad header" or missing keys

`.nsp`, `.nsz`, `.xci` and `.xcz` games are encrypted, and Kaichii needs your console's decryption
keys (`prod.keys`) to read them. Without them the game can't be read at all, and Kaichii says the
keys are missing and offers to install them. An error such as "The NCA file has a bad header" with
the keys installed usually means a damaged or incomplete file. See [Your games](Games.md) for what
each kind of file needs. `.nup` games need no keys.

## The game runs slowly

- Use the preset Kaichii recommends, or *Test this computer*, in *Configure > Performance*.
- Try **Optimize for This Game** from the game's right-click menu.
- On a laptop with two graphics cards, choose the dedicated one in *Configure > Image*, **Device**.
- See [Settings for your computer](Performance.md).

## The game stutters

The first time each effect appears its shaders are built, and that can stutter. It fades as you play
and is smoother on later visits (see [Shader stutter](Performance.md#shader-stutter)).

## A generic icon in the taskbar on Linux

The taskbar takes an app's icon from its entry in the applications menu. Turn on **Add Kaichii to
the applications menu** in *Configure > Interface*, then start Kaichii again.

## Windows or dialogs look wrong on Linux

On Wayland desktops Kaichii draws its windows through X11 (XWayland) by default, so its dialogs fit
and stay where they belong. **Force X11 as Graphics Backend** in *Configure > Interface* (with *Show
advanced options*) turns it on or off; it takes effect when Kaichii restarts.

## Reporting a bug

1. Update to the [latest build](https://github.com/javocsoft/Kaichii/releases): the problem may be
   fixed already.
2. Check the [open issues](https://github.com/javocsoft/Kaichii/issues) for the same problem.
3. [Open an issue](https://github.com/javocsoft/Kaichii/issues/new/choose) with the form that fits
   (gameplay, crash, visual or interface), and include:
   - the build (the log's first line), the game and its version;
   - your processor, graphics card, driver version and operating system;
   - what you did, what happened and what you expected, with a screenshot if it helps;
   - the log of that session.

Never upload games, keys, firmware or account details.

For questions and tips, or when you aren't sure something is a bug, ask in
[Discussions](https://github.com/javocsoft/Kaichii/discussions).
