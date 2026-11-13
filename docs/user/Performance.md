# Settings for your computer

Most of what makes a game run well on a modest computer is a handful of settings. Kaichii picks them
for you: you only need to check them when a game asks for more than your computer has.

## Presets

*Configure > Performance* shows what Kaichii found about the computer (graphics card, video memory,
processor threads and memory) and four presets. The one for this hardware is marked as recommended.

| Preset | Resolution | For |
|---|---|---|
| Performance | Half, upscaled with FSR | Integrated graphics, laptops, cards with 4 GB or less |
| Balanced | Three quarters, upscaled with FSR | Cards with 6 to 8 GB |
| Quality | Native | Cards with 10 to 12 GB |
| Ultra | One and a half times native | Cards with 16 GB or more |

Every preset also sets fast GPU accuracy and asynchronous shaders, fits the texture memory to the
card, and decodes ASTC textures on the processor when it has 8 threads or more: most PC graphics
cards can't read ASTC, the format Switch games use, and decoding it on the card competes with the
game. On processors with at least six performance cores, presets also keep the GPU threads on
performance cores of their own. In Ocarina of Time, Performance also skips the game's sharpening
pass.

- **Use this preset** fills in the settings. Nothing is saved until you press *OK*.
- **Test this computer** measures the graphics card's speed, the video memory free now and the
  processor's speed in about ten seconds, and recommends a preset from them. Close any game first.
- In a game's *Configure Game > Performance*, the same page sets that game only.

## Optimize for This Game

Right-click a game and choose **Optimize for This Game** (while playing: *Emulation > Optimize for
This Game*, or Ctrl+Shift+O). Kaichii measures the game as it runs, tries frame generation for a
few seconds, and proposes changes; nothing changes until you accept. Stand somewhere demanding, such
as outdoors with the camera still, and don't pause or open menus while it measures.

## On a card with 4 GB

- Start from the **Performance** preset.
- If busy scenes slow down sharply or images go missing, video memory is running out: lower the
  *Resolution* (*Configure > Image*) or set **VRAM Usage Mode** to *Conservative*.
- Keep **ASTC Decoding Method** on *CPU* or *CPU Asynchronous* unless the processor is slow.

## Shader stutter

The first time an effect appears, its shaders have to be built for your graphics card, and the game
can stutter for a moment. Kaichii builds new Switch 2 shaders in the background while the game keeps
running, keeps them for the next time, and prepares the ones from earlier sessions before the game
needs them. Stutter is worst on a first playthrough and fades as you play. On Linux with an NVIDIA
card, Kaichii gives the driver's shader cache room for large games.

## Measured

The Legend of Zelda: Ocarina of Time on a laptop with an RTX 3050 (4 GB), in Link's house:

| Settings | fps |
|---|---|
| Native resolution, the game's anti-aliasing, ASTC on the GPU (the old defaults) | 14 |
| Performance preset | 40 |
| Balanced preset | 32 |

One game on one computer: your results depend on the game, the hardware and the driver. The
[compatibility list](Compatibility.md) shows what other players see.

For every graphics option in detail, see [Graphics settings in detail](Graphics.md).
