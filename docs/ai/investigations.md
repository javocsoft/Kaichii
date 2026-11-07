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
| `8820bb79aa`, storage buffers sized from the mapping only in DLSS passes | Reverted and restored upstream, so its effect is contested. It changes the binding size of every Switch 2 compute shader, and nothing here can validate that without running games. |
| `6dd4781249`, `0ca2032bff`, data directory and translations renamed to Eglantine | Would strand the saves and settings in the `eden` directory. |
| `587633debe`, Star Fox support | Reverted upstream by `e87ad31068`. |
