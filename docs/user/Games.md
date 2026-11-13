# Games, keys and firmware

Kaichii only runs games you have dumped from your own console. It doesn't download or include any.

## What each kind of file needs

| File | What it is | Needs |
|---|---|---|
| `.nup` | A Switch 2 game package | Nothing else |
| `.nsp`, `.nsz`, `.xci`, `.xcz` | Switch games, installable or cartridge dumps | The decryption keys of your console; some games also need its firmware |
| `.nro`, `.nso` | Homebrew and executables | Nothing else |

## Keys

Keys come from your own console, as a `prod.keys` file. Install them with *Tools > Install
Decryption Keys* (or from the setup assistant's first page) and choose the file. Kaichii copies it to
its `keys` folder.

## Firmware

Some games and the system applets need your console's firmware. Install it with *Tools > Install
Firmware > From Folder* or *From ZIP*, whichever way you dumped it.

## Updates and DLC

Install them with *File > Install Files to NAND*. A game's right-click menu, *Configure Game > Add-Ons*,
turns each one on or off. See [Updates and DLC](InstallingUpdatesDLC.md) for the details.

Next: [Settings for your computer](Performance.md).
