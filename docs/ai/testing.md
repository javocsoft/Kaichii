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
| FSR3 shaders | `tools/nx2/test_fsr3_shaders.py`, see `tools/nx2/fsr3-shader-tests.md` | Real shaders on software Vulkan | Minutes |
| Debug server | `docs/nx2/debug-mcp.md` | Live state of a running game | Interactive |
| Playing the game | A person | What players see | The only proof of a game fix |

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

## Coverage

```sh
tools/dev/in-docker.sh tools/dev/coverage.sh
```

This builds with Clang source coverage in `build/coverage`, runs `ctest`, and prints line coverage by
area and the files where recent change lands on untested lines. The total is a poor goal, because much of
the tree only runs with a GPU or a game. Use the ranking to choose what to test next.

COVERAGE_SNAPSHOT
