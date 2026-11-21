# Sending a compatibility report

A compatibility report tells other players, and Kaichii's developers, how a game runs on your
computer. Reports are public issues in Kaichii's repository, and the
[compatibility list](Compatibility.md) gathers them by operating system, so you can see how a game
runs on Windows, Linux and macOS side by side.

## How

1. Play a Switch 2 game for a while, then stop it.
2. Right-click the game in the list and choose **Submit compatibility report**.
3. In **Experience**, choose whether the game works:

   | Compatibility | Means |
   |---|---|
   | Perfect | Works as on the console, no errors |
   | Playable | Can be played through; small errors that don't get in the way |
   | Runs | Gets into the game, but errors or crashes get in the way |
   | Loads | Starts, but doesn't get into the game |
   | Unplayable | Crashes or doesn't start |

   Then how smoothly it ran on your computer, with the settings it started with:

   | Smoothness | Means |
   |---|---|
   | Perfect | Steady at the game's frame rate, no stutters |
   | Smooth | Steady, with a rare stutter |
   | Occasional stutters | Runs well, but stutters in some places |
   | Choppy | Frequent drops or stutters you notice while playing |
   | Very poor | Slow most of the time, hard to play |

   A game that only loads or doesn't start has no smoothness to give, so the choice is turned off.
   If you know it, add the usual frame rate: the performance overlay (**View** menu) shows it.

   Then write a summary, how far you played and what worked or failed. Or click **Write it with
   AI…** and tell it in your own words, in any language: the
   [AI assistant](AIOptimization.md#write-a-compatibility-report) writes those fields in English
   for you to review. It needs your own OpenAI or Anthropic API key.
4. **System and settings** is already filled in: the game and its version, the Kaichii build, your
   processor, graphics card and driver, the operating system and the settings the game started with.
   Correct anything that changed while you played.
5. **Preview** shows the report as it will be published. Press **Open on GitHub**.
6. GitHub opens with the report filled in. Tick the two confirmations (a game you dumped yourself,
   and no game files, keys, firmware, saves or private details) and publish it with your GitHub
   account.

Kaichii sends nothing by itself: the report is only published when you publish it on GitHub. If it
is too long for a link, or the browser doesn't open, Kaichii copies it to the clipboard so you can
paste it. **Copy report** does the same at any time.

The option is there for the last game you played in this session: starting another game, or
closing Kaichii, discards it.
