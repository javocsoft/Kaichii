# First-use compute pipeline stutter

Measured on 2026-11-16 at `c9d427387f` (Linux, NVIDIA 595.104.2, RTX 3050 Laptop
4 GB) with the Ocarina of Time save in Kokiri Forest. The runs used isolated data
and driver-cache directories, the same copied save, 0.5x resolution, the performance
profile (`use_asynchronous_shaders=true`, `native_async_draws=1`) and no frame
generation. This is a measurement of that scene and driver, not a cross-platform
performance claim.

## What blocked the first load

A temporary probe timed the scheduler callbacks that hold a draw or dispatch until
its Vulkan pipeline is built. In the cold run, **25 compute dispatches waited over
50 ms**, totalling 21.7 s of serial callback time; the longest took 2.55 s. No
graphics draw callback waited over 50 ms. The driver trace contained 104 completed
compute builds (median 46.5 ms, maximum 2.97 s, 21 over 500 ms) and 628 graphics
builds (median 70.2 ms, maximum 2.00 s). Driver build durations overlap across
threads and must not be summed as elapsed time. Graphics translation ran in the
configured background mode, which logged skipped draw lookups while pipelines were
pending; the absence of a graphics wait does not mean the graphics path had no
visible cost.

A 25-second `frame_times` window taken during the cold save load had 66 presented
frames, 18 stutters, a 5.35 s p99 and a 8.00 s maximum. It covers game loading and
other work too, so it is not a pure pipeline benchmark. The long compute callback
waits establish a direct blocking mechanism, not the cause of every long frame.

On the next run, `LoadDiskResources` admitted 655 cached pipelines. The same save
reached Kokiri Forest with **no logged runtime compute pipeline builds and no compute or
graphics callback waits over 50 ms**. Its 25-second loading window still had a
176 ms p99 and a 359 ms maximum. The input timings and exact capture windows
differed, so the cold and warm frame-rate numbers are not a controlled A/B estimate
of the cache's benefit. They do show that other causes of stutter remain after the
known compute pipelines are prepared.

## Where an earlier build could start

For native compute, `NativeQmdScheduler` can read a QMD before its dependencies let
it dispatch. `KeplerCompute::DispatchNativeCompute` copies the QMD fields into
`launch_description` only when those dependencies resolve, then immediately calls
`RasterizerVulkan::DispatchCompute`. `PipelineCache::CurrentComputePipeline`
creates a new pipeline at that dispatch. `ComputeEnvironment` reads constant-buffer
values, storage-pointer classes and texture types from live GPU memory during
translation. Program bytes found in game files do not supply those values; an
earlier extraction of all 137 compute programs in this Ocarina walk could not
translate them offline without captured runtime environments.

The QMD read looked like a possible lead-time source. A follow-up cold run on the
same save timed the interval from the scheduler's initial QMD read to the point
just before its second read and dispatch. Of 121 distinct compute program addresses
first seen, all showed `0.000 ms` at the probe's three-decimal precision and no
QMD word changed between the two reads. All 66 program addresses in the logged
runtime compute builds matched that first-use set and likewise showed `0.000 ms`.
The probe logged no ready interval over 50 ms. This is a same-call interval, too
short to hide even shader translation, let alone driver compilation. Equality of
the QMD words in that interval says nothing about constant-buffer contents at an
earlier point in the game's execution.

An earlier run using `EDEN_NX2_QMD_TRACE` alone appeared to show some long
load-to-run intervals. That trace emits `run` **after** `DispatchNativeCompute`, so
its interval includes the work of the dispatch and cannot measure preparation
time. The temporary ready probe separated those two intervals. QMD prebuilding
at its current load point is therefore left out. The first required dispatch must
still wait for an unfinished pipeline; skipping it could lose game output.
`ComputePipeline::RunBuild` already claims a queued driver job on the scheduler
thread when that dispatch needs it, avoiding an additional wait behind unrelated
queued jobs.

Reducing the global pipeline worker count from 19 to 4 was tried separately on the
earlier `d581eb7bd9` checkout with the same game and save: the total of compute
waits over 50 ms rose from 16.7 s to 18.9 s. A small reuse change in SSA translation
also showed no consistent improvement in offline A/B measurements. Neither
experiment was retained. The temporary timing probes were removed after this run.

