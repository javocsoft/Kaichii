# AGENTS

Kaichii is an experimental Nintendo Switch and Switch 2 emulator in C++20: JavocSoft's fork of Lichii (`zortos293/Lichii`), itself a fork of Eglantine, Eden and yuzu. Current work is almost entirely the Switch 2 path, from the native shader lowering in `src/shader_recompiler/frontend/ampere` through the GPU in `src/video_core` to game-specific fixes.

See `docs/Coding.md` for technical coding guidance.

Keep comments to a reasonable level. Do not overexplain or add/remove comments from unrelated parts of the codebase.

## Start here

`docs/ai/README.md` indexes the pages written for agents. Read the one for your task:

| Task | Page |
|---|---|
| Where something lives, and how a game reaches the GPU | `docs/ai/architecture.md` |
| Compiling on Linux | `docs/ai/building.md` |
| What to test and how | `docs/ai/testing.md` |
| Common changes, and syncing the fork | `docs/ai/recipes.md` |
| Whether a symptom was already investigated | `docs/ai/investigations.md` |
| Any `EDEN_*` switch | `docs/ai/env-flags.md` |
| CI runs and releases | `docs/CI.md` |

## Commands

```sh
tools/dev/in-docker.sh tools/ci/quick-checks.sh       # The fast CI gate: minutes, no compile
tools/dev/in-docker.sh tools/dev/build-tests.sh       # Build and run every C++ test
python3 tools/ci/check_repo.py                        # Docs, switch catalog and test registration
```

Everything builds in the container from `tools/dev/Dockerfile`, so these work on a Linux machine with only Docker. Run the gate before every commit. Run the C++ tests when you touch anything under `src/`.

## Rules that keep the project honest

- Say what you verified. A passing build, passing tests and a game running correctly are three different claims. Unit tests and CI never run a game, so never report a game as fixed unless someone ran it.
- Keep a failing check failing until you understand it. Do not weaken a test, a checker or a linter to get to green, and do not skip a check silently.
- A rejected shader, a stub and a workaround are not fixes. Keep unknown instruction bits rejected rather than guessing: a logged failure is better than a silent wrong image.
- Never commit game files, keys, firmware, save data, `.rdc` captures, or shader and texture dumps from retail games. Tests use synthetic data, or the short captured instruction words the tests already use.
- Add a test with every behaviour change, in the same commit. See `docs/ai/testing.md` for what to do when the code cannot reach a test yet.
- Keep the old names (`yuzu`, `eden`, `EDEN_*`, the `eden` data directory). They keep users' saves and settings working.
- Add a row to `docs/ai/env-flags.md` with every new switch, and update `docs/CI.md` with every workflow change. The gate enforces both.

## Game bug reports

Users report game bugs with Kaichii logs (`eden_log.txt`, `eden_log.txt.old`). Each `Booting game: <program id>` line starts a session; the build is on the first line (`Kaichii Version: ... | nx2-<sha>`; builds from before the rename write `Lichii Version`). Known program ids:

| Program id | Game |
|---|---|
| 040074A01BF12000 | Donkey Kong Bananza |
| 040077201BE50000 | Kirby Air Riders |
| 04007C6028AA8000 | Yoshi and the Mysterious Book (demo) |
| 0400D4201C6EA000 | Mario Tennis Fever |
| 0400E88025B1A000 | Star Fox |
| 04007EF01F8AE000 | Fire Emblem: Fortune's Weave |
| 0400C3F00006E000 | Mario Kart World |
| 04008E001F098000 | Nintendo Switch Sports Resort |
| 0400A3A01EE4C000 | Pokémon Pokopia |
| 0400A4802437E000 | Drag x Drive Global Jam |
| 040041E0212AE000 | Fast Fusion |
| 04005B902517C000 | Kirby Air Riders Global Test Ride |
| 040057C02159A000 | Nintendo Switch 2 Welcome Tour |
| 040004F027560000 | Orbitals Global Friends Pass |
| 0400380020056000 | Raiders |
| 0400D1D02399A000 | The Duskbloods network test |
| 0400486024DEC000 | The Legend of Zelda: Ocarina of Time |
| 0400245021D2C000 | Yoshi and the Mysterious Book |

Add ids here when a report identifies a new game.

## Before writing a fix

