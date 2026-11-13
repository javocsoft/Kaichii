# CI and releases

`tools/ci/check_repo.py` fails the fast gate when this page and `.github/workflows` disagree, so a
workflow added, renamed or removed must be updated here in the same commit.

## What runs when

| Workflow | Runs on | What it does | Blocks a merge? |
|---|---|---|---|
| `quick-checks.yml` | PRs and pushes to `nx2` | Everything in `tools/ci/quick-checks.sh`. Takes minutes and compiles no emulator code. | Yes |
| `build-check.yml` | PRs and pushes to `nx2` | Linux GCC 13 production build with `-Werror`, `ctest`, and the debug adapter, ThreadWorker and switch discovery suites. | Yes |
| `linux-extended.yml` | PRs and pushes to `nx2` | Linux Clang 20 build with `DYNARMIC_TESTS` and `ENABLE_DEBUG_TOOLS`, `ctest`, the shader tool suites, and production shaders on Mesa's software Vulkan driver. | Yes |
| `sanitizers.yml` | PRs and pushes to `nx2` | AddressSanitizer on `tests` and `dynarmic_tests`, with UBSan logging. Stops at the first memory error. | Yes |
| `release-binaries.yml` | Manual, or pushes to `nx2-experimental` that change build inputs | Builds every platform and publishes a `build-<run number>` GitHub release. | No |
| `label-issues.yml` | New issues | Labels issues from their form fields. | No |
| `pages.yml` | Pushes to `nx2` that change the guide, compatibility reports, daily, manual | Builds the player's guide (`docs/user` and its MkDocs configuration) with the compatibility list from the reports, and publishes it on GitHub Pages. | No |

Merge blocking is a repository setting. `nx2` in `javocsoft/Kaichii` requires all four checks on an
up-to-date branch, for admins too, and allows no force pushes. So every change reaches `nx2` through
a pull request, including a sync from another repository. `zortos293/Lichii` has no branch protection.

## Reproducing CI locally

Run the same commands in the same environment before pushing. See [building](ai/building.md) for
the container and [testing](ai/testing.md) for which layer a change needs.

```sh
tools/dev/in-docker.sh tools/ci/quick-checks.sh
```

## What CI does not catch

- **Windows, macOS and aarch64 only build when a release does.** No PR compiles them, so a break
  there can reach `nx2` unnoticed.
- **Nothing runs a game.** CI has no game files, keys, firmware or GPU, and it never will: none of
  them may be uploaded. A green run shows that the code builds and that its unit tests pass, not
  that any game renders correctly. Say which of the two you checked when you report a change.
- **Some linters are not tree-wide.** `quick-checks.yml` runs `shellcheck` over the whole tree, but
  whitespace, license header and `codespell` checks on changed files only, because the inherited
  tree fails them.
- **`clang-format` is not enforced.** `tools/clang-format.sh` only covers `src/qt_common` and
  `src/yuzu`.

## Running on a fork

Actions are off on a new fork. Enable them on its Actions tab, and enable branch protection on `nx2`
if you want the checks to block merges. `build-check.yml`, `linux-extended.yml` and `sanitizers.yml`
need no secrets.
Their first runs start with an empty cache: the build alone took 41 minutes with GCC and 35 with Clang
on a fork's first run.

Every workflow installs its Ubuntu packages through `tools/ci/apt-install.sh`, with apt timeouts,
three attempts, and a step timeout of its own. The build jobs keep the downloaded packages and apt's
lists in an actions/cache entry per job (`apt-<job>-...`, keyed on the workflow file and the script):
a warm cache installs without touching the network, and a cold one is saved right after the install,
before the build. On 2026-11-11 a runner's DNS stopped resolving the Ubuntu mirrors, and then the
mirrors served a job's ~234 MB at 350 kB/s: runs failed or timed out while downloading, and one PR
waited for hours.

In `zortos293/Lichii`, a workflow run from a first-time contributor's PR stays at `action_required`
until a maintainer approves it, so those PRs have had no CI yet.

## Releases

`release-binaries.yml` is the only release path. It runs on demand from the Actions tab, or on a push
to `nx2-experimental` that touches `src/` (other than `src/tests/`), `CMakeLists.txt`, `CMakeModules/`,
`externals/`, `dist/`, `tools/`, `.patch/`, `cpmfile.json` or the workflow itself.

Its builds check for updates in the releases of the repository that built them: CMake's
`LICHII_UPDATE_REPO` defaults to `GITHUB_REPOSITORY`, and to `javocsoft/Kaichii` outside GitHub
Actions. Another repository's builds carry their own build numbers, so offering them would replace
this fork's build with one that lacks its changes.

