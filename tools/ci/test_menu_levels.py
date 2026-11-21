#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""The menu bar's levels (src/yuzu/menu_levels.h) name entries that exist in main.ui. The main window
looks each one up by object name, and a name that matches nothing leaves that entry always shown,
without an error, so nothing else would notice a renamed action."""

import pathlib
import re
import unittest
import xml.etree.ElementTree as ElementTree

ROOT = pathlib.Path(__file__).resolve().parents[2]
LEVELS = ROOT / "src" / "yuzu" / "menu_levels.h"
MAIN_UI = ROOT / "src" / "yuzu" / "main.ui"


def leveled_names():
    text = LEVELS.read_text(encoding="utf-8")
    table = text[text.index("MainMenuLevels"):]
    return re.findall(r'\{"([A-Za-z0-9_]+)", MenuLevel::(\w+)\}', table)


def ui_names():
    root = ElementTree.parse(MAIN_UI).getroot()
    actions = {action.get("name") for action in root.iter("action")}
    menus = {widget.get("name") for widget in root.iter("widget") if widget.get("class") == "QMenu"}
    return actions | menus


class MenuLevels(unittest.TestCase):
    def test_every_leveled_entry_exists(self):
        names = leveled_names()
        self.assertTrue(names, "found no entry: the pattern no longer matches menu_levels.h")
        missing = sorted(name for name, _ in names if name not in ui_names())
        self.assertEqual(missing, [])

    def test_the_table_size_matches_its_entries(self):
        text = LEVELS.read_text(encoding="utf-8")
        size = int(re.search(r"MenuLevel>, (\d+)> MainMenuLevels", text).group(1))
        self.assertEqual(size, len(leveled_names()))

    def test_the_advanced_options_switch_is_always_shown(self):
        # The quick way to turn the advanced options on must not hide itself.
        root = ElementTree.parse(MAIN_UI).getroot()
        emulation = next(widget for widget in root.iter("widget")
                         if widget.get("name") == "menu_Emulation")
        entries = [action.get("name") for action in emulation.findall("addaction")]
        self.assertIn("action_Show_Advanced_Options", entries)
        self.assertNotIn("action_Show_Advanced_Options", [name for name, _ in leveled_names()])
        self.assertNotIn("menu_Emulation", [name for name, _ in leveled_names()])

    def test_no_entry_is_listed_twice(self):
        names = [name for name, _ in leveled_names()]
        self.assertEqual(len(names), len(set(names)))


if __name__ == "__main__":
    unittest.main()
