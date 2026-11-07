# Past investigations

Read these before investigating a symptom. They record what was proved, what was only suspected, and
which fixes were tried. Most state plainly what they did not establish, such as "No game was run
during this work"; carry that distinction forward instead of reading a note as proof that a game works.
Some are superseded, as their own headers say. They describe the tree at the time, so check the code.

## By game

| Game | Program id | Notes |
|---|---|---|
| Donkey Kong Bananza | 040074A01BF12000 | [Diagnostics](../bananza-diagnostics.md), [compute](../bananza-compute-followup.md), [atomics fix](../bananza-atomic-fix.md), [global atomics](../bananza-global-atomics.md), [live trace](../bananza-live-trace-followup.md), [pacing](../bananza-pacing-followup.md), [profile fixes](../bananza-profile-fixes.md), [performance](../bananza-performance-candidates.md), [query trace](../bananza-query-trace.md), [timing trace](../bananza-timing-trace.md), [upstream comparison](../bananza-upstream-comparison.md) |
| Kirby Air Riders | 040077201BE50000 | [White world](../kirby-white-world.md) |
| Star Fox | 0400E88025B1A000 | [Crashes](../STARFOX_COMMUNITY_CRASHES.md), [Aftermath diagnostic](../STARFOX_AFTERMATH_DIAGNOSTIC.md), [upscale stand-in](../STARFOX_UPSCALE_STANDIN.md) |
| Fire Emblem: Fortune's Weave | 04007EF01F8AE000 | [Upscale stand-in](../FE_UPSCALE_STANDIN.md), [DLSS replacement](../native-dlss-replacement.md) |
| The Duskbloods network test | 0400D1D02399A000 | [Bindless surfaces](../duskbloods-bindless-surfaces.md), [uniform shift](../duskbloods-uniform-immediate-shift.md) |
| Mario Kart World | 0400C3F00006E000 | [Integration test](../performance/MKW-INTEGRATION.md), `docs/nx2/notes.md`, `docs/nx2/gpu-thread-split.md` |
| The Legend of Zelda: Ocarina of Time | 0400486024DEC000 | [Dynamic FPS add-on](../nx2/ocarina-dynamic-fps.md), [network bypass](../nx2/oot-network-bypass.md) |

## By subject

| Subject | Notes |
|---|---|
| Boot blockers, first Switch 2 bring-up | `docs/nx2/notes.md` |
| GPU thread split | `docs/nx2/gpu-thread-split.md` |
| Asynchronous native pipelines | [Worker emission](../nx2/async-native-pipelines.md) |
| Offline shader sweeps | [Report](../shader-sweep-20261104.md), [before](../shader-sweep-before-20261104.md), [after](../shader-sweep-after-20261104.md), [library report](../shader-sweep-all-games-20261104.md), [library before](../shader-sweep-all-games-before-20261104.md), [library after](../shader-sweep-all-games-after-20261104.md) |
| Video playback | [Debugging](../VideoPlaybackDebug.md), [upstream audit](../VideoPlaybackUpstreamAudit.md), [upstream inventory](../VideoPlaybackUpstreamInventory.md) |
| Debug server and MCP adapter | [Design](../nx2/debug-mcp.md) |
| Host driver bugs | [Driver bugs](../DriverBugs.md) |
| Maxwell instruction set | [NVIDIA GPU](../NvidiaGpu.md) |
| Compatibility reports | [Reporting](../compatibility-report.md) |

## Upstream changes left out

Eglantine changes reviewed and not taken. Check these before porting them.

| Change | Why it is not here |
|---|---|
| `8820bb79aa`, storage buffers sized from the mapping only in DLSS passes | Tested in Ocarina of Time on top of Zortos PR 415, on an RTX 3050 Laptop: the Deku Tree intro rendered the same, and Link's house ran at 13.8 fps against 14.3 without it, inside the 1 fps the runs vary by. Its reason, every native dispatch marking an 8 MiB binding GPU-written, is what PR 415 fixes at the root: the binding keeps its size and only the range the shader's stores reach is marked. It also takes the word after a pointer as a size, which a native program does not promise. |
| `6dd4781249`, `0ca2032bff`, data directory and translations renamed to Eglantine | Would strand the saves and settings in the `eden` directory. |
| `587633debe`, Star Fox support | Reverted upstream by `e87ad31068`. |

Zortos pull requests from the 2026-11-07 round that were not taken. Those taken are cited in their
commits.

| Pull requests | Why they are not here |
|---|---|
| 353 | Changes pipeline keys and has no tests. |
| 216 | Patches the same instruction as the built-in Dynamic FPS add-on, which 299 now keeps at real speed down to 10 fps. |
| 313 | The same eviction problem as 408, whose budget was taken. |
| 182 | Replaced by 445, frame times in `status`. |
| 206 | Already in the tree as `68caa6c75e`. |
| 437, 448, 300, 378, 354, 329, 223 | Fixes for AMD or Intel GPUs, which nothing here can run. |
| 456, 387, 253 | Three overlapping DLSS integrations for Ocarina: features, not fixes. |
| 418, 278, 235, 231, 243, 270 | Experiments behind switches, which do not merge to `nx2`. |
| 365, 377, 451, 431, 199, 186, 195 | Windows or MinGW only features and tweaks, not fixes. |
| 345 | Ported and measured on top of the presets branch, Ocarina on an RTX 3050 Laptop: the split front's waits for its back fell from 450 to 166 ms a second, but the game ran at 31.2 against 30.7 fps at 1x and 38.1 against 40.4 at 0.5x. Those waits overlapped other work, so about 1,000 lines of split synchronization bought nothing measurable here. |
| 475 | HMMA.16816.F16 on cooperative matrices: its author measured no change in fps once 259's preflush removed the read stalls (the preflush is the default now, with reads guarded), and its output is not bit-identical to the emulation. |
| 296, 204, 189, 192, 221, 246, 320, 330, 335, 467, 213, 255, 267, 214, 219, 228, 411 | Not reviewed in depth yet. |
