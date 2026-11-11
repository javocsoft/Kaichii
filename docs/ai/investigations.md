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
| The Legend of Zelda: Ocarina of Time | 0400486024DEC000 | [Dynamic FPS add-on](../nx2/ocarina-dynamic-fps.md), [network bypass](../nx2/oot-network-bypass.md). Zortos issue 653 (abort on an unimplemented `nx2:vi2` 201 at boot): never called on this tree's test machine, so no stub is guessed for it. Zortos issue 619 (GPU hang, Xid 109, after about 5 minutes on Linux with NVIDIA driver 580): 13 minutes here on driver 595 without a hang. A different hang, at 0 fps after a few minutes in Kokiri Forest with the GPU threads pinned, was the texture collector mapping memory for a GPU-only region under the cache locks (see `Nx2SparseWriteReady`). A disk cache of the CPU-transcoded textures was tried and left out (2026-11-09, RTX 3050 Laptop, 20 threads): a second session read all 744 textures from disk instead of decoding them, but Kokiri Forest ran at 35 fps in both sessions, loading took as long, and the cache held 942 MB. The asynchronous decode already happens during loading. Reading the counters of its 0xe2c occlusion macro without the split front's drain (a local switch, 800 drains a second) gave no gain over three alternating rounds (Kokiri porch 33.1 against 32.2 fps, walking 41 against 38), and `fps_limit=30` makes walking slower (26-28 fps against 38-41: a frame that misses its 33 ms slot waits for the next). `EDEN_NX2_GUEST_PROFILE` on the porch (2026-11-11, about 28 fps): a priority 55 thread runs all the time with almost no SVCs, the main thread (p47) runs 715 ms a second and waits 285 ms on one address, and the render thread (p44) waits 321 ms a second on the `nx2:syncpt:Slot0` syncpoint. At low GPU accuracy that syncpoint is raised when the GPU back thread processes it, not when the host GPU finishes, so a frame chains the main thread, the render thread and both GPU threads. The worst stall while walking (564 ms) was the main thread running 368 ms and waiting 194 ms on one address, with the render thread idle. perf of both GPU threads there: the front spends about 60% of its samples spinning in `GpuSplit::Drain` and `WaitFor`, and the back's profile is flat (no function over 6%). A local switch that skipped the front's waits for queued list writers on pages never GPU-written took those waits from 260 to 0.6 ms a second and the porch from 29.4 to 30.0 fps over three rounds, within the noise: with the 0xe2c test and Zortos PR 345, the split's syncs are not what sets the pace. First-time shaders, from the user's pack alone (`vulkan.fork-v53.bin`): a 40 s walk in Kokiri Forest builds 112-151 pipelines the pack lacks. Each costs about 11 ms on the GPU thread (median 10.4, p90 26, max 44 ms), about 1.5 s per walk; that is `TranslateProgram`, since the SPIR-V is emitted on workers (0.35 s per walk). Draws held for building pipelines add another 0.3-0.55 s. `native_async_draws=Bounded` removes the held draws but not the stalls: 65-69 frames over 60 ms, against 49-71 with Wait. Built pipelines are cached, so a second walk builds 0-2. Translating new native pipelines on workers while their draws are skipped (`native_async_draws` Bounded or Skip) took the GPU thread's builds to zero. Over three alternating rounds from the pack alone, with the Dynamic FPS add-on off so the walks follow nearly the same route, it lost 1.9, 1.8 and 2.8 s to slow frames, against 2.4, 3.3 and 4.0 s with Wait and 2.5, 3.3 and 3.3 s with Bounded alone. Frames over 200 ms went from 4-5 to 2-3, fps stayed within the noise, and the screenshots showed nothing missing. With Dynamic FPS on, the walks end in different places: game speed follows frame time. |

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
| `292a68df46`, a 4096-entry `MaxContinuousRange` cache for Ocarina | This tree answers `MaxContinuousRange` from the mapping runs it keeps at map time, so there is no cache to miss. |
| `de4ed9d9d7`, 64 storage buffers for compute | This tree already allows 64 in every stage, up to the device's limit. |
| `900ab5a825`, lowering operand lists for SHF, IADD3.UR, FSETP and I2F | Written for Eglantine's lowering, which reserves its top registers through `PrepareRegisterPressure`; this tree's lowering has no such list. Whether Ocarina's three Hyrule Field tessellation programs lower here is not checked: the save here doesn't reach the field. |
| `f7a9027ff1`, Ocarina video, NUP mods and batching | Its parts: LayeredFS mods for .nup files are already here. The swap interval exemption while a video plays only changes the `EDEN_NX2_SWAP_INTERVAL` override. The texture cache override cap is Zortos PR 692. `EDEN_NX2_FRONT_BATCH` is an experiment. Fence operation vectors (`d52e5f1036`): allocation took under 0.2% of the 2026-11-08 Ocarina profile. |
| PRs 716, 721 and 723 | Already here: the compare-exchange retry (Zortos 206), float depth-compare descriptors, and 723's shader, barrier and AVX-512 CLZ fixes. |
| PR 717, lock-free exclusive monitor | Its speed comes from not taking the global monitor's lock, and this tree's default CPU accuracy (Auto) already ignores the global monitor. Its correctness claim, closing the races that ignoring it opens, is not checked here. |
| PR 754, Intel parts | Workarounds for Intel's Windows driver, and a Mario Kart World task payload fix written for Eglantine's mesh prologue. Its all-vendor shutdown fix was taken. |
| PRs 739, 741 | arm64 hosts: a host memory reservation that only unit tests on arm64 Linux reach, and NCE SIGILL handling. Nothing here builds or runs arm64; 740's NCE fix was taken. |
| PR 758 | Adreno (Turnip) experiments for Ocarina behind switches, with an opt-in bilinear stand-in for its upscaler. |
| PR 720 | Seven fixes in 17,900 lines from another fork. Its tessellation pipeline check is already here. |
| Zortos 1210, texture collector thresholds within the host's commit | Windows only: Windows charges device memory to the commit limit (RAM and page files). Linux has no such limit for it. |
| Zortos 910, no null image prepared on a texture memo hit | Fixes `ResolveMemoizedTextures` (Zortos 858), which this tree does not have. |
| Zortos 957, the page table's last first-level index | Already fixed here (`ReserveRange` uses `start + size - 1`, with tests for a range ending at the top of the address space). |
| Zortos 1334 and 1337, graphics pipelines kept up to a quarter of the RAM and rebuilt from kept SPIR-V | This tree already destroys unused pipelines only past a budget (`pipeline_cache_policy.h`: 1/16 of the RAM at ~0.45 MB each, about 4,660 on 32 GB), the root cause 1334 fixes upstream. No session here has logged an eviction (Kokiri Forest stays under it), so the larger budget is not taken until one does. Building the user's whole pack at boot took about 0.9 MB of resident memory per pipeline. |
| Zortos 1327 and 1331, dynarmic ordered loads as plain moves and one cache line per exclusive reservation | Measured in Ocarina of Time on an RTX 3050 Laptop over three alternating rounds (2026-11-11): Kokiri porch 30.4 fps without against 29.3 with them, walking 33.7 against 32.0, lower in all six pairs. Neither fixes a bug here. |
| Zortos 1333, read-only native compute bindings sized from their size word | Undoes Zortos 1231 (bindings to the end of the mapping, a 1 GiB buffer for Ocarina of Time's heap), which this tree does not have: native compute bindings here keep the 8 MiB bound. Its other part, growing past 8 MiB through the size word for Mario Kart World's lighting, is untested here. |
| Zortos 1353, `ZeroTrim` allocation failures | This tree has no `TrimZeroPages` worker. |
| Zortos 1232, nxformat linked statically | Fixes a crash when `nxformat.dll` is missing; this tree doesn't use nxformat. The rest of the PR is NX archive features (XCI conversion, icon editing). |
| Zortos 1336, the front reads the list words the host GPU wrote | Like 345, measured here: the front's waits fell from 450 to 166 ms a second and the frame rate didn't change. |

Zortos pull requests from the 2026-11-07 round that were not taken. Those taken are cited in their
commits. The evening sync merged `zortos293/Lichii` `nx2`, which brought in 182, 186, 189, 192, 199,
214, 221, 228, 246, 255, 270, 320, 329, 330, 353, 365, 377, 418, 456 and 475 as Zortos merged them:
353 joined this tree's fragment color mask, and 182's `frame_times` reads the present history of 445.

| Pull requests | Why they are not here |
|---|---|
| 216 | Patches the same instruction as the built-in Dynamic FPS add-on, which 299 now keeps at real speed down to 10 fps. |
| 313 | The same eviction problem as 408, whose budget was taken. |
| 206 | Already in the tree as `68caa6c75e`. |
| 437, 448, 300, 378, 354, 223 | Fixes for AMD or Intel GPUs, which nothing here can run. |
| 387, 253 | Two more DLSS integrations for Ocarina, overlapping 456. |
| 278, 235, 231, 243 | Experiments behind switches, which do not merge to `nx2`. |
| 451, 431, 195 | Windows or MinGW only features and tweaks, not fixes. |
| 345 | Ported and measured on top of the presets branch, Ocarina on an RTX 3050 Laptop: the split front's waits for its back fell from 450 to 166 ms a second, but the game ran at 31.2 against 30.7 fps at 1x and 38.1 against 40.4 at 0.5x. Those waits overlapped other work, so about 1,000 lines of split synchronization bought nothing measurable here. Outside in Kokiri Forest at half resolution, where the emulated CPU sets the pace, the macro DMA read waits fell from 274 to 61 ms a second but list writer waits rose from 128 to 296, and the game ran at 23.1 against 22.6 fps. |
| 589 | Replays a macro the split front's compiled program rejects on the interpreter. The rejections it hides ("instruction outside uploaded RAM" on Linux AppImages) were GCC 13 dropping macro uploads, fixed at the root by 524. |
| 559 | Waits only for a macro DMA read's own query reports. Its author's profile shows less waiting, but its own frame rates went down (walking 36.1 to 31.9 fps). |
| 591, 640 | Pipeline cache migration and pruning of damaged, stale and duplicate records (+2,300 lines). 591 overlaps this tree's own migration from 52-54 to 55, and on Ocarina here only three compute records are stale (the "Uncached storage pointer classification" lines at boot), none graphics. Left until Zortos merges them. |
| 615 | Channel-aware state tracking from Eglantine #754: its author neither built nor ran it. |
| 690 | Mario Kart World water: the collision write-back is a fix, but it comes with a wave-shader workaround keyed to one program, and nothing here runs the game. |
| 692 | Caps the `EDEN_NX2_TC_*_MIB` tuning overrides at the device budget: only matters to someone setting them past their card. |
| 652 | Keeps stale Switch 2 graphics pipelines within a RAM budget, a different eviction policy from this tree's (only the excess over the budget, least recently used first, prebuilt unused ones kept). |
| 655, 678, 695 | Ocarina features: FPS unlock and frame limit slider (this tree has Dynamic FPS and 270's limit), DLSS/DLAA and FSR 3.1 in place of the game's network. Features, not fixes. |
| 645, 656 | Windows fastmem retry and macOS/MoltenVK work: nothing here runs either. |
| 715 | Copies Ocarina's UI storage at each present while it runs a frame ahead (issue 286, reported on AMD: the dialogue panel and HUD missing in half the frames). Not seen here: on the RTX 3050 the hearts, minimap, button prompts and Saria's speech bubble were in all eight screenshots taken in a row in Kokiri Forest. The dialogue panel itself was not checked. |
| 712, first half | Compares motion with the first value since mapping began. While mapping, `InputEngine::SetMotion` already keeps the value from before mapping began, so the comparison already has a fixed baseline. Its sensor restart was taken. |
| 635 | DLSS fixes for Ocarina: DLSS builds only with MSVC here, and nothing here runs Windows. |
| 638 | Clears a whole 3D target on its first slice's zero clear for The Duskbloods: a guess about the other slices, and nothing here runs the game. |
| 621 | Already in the tree. |
| 718 | Keeps a samples query running up to 4 banks without a reset or report. This tree drops one only past 8,192 slots (`samples_query_policy.h`), and Ocarina here drops two a boot, both at the title. |
| 721 | Replaces Ocarina's movie network (intro, title menu, cutscenes) with a bilinear copy, always on, so the movies lose the network's picture. Its 33 ms a frame were measured on an RX 6800 XT, which emulates the network's matrix operations; on NVIDIA this tree runs them on cooperative matrices. Not measured here. |
| 722 | Interpolates noperspective varyings through perspective interpolation, for the red line in Ocarina's Royal Family's Tomb on an AMD driver. The NVIDIA report in issue 441 says the latest release is fine there. |
| 727, and 600's `IAudioRenderer` command 14 | Stubs for `nx2:vi2` 201, `IGeneralService` 101 and `SetAsyncMemoryPoolControlEnabled`. Unknown commands already get the automatic stub reply here: Ocarina calls 101 at every boot and goes on. |
| 738, second half | Shows a .nup's en-us title in the game list. Here that would hide a title edited in the .nup metadata. The first half, the en-us control data slot, was taken. |
| 740 | Serves a file with a similar name in place of a damaged one from a bad dump: a stand-in, not a fix. |
| 741 | Prepares shaders on the loading screen. Built on 579, which is not merged. |
| 488 | Its first half, dropping same-value vertex buffer stores on the split front, is here as 178. The second half reads two per-method switches without a function-local static: 1.2% of the back thread by its author's profile. |
| 161 | Its desktop submission budget and deferred fragment barrier are already here. |
| 717, the parts not taken | Measured in Kokiri Forest on an RTX 3050 Laptop, built as-is with GCC 13: 32.0 fps against 25.7 for this tree. The parts taken, Occlusion Off and the kernel lock and trap paths, are cited in their commits. Its switch groups, each turned off on top of the whole PR: its JIT changes 29.9, its buffer cache 29.4, its split GPU thread 30.6, its back-thread caches 30.9, its Ocarina macros 31.1. Its reactive readback is what this tree's preflush already does (26.0 with it off, this tree's level). Ported onto this tree (28.3 fps with all four of its Ocarina patches), turning off native zstd gave 29.0, the fast string lookup 29.4 and the resource poll backoff 28.5, within the runs' spread, against 24.9 for Occlusion Off. Its widened thread affinity for Ocarina's main threads changed nothing (29.4 with and without). Its JIT options: `elide_redundant_zext` makes Ocarina read unmapped memory at main+0x1753024 (32 to 535 times a run; none with only that option off). Without it, ported onto this tree, the rest gave Kokiri Forest 28.8 -> 29.0 and Link's house 36.2 -> 37.5 fps over three rounds. Not taken: no gain outdoors, and a differential test against the plain JIT (random programs, fastmem only) did not catch the zero-extend bug either, so it does not cover the paths that have one. |
| 204, 335, 467, 213, 267, 219, 411 | Not reviewed in depth yet. |
