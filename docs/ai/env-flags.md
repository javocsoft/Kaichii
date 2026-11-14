# `EDEN_*` environment switches

Every switch the emulator reads, grouped by what you would use it for. `tools/ci/check_repo.py`
fails the fast CI gate when `src/` reads a switch missing from this page, or this page names one
`src/` no longer reads, so add a row in the same commit that adds a switch.

All of them are off unless set. Most are read once at startup into a `static const`, so changing
one needs a relaunch. The GUI cannot set environment variables: put `NAME=VALUE` lines in
`nx2_switches.txt` next to the executable or in the `eden` directory of the user's configuration.
`src/yuzu/nx2_switches.h` loads those files and keeps only `EDEN_NX2_*` and an explicit list of other
names. A variable in the real environment always wins.

Kinds, which decide what may merge (see "Toggles and merging" in `AGENTS.md`):

- **diag**: log or dump only. Diagnostic tools stay in nx2, off by default.
- **A/B**: restores an alternative to the shipped behaviour, for comparing. New experiment switches
  of this kind do not merge; these are the ones already in the tree.
- **tune**: overrides a number the code otherwise picks itself.

Unless a row says otherwise, a boolean diag switch only tests for presence, so `=0` still enables
it. A/B switches named for what they turn off take `=0`, as the row shows.

