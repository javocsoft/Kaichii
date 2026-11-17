# Mesh pipeline vertex input aliases (2026-11-18)

Source: [Lichii #2012](https://github.com/zortos293/Lichii/pull/2012), reviewed as a
candidate for Ocarina of Time's first-use pipeline stalls. A native mesh pipeline has no
vertex input, yet a static Vulkan key can retain the attributes left by an earlier vertex
draw. That produces different keys and driver builds for the same mesh shaders.

Kaichii now keys static mesh pipelines by their first seen vertex input for each otherwise
identical graphics key. It seeds those aliases from pipelines already loaded from disk,
normalizes the key before transition lookup, and refreshes the actual vertex registers before
the next vertex draw. The dynamic vertex input path already masks unused attribute types, so
this alias step is skipped there unless transform feedback makes the entire state part of the
key. No setting or environment switch was added.

An isolated A/B run used copies of one Ocarina save and cache, the same RTX 3050 Laptop, and
`EDEN_NX2_PIPELINE_BUILD_TRACE` on each build. The usual dynamic vertex input setting produced
the same 31 mesh pipeline keys in both builds. With `vertex_input_dynamic_state=false`, the
baseline built 72 mesh pipeline keys for 26 shader sets; the candidate built 31 keys for the
same 26 shader sets. Both loaded the save and rendered Link's house. This measures driver
pipeline build count in one scene; it does not establish an FPS gain, a reduction in visible
stutter, or a benefit on other GPUs. Disabling dynamic vertex input made the loading run much
slower and is only a test of the fallback path, not a recommended user setting.

Synthetic tests cover reused static input, distinct shaders and render targets, alternating
formats, and fields omitted from the Vulkan key. The Linux Clang and GCC suites were run;
neither runs a game or builds Windows and macOS.
