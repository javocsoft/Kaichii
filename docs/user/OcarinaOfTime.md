# The Legend of Zelda: Ocarina of Time

The Switch 2 edition (program ID `0400486024DEC000`) is the game Kaichii is tuned and measured
with. It comes as a NUP package, so it needs no keys or firmware.

## Recommended settings

- **On a card with 4 GB, the Performance preset.** It draws at half resolution, upscales with FSR,
  decodes textures on the processor, and turns off the game's sharpening pass.
- **Ocarina of Time anti-aliasing**, in *Configure Game > Image* (it only appears in this game's
  settings):
  - *Game* keeps the game's own anti-aliasing network, which sharpens heavily.
  - *No sharpening* shows the image as the game renders it, without that network: faster, and a
    good match for a higher resolution.

  It takes effect the next time the game starts, and does nothing while NVIDIA DLSS is on.

## What to expect

Measured on a laptop with an RTX 3050 (4 GB), in Link's house:

| Settings | fps |
|---|---|
| Native resolution, the game's anti-aliasing, textures decoded on the graphics card | 14 |
| Native resolution, *No sharpening*, textures decoded on the processor | 31 |
| Balanced preset | 32 |
| Performance preset | 40 |

Outdoors, the game's own main thread on the processor sets the pace, so lower graphics settings help
less there than indoors, and a faster processor helps more. The first visit to each area can stutter
while its shaders are built; later visits are smoother (see
[Shader stutter](Performance.md#shader-stutter)).

## Reports

The [compatibility list](Compatibility.md) shows how the game runs for other players. After playing,
[send a report](CompatibilityReports.md) of your own: it says which hardware and settings work.
