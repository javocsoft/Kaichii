# Testing

## What a green run means

Tests here prove that code behaves as specified on synthetic inputs. They do not prove that a game
works: CI has no games, keys, firmware or GPU, and most rendering bugs only show on screen. Report the
two separately, and never call a game fixed because the tests pass.

## Layers

| Layer | Command | Covers | Time |
|---|---|---|---|
| Fast gate | `tools/dev/in-docker.sh tools/ci/quick-checks.sh` | Docs, switch catalog, test registration, linters on changed files, Python suites | Under a minute |
| C++ tests | `tools/dev/in-docker.sh tools/dev/build-tests.sh` | `tests`, `dynarmic_tests`, `config_tests`, `update_checker_tests`, `compatibility_report` | 20 s to run, after the build |
| Shader tools | See `.github/workflows/linux-extended.yml` | `shader-sweep`, `nup-shader-export`, the fixtures in `tools/shader-sweep/fixtures` | Seconds |
| Shader sweep | `tools/nx2/shader_sweep.py`, see `tools/nx2/shader-sweep.md` | Every captured native program through `spirv-val`. Needs your own dumps | Minutes |
| Vulkan shaders | `tools/nx2/test_fsr3_shaders.py`, `tools/nx2/test_buffer_host_visibility.py`; `.github/workflows/linux-extended.yml` shows how. Give `--build` a Clang tree: with GCC, `-Werror=missing-declarations` stops their probe | Production shaders and barriers on Mesa's software Vulkan, with validation layers | Seconds |
| Sanitizers | `LICHII_SANITIZE=asan tools/dev/build-tests.sh`, see below | Memory errors and undefined behaviour in everything the C++ tests reach | 20 minutes to build |
| Static analysis | `tools/dev/in-docker.sh tools/dev/tidy.sh` | The checks in `.clang-tidy`: bugs, not style. Findings go to `build/clang/tidy.log` | 20 minutes |
| Debug server | `docs/nx2/debug-mcp.md` | Live state of a running game | Interactive |
| Playing the game | A person, or an agent through the debug server, see below | What players see | The only proof of a game fix |

## Which checks a change needs

| You changed | Run at least |
|---|---|
| Docs, workflows, tools | Fast gate |
| `src/shader_recompiler/frontend/ampere` | Fast gate, `tests '[ampere]'`, shader sweep if you have captures |
| Other `src/` code | Fast gate, the C++ tests |
| `src/dynarmic` | Fast gate, `dynarmic_tests` |
| `tools/shader-sweep` | The shader tool suites |
| `CMakeLists.txt`, `cpmfile.json`, `externals/` | A clean configure and both compilers: `tools/dev/build-tests.sh gcc`, then `clang` |
| Anything a player sees | All of the above, then play the game and say so in the PR |

## Adding a C++ test

Tests use Catch2 in `src/tests/<subsystem>/`, one file per subject. The `tests` target compiles them
all into a single binary.

1. Add the file to the list in `src/tests/CMakeLists.txt`. The gate fails if you forget.
2. Start it with the two SPDX lines, and tag cases `[subsystem][subject]`.
3. Run one subject without rebuilding the rest:

```sh
tools/dev/in-docker.sh cmake --build build/clang --target tests --parallel 12
tools/dev/in-docker.sh build/clang/bin/tests '[native_qmd]'
```

Conventions that keep tests useful here:

- **Take expected values from the format, not from the code.** Copying what the function returns only
  freezes today's bugs. Build inputs from the documented layout, as `src/tests/core/program_metadata.cpp`
  does, or take them from a capture.
- **Test the failure paths too.** Most value in a parser test is in what it refuses. The Ampere tests
  flip each reserved bit and check the instruction is rejected.
- **Use synthetic data.** Never add game files, captured textures or anything else from a retail game.
  Short captured instruction words, as in `src/tests/video_core/ampere_*.cpp`, are the accepted exception.
- **Name the test after the behaviour**, and say in a comment which game or bug needs it, as
  `src/tests/video_core/native_qmd.cpp` does.
- **Mind shared state.** `NativeQmdScheduler` keeps one process-wide table and `src/video_core/nx2.h` has
  process-wide globals, so use a fresh address or reset them in a fixture.
- **Prove the test can fail.** Break the code on purpose and watch the test go red before you trust it.

`ScopedSystemTestContext` in `src/tests/core/system_test_context.h` gives a test an isolated `Core::System`
environment when it cannot avoid one.

### Code a test cannot reach

Much of the riskiest code needs `Core::System` or Vulkan. Do not build a mock of them. Move the decision
into a small header with no such dependency, as `src/core/hle/service/nx2_present_pacer.h` and
`src/video_core/renderer_vulkan/async_draw_policy.h` do, and test that. Keep the move small: these files
take frequent upstream merges, and a large refactor costs a conflict on every one.

