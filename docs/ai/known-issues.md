# Known issues

Bugs found and not fixed yet. Each one was reproduced as stated unless it says otherwise. Fix them as
ordinary changes, with a test, and delete the entry.

## Tooling

| Where | Problem |
|---|---|
| Bundled OpenSSL | With no system OpenSSL headers, the first configure builds it from source and every later configure fails. See [Building](building.md). |
