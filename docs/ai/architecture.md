# Architecture

Lichii is about 790,000 lines of C++20 that emulate a Nintendo Switch and Switch 2 by high-level
emulation: it recompiles the game's CPU and GPU code and reimplements the system services, instead of
emulating hardware or running firmware. [Design Overview](../DesignOverview.md) and
[HOS Kernel](../HosKernel.md) explain the inherited Switch 1 design. This page covers the map and the
Switch 2 path, where nearly all current work happens.

## Names that mislead

The project was renamed several times (yuzu, Eden, Eglantine, Lichii) and deliberately kept the old
identifiers so that saves and settings keep working. Do not rename them in passing.

| You will see | It means |
|---|---|
| `nx2` | Switch 2, and the default branch |
| Ampere | The Switch 2 GPU. Its shaders are "native programs" |
| Maxwell | The Switch 1 GPU, and the instruction set native programs are lowered to |
| NVN2, NvRm | The Switch 2 graphics API and driver inside the game |
| Target `yuzu` | The Qt desktop app, output `lichii` or `lichii.exe` |
| Target `yuzu-cmd` | The SDL command line app, output `eden-cli` |
| `eden` directory, `EDEN_*` | The user data directory and the switch prefix, unchanged on purpose |
| Front, back, the split | The two halves of the GPU thread |
| DKB | Donkey Kong Bananza |
| Title 1, 2, 3, 4 in comments and `NX2_NOTES.md` | Mario Kart World, Pokémon Pokopia, Kirby Air Riders Global Test Ride, Raiders |

`NX2_NOTES.md` opens with paths from an early local experiment; its blockers table is still the best
history of why each Switch 2 boot quirk exists.

## Source map

| Path | Lines | What it is |
|---|---:|---|
| `src/core` | 285k | System emulation: kernel, services, loaders, filesystems, ARM interface, debugger |
| `src/video_core` | 130k | GPU engines, command processing, caches, Vulkan and OpenGL renderers |
| `src/dynarmic` | 128k | ARM to host JIT, with this fork's additions for Switch 2 instructions |
| `src/shader_recompiler` | 55k | Guest shaders to SPIR-V, GLSL or GLASM |
| `src/yuzu`, `src/qt_common` | 49k | Desktop frontend |
| `src/audio_core` | 33k | Audio rendering and output |
| `src/tests` | 30k | Catch2 tests |
| `src/common` | 28k | Shared utilities, settings, logging |
| `src/hid_core`, `src/input_common` | 37k | Emulated controllers and host input |
| `src/android` | 6k C++ | Android frontend, mostly Kotlin. Not built by any workflow here |
| `src/yuzu_cmd` | 2k | Command line frontend, the usual choice for scripted runs |
| `externals/`, `.patch/`, `cpmfile.json` | | Third-party code and pinned dependencies. See [CPMUtil](../CPMUtil.md) |
| `tools/nx2` | | Switch 2 tooling: shader sweep, debug adapter, binary inspection |

## How a Switch 2 game runs

```
yuzu / yuzu-cmd -> Core::System -> loader -> KProcess
  CPU:     dynarmic JIT -> SVC -> kernel -> HLE services
  GPU:     nvdrv + nx2 sessions -> GPU thread (front, back) -> engines -> rasterizer -> Vulkan
  Shaders: capture -> lower to Maxwell -> Maxwell frontend -> IR passes -> SPIR-V
```

**Detection.** `src/core/file_sys/program_metadata.cpp` recognises the Switch 2 ACID layout and sets
`IsSwitch2()`, which reaches `KProcess::IsSwitch2()` and `Settings::CurrentProgramIsSwitch2()`.
Separately, the GPU side turns on when the game's driver asks for characteristics:
`src/core/hle/service/nvdrv/devices/nvhost_ctrl_gpu.cpp` sets `Tegra::g_nx2_ampere` and reports the
Ampere chip. `g_nx2_ampere` gates command stream behaviour in `src/video_core/dma_pusher.cpp` and
`src/video_core/engines/puller.cpp`.