## Capture and dumps

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_FRAME_DUMP` | diag | `=<dir>`: one frame's pass colour targets, dispatch storage images and present source. Needs a Vulkan renderer. See `tools/nx2/frame_dump_index.py`. |
| `EDEN_NX2_FRAME_DUMP_AT` | diag | When: `<seconds>` (default 60), `<frame>f`, or `file` to wait for `<dir>/capture.now`. |
| `EDEN_NX2_FRAME_DUMP_SAMPLED` | diag | Adds every texture each pass samples. Slow; use only when you need pass inputs. |
| `EDEN_NX2_FRAME_CAPTURE` | diag | `=<dir>`: saves what the renderer shows every 120th present as `nx2-frame-<n>.tga`. |
| `EDEN_NX2_FRAME_CAPTURE_EVERY` | diag | `=<n>`: interval for `EDEN_NX2_FRAME_CAPTURE`. |
| `EDEN_NX2_SHADER_DUMP` | diag | `=<dir>`: native programs with `.header.bin` sidecars and `failed-compute-*.env` for failed compute pipelines. Input for `tools/nx2/shader_sweep.py`. |
| `EDEN_NX2_REJECT` | A/B | `=<hex address>,...`: treat these programs as rejected, by guest address. |
| `EDEN_NX2_REJECT_HASH` | A/B | `=<hash>,...`: the same by native program key or shader hash. The debug server changes this list live. |
| `EDEN_NX2_TEXTURE_DUMP` | diag | `=<file>`: first 4 MiB of the last traced texture's memory. |
| `EDEN_NX2_DUMP` | diag | `=<present>:<hex address>:<hex size>[,...]`: at that present, flush those GPU ranges from the host and save their bytes. |
| `EDEN_NX2_PB_DUMP` | diag | `=<file>`: with `EDEN_NX2_PB_CHECK`, writes the whole command list for offline replay. |
| `EDEN_NX2_MME_DUMP` | diag | Writes compute macro code once per engine and logs each call with entry and parameters. |
| `EDEN_NX2_MME_RECORD` | diag | `=<file>`: first 200000 macro calls, then the code to `<file>.code`, to check interpreter changes offline. |
| `EDEN_NX2_MME_RECORD_SKIP` | diag | `=<n>`: skip the first n calls before recording. |
| `EDEN_NATIVE_FRAME_CAPTURE` | diag | Writes `native-graphics-mme-<context>.bin` once per macro context, and captures frame state. |
| `EDEN_NATIVE_DRAW_CAPTURE` | diag | Captures the first 16 native draws. |
| `EDEN_NX2_CRASH_DUMP` | diag | `=<dir>`: on an unhandled exception in `eden-cli`, write `<dir>/eden-crash-<pid>.dmp`. Windows. |

## GPU and rendering traces

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_RENDER_TRACE` | diag | Per-draw state, rejected stages and zero-format descriptors. Use the filter `*:Info HW.GPU:Debug Render.Vulkan:Debug Shader:Debug`. |
| `EDEN_NX2_WINDOW_TRACE` | diag | Describes clears and draws into screen-sized colour targets. |
| `EDEN_NX2_WINDOW_TRACE_SKIP` | diag | `=<n>`: skip the first n of those. |
| `EDEN_NX2_RT_TRACE` | diag | Render target, 2D and DMA traffic. |
| `EDEN_NX2_TRACE_VS` | diag | `=<address>`: describe the next few draws using that vertex shader. |
| `EDEN_NX2_TRACE_PS` | diag | `=<hex>`: restrict the upscale trace to that pixel shader. |
| `EDEN_NX2_TRACE_DST` | diag | `=<hex address>`: trace blits and DMA copies landing on that destination. |
| `EDEN_NX2_CB24_TRACE` | diag | For draws whose vertex shader reads bank 24, show the inline constant buffer upload target and first words. |
| `EDEN_NX2_UPSCALER_TRACE` | diag | Logs the resources of native compute dispatches around the first one using software HMMA. Prints addresses and hashes, never data. |
| `EDEN_NX2_UPSCALER_TRACE_FRAME` | diag | `=<n>`: earliest frame to arm it (default 600). |
| `EDEN_NX2_UPSCALER_TRACE_FRAMES` | diag | `=<n>`: frames to log (default 3). |
| `EDEN_NX2_DLSS_CONTRACT_TRACE` | diag | Inputs and outputs of replaced upscaler networks. |
| `EDEN_NX2_DLSS_CONTRACT_SKIP` | diag | `=<n>`: skip the first n, at most 1000000. |
| `EDEN_NX2_DLSS_LOG` | diag | `=<level>`: Streamline log level. |
| `EDEN_NX2_PIPELINE_DIFF` | diag | For a new pipeline reusing an earlier one's shaders, logs which fixed state differs. Finds state that splits the same program. |
| `EDEN_NX2_PIPELINE_BUILD_TRACE` | diag | `=<file>`: a line before and after every driver pipeline build, flushed at once, so the last BEGIN without an END names the shaders a driver crashed on. |
| `EDEN_NX2_PENDING_WRITE_TRACE` | diag | Reports shaders reading buffer bytes that guest CPU writes have not reached yet. |
| `EDEN_NX2_TABLE_POINTER_TRACE` | diag | Logs table-pointer slot changes and whether guest and host copies can differ. |
| `EDEN_NX2_DOWNLOAD_TRACE` | diag | What the buffer cache copies back at fences, and the pages whose CPU reads had to flush, every 10 s. |
| `EDEN_NX2_RESCALE_TRACE` | diag | `=1`: with a resolution scale other than 1x, logs (`HW.GPU`, Info) every image the texture cache scales up or down, what caused it, and when it scales one down, the image and the reason that keep it from the scale (`not downscaleable` is 512 or fewer rows tall). Only changes are logged; after 4,000, one in a thousand. |
| `EDEN_NX2_READBLOCK_TRACE` | diag | `=1`: logs every unmapped device memory read with its caller, not only the first 16. |
| `EDEN_NX2_QUERY_TRACE` | diag | Query cache activity. |
| `EDEN_NX2_QUERY_DMA_TRACE` | diag | MME DMA reads of reports whose host result is not in guest memory yet. |
| `EDEN_NX2_QMD_TRACE` | diag | QMD scheduler loads, dependency counts and launches. |
| `EDEN_NX2_METHOD_TRACE` | diag | Every GPU engine method call. |
| `EDEN_NX2_PULLER_TRACE` | diag | Every puller method. |
| `EDEN_NX2_3D_METHODS` | diag | First write to each 3D register. Finds methods the engine does not model. |
| `EDEN_NX2_3D_HISTORY` | diag | Keeps a register write history for macro parameter errors. |
| `EDEN_NX2_SEM_WATCH` | diag | `=<gpu address>`: logs every method whose argument matches those 16 bytes. Finds what releases a semaphore. |
| `EDEN_NX2_SEM_WATCH_EMITS` | diag | Adds compute macro emits to that watch. |
| `EDEN_NX2_CMDPAGE_DIAG` | diag | For command list pages GPU work writes: whether it writes the lists' words or only other bytes. |
| `EDEN_NX2_PB_CHECK` | diag | Counts pushbuffer entries that change between submit and parse. |
| `EDEN_NX2_TC_LOG` | diag | Device memory against the collector's thresholds, every 600 frames: the images' real allocations (GPU-written and rescaled shares), the transcode cache's hits, evictions put off until nx2:gpu backed the image's memory, and the staging buffers. |
| `EDEN_NX2_SLOW_FRAME_MS` | diag | `=<n>`: logs each present more than n ms after the last, with its fence wait, and the slow work around it (file reads over 8 ms, CPU read flushes and texture collector GPU waits over 5 ms), to line stutters up with their cause. |
| `EDEN_NX2_TC_SKIP_STALE` | A/B | `=0`: uploads guest memory into new images even over pages a dropped GPU image left stale. |
| `EDEN_NX2_MME_LOG` | diag | Every macro call. Very high volume. |
| `EDEN_NX2_MME_MEMO_STATS` | diag | Macro memo counters every 2^24 calls. |
| `EDEN_DKB_INDIRECT_READBACK` | diag | `=1`: reads back indirect draw records while `EDEN_DKB_FRAME_TRACE` is on. |
| `EDEN_NATIVE_DRAW_DIAGNOSTICS` | diag | Bounded native query and draw event log. |
| `EDEN_STARFOX_WINDOW_TRACE` | diag | Star Fox window trace from the dynarmic layer. |

