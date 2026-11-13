# Building on Linux

[Build](../Build.md) and [Dependencies](../Deps.md) are the full guides for every platform. This page
is the short path for agents on a Linux machine: everything runs in a container, so the host needs
only Docker and nothing is installed on it.

## The container

`tools/dev/Dockerfile` mirrors `.github/workflows/build-check.yml`: Ubuntu 24.04, GCC 13, Clang 20,
CMake 4.4.4, Qt 6.8.3, Ninja, ccache, and the linters and Mesa's software Vulkan driver.
`tools/dev/in-docker.sh` builds the image on first use, mounts the repository at `/src`, and runs a
command as your user, so its files stay yours.

```sh
tools/dev/in-docker.sh tools/ci/quick-checks.sh        # The fast gate
tools/dev/in-docker.sh tools/dev/build-tests.sh        # Build everything, run ctest, with Clang
tools/dev/in-docker.sh tools/dev/build-tests.sh gcc    # The same with GCC 13
tools/dev/in-docker.sh tools/dev/build-tests.sh -R nup # Only tests matching a regex
```

Build output goes to `build/<compiler>`. Dependency sources and compiler objects go to `.cache/`. Git
ignores both, and both survive between runs, so only the first build is slow.

| Variable | Default | Use |
|---|---|---|
| `LICHII_JOBS` | 12 | Parallel compile jobs. A job can take 2 GB of memory |
| `LICHII_DOCKER_MEMORY` | `24g` | Memory cap for the container |

Set them before `tools/dev/in-docker.sh` for the memory cap. `LICHII_JOBS` is read inside the container,
so on a smaller machine run `tools/dev/in-docker.sh env LICHII_JOBS=6 tools/dev/build-tests.sh`.

If the host has less than about 24 GB free, lower both: an out-of-memory compile shows up as `Killed`
or a signal 9 in the log, not as a compiler error.

## Requirements the docs used to get wrong

- **Clang 19 or newer** on x86-64. Ubuntu 24.04's default `clang` is 18, and the build fails on every
  file with `unsupported argument 'gnu2' to option '-mtls-dialect='`.
- **CMake 3.31 or newer.** Ubuntu 24.04 ships 3.28, which fails at configure.
- `DYNARMIC_TESTS` does not follow `BUILD_TESTING` in practice. Pass it explicitly.
- **`gcc` on the `PATH` without `libssl-dev`.** CMake then builds OpenSSL from source, and that build
  runs `gcc` whatever `CMAKE_C_COMPILER` says, so a machine with only `gcc-13` stops with error 127.

## Targets

The target names are not the file names:

| Target | Output in `build/<compiler>/bin` | Needs |
|---|---|---|
| `yuzu` | `kaichii` | `ENABLE_QT=ON` |
| `yuzu-cmd` | `eden-cli` | `YUZU_CMD=ON` |
| `tests` | `tests` | `YUZU_TESTS=ON` |
| `dynarmic_tests` | `dynarmic_tests` | `DYNARMIC_TESTS=ON` |
| `shader-sweep`, `nup-shader-export`, `maxwell-ir`, `maxwell-spirv`, `maxwell-disas` | Same names | `ENABLE_DEBUG_TOOLS=ON` |

To rebuild one target after a change:

```sh
tools/dev/in-docker.sh cmake --build build/clang --target tests --parallel 12
tools/dev/in-docker.sh build/clang/bin/tests '[ampere]'
```

[CMake Options](../Options.md) lists the rest.

## Running the emulator

You can build `kaichii` and `eden-cli` here, but running a game needs your own legally obtained game
files, plus a GPU with a recent Vulkan driver, and keys and firmware for anything but a `.nup`. The
container has neither a GPU nor a display, so run the binaries on the host. [Testing](testing.md#running-a-game)
shows how to do that without touching a player's data.

Windows is not built here. See the [Windows steps](../Build.md).
