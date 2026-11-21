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

## How the list shows your games

*View > Game List Mode* picks one of five views: *Tree View* (a table with details), *Grid View*,
*Carousel View*, *Cover View* (the games' icons, large, as on a shelf) and *Full Cover View* (each
game's case, as in a shop).

The full cover view shows the box art you choose. Open a game's properties (right-click,
*Configure Game*), choose the *Box Art* tab above the game's details, then *Choose Box Art...* and
*OK*. *Remove* goes back to the icon. A game without box art shows its icon on a case, so the shelf
has no gaps. Box art works for every game format and never changes the game file.

Kaichii keeps box art in the `box_art` folder of its data folder (see
[where your data lives](Install.md#where-your-data-lives)). To add many at once, copy the pictures there named
after each game's title id, as *Configure Game* shows it, in capitals: `0400486024DEC000.jpg` for
Ocarina of Time. PNG, JPEG and WebP work, best at about 105 by 170 in shape; other shapes are
cropped to fill the case.

### Your own icon for a game

The *Icon* tab in a game's properties changes the icon every view shows. For NSP, XCI and the other
formats, *Upload Icon...* takes a picture of your own, cropped to a square, and *Restore Game Icon*
goes back to the game's; the game file is never changed, and the icon is kept in the
`custom_icons` folder of Kaichii's data folder, named like box art. NUP games work as before: their
*Upload Icon...* and *Detect Icon* write the icon into the `.nup` file itself.

Next: [Settings for your computer](Performance.md).
