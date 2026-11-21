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

*View > Sort Games By* orders every view but the tree, and remembers your choice:

- *Name*: as you'd read them, case and accents aside, numbers by value (*Mario 2* before
  *Mario 10*).
- *Date Added*: when the game file arrived on this computer (copied, downloaded or converted).
- *Play Time*: how long you have played each game; it updates when you close a game.

*Ascending* and *Descending* turn any of them around. Picking an order starts it the way it's most
useful: names from A, the newest and the most played first. Games equal in the order, such as the
ones you never played, follow by name. The tree view sorts by whichever column you click.

Right-click an empty part of the list for the same choices: the list mode, the order and the Switch 2
sections. In the two cover views, *View > Separate Switch 2 Games* shows Switch 2 games first, under
their own heading, then the Switch ones under theirs. A game counts as Switch 2 by its title id
(Switch 2 ids start with `04`), so a Switch game with a Switch 2 Edition upgrade stays with the
Switch games.

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
