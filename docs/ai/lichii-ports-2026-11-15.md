# Lichii fixes reviewed on 2026-11-15

This Kaichii port adapts selected fixes from Lichii to this tree. The source PRs are
references for the intended behavior, not patches applied wholesale. The commit keeps the existing
data directory and configuration names and adds no setting or environment switch.

| Source | Change in Kaichii | Check added |
|---|---|---|
| [1736](https://github.com/zortos293/Lichii/pull/1736) | DMA copies and clears update Switch 2 list pages, including destinations whose source has no GPU-modified range. | Synthetic DMA copy and clear cases in `buffer_cache_ports.cpp`. |
| [1835](https://github.com/zortos293/Lichii/pull/1835) | A busy first region prevents a streaming staging-buffer wrap without losing the current position. | Wrap policy test. |
| [1766](https://github.com/zortos293/Lichii/pull/1766) | A failed render-target lookup returns an invalid view ID, distinct from the valid null texture descriptor. | View ID test. |
| [1783](https://github.com/zortos293/Lichii/pull/1783), [1794](https://github.com/zortos293/Lichii/pull/1794) | Guest memory spans require a mapped, contiguous range across every page; the exclusive end is checked. | Cross-page and boundary tests. |
| [1825](https://github.com/zortos293/Lichii/pull/1825) | Constant folding of unsigned bitfield extracts handles a zero count and shifts at the word boundary. | Bitfield edge cases. |
| [1828](https://github.com/zortos293/Lichii/pull/1828) | Short hexadecimal key values are rejected before conversion rather than read past their end. | Malformed synthetic keys. |
| [1802](https://github.com/zortos293/Lichii/pull/1802) | CMIF out buffers begin with their guest contents, so a short file read preserves the untouched tail. | Synthetic IPC file read. |
| [1842](https://github.com/zortos293/Lichii/pull/1842), [1844](https://github.com/zortos293/Lichii/pull/1844) | Network requests and the updater use system CA roots when available, respect explicit OpenSSL overrides and retain the bundled fallback. | CA selection tests. |
| [1847](https://github.com/zortos293/Lichii/pull/1847) | A game's target volume and mute state no longer rewrite the user's saved audio settings. The game may still change the current output level. | IPC audio-controller test. |
| [1805](https://github.com/zortos293/Lichii/pull/1805) | SDL motion timestamps and restart backoff reset when a controller reconnects. | Motion-clock test. |
| [1817](https://github.com/zortos293/Lichii/pull/1817) | Mario Kart World uses the legacy GPU timestamp calculation on the Switch 2 path; other titles keep the Ampere calculation. | Program-ID policy test. |
| [1812](https://github.com/zortos293/Lichii/pull/1812) | Vulkan descriptor storage can grow within a bounded frame budget, and a pending command buffer is submitted before its descriptor memory is reused. Retired ranges no longer force a wait during a draw. | Descriptor submission policy test. |
| [1796](https://github.com/zortos293/Lichii/pull/1796), [1823](https://github.com/zortos293/Lichii/pull/1823) | ASTC-to-BC output staging stays reserved through upload recording. On integrated GPUs, large decode images use bounded layer batches and retry with fewer layers after allocation failure. | Staging lifetime and batch-size tests. |

## Verification and limits

The Linux Clang build and all 11 CTest suites passed with four build jobs. The quick repository gate
passed. These tests use synthetic data and do not render a game. The user reported that the Linux
build works on their machine, without a game-specific scene, settings or frame-time comparison in
this report. Windows and macOS were not built or run here. In particular, the descriptor and ASTC
paths still need game and GPU coverage before claiming a visual or performance improvement.

## Reviewed and left out

- [1780](https://github.com/zortos293/Lichii/pull/1780) is a Windows-only fast path; it awaits a
  Windows build and test.
- [1818](https://github.com/zortos293/Lichii/pull/1818) addresses generic movie frame dropping;
  Kaichii uses a different Ocarina-specific late-frame path. The readback constants in
  [1795](https://github.com/zortos293/Lichii/pull/1795) are absent here.
- The previously identified observation items—1719, 1803, 1807, 1821, 1830, 1845, 1848 and
  1850—were excluded as requested. They change broader GPU, shader preload or cache policies and
  were not established as necessary by a local game trace.
