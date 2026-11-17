# Lichii fixes reviewed on 2026-11-17

This round ports the behaviour of selected Lichii fixes into Kaichii's current renderer and caches.
It adds no setting or environment switch. The source PRs and the local checks are separate evidence:
none of the synthetic tests proves that a game displays or runs faster.

| Source | Kaichii change | Regression check |
|---|---|---|
| [#2021](https://github.com/zortos293/Lichii/pull/2021) | Frame Pacing targets count game frames; generated presents divide each game's interval. Auto pacing stays unchanged. | Different target rates and generation counts, a changing count, and a late frame. |
| [#2023](https://github.com/zortos293/Lichii/pull/2023) | Frame generation uses only free present frames and reserves one for the next game frame. | Generation limits at zero, partial and full free capacity. |
| [#2062](https://github.com/zortos293/Lichii/pull/2062) | An image unregisters from the GPU page tables of the address space where it was registered, regardless of the currently bound channel. | Synthetic two-address-space table ownership and missing-owner cases. |
| [#2070](https://github.com/zortos293/Lichii/pull/2070) | VP9 range-encoder carries update already written bytes directly. | Deterministic comparison with a reference writer; the old code threw `Attempting to read a byte not within the buffer range`. |
| [#2041](https://github.com/zortos293/Lichii/pull/2041) | A Windows sparse-vector fault never removes write access from a page another thread committed. | Windows-only test that stores before and after the fault handler's commit. |

The fast gate and the Linux Clang and GCC C++ suites passed. The VP9 test was run against the old code and failed
with the reported exception before the fix. An isolated Donkey Kong Bananza launch with FSR3 2x and
Target 60 reached the running generation state and produced over 1,600 generated frames without shader
rejections. This confirms that the path runs, not that frame rate or pacing improved against a baseline.
Both Windows binary variants built in a [non-publishing workflow](https://github.com/javocsoft/Kaichii/actions/runs/44129996771).
The Windows-only sparse-vector test was not run, so the page-fault behaviour still needs a Windows test.
The macOS job was cancelled before a runner was assigned and provides no build result. The texture
cache ownership fix requires a multi-channel game case to confirm the reported assertion is gone.

Reviewed and left for a later, game-tested round: [#2012](https://github.com/zortos293/Lichii/pull/2012)
changes native mesh pipeline lookup and was not run in a game by its author;
[#2074](https://github.com/zortos293/Lichii/pull/2074) adds a shader read for Switch Sports Resort
but was also not verified in game. Neither is part of this port.