### Known bugs

When a test exposes a bug you are not fixing now, tag the case `[!shouldfail]` and add it to
[Known issues](known-issues.md). Catch2 then reports it as an expected failure, and the suite fails once
the bug is fixed, so the tag cannot go stale. Do not use this for tests that hang or read invalid memory:
list those in the page only.

## Sanitizers

`LICHII_SANITIZE=asan` builds `tests` and `dynarmic_tests` with AddressSanitizer and UBSan in
`build/clang-asan`, and `tsan` does the same with ThreadSanitizer. Run them with these options:

```sh
tools/dev/in-docker.sh env LICHII_SANITIZE=asan tools/dev/build-tests.sh
tools/dev/in-docker.sh env \
    ASAN_OPTIONS=detect_stack_use_after_return=0:detect_leaks=0:halt_on_error=0:allow_user_segv_handler=0:handle_segv=2:allocator_may_return_null=1:detect_odr_violation=0 \
    UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0 \
    build/clang-asan/bin/tests
```

Two settings are easy to get wrong:

- `allow_user_segv_handler=0` stops Catch2 catching the signal. Otherwise it re-raises it, and ASan
  reports the wrong place and a fault address made of the process and user ids.
- `-fsanitize=function` stays off. It reads the 8 bytes before every call target, which faults when
  generated code starts a mapping, as the macro JIT and dynarmic's spin lock can. The result looks like
  a crash that depends on which tests ran first.

## Running a game

A machine with a GPU and your own legally obtained game can run one without anyone at the controls.
`eden-cli` from the container runs on an Ubuntu 24.04 host as it is; `kaichii` needs the container's
Qt. A `.nup` boots with no keys or firmware.

Point every data directory somewhere else first, even for `--help`, or the run writes into the
player's own configuration and saves. Set the audio to null so nothing plays on their machine.

```sh
export XDG_DATA_HOME=$PWD/run/data XDG_CONFIG_HOME=$PWD/run/config XDG_CACHE_HOME=$PWD/run/cache
EDEN_NX2_DEBUG_SERVER=127.0.0.1:6545 EDEN_NX2_GPU_PROFILE=1 build/gcc/bin/eden-cli -g game.nup &

python3 tools/nx2/debug_mcp.py --server 127.0.0.1:6545 \
    --call '{"method":"input_sequence","params":{"steps":[{"buttons":["A"],"frames":6},{"frames":60}]}}'
python3 tools/nx2/debug_mcp.py --server 127.0.0.1:6545 --call '{"method":"screenshot"}' --out shot.png
python3 tools/nx2/debug_mcp.py --server 127.0.0.1:6545 \
    --call '{"method":"status","params":{"fps_window_ms":10000}}'
```

- The server answers one request at a time. While `input_sequence` runs, `status` waits, which looks
  like a hang.
- Keep a copy of `nand/user/save` once you reach a useful spot, and restore it for each run, so that
  every comparison starts from the same place.
- Check what you are looking at. A cinematic can be a movie: `nx2:vdecsvc frame` in the log and few
  draws per second mean the picture says nothing about rendering, and the frame rate nothing about
  the GPU.
- Compare binaries, not recollections: same save, same steps, one change between them, and the
  frame times from `status`. The first visit to an area also builds its pipelines, so run the
  sequence twice and compare the second.
- Screenshots, saves and shader caches come from the game, so they never go in the repository.

## Coverage

```sh
tools/dev/in-docker.sh tools/dev/coverage.sh
```

This builds with Clang source coverage in `build/coverage`, runs `ctest`, and prints line coverage by
area and the files where recent change lands on untested lines. The total is a poor goal, because much of
the tree only runs with a GPU or a game. Use the ranking to choose what to test next.

### Where the gaps are

Measured on 2026-11-07, `ctest` only: 18.1% of 312,643 instrumented lines.

| Area | Lines covered |
|---|---:|
| `src/shader_recompiler/ir_opt` | 53% |
| `src/shader_recompiler/frontend` | 49% |
| `src/common` | 47% |
| `src/dynarmic` | 32% |
| `src/video_core/buffer_cache` | 31% |
| `src/core/file_sys` | 26% |
| `src/core/hle/kernel` | 20% |
| `src/video_core/engines` | 13% |
| `src/video_core/texture_cache` | 6% |
| `src/audio_core` | 3% |
| `src/video_core/renderer_vulkan` | 1% |
| `src/core/hle/service/nx2_gsv.h` | 0% |

The riskiest code is the least tested, and that is not an oversight. `nx2_gsv.h`, `vk_rasterizer.cpp` and
`texture_cache.h` change the most and sit near 0%, because they need `Core::System`, a Vulkan device or a
running game. Treat changes there as untested: read them closely, keep them small, and test them in the
game. Moving a decision out of them into a small header, as described above, is what lets that improve.