This repository diverges on purpose: nothing here has to stay mergeable with `zortos293/Lichii` or [Eglantine](https://github.com/Djihads80/Eglantine). They are still where most fixes come from, so do not guess, and do not rewrite a fix one of them already has.

1. `git fetch lichii && git fetch upstream`. `docs/ai/recipes.md` lists the remotes of this clone.
2. Check `lichii/nx2` and `upstream/nx2`, their open and closed PRs, unmerged branches, and issues for the game and symptom (`gh pr list -R zortos293/Lichii --state all`, `gh pr list -R Djihads80/Eglantine --state all`, `gh pr diff <n>`). Read the diffs, not only the titles.
3. Port an existing fix before writing a new one, and cite its PR or commit. `docs/ai/investigations.md` lists changes that were reviewed and left out, and why.
4. Write a new fix only when nothing there addresses the root cause the log shows.

## Live debug server

Launch a debug build with `EDEN_NX2_DEBUG_SERVER=127.0.0.1:6545` (`EDEN_NX2_DEBUG_DIR=<dir>` moves its dumps) and ask the running game from another shell:

```sh
python3 tools/nx2/debug_mcp.py --server 127.0.0.1:6545 --call '{"method":"status"}'
python3 tools/nx2/debug_mcp.py --server 127.0.0.1:6545 --call '{"method":"screenshot"}' --out shot.png
```

Methods: `status`, `frame_times` (`seconds`, `refresh_hz`), `screenshot`, `log_tail` (`after`, `contains`), `log_filter`, `capture_frame` (the frame dump below, taken now; `sampled`, `index`), `capture_image`, `reject_shaders` (`EDEN_NX2_REJECT_HASH` at runtime; `"reject": false` restores). `setting` (`key`, `value`) reads or changes a setting while the game runs. Without `--call` the script is a stdio MCP server. See `docs/nx2/debug-mcp.md`.

- Try a live experiment (screenshot, `reject_shaders` A/B, `capture_frame`, `log_tail`) before asking for a new build.
- Use full 16-hex-digit hashes; shorter ones are refused.
- Restore every hash you reject before you finish.
- Never start or stop an emulator someone else is running. If nothing listens, ask them to launch the game.
- Keep sessions light while someone plays: one request at a time, no capture loops, `sampled` only when you need pass inputs.

## RenderDoc and the live server

Use both for graphics investigations. The live server provides emulator state, native shader hashes, logs and reversible shader A/B tests. [RenderDoc](https://github.com/baldurk/renderdoc) inspects the translated Vulkan frame: draw and dispatch state, resource contents, pixel history and shader debugging where supported. Prefer it over extending our dumps when it already exposes the missing evidence.

- Use the live server to identify the failing scene and programs, then capture one affected frame with RenderDoc. Find the first incorrect output and inspect its inputs, bindings, blend/depth state and resource dependencies. Record the build, settings, event IDs and shader hashes so the two views can be correlated.
- The existing integration is `src/core/tools/renderdoc.cpp`, gated by `enable_renderdoc_hotkey`. It requires RenderDoc to be loaded into the process; enabling the hotkey alone does not load it. Never start, stop or inject into an emulator someone else is running without their approval.
- [renderdoc-mcp](https://github.com/Linkingooo/renderdoc-mcp) is a third-party replay adapter, not part of RenderDoc. Pin and inspect its version, use a compatible RenderDoc Python module, and verify a real capture opens before relying on it. If an extension, pixel-history query or shader debug operation is unsupported, report that limit rather than treating missing data as a clean result.
- Keep `.rdc` captures on the machine that took them: they can contain game shaders, textures and buffers. Do not upload them to public issues or services. Share only the specific permitted evidence needed for the investigation.
- Capture and replay can alter timing or hide races. Compare with an uncaptured live run before ruling out a synchronization bug. Distinguish measured GPU counters from adapter estimates; use CPU/thread and frame traces for emulator performance.
- A shader replacement that reduces fog or exposure can be a useful visual comparison, but is not proof of an emulator fix. Trace the original pipeline's first incorrect result before shipping a change. Matching shader arithmetic alone does not validate bindings, synchronization or input data.

## Debugging tools

Use these before guessing. All are off unless the variable is set. Read them from the log at `*:Info` unless noted.

| Symptom | Tool | Output |
|---|---|---|
| Wrong image (white, black, missing objects, smearing) | `EDEN_NX2_FRAME_DUMP=<dir>` with `EDEN_NX2_FRAME_DUMP_AT=<seconds>`, `<frame>f` or `file` (captures once `<dir>/capture.now` exists; default 60); add `EDEN_NX2_FRAME_DUMP_SAMPLED=1` for pass inputs | One frame's pass colour targets, dispatch storage images (3D included) and present source; with `_SAMPLED`, every texture each pass samples and each compute shader samples before its first dispatch (`kind: sampled`, with stage (5 = compute), TIC index, 3D and depth included) as `.png` and `.raw.zst`, plus `manifest.jsonl` with shader hashes, formats, addresses and per-channel stats. `python3 -I tools/nx2/frame_dump_index.py <dir>` builds `index.html`. Find the first pass whose output is wrong, then inspect its shaders. |
| Rejected or miscompiled shaders | `EDEN_NX2_SHADER_DUMP=<dir>` | Native programs with `.header.bin` sidecars and `failed-compute-*.env` for failed compute pipelines. Replay offline with `tools/nx2/shader_sweep.py` (see `tools/nx2/shader-sweep.md`). |
| A newly lowered program changes the image | `EDEN_NX2_REJECT_HASH=<hash>,...` (native program keys from shader dump file names, or shader hashes from the frame dump) | Those programs are rejected as if unsupported; the log shows `rejected by EDEN_NX2_REJECT_HASH` per hit. Bisect by halving the list. The key covers the captured code, so recompute it when the capture rules change. |
| Draw and render-state questions | `EDEN_NX2_RENDER_TRACE=1`, filter `*:Info HW.GPU:Debug Render.Vulkan:Debug Shader:Debug` | Per-draw state, rejected stages and zero-format texture descriptors. |
| Video playback | `EDEN_NX2_VDEC_TRACE=1`, `EDEN_NX2_VDEC_DUMP=<dir>` | Decoder submit, decode and composite trace; decoded frames. |
| Performance and frame pacing | `EDEN_NX2_GPU_PROFILE=1`, `EDEN_DKB_FRAME_TRACE=1` | GPU thread time per second by zone; per-present interval, CPU and fence timing. |
| Command corruption or pop-in | `EDEN_NX2_PB_CHECK=1`; A/B with `EDEN_NX2_PB_SNAPSHOT=0` or `EDEN_NX2_FETCH_DRAIN=1` | How many pushbuffer entries change between submit and parse. |
| Split GPU thread races | `EDEN_NX2_SPLIT_CHECK=1` | Reports scheduler use from the wrong thread. |
| Upscaler and DLSS contracts | `EDEN_NX2_DLSS_CONTRACT_TRACE=1` | Inputs and outputs of replaced upscaler networks. |
| Service calls | `EDEN_NX2_IPC_TRACE=1` | IPC command trace. |
| Unmapped device reads (`Unmapped Device ReadBlock`) | `EDEN_NX2_READBLOCK_TRACE=1` | Every read of unmapped device memory, one line per read with its first unmapped address, unmapped byte count and caller (`file:line`). Without it only the first 16 are logged. |
| Device loss (`Device loss occurred!`) | Always on | The `Device fault` lines after it: what VK_EXT_device_fault reports (fault addresses and their type, vendor fault codes). `not supported by the driver` means the driver has no such data. |
| Driver crash, stall or slow pipeline builds | `EDEN_NX2_PIPELINE_BUILD_TRACE=<file>` | A flushed `BEGIN`/`END` line per driver pipeline build with the pipeline and stage hashes; `END` adds the wall time (`ms=`). The last `BEGIN` without an `END` names the shaders the driver crashed or is stuck on. Times include waiting on other builds, so compare runs from the same cleared driver cache rather than single builds. |
| Host crash (log just stops) | Windows minidump in `%LOCALAPPDATA%\CrashDumps\kaichii.exe.<pid>.dmp` (`lichii.exe` before the rename) | Read the exception and stack with the `minidump` Python package, then symbolize with `llvm-symbolizer --obj=kaichii.exe --inlines --relative-address <offset>` next to the build's `kaichii.pdb`. |

## Builds

- `tools/dev/in-docker.sh` builds on any Linux machine with Docker. See `docs/ai/building.md`.
- Clang must be 19 or newer: older ones reject `-mtls-dialect=gnu2`.
- Windows and macOS are built by `.github/workflows/release-binaries.yml` and by hand, as `docs/Build.md` describes. No PR compiles them.
- Bundle a round's candidate fixes into one build behind env switches, so a tester runs one package and A/Bs it live through the debug server instead of one build per idea.

## Toggles and merging

- Experiment switches (env flags that pick between candidate fixes) never merge to nx2. A proven fix ships as unconditional behaviour, and a disproven one is dropped.
- Diagnostic tools (frame dump, `EDEN_NX2_REJECT_HASH`, traces, the debug server) stay in nx2, off unless their variable is set.
