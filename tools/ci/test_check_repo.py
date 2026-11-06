#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

import pathlib
import subprocess
import tempfile
import unittest
from unittest import mock

import check_repo


class CheckRepo(unittest.TestCase):
    """Each check runs against a small git repository, so a check that stops finding anything fails."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        patcher = mock.patch.object(check_repo, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_git("init", "-q", "-b", "nx2")
        self.run_git("config", "user.email", "test@example.invalid")
        self.run_git("config", "user.name", "test")
        self.write("README.md", "seed\n")
        self.base = self.commit()

    def run_git(self, *args):
        return subprocess.run(
            ["git", *args], cwd=self.root, check=True, capture_output=True, text=True
        ).stdout.strip()

    def write(self, path, content):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")

    def commit(self):
        self.run_git("add", "-A")
        self.run_git("commit", "-q", "-m", "change", "--allow-empty")
        return self.run_git("rev-parse", "HEAD")

    def test_unregistered_test_is_reported(self):
        self.write("src/tests/CMakeLists.txt", "add_executable(tests common/listed.cpp)\n")
        self.write("src/tests/common/listed.cpp", "")
        self.write("src/tests/common/forgotten.cpp", "")
        self.commit()
        problems = check_repo.check_tests_registered()
        self.assertEqual(len(problems), 1)
        self.assertIn("forgotten.cpp", problems[0])

    def test_doc_links(self):
        self.write("docs/guide.md", "[ok](other.md) [root](/docs/other.md) [web](https://a.b) "
                                    "[anchor](#top) [bad](missing.md)\n")
        self.write("docs/other.md", "")
        self.commit()
        problems = check_repo.check_doc_links()
        self.assertEqual(problems, ["docs/guide.md:1: broken link missing.md"])

    def test_env_flags_must_match_the_catalog_both_ways(self):
        self.write("src/a.cpp", 'getenv("EDEN_NX2_NEW"); getenv("EDEN_NX2_SHARED"); // EDEN_NX2_TC_*\n')
        self.write("docs/ai/env-flags.md", "| `EDEN_NX2_SHARED` | diag | x |\n"
                                           "| `EDEN_NX2_GONE` | diag | x |\n")
        self.commit()
        problems = check_repo.check_env_flags()
        self.assertEqual(len(problems), 2)
        self.assertIn("EDEN_NX2_GONE", problems[0])
        self.assertIn("EDEN_NX2_NEW", problems[1])

    def test_env_flags_outside_the_nx2_families_count_only_when_quoted(self):
        self.write("src/a.h", '#define EDEN_DIR "eden"\ngetenv("EGLANTINE_OTHER");\n')
        self.write("docs/ai/env-flags.md", "\n")
        self.commit()
        problems = check_repo.check_env_flags()
        self.assertEqual(len(problems), 1)
        self.assertIn("EGLANTINE_OTHER", problems[0])

    def test_env_flags_in_tests_do_not_need_documenting(self):
        self.write("src/tests/a.cpp", 'setenv("EDEN_NX2_TEST_ONLY")\n')
        self.write("docs/ai/env-flags.md", "\n")
        self.commit()
        self.assertEqual(check_repo.check_env_flags(), [])

    def test_agent_doc_paths(self):
        self.write("src/real.cpp", "")
        self.write("AGENTS.md", "`src/real.cpp:12` `src/real.cpp` `src/gone.cpp` `src/<game>.cpp`\n"
                                "`tools/ci/check.sh --flag` `not/a/path`\n")
        self.commit()
        problems = check_repo.check_agent_doc_paths()
        self.assertEqual(problems, ["AGENTS.md:1: src/gone.cpp does not exist"])

    def test_ci_doc_must_match_workflows(self):
        self.write(".github/workflows/real.yml", "")
        self.write(".github/workflows/hidden.yml", "")
        self.write("docs/CI.md", "real.yml and imaginary.yml\n")
        self.commit()
        problems = check_repo.check_ci_doc()
        self.assertEqual(len(problems), 2)
        self.assertIn("imaginary.yml", problems[0])
        self.assertIn("hidden.yml", problems[1])

    def test_whitespace_checks_only_the_diff(self):
        self.write("src/old.cpp", "int a; \n")
        base = self.commit()
        self.assertEqual(check_repo.check_whitespace(base), [])
        self.write("src/new.cpp", "int b; \nint\tc;\n")
        self.commit()
        problems = check_repo.check_whitespace(base)
        self.assertTrue(any("trailing whitespace" in problem for problem in problems))
        self.assertTrue(any("contains a tab" in problem for problem in problems))
        self.assertFalse(any("old.cpp" in problem for problem in problems))

    def test_spdx_checks_only_changed_sources(self):
        self.write("src/old.cpp", "int a;\n")
        base = self.commit()
        self.write("src/good.cpp", "// SPDX-License-Identifier: GPL-3.0-or-later\n")
        self.write("src/bad.py", "print()\n")
        self.write("docs/note.md", "no header needed\n")
        self.write("externals/vendored.cpp", "int v;\n")
        self.commit()
        self.assertEqual(check_repo.check_spdx(base),
                         ["src/bad.py: no SPDX-License-Identifier in its first five lines"])


if __name__ == "__main__":
    unittest.main()
