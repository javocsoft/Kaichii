<!--
Written notes for the next release. release-binaries.yml puts them under "Changes since build N",
before the list of everything merged and the download tables, and only when this file changed since
the previous release: rewrite it in a pull request before each release (see docs/CI.md).

Style, as in Lichii's releases: sections such as "Big fixes", "New features", "Performance", one per
game, "Crashes and stability" and "For developers". Each entry starts with a bold sentence a player
understands, then says what they saw, what caused it, what was measured or played, and ends with the
pull request number. Say plainly what was not confirmed. HTML comments like this one are dropped.
-->

### New features
- **An AI settings assistant, only in Kaichii.** *Emulation > Ask AI to tune this computer*, or *Ask AI to tune this game* from a game's right-click menu, takes a description of the problem in your own words and asks OpenAI or Anthropic, with your own API key, which settings suit your computer. It can only propose options from a fixed catalog, shows each change with its reason and previous value before applying it, and *Undo last AI change* restores them. It is optional: presets and *Optimize for This Game* work as before (#88).
  - **Your key can be remembered** in the system's credential store (Keychain on macOS, Credential Manager on Windows, Secret Service on Linux), never in Kaichii's files. It is saved only after a request succeeds, and *Forget saved key* removes it.
  - **Pick a model from a list** of each provider's main models, or load the ones your key can use; the last provider and model are remembered. A link under the key field opens the provider's page to create a key.
  - **A spinner and the seconds spent** show while the provider answers, and a second click cancels the request.
  - The exact data sent is shown in developer mode; otherwise one sentence says what is sent, and games, saves, keys and screenshots never are; log lines only when you ask it to explain a problem.
- **The AI assistant measures its changes, explains problems and writes reports.** A change applied while a game runs is measured before and after the restart, with a verdict (better, worse or no clear change), *Keep these settings* or undo, and the result goes into the next request. *Help > Explain the last problem* reads Kaichii's log for the usual causes (keys, firmware, a damaged game file, the graphics driver, video memory, untranslated shaders, emulator crashes) without AI or sending anything, can ask the AI to explain the rest from the lines it shows you first, and drafts a bug report when it is an emulator bug. In a compatibility report, *Write it with AI…* turns your own words, in any language, into the report's fields in English (#89).

Checked on Linux: the dialogs on screen, a key saved, read and removed in GNOME Keyring, and real requests answered for the settings assistant and for its three new uses (measurement, log explainer and report writer). Windows and macOS binaries built in a non-publishing workflow; the Windows and macOS credential stores were not run.