## Guest, timing and performance

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_GUEST_PROFILE` | diag | `=1`: per game thread, the time outside SVCs (running or waiting for a core) and in each SVC, by what it waited on (the IPC service, the event, the lock or condition variable address). Logs the busiest threads every 10 s, and each frame slower than `EDEN_NX2_SLOW_FRAME_MS` (60 ms by default) with what the threads did during it. |
| `EDEN_NX2_SVC_TRACE` | diag | `=1`: every SVC with arguments and results. `=<first>:<last>`: only those presents. |
| `EDEN_NX2_IPC_TRACE` | diag | Every dispatched service command and its result word. |
| `EDEN_NX2_VIBRATION_TRACE` | diag | `=1`: logs each vibration value a game sends through hid command 214 when it differs from the last one for that controller (`Service.HID`), what the controller layer did with it (`Input`: dropped within 15 ms of the last one, or passed on), and each rumble given to SDL after the motor mapping (`Input`). After 4,000 lines of each, one in a hundred. |
| `EDEN_NX2_IPC_SUMMARY` | diag | Every 5 s, how often each command ran and with which result. Finds a game polling something that never becomes ready. |
| `EDEN_NX2_MEM_TRACE` | diag | The title's memory layout and size queries. |
| `EDEN_HYBRID_CORES` | A/B | Windows only. `=0` stops keeping time-critical threads on the performance cores of hybrid CPUs, which is done only with 8 or more performance logical processors. |
| `EDEN_NX2_AUDIO_LEVEL` | diag | Output peak about every two seconds. |
| `EDEN_NX2_VDEC_TRACE` | diag | Video decoder submit, decode and composite trace. |
| `EDEN_NX2_MOVIE_CLOCK` | diag | `=1`: logs wall and guest time, presents, video takes and audio counters once a second, to check a movie against its sound. |
| `EDEN_NX2_VDEC_DUMP` | diag | `=<dir>`: luma of every 120th decoded frame at quarter size. |
| `EDEN_NX2_FPS_LOG` | diag | `eden-cli`: logs the title-bar performance line. |
| `EDEN_NX2_GPU_PROFILE` | diag | GPU thread time per second by zone, and split drains by site. |
| `EDEN_DKB_FRAME_TRACE` | diag | `=1`: per-present interval, CPU and fence timing. |
| `EDEN_NX2_SAMPLE` | diag | `=<thread names>`: sample those host threads. |
| `EDEN_NX2_SAMPLE_OUT` | diag | Output file (default `nx2_samples.txt`). |
| `EDEN_NX2_SAMPLE_START` | diag | Seconds before sampling (default 60). |
| `EDEN_NX2_SAMPLE_SECONDS` | diag | Sampling length (default 30). |

## Automation and the debug server

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_DEBUG_SERVER` | diag | `=<host>:<port>`: starts the in-process debug server. See `docs/nx2/debug-mcp.md`. |
| `EDEN_NX2_DEBUG_DIR` | diag | `=<dir>`: where the server writes its dumps. |
| `EDEN_NX2_HIDE_WINDOW` | diag | `eden-cli`: create the window hidden and unfocused, for unattended runs. `=0` disables. |
| `EDEN_NX2_KEY_SCRIPT` | diag | `eden-cli`: `seconds:code,seconds:-code,...` or `@<file>`. Presses and releases keys by SDL scancode. |
| `EDEN_NX2_KEY_RECORD` | diag | `eden-cli`: `=<file>`: appends each press and release in that format. |
| `EDEN_CLI_RESTART_HANDOFF` | diag | Set by `eden-cli` for itself when an application restarts; not for manual use. |

