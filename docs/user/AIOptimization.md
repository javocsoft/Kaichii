# AI settings assistant

Kaichii can ask OpenAI or Anthropic to propose settings for your computer. You need your own API key
for the chosen provider; the provider may charge for each request. This is optional: performance
presets and **Optimize for This Game** work without AI.

The assistant is Kaichii's own: Lichii, Eden and yuzu don't have it. Besides tuning settings, it
can [explain the last problem](#explain-the-last-problem) from Kaichii's log and
[write a compatibility report](#write-a-compatibility-report) from your own words.

## Use it

- **All games:** open *Emulation > Ask AI to tune this computer*. Stop any game first. The proposal
  changes global settings and can be tried after restarting Kaichii.
- **One game:** right-click it and choose *Ask AI to tune this game*. The same action is in
  *Emulation* while playing. The game need not be running. When it is running, recent FPS and memory
  figures are included if available. Kaichii asks before stopping it to save changes; try them on
  the next start.

To get a key, follow the link under the key field: it opens the provider's key page
([OpenAI](https://platform.openai.com/api-keys), [Anthropic](https://platform.claude.com/settings/keys)).
Choose a provider and a model, enter your key, describe the issue and click **Ask for a
proposal**. The model list shows each provider's main models, first the recommended one; you can
type another name, or click **Load from provider** to list the models your key can use. Kaichii
remembers the provider and model you used last.

To avoid typing the key every time, tick **Remember this key in the system's credential store**.
Once a request succeeds, the operating system keeps it: the Keychain on macOS, the Credential
Manager on Windows, and the Secret Service (GNOME Keyring, KWallet) on Linux, through libsecret. It
is never written to Kaichii's settings files, and Kaichii never sends it anywhere but to your
provider. **Forget saved key** removes it, and so does a successful request with the box unticked.
Without a credential store (Linux without libsecret) the box is greyed out and the key stays in
memory only while the window is open.

Review each proposed change, its reason and its previous value. Uncheck any you do not want. Kaichii
accepts only catalogued options and values and records the previous settings. Reopen the assistant
and click **Undo last AI change** to restore them. If you have manually changed one of those settings
since then, undo will leave your newer choices alone.

### Measure the result

Settings change when the game starts, so the assistant measures across a restart. If the game is
running when you apply a change, Kaichii keeps the frame rate, 1% low and stutters of the last 10
seconds before stopping it. Start the game again, play the same scene for a few seconds, reopen the
assistant and click **Measure now (last 10 s)**. It shows both measurements and a verdict: better,
worse or no clear change (a few percent either way is the difference between two runs of one
scene). Click **Keep these settings** if the game also looks right, or **Undo last AI change** if
not. The next request includes the measurements, so the AI knows whether its last change helped.

Compare in the same scene: a different scene or first-time shader compilation changes the frame
rate. AI can interpret symptoms and suggest settings; it cannot repair an emulator bug. For
persistent missing textures, see [Troubleshooting](Troubleshooting.md).

## Explain the last problem

*Help > Explain the last problem* reads Kaichii's log for the usual causes of a game that doesn't
start, crashes or shows something wrong: missing or old keys, a damaged game file, no firmware,
Vulkan or the graphics driver, video memory, shaders Kaichii can't translate yet, and crashes in the
emulator. This part needs no AI and sends nothing; each finding links to the page of this guide that
explains it. Choose **The previous session** when Kaichii was restarted after the problem.

For more, describe what happened (optional) and click **Ask the AI to explain**. The AI answers in
your language, with what most likely went wrong and what to try. Only the log lines about the
problem and the build are sent, with your user name taken out of paths; tick **Show the log lines
that would be sent** to read them first. When the answer points at a bug in Kaichii, **Draft a bug
report on GitHub** opens the bug form with a title, a description and those lines filled in: review
it, attach `eden_log.txt` and publish it with your account.

## Write a compatibility report

In a [compatibility report](CompatibilityReports.md), click **Write it with AI…**, tell how the game
ran in your own words and in any language, and click **Write the report**. The AI writes the summary,
progress, details and steps in English, using only what you said; you still choose the compatibility
and smoothness, and review everything before opening it on GitHub. Sent: your words and what the
report already includes (the game, the build, your graphics card, operating system and launch
settings).

## Data and scope

A settings request includes your description, the game's name and program ID, operating system, GPU, VRAM,
CPU thread count, RAM, relevant settings, the assistant's last change, any available recent
gameplay metrics, and the measured result of the last change. Games, saves, console keys,
screenshots and full logs are not sent. With developer mode on (in the settings), the window also
shows the exact diagnostic, under **Data sent with this request**, before sending it.
Kaichii validates the result locally; an invalid reply changes no settings.
