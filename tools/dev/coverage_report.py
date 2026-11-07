#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ranks files by how much recent change lands on untested lines, from `llvm-cov export`.

The total is not a target: most of the renderer needs a GPU and most services a running game. The
useful output is the second table, which leaves out code that needs Core::System or Vulkan to run,
because that is where a new unit test can go today.
"""

import argparse
import collections
import json
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
NEEDS_RUNTIME = re.compile(
    r"Core::System|KernelCore|vk::|VkDevice|Vulkan::|RasterizerInterface|QWidget|ServiceFramework")
SKIPPED = ("src/android", "src/video_core/renderer_opengl", "src/dynarmic")


def load_coverage(path):
    files = {}
    for entry in json.loads(pathlib.Path(path).read_text())["data"][0]["files"]:
        lines = entry["summary"]["lines"]
        name = entry["filename"].replace("/src/", "", 1)
        files[name] = (lines["count"], lines["covered"])
    return files


def load_churn(since):
    log = subprocess.run(["git", "log", f"--since={since}", "--no-merges", "--name-only",
                          "--format=", "--", "src"],
                         cwd=ROOT, check=True, capture_output=True, text=True).stdout
    return collections.Counter(line for line in log.split("\n") if line)


def area(path):
    parts = path.split("/")
    if parts[1:3] == ["core", "hle"]:
        return "/".join(parts[:5 if parts[3:4] == ["service"] else 4])
    return "/".join(parts[:3 if parts[1] in ("core", "video_core", "shader_recompiler") else 2])


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("coverage", help="output of llvm-cov export -summary-only")
    parser.add_argument("--since", default="90 days ago", help="churn window for git log")
    parser.add_argument("--limit", type=int, default=25, help="rows per table")
    args = parser.parse_args()

    files = {name: value for name, value in load_coverage(args.coverage).items()
             if name.startswith("src/") and value[0]}
    churn = load_churn(args.since)

    total = sum(count for count, _ in files.values())
    covered = sum(hits for _, hits in files.values())
    print(f"Total: {covered:,} of {total:,} lines, {covered / total:.1%}\n")

    areas = collections.defaultdict(lambda: [0, 0])
    for name, (count, hits) in files.items():
        areas[area(name)][0] += count
        areas[area(name)][1] += hits
    print(f"{'Area':<46}{'Lines':>8}{'Covered':>9}")
    for name, (count, hits) in sorted(areas.items(), key=lambda item: -item[1][0])[:args.limit]:
        print(f"{name:<46}{count:>8}{hits / count:>9.1%}")

    ranked = []
    for name, (count, hits) in files.items():
        if count < 25 or name.startswith(SKIPPED) or hits / count > 0.6:
            continue
        path = ROOT / name
        source = path.read_text(errors="replace") if path.exists() else ""
        # Weighting by the root of the size keeps huge files from drowning out small, hot ones.
        score = churn[name] * (1 - hits / count) * count ** 0.5
        ranked.append((score, name, count, hits / count, churn[name],
                       bool(NEEDS_RUNTIME.search(source))))
    ranked.sort(reverse=True)

    print(f"\n{'Testable today, by risk':<60}{'Lines':>7}{'Covered':>9}{'Changes':>9}")
    for _, name, count, ratio, changes, _ in [row for row in ranked if not row[5]][:args.limit]:
        print(f"{name:<60}{count:>7}{ratio:>9.1%}{changes:>9}")


if __name__ == "__main__":
    main()