## Guest compatibility

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_AUTO_STUB` | A/B | Answers every unknown service command with an empty success. Hides bugs; do not ship a fix that needs it. |
| `EDEN_NX2_EMPTY_SYSTEM_DATA` | A/B | `=0`: fail Switch 2 system data requests instead of serving an empty archive. |
| `EDEN_NX2_CNTFRQ` | tune | `=<Hz>`: overrides the 31.25 MHz Switch 2 counter frequency. |
| `EDEN_NX2_CNTPCT_PAUSE` | A/B | `=0`: no pause in guest counter spin loops. |
| `EDEN_NX2_SWAP_INTERVAL` | tune | `=<n>`: forces the present swap interval. |
| `EDEN_NX2_FPS_LIMIT` | tune | `=<fps>`: holds presents to that rate when it is below the swap interval's; overrides the FPS limit setting. |
| `EDEN_NX2_VSYNC_HZ` | tune | `=<rate>`: vsync event rate. |
| `EDEN_NX2_CROP_WIDTH` | tune | `=<n>`: overrides the presented crop width. |
| `EDEN_NX2_CROP_HEIGHT` | tune | `=<n>`: overrides the presented crop height. |
| `EDEN_NX2_VDEC_EOS_RELEASE_NOW` | A/B | Non-zero: release synthetic end-of-stream video frames immediately. |
| `EDEN_NX2_IDLE_SPIN_US` | tune | `=<n>`: microseconds an idle core polls before blocking (Switch 2 default 100 outside Android). |
| `EDEN_VI2_MAX_RESOLUTION_MODE` | tune | `=<n>`: forces the mode vi2 reports (default 4 docked, 2 handheld). |
| `EDEN_NX2_IPC_MIN_US` | tune | `=<n>`: how long to hold an HLE reply when another thread took the client's core. `0` turns it off. |

## GPU threading and command handling

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_SPLIT` | A/B | GPU thread split mode: `2` (default) macros on the front, `1` macros on the back, anything else off. Only with Vulkan and neither safe DMA nor high GPU accuracy. See `docs/nx2/gpu-thread-split.md`. |
| `EDEN_NX2_SPLIT_CHECK` | diag | Reports scheduler use from the wrong thread. |
| `EDEN_NX2_SPLIT_SKIP` | A/B | `=0`: send state writes that keep the register's value too. |
| `EDEN_NX2_SPLIT_VERIFY` | diag | `=1`: every 4096 front macro calls, compare front and back registers. |
| `EDEN_NX2_FETCH_DRAIN` | A/B | `=1`: wait for the back before every entry, as before the split overlapped. |
| `EDEN_NX2_FRAMES_AHEAD` | A/B | `=1` always wait for the previous frame, `=0` always for this one. Default is per game. |
| `EDEN_NX2_SYNC_SUBMIT` | A/B | Non-zero: synchronous GPFIFO submission. |
| `EDEN_NX2_PARK_SPLIT_METHODS` | A/B | Restores the old handling of a method's data continuing across GP entries. |
| `EDEN_NX2_PB_SNAPSHOT` | A/B | `=0`: execute pushbuffers from memory rather than from words kept at submit. |
| `EDEN_NX2_RESNAPSHOT` | A/B | `=0`: no re-copy of command words after an acquire is released. |
| `EDEN_NX2_PERF_VERIFY` | diag | Compares `CallMethodFast` register state against `CallMethodSlow`. |

