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


# The actions that ask an AI provider. They share an icon no other action uses, so the AI features
# stand apart in the menus from the ones that don't use AI, such as Optimize for This Game.
AI_ACTIONS = {"action_AI_Tune_Computer", "action_AI_Optimize_Current_Game", "action_Explain_Problem"}


def icon_table():
    """UpdateMenuIcons' table, action name to icon name."""
    text = (ROOT / "src" / "yuzu" / "main_window.cpp").read_text(encoding="utf-8")
    return dict(re.findall(r'\{ui->([A-Za-z0-9_]+),\s*"([a-z_]+)"\}', text))


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

    def test_the_ai_features_have_an_icon_of_their_own(self):
        table = icon_table()
        self.assertTrue(AI_ACTIONS <= table.keys(), sorted(AI_ACTIONS - table.keys()))
        ai_icons = {table[action] for action in AI_ACTIONS}
        self.assertEqual(len(ai_icons), 1, ai_icons)
        others = {action for action, icon in table.items() if icon in ai_icons} - AI_ACTIONS
        self.assertEqual(others, set())

    def test_every_icon_uses_the_colour_menu_icon_replaces(self):
        for path in ICONS.glob("*.svg"):
            text = path.read_text(encoding="utf-8").lower()
            colors = set(re.findall(r"#[0-9a-f]{6}", text))
            self.assertEqual(colors, {SOURCE_COLOR}, path.name)


if __name__ == "__main__":
    unittest.main()
