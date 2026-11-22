# Lichii and Fritchii fixes reviewed on 2026-11-22

This round reviewed the 200 Lichii PRs merged from 2026-11-17 to 2026-11-22 (#1422 to #2430) and
Fritchii v6 to v6.5. Lichii has renamed its frontend folder and has many test files this tree
lacks, so most PRs need adapting. Each port below was first checked against this tree: the bug it
fixes is present here.

| Source | Kaichii change | Regression check |
|---|---|---|
| [#2266](https://github.com/zortos293/Lichii/pull/2266) (open; Fritchii `2ef3934eea`) | The x64 FRINT fallback reads the rounding mode and exact flag from the bits `EmitFPRound` writes. | `jit_round_int`: FRINTA, FRINTM, FRINTI and FRINTX on halves, singles and doubles. |
| [#2265](https://github.com/zortos293/Lichii/pull/2265) | Only 32-hex user ID folders count as orphaned profiles; a folder whose scan throws is kept. | Title, other and orphaned profile folders on a scratch NAND. |
| [#1971](https://github.com/zortos293/Lichii/pull/1971) | A play time manager saves on destruction only if it changed the database. The tests' stub of it goes. | A manager that changed nothing doesn't save over a newer session. |
| [#2298](https://github.com/zortos293/Lichii/pull/2298) (open; Fritchii `d65ae21a20`) | A cancelled game list scan swaps the registered updates and add-ons back. | `SwapEntries` sets entries aside and restores them. |
| [#2057](https://github.com/zortos293/Lichii/pull/2057) | The slope depth bias is scaled by the resolution factor on rescaled targets (Vulkan, OpenGL). | `VideoCore::RescaledSlopeScale` at 2x, 0.5x and unscaled. |
| [#2415](https://github.com/zortos293/Lichii/pull/2415) | A split register write is parked across an interleaved GPFIFO call. | The reported entry pattern, in `command_writer.cpp`. |
| [#2252](https://github.com/zortos293/Lichii/pull/2252) | CoreTiming pops a due event whose owner expired. | Upstream's test in `core_timing.cpp`. |
| [#2053](https://github.com/zortos293/Lichii/pull/2053) | Code-page invalidation from other threads advances an atomic generation. | None: the race needs hooks inside the page read. |
| [#2360](https://github.com/zortos293/Lichii/pull/2360), [#2414](https://github.com/zortos293/Lichii/pull/2414), [#2301](https://github.com/zortos293/Lichii/pull/2301), [#2299](https://github.com/zortos293/Lichii/pull/2299), [#2052](https://github.com/zortos293/Lichii/pull/2052), [#1972](https://github.com/zortos293/Lichii/pull/1972) | Applied as upstream wrote them. | None here: their tests need a texture cache, shader IR, sysmix or GSV fixture this tree lacks, or fsync behaviour. |

None of these was run in a game here.

## Left for a measured round

For an RTX 3050 Laptop with 4 GB and Ocarina of Time, to measure before and after rather than port
on trust: Fritchii `7994ea8760` (texture cache churn guard and pool floor), Fritchii `2d0da06998` /
Lichii #2316 (4 MiB device-buffer blocks), #2054, #2167, #2104 with #2098 (skipping unbuilt compute
dispatches), #2237 (the GPU front sleeps instead of spinning) and Fritchii's host zstd for Ocarina
(`FRITCHII_HOST_ZSTD`, tied to one build of the game). #2103, #2162 and #2291 build on #1848's NVIDIA
select path, which this tree doesn't have.

## Not taken

Windows-only and macOS-only changes, HOME Menu, BCAT and Nextendo services, the Ocarina FPS Unlock
series (this tree has no FPS Unlock), the opt-in render follower (#2425), Fritchii's AMD work, and
shader lowering for games no Kaichii report names.
