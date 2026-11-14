#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""Writes the "Changes since build N" part of a release's notes to stdout.

release-binaries.yml runs it in a full checkout before it adds the download tables:

    release_changes.py --repo-dir source --previous build-36 --sha <sha> --ref nx2

It names the commit and the previous release, includes the written notes in
.github/release-notes.md when that file changed since the previous release (so old notes never
repeat), and lists every change on the branch's first-parent history since then: merged pull
requests by title and number, and commits pushed straight to the branch. Without a previous
release it writes nothing.
"""

import argparse
import re
import subprocess
import sys

NOTES_FILE = ".github/release-notes.md"
MERGE_SUBJECT = re.compile(r"^Merge pull request #(\d+) from (\S+)")
FIELD = "\x1f"
RECORD = "\x1e"


def git(repo_dir, *args):
    return subprocess.run(
        ["git", *args], cwd=repo_dir, check=True, capture_output=True, text=True
    ).stdout


def changes(repo_dir, previous, sha):
    """(pull request number or None, title, short sha) for each first-parent change, oldest first."""
    log = git(
        repo_dir,
        "log",
        "--first-parent",
        "--reverse",
        f"--format=%h{FIELD}%s{FIELD}%b{RECORD}",
        f"{previous}..{sha}",
    )
    result = []
    for entry in log.split(RECORD):
        entry = entry.strip("\n")
        if not entry:
            continue
        short, subject, body = entry.split(FIELD, 2)
        match = MERGE_SUBJECT.match(subject)
        if match:
            # GitHub puts the pull request's title on the first line of the merge commit's body.
            title = next((line.strip() for line in body.splitlines() if line.strip()), "")
            result.append((int(match.group(1)), title or match.group(2), short))
        else:
            result.append((None, subject, short))
    return result


def written_notes(repo_dir, previous, sha):
    """The notes file at `sha` when it changed since `previous`, else None."""
    changed = git(repo_dir, "diff", "--name-only", previous, sha, "--", NOTES_FILE).strip()
    if not changed:
        return None
    try:
        text = git(repo_dir, "show", f"{sha}:{NOTES_FILE}")
    except subprocess.CalledProcessError:
        return None  # Deleted since the previous release.
    # HTML comments hold the file's instructions for whoever writes the notes.
    text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL).strip()
    return text or None


def render(previous, previous_sha, sha, ref, entries, notes):
    build = previous.removeprefix("build-")
    count = len(entries)
    noun = "change" if count == 1 else "changes"
    out = [
        f"## Changes since build {build}",
        "",
        f"Built from `{ref}` at `{sha[:10]}`. Build {build} was `{previous_sha[:7]}`. "
        f"That's {count} merged {noun}.",
        "",
    ]
    if notes:
        out += [notes, ""]
    if entries:
        out += ["### Everything merged", ""]
        for number, title, short in entries:
            out.append(f"- {title} (#{number})" if number is not None else f"- {title} (`{short}`)")
        out.append("")
    return "\n".join(out)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--repo-dir", default=".")
    parser.add_argument("--previous", default="", help="Tag of the previous release, or empty")
    parser.add_argument("--sha", required=True)
    parser.add_argument("--ref", required=True)
    args = parser.parse_args(argv)

    if not args.previous:
        return 0
    try:
        previous_sha = git(args.repo_dir, "rev-parse", f"{args.previous}^{{commit}}").strip()
    except subprocess.CalledProcessError:
        print(f"release_changes: tag {args.previous} not found, writing no changes", file=sys.stderr)
        return 0
    entries = changes(args.repo_dir, previous_sha, args.sha)
    notes = written_notes(args.repo_dir, previous_sha, args.sha)
    sys.stdout.write(render(args.previous, previous_sha, args.sha, args.ref, entries, notes))
    return 0


if __name__ == "__main__":
    sys.exit(main())
