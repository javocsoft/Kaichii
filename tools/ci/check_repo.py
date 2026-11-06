#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later
"""Repository consistency checks for the fast CI gate (docs/ai/testing.md).

The tree-wide checks guard what drifts silently: tests nothing builds, broken links, undocumented
EDEN_* switches and paths the agent docs no longer match. With --base, the policy checks cover only
the files changed since that commit, because the inherited tree does not meet them yet.
"""

import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]

# Third-party or generated: as in .ci/license-header.sh, plus what it never reaches.
EXTERNAL_PREFIXES = (
    "externals/",
    ".patch/",
    "CMakeModules/CPM",
    "tools/cpm/",
    "tools/windows/vcvarsall.sh",
    "src/dynarmic/externals/",
)
FLAG_RE = re.compile(r"(?:EDEN|EGLANTINE)_[A-Z0-9_]+")
# Macros such as EDEN_DIR share the prefix, so outside the NX2 families only a quoted name counts.
SOURCE_FLAG_RE = re.compile(r"EDEN_(?:NX2|DKB)_[A-Z0-9_]+|\"((?:EDEN|EGLANTINE)_[A-Z0-9_]+)\"")
LINK_RE = re.compile(r"\]\(([^)\s]+)")
CODE_SPAN_RE = re.compile(r"`([^`\n]+)`")
PATH_PREFIXES = ("src/", "tools/", "docs/", ".github/", ".ci/", "hooks/", "CMakeModules/", "dist/")
SPDX_SUFFIXES = (".cpp", ".h", ".hpp", ".inc", ".c", ".kt", ".py", ".sh", ".cmake", ".ps1")


def git(*args):
    return subprocess.run(
        ["git", *args], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout


def tracked(*patterns):
    return [path for path in git("ls-files", "--", *patterns).split("\n") if path]


def is_external(path):
    return path.startswith(EXTERNAL_PREFIXES)


def read(path):
    return (ROOT / path).read_text(encoding="utf-8", errors="replace")


def check_tests_registered():
    """A test file missing from every CMakeLists.txt compiles nowhere and passes forever."""
    listed = "\n".join(read(path) for path in tracked("src/tests/*CMakeLists.txt"))
    return [
        f"{path}: not listed in any src/tests CMakeLists.txt"
        for path in tracked("src/tests/*.cpp")
        if pathlib.PurePosixPath(path).name not in listed
    ]


def check_doc_links():
    problems = []
    for doc in tracked("*.md"):
        if is_external(doc):
            continue
        for number, line in enumerate(read(doc).split("\n"), 1):
            for target in LINK_RE.findall(line):
                target = target.split("#")[0]
                if not target or re.match(r"[a-z][a-z0-9+.-]*:", target):
                    continue
                base = ROOT if target.startswith("/") else (ROOT / doc).parent
                if not (base / target.lstrip("/")).exists():
                    problems.append(f"{doc}:{number}: broken link {target}")
    return problems


def source_flags():
    flags = set()
    for path in tracked("src/*.cpp", "src/*.h", "src/*.inc"):
        if not path.startswith("src/tests/"):
            flags.update(match.group(1) or match.group(0)
                         for match in SOURCE_FLAG_RE.finditer(read(path)))
    # A trailing underscore is prose for a family, such as EDEN_NX2_TC_*_MIB.
    return {flag for flag in flags if not flag.endswith("_")}


def check_env_flags():
    documented = {row.split("`")[1] for row in read("docs/ai/env-flags.md").split("\n")
                  if row.startswith("| `")}
    problems = [f"docs/ai/env-flags.md: {flag} is not read by src/" for flag in sorted(documented - source_flags())]
    problems += [f"{flag}: read by src/ but missing from docs/ai/env-flags.md" for flag in sorted(source_flags() - documented)]
    return problems


def check_agent_doc_paths():
    """Agents act on these paths without checking them, so a stale one costs a wrong turn."""
    problems = []
    for doc in ["AGENTS.md", *tracked("docs/ai/*.md")]:
        for number, line in enumerate(read(doc).split("\n"), 1):
            for span in CODE_SPAN_RE.findall(line):
                if not span.startswith(PATH_PREFIXES) or re.search(r"[\s<>*{}$]", span):
                    continue
                path = span.split(":")[0].rstrip("/")
                if not (ROOT / path).exists():
                    problems.append(f"{doc}:{number}: {path} does not exist")
    return problems


def check_ci_doc():
    """docs/CI.md must describe the workflows that exist, and only those."""
    existing = {path.name for path in (ROOT / ".github/workflows").glob("*.yml")}
    mentioned = set(re.findall(r"[\w.-]+\.yml", read("docs/CI.md")))
    problems = [f"docs/CI.md: names {name}, which is not a workflow" for name in sorted(mentioned - existing)]
    problems += [f"docs/CI.md: does not describe {name}" for name in sorted(existing - mentioned)]
    return problems


def changed_files(base):
    names = git("diff", "--name-only", "--diff-filter=ACMR", f"{base}...HEAD")
    return [name for name in names.split("\n") if name and not is_external(name)]


def check_whitespace(base):
    """The policy of hooks/pre-commit, which nothing enforces unless a contributor installs it."""
    paths = ["src/", "CMakeLists.txt"]
    diff_check = subprocess.run(
        ["git", "-c", "core.whitespace=tab-in-indent,trailing-space", "diff", "--check",
         f"{base}...HEAD", "--", *paths],
        cwd=ROOT, capture_output=True, text=True,
    )
    problems = [line for line in diff_check.stdout.split("\n") if re.match(r"\S+:\d+: ", line)]
    current = ""
    for line in git("diff", "-U0", f"{base}...HEAD", "--", *paths).split("\n"):
        if line.startswith("+++ b/"):
            current = line[len("+++ b/"):]
        elif line.startswith("+") and not line.startswith("+++") and "\t" in line:
            problems.append(f"{current}: added line contains a tab: {line[1:].strip()[:60]}")
    return problems


def check_spdx(base):
    problems = []
    for path in changed_files(base):
        is_cmake = pathlib.PurePosixPath(path).name == "CMakeLists.txt"
        if not (path.endswith(SPDX_SUFFIXES) or is_cmake) or not (ROOT / path).is_file():
            continue
        if "SPDX-License-Identifier:" not in "\n".join(read(path).split("\n")[:5]):
            problems.append(f"{path}: no SPDX-License-Identifier in its first five lines")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--base", help="commit to diff against for the changed-file checks")
    args = parser.parse_args()

    checks = [
        check_tests_registered,
        check_doc_links,
        check_env_flags,
        check_agent_doc_paths,
        check_ci_doc,
    ]
    if args.base:
        checks += [lambda: check_whitespace(args.base), lambda: check_spdx(args.base)]

    problems = [problem for check in checks for problem in check()]
    for problem in problems:
        print(problem)
    print(f"check_repo: {len(problems)} problem(s) in {len(checks)} checks")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
