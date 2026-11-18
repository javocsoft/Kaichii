# Task and mesh output stage buffer reads (2026-11-18)

Source: [Lichii #2074](https://github.com/zortos293/Lichii/pull/2074), for the rejected
`ISBERD.ATTR.32.O` instruction reported in Switch Sports Resort Power Cruising
([#2066](https://github.com/zortos293/Lichii/issues/2066)). Kaichii's Ampere frontend
previously accepted reads of the input attribute region and writes to the output region,
but rejected a read back from the output region.

The port accepts only the 32-bit output attribute read without `.SKEW`. The translator
keeps it distinct from an input read, and the SPIR-V backend reads the same task payload
or mesh shared attribute word that an output attribute write stores. Other unmodelled
access modes remain rejected. No switch was added and no Switch 1 shader path changed.

Synthetic tests use the short instruction words from the report. They check the accepted
mode, rejected variants, IR translation, and SPIR-V emission for native and compute-backed
task and mesh programs. The tests establish that the shader compiler accepts and emits the
instruction. The Linux Clang and GCC C++ suites passed, and the four emitted native and
compute-backed task and mesh modules passed `spirv-val --target-env vulkan1.3`. None of these
checks proves that the water renders correctly. The affected game is not
available on this machine, so a game run and a visual comparison are still needed. Reads
of words written by another invocation also require the game to order those writes and
reads; this port adds no synchronization that the guest program did not request.
