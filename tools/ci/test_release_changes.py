#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

import contextlib
import io
import pathlib
import subprocess
import tempfile
import unittest

import release_changes


class ReleaseChanges(unittest.TestCase):
    """Runs against a small git repository shaped like nx2: pull requests merged with merge commits."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.git("init", "-q", "-b", "nx2")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "test")
        self.git("config", "commit.gpgsign", "false")
        self.write("README.md", "seed\n")
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "seed")
        self.git("tag", "build-36")

    def git(self, *args):
        return subprocess.run(
            ["git", *args], cwd=self.root, check=True, capture_output=True, text=True
        ).stdout.strip()

    def write(self, path, content):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")

    def merge_pull_request(self, number, branch, title, path):
        self.git("switch", "-q", "-c", branch)
        self.write(path, f"{title}\n")
        self.git("add", "-A")
        self.git("commit", "-q", "-m", f"{title} (commit)")
        self.git("switch", "-q", "nx2")
        self.git("merge", "-q", "--no-ff", branch, "-m",
                 f"Merge pull request #{number} from someone/{branch}\n\n{title}")

    def run_script(self, previous="build-36"):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = release_changes.main(["--repo-dir", str(self.root), "--previous", previous,
                                         "--sha", self.git("rev-parse", "HEAD"), "--ref", "nx2"])
        self.assertEqual(code, 0)
        return out.getvalue()

    def test_lists_merged_pull_requests_and_direct_commits_oldest_first(self):
        self.merge_pull_request(70, "fix/a", "kernel: fix the boot abort", "a.txt")
        self.write("b.txt", "direct\n")
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "docs: a commit pushed to nx2")
        self.merge_pull_request(71, "fix/b", "tests: reserve the range", "c.txt")

        notes = self.run_script()

        self.assertIn("## Changes since build 36", notes)
        self.assertIn("That's 3 merged changes.", notes)
        self.assertIn(f"Build 36 was `{self.git('rev-parse', '--short=7', 'build-36')}`", notes)
        lines = [line for line in notes.splitlines() if line.startswith("- ")]
        self.assertEqual(lines[0], "- kernel: fix the boot abort (#70)")
        self.assertRegex(lines[1], r"^- docs: a commit pushed to nx2 \(`[0-9a-f]+`\)$")
        self.assertEqual(lines[2], "- tests: reserve the range (#71)")
        # The pull requests' own commits are not listed: only what nx2 merged.
        self.assertNotIn("(commit)", notes)

    def test_includes_written_notes_only_when_they_changed_since_the_previous_release(self):
        self.write(".github/release-notes.md", "<!-- how to write these -->\n### Big fixes\n- Old\n")
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "notes for build 36")
        self.git("tag", "-f", "build-36")
        self.merge_pull_request(72, "fix/c", "a fix", "d.txt")

        self.assertNotIn("Big fixes", self.run_script())

        self.write(".github/release-notes.md", "<!-- how to write these -->\n### Big fixes\n- New\n")
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "notes for build 37")
        notes = self.run_script()
        self.assertIn("### Big fixes\n- New", notes)
        self.assertNotIn("how to write these", notes)
        # The written notes come before the full list.
        self.assertLess(notes.index("### Big fixes"), notes.index("### Everything merged"))

    def test_a_single_change_is_singular(self):
        self.merge_pull_request(70, "fix/a", "one fix", "a.txt")
        self.assertIn("That's 1 merged change.", self.run_script())

    def test_writes_nothing_without_a_previous_release(self):
        self.merge_pull_request(70, "fix/a", "one fix", "a.txt")
        self.assertEqual(self.run_script(previous=""), "")

    def test_writes_nothing_when_the_previous_tag_is_missing(self):
        self.merge_pull_request(70, "fix/a", "one fix", "a.txt")
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(self.run_script(previous="build-999"), "")


if __name__ == "__main__":
    unittest.main()