## Memory, buffers and textures

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_READ_TRAPS` | A/B | `=0`: no per-page read traps. |
| `EDEN_NX2_NO_REACTIVE` | A/B | `=1`: drops those traps on Switch 2 programs. A CPU read of GPU-written memory can then see stale data. |
| `EDEN_NX2_BUFFER_DOWNLOADS` | A/B | `=0`/`=1` forces asynchronous buffer downloads. |
| `EDEN_NX2_EXACT_PREFLUSH` | A/B | `=0`: queue whole writes rather than only the marked pages. |
| `EDEN_NX2_INLINE_DEFER` | A/B | `=0`: invalidate every cache at once on inline uploads. |
| `EDEN_NX2_INLINE_PAGE_WRITES` | A/B | `=0`/`=1` forces uploading CPU bytes into the buffer for GPU-owned pages. |
| `EDEN_NX2_CMDPAGE_DRAIN` | A/B | `=1`: drain and flush for every list on draw-written pages. |
| `EDEN_NX2_UBOREUSE` | A/B | `=0`: no reuse of identical uniform stream allocations. |
| `EDEN_NX2_FENCE_BATCH` | A/B | `=0`: one fence at a time. |
| `EDEN_NX2_FENCE_FLUSH_US` | tune | `=<n>`: window for batching fence submissions (default 500). |
| `EDEN_NX2_FLUSH_DRAWS` | tune | Native draw budget per submission; see `src/video_core/renderer_vulkan/submission_policy.h`. |
| `EDEN_NX2_PRIVATE_DEVICE_RANGES` | A/B | `=1`: one device range per address space. |
| `EDEN_NX2_GPU_ONLY_LAZY` | A/B | `=0`: back GPU-only regions whole. |
| `EDEN_NX2_SPARSE_LAZY` | A/B | `=0`: back sparse regions whole. |
| `EDEN_NX2_SPARSE_REMAP` | A/B | `=0`: do not advertise `NVGPU_GPU_FLAGS_SUPPORT_REMAP`. |
| `EDEN_NX2_SPARSE_UNBACKED` | A/B | `=1` allows unbound images on any driver, `=0` on none. AMD drivers fault on them. |
| `EDEN_NX2_SPARSE_ALIASED` | A/B | `=1` shared zeroed block wherever supported, `=0` never. |
| `EDEN_NX2_VRAM_LIMIT_MIB` | tune | `=<MiB>`: VMA fails allocations past this, like a smaller card. |
| `EDEN_PUNCH_HOLE` | A/B | `=0`: no hole punching to return freed host memory (Linux). |
| `EDEN_NX2_TC_EXPECTED_MIB` | tune | Texture cache collector's expected threshold. |
| `EDEN_NX2_TC_CRITICAL_MIB` | tune | Texture cache collector's critical threshold. |
| `EDEN_NX2_TC_MIN_AGE` | tune | `=<frames>`: never evict images used more recently. |
| `EDEN_NX2_TC_OLD_AGE` | tune | `=<frames>`: age for evicting costly images under pressure (default 1800, `0` off). |
| `EDEN_NX2_ASTC_ARRAY_MIB` | tune | `=<MiB>`: most a trimmed ASTC array keeps on the host. |
| `EDEN_NX2_ASTC_BC1` | A/B | `=0`: keep every re-compressed ASTC image in BC3. |
| `EDEN_NX2_ASTC_FLUSH` | A/B | `=0`: wait for the host GPU to go idle after each ASTC decode. |
| `EDEN_NX2_KEEP_ASTC` | A/B | `=1`: keep ASTC uncompressed below a 20 GiB memory budget. |
| `EDEN_NX2_TRANSCODE_THREADS` | tune | `=<n>`: texture transcode workers (default a quarter of hardware threads). |

## Macros and engines

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_MME_JIT` | A/B | `=0`: interpret macros instead of compiling them (x86-64). |
| `EDEN_NX2_MME_MEMO` | A/B | `=0`: no replay of repeated macro calls. |
| `EDEN_NX2_MACRO_COPY` | A/B | Always copy macro parameters into the vector. |
| `EDEN_NX2_SKIP_SAME` | A/B | `=0`: process 3D register writes that keep the value. |

