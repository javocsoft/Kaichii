# Compatibility reports

After stopping a Switch 2 game, right-click that game and choose **Submit compatibility report**.
The action is available only for the latest session in the current Kaichii process. Starting another
game clears that session, and closing Kaichii discards it. This feature does not store play history.

The dialog prepares a report and **Open on GitHub** opens a new issue with it in the repository the
build comes from (`javocsoft/Kaichii` for this fork's releases), on the
[compatibility report form](../.github/ISSUE_TEMPLATE/compatibility_report.yml). The player reviews
it there, ticks the form's two confirmations (a legally dumped game; no game files, keys, firmware,
saves or private details) and publishes it with their GitHub account. Kaichii itself sends nothing:
it opens a link in the browser, and nothing else touches the network. **Copy report** puts the same
text on the clipboard.

## What the report holds

The **Experience** tab is the player's: their assessment (Perfect, Playable, Runs, Loads or
Unplayable), a summary, how far they played, what worked or failed, and optional steps to reproduce.
The **System and settings** tab is prefilled and editable: the game and its version, the program ID,
the Kaichii build, CPU, GPU and driver, graphics API, operating system, RAM, launch settings, and the
updates and mods enabled at launch. Missing values read Unknown. Prefills are not performance
measurements, and settings changed during play must be corrected by the player. **Preview** shows
the Markdown exactly as it goes to GitHub.

The report is written in English whatever the interface language, so every report reads and can be
parsed alike: the assessment and summary first, the player's paragraphs, then a table of the system
and settings. Table values have their pipes escaped and their lines joined.

A link longer than about 7,000 characters fails on GitHub. A longer report leaves the link with a
line that asks for a paste, and goes to the clipboard instead. When the browser doesn't open, the
report also goes to the clipboard, and the dialog shows the page to open.

## Implementation boundary

`MainWindow::BootGame` captures one allowlisted value snapshot after successful core loading and
before starting the emulation thread. `OnEmulationStopped` enables the context action for that
snapshot's exact local program ID. Application changes invalidate it. The dialog receives copied
values, not a core object, filesystem object, settings serializer or log path.

Each graphics backend copies its device identity during construction while its selected
device/context is available. The session then copies those strings before teardown; it performs no
GL calls, adapter enumeration or log reading. The null backend leaves the identity empty.

Mods and updates use the add-on catalog's enabled entries at launch. Only their display names and
version/type labels enter the draft, never their locations or file contents. Disabled entries and
DLC are excluded. NUP packages select their embedded update internally, outside that catalog; their
effective game version comes from the loaded package's display-version metadata and is included as
**Loaded NUP package** in the updates field.

`NewIssueLink` builds the link: `https://github.com/<owner>/<name>/issues/new` with the form
(`template`), the title and the `report` field, each percent-encoded. A repository that isn't
`owner/name` gives no link. The form's field ID and title prefix are checked against the code by
`compatibility_report_tests`.

Upstream Lichii sends these reports to its own website. Kaichii doesn't: reports of a fork with its
own changes don't belong in Lichii's data, and the reports stay where this fork can read and moderate
them.

## Verification without a game

Build the standalone Qt test target with clang and Ninja:

```sh
cmake -S src/tests/qt_compatibility -B build/compatibility-report -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build/compatibility-report --parallel 4
ctest --test-dir build/compatibility-report --output-on-failure
```

The target also joins the normal Qt build when `YUZU_TESTS` is enabled. Tests replace the browser,
so they never open a page. They cover the link, the required fields, long reports, browser failure,
invalid repositories, table escaping, clearing, translation and the prefills.

For a synthetic dialog that opens nothing:

```sh
build/compatibility-report/compatibility_report_tests --preview
QT_QPA_PLATFORM=offscreen build/compatibility-report/compatibility_report_tests \
  --screenshot /tmp/compatibility-report.png
```
