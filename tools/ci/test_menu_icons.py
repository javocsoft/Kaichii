#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""The menu icons (dist/icons/menu, src/yuzu/util/menu_icon.cpp) stay usable: every icon the frontend
asks for exists and is in the resource file, and each is drawn in the one colour MenuIcon replaces.
A missing one shows no icon, without an error, so nothing else would notice."""

import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
ICONS = ROOT / "dist" / "icons" / "menu"
SOURCE_COLOR = "#b4b6bc"


def requested_names():
    names = set()
    for path in (ROOT / "src" / "yuzu").rglob("*.cpp"):
        text = path.read_text(encoding="utf-8", errors="replace")
        names.update(re.findall(r'MenuIcon\("([a-z_]+)"\)', text))
        # UpdateMenuIcons' table: {ui->action, "name"}.
        names.update(re.findall(r'\{ui->[A-Za-z0-9_>()\-]+,\s*"([a-z_]+)"\}', text))
    return names


class MenuIcons(unittest.TestCase):
    def test_every_requested_icon_exists(self):
        names = requested_names()
        self.assertTrue(names, "found no MenuIcon use: the patterns no longer match the code")
        missing = sorted(name for name in names if not (ICONS / f"{name}.svg").is_file())
        self.assertEqual(missing, [])

    def test_the_resource_file_lists_exactly_the_icons(self):
        qrc = (ICONS / "menu.qrc").read_text(encoding="utf-8")
        listed = set(re.findall(r"<file>([^<]+\.svg)</file>", qrc))
        present = {path.name for path in ICONS.glob("*.svg")}
        self.assertEqual(listed, present)
        self.assertIn('prefix="menu_icons"', qrc)

    def test_every_icon_uses_the_colour_menu_icon_replaces(self):
        for path in ICONS.glob("*.svg"):
            text = path.read_text(encoding="utf-8").lower()
            colors = set(re.findall(r"#[0-9a-f]{6}", text))
            self.assertEqual(colors, {SOURCE_COLOR}, path.name)


if __name__ == "__main__":
    unittest.main()