A release publishes the platforms that built. One whose build failed, or that GitHub couldn't run
(macOS runners are often short), is left out and named in the release notes, so it doesn't hold
back the others; the update checker on that platform skips a release without its file. A run
cancelled by hand publishes nothing, and the release job fails when no platform built.

Release assets are named `kaichii-<platform>[-pgo].<extension>`, and the update checker looks for
exactly those names. Builds from before the rename to Kaichii published `lichii-*` assets and look
for those, so they don't update to Kaichii by themselves; the release notes say so.

| Platform | Runner | Variants |
|---|---|---|
| Linux x86-64 | `ubuntu-24.04` | Standard and PGO |
| Linux aarch64 | `ubuntu-24.04-arm` | Standard and PGO |
| Windows x86-64 | `windows-2022` | Standard and PGO |
| macOS arm64 | `macos-15` | Standard |

Each publish creates the tag `build-<run number>`. Releases configure with `BUILD_TESTING=OFF`, so a
release proves nothing about the tests. Linux PGO needs a Clang at least as new as the one that wrote
its profile; `PGO_LLVM_VERSION` in the workflow records it. The x86-64 PGO build uses
`lichii-linux.profdata` and the aarch64 one `eglantine.profdata` (see "Linux PGO profile" below). Both Windows builds use clang-cl
from a pinned LLVM (`tools/windows/install-llvm.ps1`) with ThinLTO against the MSVC headers and SDK;
the PGO one adds `lichii-windows.profdata`, an MSVC-ABI profile trained on Ocarina of Time. Zortos's
Windows clang-cl job in `build-check.yml` is not taken here: Windows does not build on pull requests.

## Linux PGO profile

`lichii-linux.profdata` is `eglantine.profdata` (trained upstream on Mario Kart World) merged with
a run of Ocarina of Time on this tree. Measured on 2026-11-12 on an i7-12700H with an RTX 3050
Laptop, standing on Link's porch, four alternating rounds, all three compiled from the same source:
the GCC build 30.6 fps, the PGO build with `eglantine.profdata` 32.4, with the merged profile 33.1
(ahead in three rounds of four). Most of the gain is the PGO build itself; the retrained profile
adds about 2 %.

To train it again after large changes (functions changed since training lose their profile data):

1. Build `eden-cli` with clang and `-fprofile-generate=<dir>`, without Qt and tests. Leave out
   `-fprofile-update=atomic`: with it Ocarina of Time runs at about 5 fps.
2. Run a game with `LLVM_PROFILE_FILE=<dir>/run-%p.profraw`: boot, load a save, play a few
   minutes. `eden-cli` does not exit on SIGINT, and a killed process writes no profile, so ask it
   to write one first: `gdb -p <pid> -batch -ex 'call (int)__llvm_profile_write_file()'`.
3. Merge with an `llvm-profdata` at least as new as the one that wrote `eglantine.profdata` (23):
   `llvm-profdata merge -o run.profdata <dir>/*.profraw`, then
   `llvm-profdata merge -o lichii-linux.profdata -weighted-input=1,eglantine.profdata
   -weighted-input=<w>,run.profdata`. Pick `<w>` so both have about the same
   `Maximum function count` (`llvm-profdata show`); it was 6 for this profile.
4. Compare the PGO build with each profile, alternating runs, before replacing the file.

The profile holds counters of the emulator's own functions, no game data.

## The player's guide

`pages.yml` publishes `docs/user` at `https://javocsoft.github.io/Kaichii/`, English at the root and
Spanish under `es/` (`Page.md` and `Page.es.md`; a page without a Spanish version shows the English
one). It runs when the guide, its MkDocs configuration or `tools/site` change on `nx2`, when an issue labelled
`compatibility` is opened, edited, closed or relabelled, once a day, and on demand.

Before building, `tools/site/compatibility.py` replaces `Compatibility.md` and `Compatibility.es.md`
with the list of reports read from the issues. Report text comes from anyone who opens an issue: the
script escapes every value and writes every link itself, and `tools/site/test_compatibility.py`
checks that. Issues closed as not planned, or labelled `duplicate`, `invalid` or `spam`, are left
out. The repository's Pages source must be "GitHub Actions".

To build the guide locally:

```sh
python3 -m venv /tmp/site && /tmp/site/bin/pip install -r tools/site/requirements.txt
python3 -I tools/site/compatibility.py --repo javocsoft/Kaichii --out docs/user
/tmp/site/bin/mkdocs serve
```

Restore the placeholders afterwards with `git checkout docs/user/Compatibility*.md`.

