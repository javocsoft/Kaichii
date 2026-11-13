#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""Tests for compatibility.py: reading reports and writing the list. No network."""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import compatibility  # noqa: E402

REPO = "javocsoft/Kaichii"

# What GitHub stores for a report opened from Kaichii's dialog: the form's sections around the
# report Kaichii wrote.
DIALOG_BODY = """### Report

**Compatibility:** Playable

Runs well indoors.

**How far I played**

Kokiri Forest

**System and settings**

| | |
|---|---|
| Game | The Legend of Zelda: Ocarina of Time |
| Program ID | 0400486024DEC000 |
| Game version | 1.1.1 |
| Kaichii build | Kaichii build-92 (80a7b70a72aa) |
| CPU | 12th Gen Intel(R) Core(TM) i7-12700H |
| GPU | NVIDIA GeForce RTX 3050 Laptop GPU \\| 4 GB |
| GPU driver | Unknown |

<sub>Prepared by Kaichii's compatibility report.</sub>

### Before publishing

- [X] My game is legally dumped from my own console
"""


def issue(number, title, body="", labels=("compatibility",), created="2026-11-14T10:00:00Z",
          **extra):
    return {"number": number, "title": title, "body": body, "created_at": created,
            "labels": [{"name": name} for name in labels], **extra}


class ParseReport(unittest.TestCase):
    def test_reads_what_the_dialog_writes(self):
        report = compatibility.parse_report(
            "[Compatibility] The Legend of Zelda: Ocarina of Time (1.1.1): Playable", DIALOG_BODY)
        self.assertEqual(report["rating"], "Playable")
        self.assertEqual(report["game"], "The Legend of Zelda: Ocarina of Time")
        self.assertEqual(report["version"], "1.1.1")
        self.assertEqual(report["build"], "Kaichii build-92 (80a7b70a72aa)")
        self.assertEqual(report["gpu"], "NVIDIA GeForce RTX 3050 Laptop GPU | 4 GB")

    def test_falls_back_to_the_title(self):
        report = compatibility.parse_report("[Compatibility] Mario Kart World (1.2.0): Runs",
                                            "It runs, written by hand.")
        self.assertEqual((report["game"], report["version"], report["rating"]),
                         ("Mario Kart World", "1.2.0", "Runs"))
        self.assertEqual(report["gpu"], "")

    def test_unknown_values_stay_empty_and_long_ones_are_cut(self):
        body = "| GPU | Unknown |\n| Kaichii build | " + "x" * 200 + " |"
        report = compatibility.parse_report("[Compatibility] Game", body)
        self.assertEqual(report["gpu"], "")
        self.assertEqual(len(report["build"]), compatibility.CELL_LIMIT)
        self.assertTrue(report["build"].endswith("…"))

    def test_an_unknown_rating_is_not_taken(self):
        report = compatibility.parse_report("[Compatibility] Game: Great",
                                            "**Compatibility:** Amazing")
        self.assertEqual(report["rating"], "")


class Render(unittest.TestCase):
    def test_lists_reports_by_game_newest_first(self):
        issues = [
            issue(3, "[Compatibility] Zelda (1.0): Runs", created="2026-11-10T00:00:00Z"),
            issue(5, "[Compatibility] Zelda (1.1): Playable", created="2026-11-12T00:00:00Z"),
            issue(4, "[Compatibility] Animal game: Perfect", created="2026-11-11T00:00:00Z"),
        ]
        page = compatibility.render(REPO, issues, "en")
        order = [page.index(f"/issues/{n}\"") for n in (4, 5, 3)]
        self.assertEqual(order, sorted(order))
        self.assertIn("3 reports", page)
        self.assertIn("Playable: 1", page)

    def test_skips_dismissed_issues_and_pull_requests(self):
        issues = [
            issue(1, "[Compatibility] Kept: Playable"),
            issue(2, "[Compatibility] Spam: Perfect", labels=("compatibility", "spam")),
            issue(3, "[Compatibility] Dismissed: Perfect", state_reason="not_planned"),
            issue(4, "[Compatibility] Unlabelled: Perfect", labels=()),
            issue(5, "[Compatibility] A pull request: Perfect", pull_request={}),
        ]
        page = compatibility.render(REPO, issues, "en")
        self.assertIn("/issues/1\"", page)
        for number in (2, 3, 4, 5):
            self.assertNotIn(f"/issues/{number}\"", page)

    def test_escapes_everything_a_report_says(self):
        body = ("**Compatibility:** Perfect\n\n| Game | <script>alert(1)</script> |\n"
                "| GPU | <img src=x onerror=alert(1)> [link](javascript:alert(1)) |")
        page = compatibility.render(REPO, [issue(7, "[Compatibility] x", body)], "en")
        self.assertNotIn("<script", page)
        self.assertNotIn("<img", page)
        self.assertNotIn('href="javascript', page)
        self.assertIn("&lt;script&gt;", page)
        # The only links are the report's own and the page's fixed ones.
        self.assertIn(f'<a href="https://github.com/{REPO}/issues/7">#7</a>', page)

    def test_spanish_names_the_ratings_in_spanish(self):
        page = compatibility.render(REPO, [issue(1, "[Compatibility] Zelda: Unplayable")], "es")
        self.assertIn("# Lista de compatibilidad", page)
        self.assertIn(">Injugable<", page)
        self.assertIn("CompatibilityReports.es.md", page)

    def test_unavailable_and_empty_lists(self):
        reports = f"https://github.com/{REPO}/issues?q=label%3Acompatibility"
        unavailable = compatibility.render(REPO, None, "en")
        self.assertIn(reports, unavailable)
        self.assertNotIn("<table", unavailable)
        empty = compatibility.render(REPO, [], "en")
        self.assertIn("No reports yet", empty)
        self.assertIn("CompatibilityReports.md", empty)
        self.assertNotIn("<table", empty)


if __name__ == "__main__":
    unittest.main()
