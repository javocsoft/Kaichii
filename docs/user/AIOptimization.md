# AI settings assistant

Kaichii can ask OpenAI or Anthropic to propose settings for your computer. You need your own API key
for the chosen provider; the provider may charge for each request. This is optional: performance
presets and **Optimize for This Game** work without AI.

The assistant is Kaichii's own: Lichii, Eden and yuzu don't have it.

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

To refine the result, try the game and describe what happened on the next request. Compare in the
same scene: a different scene or first-time shader compilation can change the frame rate. AI can
interpret symptoms and suggest settings; it cannot repair an emulator bug. For persistent missing
textures, see [Troubleshooting](Troubleshooting.md).

## Data and scope

The request includes your description, the game's name and program ID, operating system, GPU, VRAM,
CPU thread count, RAM, relevant settings, the assistant's last change, and any available recent
gameplay metrics. Games, saves, console keys,
screenshots and full logs are not sent. With developer mode on (in the settings), the window also
shows the exact diagnostic, under **Data sent with this request**, before sending it.
Kaichii validates the result locally; an invalid reply changes no settings.
