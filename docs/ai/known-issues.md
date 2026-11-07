# Known issues

Bugs found and not fixed yet. Each one was reproduced as stated unless it says otherwise. Fix them as
ordinary changes, with a test, and delete the entry.

## Tooling

| Where | Problem |
|---|---|
| `tools/shellcheck.sh` | `find -exec ... \;` discards `shellcheck`'s exit status, so the script passes whatever it finds. It also forces `-s sh` on the `bash` scripts in `tools/ci`. `tools/ci/quick-checks.sh` runs `shellcheck` itself instead. |
| `EDEN_NX2_FRONT_VERIFY` | Named in a comment in `src/video_core/gpu_split.h`, but nothing reads it. |
| Bundled OpenSSL | With no system OpenSSL headers, the first configure builds it from source and every later configure fails. See [Building](building.md). |
