# Recipes

Short paths for the common changes. Each ends at a test, because a change with no test is the one
that breaks again. `AGENTS.md` still applies: for a game bug, check upstream before writing a fix.

## Sync the fork

The working clone uses three remotes. `AGENTS.md` assumes `upstream`, and its first step is safe to
skip here because the remote already exists.

| Remote | Repository | Use |
|---|---|---|
| `origin` | `javocsoft/Lichii` | Your fork. Push here |
| `lichii` | `zortos293/Lichii` | The main project. Read-only: pushing is disabled |
| `upstream` | `Djihads80/Eglantine` | The project Lichii forks. Check it before writing a game fix |

```sh
git fetch lichii
git switch -c sync/zortos nx2
git merge lichii/nx2
```

`nx2` is protected, so push the branch and open a PR. `gh pr create` in a fork targets the parent
repository by default, so pass `-R javocsoft/Lichii`. The merge can bring switches or typos the gate
rejects: fix those on the same branch.

## Lower a new Ampere instruction

When a game shows a rejected program, the log and `EDEN_NX2_SHADER_DUMP` give you its bytes.

1. Read an existing lowering of a similar instruction in
   `src/shader_recompiler/frontend/ampere/lower_to_maxwell.h`. Encodings come from Mesa's NAK and from
   `nvdisasm`, as the header says. Never guess a field; keep unknown bits rejected.
2. Lower only what the Maxwell frontend can represent exactly. A close approximation that passes is
   worse than a rejection, because it turns a logged failure into a silent wrong image.
3. Add a case to the matching `src/tests/video_core/ampere_*.cpp`, using the captured instruction words.
   Follow the pattern there: check the exact Maxwell output for several guards, then flip each reserved
   bit and `CHECK_FALSE` that the lowering refuses it.
4. Replay offline with `tools/nx2/shader_sweep.py` if you have captures, as in `tools/nx2/shader-sweep.md`.
   It compiles every program to SPIR-V and runs `spirv-val` without a game.

## Handle a missing service command

1. Find what the game called with `EDEN_NX2_IPC_TRACE=1` or `EDEN_NX2_IPC_SUMMARY=1`.
2. Add the command to the `functions` table in the service's constructor, next to its neighbours.
3. Prefer real behaviour from [SwitchBrew](https://switchbrew.org/wiki/Main_Page) knowledge or the
   caller's disassembly (`tools/nx2/nso_dis.py`, `tools/nx2/nso_syms.py`). A stub must log with
   `LOG_WARNING` and say `(STUBBED)`.
4. `EDEN_NX2_AUTO_STUB=1` shows how far a game gets. It proves nothing about what the command must do.
5. Move decision logic into a small header with no `Core::System` dependency, as
   `src/core/hle/service/nx2_present_pacer.h` does, so a test can reach it. Then test the header.

## Add a switch

1. Decide its kind. A diagnostic stays, off by default. An experiment switch never merges: a proven fix
   ships unconditionally and a disproven one is dropped.
2. Read it once into a `static const`, as the code around it does.
3. Add a row to `docs/ai/env-flags.md`. The gate fails without it.
4. If it must work from the GUI and is not `EDEN_NX2_*`, add it to the list in
   `src/yuzu/nx2_switches.h`.

## Add a setting

Follow [Settings](../Settings.md) and the wording rules in [Development](../Development.md).

## Add a C++ test

See [Testing](testing.md#adding-a-c-test).

## Add a dependency

Add it through `cpmfile.json`; see [CPMUtil](../CPMUtil.md) and [Dependencies](../Deps.md). [Coding
guidelines](../Coding.md) ask you to avoid new dependencies, so justify it in the PR.
