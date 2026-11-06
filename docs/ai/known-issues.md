# Known issues

Bugs found while building the test suite, none of them fixed yet. Each one was reproduced as stated
unless it says otherwise. Fix them as ordinary changes, with a test, and delete the entry.

## Memory safety and hangs on malformed files

These are reached by parsing files, so a corrupt or hostile game dump, update or mod can trigger them.

| Where | Problem | How it was confirmed |
|---|---|---|
| `src/core/file_sys/vfs/vfs_vector.cpp:43` | `VectorVfsFile::Read` computes `data.size() - offset`, which wraps when `offset` is past the end. The `memcpy` then reads outside the buffer and reports success. `ArrayVfsFile::Read` in `src/core/file_sys/vfs/vfs_vector.h` does the same. Fix: return 0 when `offset >= size`. | An NPDM with its ACI0 offset past the end loaded as `Success`, reading heap memory. Tests use an offset at the end of the file to stay defined. |
| `src/core/file_sys/vfs/vfs_offset.cpp:94` | `OffsetVfsFile::TrimToFit` has the same wrap in `size - r_offset`, so a read past the end of a slice reaches whatever follows it in the parent file. | By reading the code. |
| `src/core/file_sys/romfs.cpp` | `GetEntry` returns an empty entry for an out-of-range offset. Its sibling field is 0, not `ROMFS_ENTRY_EMPTY`, so `ProcessDirectory` loops forever and allocates as it goes. A RomFS whose directory table is empty is enough. | A test with an empty directory table hung until killed. |
| `src/core/file_sys/partition_filesystem.cpp` | The entry loop counts in a `u16` against a `u32` `num_entries`, so 65536 or more entries never finish. | A test with 65536 entries hung until killed. |
| `src/core/file_sys/partition_filesystem.cpp` | `entry.strtab_offset` is not checked against the string table, so a bad entry reads past the metadata buffer. | By reading the code. |

## Wrong behaviour

| Where | Problem | How it was confirmed |
|---|---|---|
| `src/core/file_sys/ips_layer.cpp` | A plain IPS record running past the end of the file discards the whole patch. The code clips the read to the file, then compares the result against the unclipped size. Run-length records clip correctly. | The `[!shouldfail]` test in `src/tests/core/ips_layer.cpp`. It starts failing the suite once this is fixed, so remove the tag then. |
| `src/core/file_sys/program_metadata.cpp` | `Load` never checks the `META` or `ACI0` magic. Only `ACID` is validated. | By reading the code. |

## Tooling

| Where | Problem |
|---|---|
| `tools/shellcheck.sh` | `find -exec ... \;` discards `shellcheck`'s exit status, so the script passes whatever it finds. It also forces `-s sh` on the `bash` scripts in `tools/ci`. `tools/ci/quick-checks.sh` runs `shellcheck` itself instead. |
| `EDEN_NX2_FRONT_VERIFY` | Named in a comment in `src/video_core/gpu_split.h`, but nothing reads it. |
| Bundled OpenSSL | With no system OpenSSL headers, the first configure builds it from source and every later configure fails. See [Building](building.md). |