**CPU.** `src/core/arm/dynarmic` drives the JIT. Switch 2 titles need LSE atomics, pointer
authentication, `LDAPR` and more, added under `src/dynarmic/src/dynarmic/frontend/A64`. `svc` calls land
in `src/core/hle/kernel/svc.cpp`. The kernel emulates six cores (`NUM_CPU_CORES`), and thread creation
folds any core id past them back onto the low ones. `docs/HosKernel.md` and the early rows of
`NX2_NOTES.md` still say four.

**Services.** `src/core/hle/service` reimplements system services. Switch 2 only sessions live in the
header-only `src/core/hle/service/nx2_gsv.h`, at 4,000 lines the largest file on this path: GPU memory,
syncpoints, `vi2` presentation, video and the graphics system. Unknown commands on interfaces from
`src/core/hle/service/nx2_stub_interface.h` answer with an empty success and log what was called.

**GPU commands.** Submission enters at `src/core/hle/service/nvdrv/devices/nvhost_gpu.cpp`, which keeps a
snapshot of pushbuffer words because games reuse submitted memory. `src/video_core/dma_pusher.cpp` parses
them, and `src/video_core/gpu_split.h` optionally runs the work as two threads. The front parses commands
and runs macros against mirrored registers; the back executes engine methods and owns the rasterizer. See
`NX2_GPU_THREAD_SPLIT.md`. Most draws then reach `src/video_core/renderer_vulkan/vk_rasterizer.cpp`.

**Shaders.** `src/video_core/shader_environment.cpp` captures native programs from guest memory.
`src/shader_recompiler/frontend/ampere/lower_to_maxwell.h` rewrites each one into Maxwell instructions.
This is the main Switch 2 file: every instruction a game uses that it cannot express is a rejected
program, which is a missing or wrong effect on screen. Lowered code then takes the inherited path through
`src/shader_recompiler/frontend/maxwell`, `src/shader_recompiler/ir_opt` and
`src/shader_recompiler/backend/spirv`. Pipelines build in
`src/video_core/renderer_vulkan/vk_pipeline_cache.cpp`, on `VkPipelineBuilder` workers where possible
(see `docs/nx2/async-native-pipelines.md`).

A rejection surfaces as a `Shader::RuntimeError`, and the pipeline does not exist. Native programs are
identified by `NativeProgramKey`, the hash `EDEN_NX2_REJECT_HASH` and the dump file names use.

**Present and upscaling.** Frames leave through `vi2` in `nx2_gsv.h` and the code under
`src/video_core/renderer_vulkan/present`. Games ship DLSS-style tensor networks that real hardware
accelerates; see `docs/native-dlss-replacement.md` for the replacement and `src/video_core/frame_gen` for
frame generation.

## Threads

| Thread | Job |
|---|---|
| `CPUCore_<n>` | Guest code through dynarmic |
| `GPU` | The GPU thread, or the front of the split |
| `GPUBack` | The back of the split: engines and rasterizer |
| `VulkanWorker` | Records and submits Vulkan work for the scheduler |
| `VulkanPresent` | Presents frames |
| `VkPipelineBuilder` | Builds pipelines |
| `GPUFencingThread` | Waits for fences and releases guest work |
| `HostTiming` | Timed events |
| `DebugServer` | The debug server, when enabled |

Threading bugs here hide behind timing. Before calling a race fixed, read how `EDEN_NX2_SPLIT_CHECK` and
`EDEN_NX2_SPLIT_VERIFY` work, and compare with an uncaptured run.

## Game-specific code

Behaviour keyed on a program id is spread over about 30 files, so `grep` for the id from the table in
`AGENTS.md` before assuming a game has no special handling. Built-in fixes for particular games also
live in `src/core/file_sys/patch_manager.cpp`.
