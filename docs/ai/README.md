# Documentation for agents

`AGENTS.md` at the repository root holds the rules. These pages hold what you need to follow them.
Read the page for your task, not all of them.

| Task | Page |
|---|---|
| Find where something lives, or how a Switch 2 game reaches the host GPU | [Architecture](architecture.md) |
| Compile on Linux, or find out why a build failed | [Building](building.md) |
| Decide what to test, write a test, or read coverage | [Testing](testing.md) |
| Lower an instruction, stub a service, add a switch, sync the fork | [Recipes](recipes.md) |
| Check whether a symptom was already investigated | [Investigations](investigations.md) |
| See bugs found but not fixed yet | [Known issues](known-issues.md) |
| Look up an `EDEN_*` switch | [Environment switches](env-flags.md) |
| Understand a CI run | [CI and releases](../CI.md) |

The checker in `tools/ci/check_repo.py` keeps these pages honest: it fails on a path in them that no
longer exists, on a switch missing from the catalog, and on a broken link. It cannot check prose, so
when a page disagrees with the code, trust the code and fix the page in the same change.
