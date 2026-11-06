# CI and releases

`tools/ci/check_repo.py` fails the fast gate when this page and `.github/workflows` disagree, so a
workflow added, renamed or removed must be updated here in the same commit.

## What runs when

| Workflow | Runs on | What it does | Blocks a merge? |
|---|---|---|---|
| `quick-checks.yml` | PRs and pushes to `nx2` | Everything in `tools/ci/quick-checks.sh`. Takes minutes and compiles no emulator code. | Only with branch protection |
| `build-check.yml` | PRs and pushes to `nx2` | Linux GCC 13 production build with `-Werror`, `ctest`, and the debug adapter, ThreadWorker and switch discovery suites. | Only with branch protection |
| `linux-extended.yml` | PRs and pushes to `nx2` | Linux Clang 20 build with `DYNARMIC_TESTS` and `ENABLE_DEBUG_TOOLS`, `ctest`, and the shader tool suites. | Only with branch protection |
| `release-binaries.yml` | Manual, or pushes to `nx2-experimental` that change build inputs | Builds every platform and publishes a `build-<run number>` GitHub release. | No |
| `label-issues.yml` | New issues | Labels issues from their form fields. | No |

Merge blocking is a repository setting. `nx2` has no branch protection in `zortos293/Lichii`, and a
fork needs its own.

## Reproducing CI locally

Run the same commands in the same environment before pushing. See [building](ai/building.md) for
the container and [testing](ai/testing.md) for which layer a change needs.

```sh
tools/dev/in-docker.sh tools/ci/quick-checks.sh
```

## What CI does not catch

- **Windows, macOS and aarch64 only build when a release does.** Windows is where the owner tests
  and no PR compiles it, so a break there can reach `nx2` unnoticed.
- **Nothing runs a game.** CI has no game files, keys, firmware or GPU, and it never will: none of
  them may be uploaded. A green run shows that the code builds and that its unit tests pass, not
  that any game renders correctly. Say which of the two you checked when you report a change.
- **Linters are not tree-wide.** `quick-checks.yml` runs whitespace, license header, `shellcheck`
  and `codespell` checks on changed files only, because the inherited tree fails them.
- **`clang-format` is not enforced.** `tools/clang-format.sh` only covers `src/qt_common` and
  `src/yuzu`.

## Running on a fork

Actions are off on a new fork. Enable them on its Actions tab, and enable branch protection on `nx2`
if you want the checks to block merges. `build-check.yml` and `linux-extended.yml` need no secrets.
Their first runs start with an empty cache and take far longer than later ones: see the
`timeout-minutes` comment in `linux-extended.yml`.

In `zortos293/Lichii`, a workflow run from a first-time contributor's PR stays at `action_required`
until a maintainer approves it, so those PRs have had no CI yet.

## Releases

`release-binaries.yml` is the only release path. It runs on demand from the Actions tab, or on a push
to `nx2-experimental` that touches `src/` (other than `src/tests/`), `CMakeLists.txt`, `CMakeModules/`,
`externals/`, `dist/`, `tools/`, `.patch/`, `cpmfile.json` or the workflow itself.

| Platform | Runner | Variants |
|---|---|---|
| Linux x86-64 | `ubuntu-24.04` | Standard and PGO |
| Linux aarch64 | `ubuntu-24.04-arm` | Standard and PGO |
| Windows x86-64 | `windows-2022` | Standard and PGO |
| macOS arm64 | `macos-15` | Standard |

Each publish creates the tag `build-<run number>`. Releases configure with `BUILD_TESTING=OFF`, so a
release proves nothing about the tests. Linux PGO needs a Clang at least as new as the one that wrote
`eglantine.profdata`; `PGO_LLVM_VERSION` in the workflow records it.

## Leftovers that do nothing

These are inherited and not wired to anything. Do not read them as describing how this repository
works, and do not spend time fixing them.

- `.forgejo/workflows/` belongs to Eden's Forgejo. It targets `master` and a `source` runner that do
  not exist here, so the license, translation, string and dependency jobs do not run.
- `tools/ci/bump-version.sh`, `changelog-section.sh`, `prepare.sh`, `release-notes.sh` and
  `slugify.sh` served a `ci` workflow removed in commit `f176172f45`. `VERSION` and `CHANGELOG.md` came
  with it: releases use `build-<run number>`, and neither file changes them.
- `.ci/license-header.sh` compares against a `master` branch this repository does not have. Use
  `tools/ci/quick-checks.sh`, which applies a looser check to changed files.
- `docs/CODEOWNERS` still names Eden's maintainers.
