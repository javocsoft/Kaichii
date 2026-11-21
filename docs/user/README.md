# Kaichii player's guide

Kaichii is a Nintendo Switch and Switch 2 emulator for the computers people actually have: laptops
and graphics cards with 4 GB of video memory included. This guide takes you from the download to
playing, and helps when something goes wrong.

> [!NOTE]
> Kaichii is not affiliated with Nintendo. Games, keys and firmware are not included: use only games
> and system files you have legally obtained from your own console.

> [!TIP]
> **New, and only in Kaichii: an [AI settings assistant](AIOptimization.md).** Describe the problem
> in your own words ("it stutters when I enter a new area") and Kaichii asks OpenAI or Anthropic,
> with your own API key, which settings suit your computer or one game. You review every change
> before it is applied, measure the result and undo it. It can also explain the last problem from
> the log and write a compatibility report from your own words. It is optional: everything else
> works without it.

## Start here

1. [Install Kaichii](Install.md) on Linux, Windows or macOS.
2. [First launch](FirstLaunch.md): the setup assistant, your games folder and your controller.
3. [Games, keys and firmware](Games.md): what each kind of game file needs.
4. [Settings for your computer](Performance.md): presets, *Optimize for This Game*, and what to
   expect on a 4 GB card.
5. [AI settings assistant](AIOptimization.md): optional proposals using your own OpenAI or Anthropic
   key.

## Games

- [The Legend of Zelda: Ocarina of Time](OcarinaOfTime.md)
- [Compatibility list](Compatibility.md): how games run, as players report them.
- [Sending a compatibility report](CompatibilityReports.md)

## When something goes wrong

- [Troubleshooting](Troubleshooting.md): the usual problems, where the log is, and how to report a
  bug.
- Questions and tips: [Discussions](https://github.com/javocsoft/Kaichii/discussions).

## More guides

These come from the projects Kaichii builds on (Eden and yuzu). They still apply, but may mention
those names; the data folder is still called `eden`.

- [Controllers](Controllers.md) and [gyro controls on the Steam Deck](GyroControls.md)
- [Updates and DLC](InstallingUpdatesDLC.md), [mods](Mods.md),
  [Atmosphère mods](InstallingAtmosphereMods.md), [cheats](UsingCheats.md) and
  [amiibo](UsingAmiibo.md)
- [Importing saves](ImportingSaves.md), [data and storage](Storage.md) and
  [orphaned profiles](Orphaned.md)
- [Multiplayer](Multiplayer.md), the [command line](CommandLine.md) and
  [a custom date and time](AlterDateTime.md)
- [Graphics settings in detail](Graphics.md)