Lichii [PR #1366](https://github.com/zortos293/Lichii/pull/1366) prepares pipelines
from the player's own recorded play. It is still a draft and its reported tests do
not cover this NVIDIA machine or Windows. It targets later sessions; Kaichii's
existing disk cache already eliminated first-use compute waits in the measured
second run.

## Stutters after the shader cache is warm

Additional runs used the same save, settings and isolated cache. A lightly
instrumented run reached controllable gameplay in Kokiri Forest. Its 20-second
stationary gameplay window averaged 34.35 presented frames/s, with a 39.7 ms p99,
one reported stutter and a 172.6 ms maximum. This is a scene measurement, not an
estimate of the improvement from any proposed change.

`EDEN_DKB_FRAME_TRACE` and `EDEN_NX2_GPU_PROFILE` showed that a 1.15 s loading
interval had no present-fence wait and only 7.9 ms of GPU work. A different loading
interval included 81 ms of texture-copy work within a 236 ms GPU-busy frame. Those
traces add substantial logging overhead and do not identify the cause of every
part of either interval.

A separate run used `EDEN_NX2_SLOW_FRAME_MS=80` and `EDEN_NX2_IPC_SUMMARY=1`,
without per-thread guest profiling. The save was visibly loaded before measuring
gameplay. The 20-second gameplay window averaged 31.36 presented frames/s, with a
45.4 ms p99, one reported stutter and a 230.7 ms maximum. The largest logged slow
present waited 0.0 ms for its fence; no slow storage read, CPU read flush or texture
collector wait was logged next to it. The measurements cannot distinguish guest
CPU execution from uninstrumented emulator work or host scheduling.

The save-load transition, by contrast, showed repeated CPU reads of GPU-written
memory that synchronously flush it back to guest memory. Several flushes took
5-40 ms, and another profiling run measured one at 103.8 ms. The IPC summary showed
hundreds of `IStorage::Read` requests per five seconds around the transition, but
individual storage reads over 8 ms were not logged there. The two slow storage
reads in the earlier boot occurred before gameplay. Read counts alone
do not make the storage service the cause of the gameplay hitches. A run with
`EDEN_NX2_GUEST_PROFILE=1` also showed long `fsp-srv` service time during loading,
but its much worse frame times show enough observer effect that it cannot be used
as a performance comparison.

No change to compute scheduling, storage reads or GPU readback follows from these
observations. GPU-to-CPU flushes are a separate, concrete loading-time cost to
inspect before changing them: the guest may require their data, so removing a wait
without proving the dependency would risk incorrect emulation.

## Host-memory paging candidate

A later run sampled `/proc/<pid>/task/*/{stat,schedstat}` every 20 ms and aligned
each sample with the timestamp in the emulator's slow-frame log. Reading the log
at its arrival time would have been wrong: some lines were buffered for up to 19 s.
The 25-second `frame_times` window covered the transition into Kokiri Forest and
early gameplay: 28.83 presented frames/s, 54.2 ms p99, three reported stutters and
a 269.3 ms maximum. It is not comparable with the stationary windows above.

The 269.3 ms present occurred about nine seconds after a screenshot confirmed the
game had entered Kokiri Forest. In the preceding 350 ms, five `ImageTranscode`
workers each used about 287-290 ms of CPU; `DeviceMemory:ColdHint` used 232 ms.
`GPUBack` and three guest-core threads were sampled in uninterruptible sleep (`D`)
several times, while their runnable queue waits remained below 7 ms. The sample
does not contain user or kernel stacks, so it cannot prove which operation put each
thread to sleep. The earlier disk cache for CPU-transcoded textures was already
tried and left out: all 744 textures were read from disk in its second session,
but loading time and Kokiri frame rate did not improve (see the Ocarina row in
`docs/ai/investigations.md`). Saturated transcode workers alone are not grounds to
repeat that cache.

`DeviceMemory:ColdHint` is the worker used by the buffer and texture caches' upload
passes. Kaichii currently calls Linux `madvise(MADV_PAGEOUT)` there, which asks the
kernel to reclaim those guest pages immediately, including swap-out for anonymous
pages. The test host still had about 18 GiB of available RAM and used zram swap.
The same code also runs from the Switch 2 sparse/GPU-only backing pass every 30 s.
This is a plausible source of stalls while other threads touch or remap those pages,
but the local run did not record a controlled A/B or the kernel wait sites.

The upstream work already addresses it:

- Lichii [PR #756](https://github.com/zortos293/Lichii/pull/756) replaces Linux
  `MADV_PAGEOUT` with `MADV_COLD`. Its author measured 3-15 s freezes with a
  guest core waiting in `mprotect`, and no such freezes in two sessions after the
  change. The Windows and macOS code paths were unchanged.
- Lichii [PR #1581](https://github.com/zortos293/Lichii/pull/1581) skips upload and
  sparse/GPU-only cold hints unless available RAM is below both one quarter of
  physical RAM and 4 GiB. Its Windows Mario Kart World A/B improved the reported
  1% lows; its Ocarina A/B was within noise. This run's 18 GiB available would
  skip the hint. Eglantine `42fdfac891` takes a larger approach with a setting,
  pressure checks and a paging-out path that its commit says was not exercised.
- Lichii [PR #1568](https://github.com/zortos293/Lichii/pull/1568) bounds hint
  ranges to device memory so placeholder addresses cannot index the page table
  out of bounds. Kaichii does not yet have that guard.

## Kaichii port and verification

The `fix/cold-hint-memory-policy` branch ports the three fixes into Kaichii's
memory path. Linux uses `MADV_COLD`, while the Windows `VirtualUnlock` and other
platform branches are unchanged. Upload hints and the Switch 2 GPU-only backing
pass run only below the available-RAM threshold, checked at most once a second.
The upload path clips ranges to the device address space before reading page
metadata. There is no player setting or experiment switch.

The fast gate and all 11 C++ test suites passed. New tests cover the RAM threshold,
placeholder and crossing ranges, and preservation of guest data through a cold
hint. Deliberately changing the threshold and range helper made the focused tests
fail; after restoring the code, all 18 focused assertions passed.

A Linux run of the same isolated save reached Kokiri Forest and showed the same
porch scene. The 25-second `frame_times` window after the automated load measured
33.12 presented frames/s, 44.0 ms p99 and a 63.3 ms maximum. The earlier baseline
window measured 28.83 presented frames/s, 54.2 ms p99 and a 269.3 ms maximum.
The windows include a changing load transition, so these figures are only a smoke
comparison, not a controlled estimate of the gain. With ample RAM, the fixed run
had no `DeviceMemory:ColdHint` worker; the always-present `nx2:ColdHint` thread
still existed but returned before its backing pass. Texture transcode workers
remained busy during loading, and a separate 164.7 ms slow present occurred later
in the fixed run. The change does not claim to remove every hitch.

The low-RAM branch was exercised by synthetic threshold and range tests, not by
playing under host memory pressure. Windows and macOS were not built or run in
this local validation. Their platform-specific hint implementations remain as
before; Lichii #1581 supplies a Windows game A/B, while this Kaichii port still
needs its own platform smoke tests before release.