## Shaders and pipelines

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_NX2_HANDLE_TABLE_NOTES` | diag | `=1`: logs each handle table a shader indexes per invocation (a light table's shadow maps in Ocarina of Time), once per program and texture, with the TIC indices it read; the draws and dispatches gather them only while it is set. |
| `EDEN_NX2_FORCE_MESH_FALLBACK` | A/B | `=1`: leaves VK_EXT_mesh_shader off, so task and mesh draws take the compute fallback of devices without it. |
| `EDEN_NX2_MESH_EMULATION_DUMP` | diag | `=<dir>`: writes the vertex shader of each emulated mesh program there, for spirv-val. |
| `EDEN_NX2_MESH_EMULATION_PROBE` | diag | `=1`: every 60 frames, reads one emulated mesh draw's output back and logs it. |
| `EDEN_NX2_EXPERIMENTAL_GEOMETRY` | A/B | `=0`: disable native geometry stage lowering. |
| `EDEN_NX2_NO_TENSOR` | A/B | Disables tensor multiply-add translation. |
| `EDEN_NX2_UNDEF_ZERO` | A/B | `=0`: emit `OpUndef` for registers read before any write, rather than zero. |
| `EDEN_NX2_NO_MESH` | A/B | `=1`: skip task and mesh draws. |
| `EDEN_NX2_NO_MESH_FILE` | A/B | `=<path>`: skip them while that file exists, to compare within one run. |
| `EDEN_NX2_SKIP_VS` | diag | `=<address>,...`: drop draws using those vertex shaders. |
| `EDEN_NX2_FORCE_LOD0` | diag | Sample only the top mip level. |
| `EDEN_NX2_RESETFIX` | A/B | `=0`: no host execution of the reset shader's indirect record writes. |
| `EDEN_NX2_ASYNC_DISCARD` | A/B | `=1`: let native draws follow the async-shader skip heuristic. |
| `EDEN_NX2_PRELOAD_ALL_PIPELINES` | A/B | `=1`: build every cached pipeline at boot. |
| `EDEN_NATIVE_SHADER_OVERRIDE_DIR` | diag | `=<dir>`: a rejected program waits up to 30 minutes for `<key>.override.bin`, dumping `<key>.source.bin` and `<key>.mapped.bin` there, so a lowering can be tried without a rebuild. Stalls the guest while it waits. |
| `EGLANTINE_TEST_VERTEX_REPACK` | A/B | `=0`: no vertex repacking. |
| `EGLANTINE_EXPERIMENTAL_STATIC_VERTEX_REPACK` | A/B | Selects static vertex repacking; see `src/video_core/renderer_vulkan/vertex_repack.h`. |

## Upscaling

| Switch | Kind | Effect |
|---|---|---|
| `EDEN_STREAMLINE_DIR` | tune | `=<dir>`: first place to look for NVIDIA Streamline. |
| `EDEN_NX2_DLSS_REPLACEMENT` | A/B | Selects the replacement for a game's DLSS network, such as `=dlss` for Fortune's Weave. See `docs/native-dlss-replacement.md`. |
| `EDEN_NX2_DLSS_PRESET` | tune | `=J`, `K`, `L`, `M`, or `default` to leave the choice to NVIDIA. |
| `EDEN_NX2_DLSS_HDR` | A/B | `=0`: no HDR path for DLSS. |
| `EDEN_NX2_DLSS_AUTO_EXPOSURE` | A/B | `=0`: no DLSS auto exposure. |
| `EDEN_NX2_TEMPORAL_WEIGHT` | tune | Current-frame weight of the temporal stand-in (default 0.1). |
| `EDEN_NX2_TEMPORAL_KERNEL` | tune | Sample kernel scale for still pixels (default 3.0). |
| `EDEN_NX2_TEMPORAL_KERNEL_MOTION` | tune | Sample kernel scale for moving pixels (default 1.0). |
| `EDEN_NX2_TEMPORAL_CLIP` | tune | Neighbourhood clip width for still pixels (default 1.5). |
| `EDEN_NX2_TEMPORAL_CLIP_MOTION` | tune | Neighbourhood clip width for moving pixels (default 1.0). |
| `EDEN_NX2_TEMPORAL_SHARPEN` | tune | `=<stops>` or `off`: overrides the sharpness slider. |
