#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""Writes the guide's compatibility list (Compatibility.md and Compatibility.es.md) from the
repository's compatibility reports: its issues labelled `compatibility`.

  python3 -I tools/site/compatibility.py --repo javocsoft/Kaichii --out docs/user

Reads GITHUB_TOKEN or GH_TOKEN when set. Without them, or without network, the pages say the list
couldn't be loaded and link to the reports. Report text comes from anyone who opens an issue, so
every value is escaped and only links this script builds reach the page.
"""

import argparse
import html
import json
import os
import pathlib
import re
import sys
import urllib.error
import urllib.request

LABEL = "compatibility"
RATINGS = ("Perfect", "Playable", "Runs", "Loads", "Unplayable")
# How smoothly a game that got into play ran on the reporter's computer. Reports from before the
# smoothness question, and games that didn't get into play ("Not applicable"), have none.
SMOOTHNESS = ("Perfect", "Smooth", "Stutters", "Choppy", "Poor")
# "30", "25-30", "~45": the usual frame rate a report gives. Anything else is left out.
FRAME_RATE = re.compile(r"^~?\d{1,3}(?:\.\d)?(?:\s*[-–]\s*\d{1,3}(?:\.\d)?)?$")
# Issues closed with these, or carrying these labels, are not reports to list.
SKIPPED_LABELS = {"duplicate", "invalid", "spam"}
CELL_LIMIT = 80
# Kaichii writes Qt's QSysInfo::prettyProductName(): "Windows 11 Version 24H2", "macOS Sequoia
# (15.1)", or on Linux the distribution's own name ("Pop!_OS 24.04 LTS", "Arch Linux").
FAMILIES = ("Windows", "Linux", "macOS")
MACOS_NAMES = ("macos", "mac os", "os x", "darwin")
LINUX_NAMES = ("linux", "ubuntu", "pop!_os", "debian", "fedora", "arch", "manjaro", "endeavouros",
               "cachyos", "mint", "opensuse", "suse", "steamos", "bazzite", "nobara", "zorin",
               "elementary", "nixos", "gentoo", "void", "garuda", "chimeraos", "solus", "rocky",
               "almalinux", "centos", "red hat", "deepin", "kali", "slackware", "alpine",
               "kubuntu", "xubuntu", "lubuntu", "neon", "tumbleweed", "aurora", "bluefin",
               "pikaos")
REPOSITORY = re.compile(r"^[A-Za-z0-9-]+/[A-Za-z0-9._-]+$")

TEXT = {
    "en": {
        "title": "Compatibility list",
        "intro": "How games run in Kaichii, as players report them. Results depend on the "
                 "computer and its operating system: the same game can run better on Linux than "
                 "on Windows on the same PC, so the reports are grouped by system.",
        "legend": "**Compatibility** says whether a game works: its graphics, sound and progress. "
                  "**Smoothness** says how fluid it felt on that computer, with the usual frame "
                  "rate when the player gave it.",
        "send": "Played a game? [Send a report](CompatibilityReports.md).",
        "count": "{n} reports",
        "none": "No reports yet: yours can be the first.",
        "unavailable": "The reports couldn't be loaded when this page was built.",
        "browse": "Browse them on GitHub",
        "columns": ("Game", "Compatibility", "Smoothness", "Game version", "Kaichii build",
                    "Operating system", "Graphics card", "Date", "Report"),
        "ratings": {r: r for r in RATINGS},
        "smoothness": {"Perfect": "Perfect", "Smooth": "Smooth", "Stutters": "Occasional stutters",
                       "Choppy": "Choppy", "Poor": "Very poor"},
        "unknown": "Unknown",
        "by_system": "By operating system",
        "by_system_intro": "The newest report for each game on each system: its compatibility, "
                           "its smoothness and how many reports there are. Select the "
                           "compatibility to open that report.",
        "game": "Game",
        "no_report": "No report",
        "families": {"Windows": "Windows", "Linux": "Linux", "macOS": "macOS",
                     "": "Unknown system"},
    },
    "es": {
        "title": "Lista de compatibilidad",
        "intro": "Cómo funcionan los juegos en Kaichii, según los informes de los jugadores. El "
                 "resultado depende del equipo y de su sistema operativo: un mismo juego puede ir "
                 "mejor en Linux que en Windows en el mismo PC, así que los informes se agrupan "
                 "por sistema.",
        "legend": "La **compatibilidad** dice si un juego funciona: sus gráficos, su sonido y si se "
                  "puede avanzar. La **fluidez** dice cómo de fluido se sintió en ese equipo, con "
                  "los FPS habituales cuando el jugador los indica.",
        "send": "¿Has jugado a algo? [Envía un informe](CompatibilityReports.es.md).",
        "count": "{n} informes",
        "none": ("Todavía no hay informes: el tuyo puede ser "  # codespell:ignore ser
                 "el primero."),
        "unavailable": "No se pudieron cargar los informes al construir esta página.",
        "browse": "Consúltalos en GitHub",
        "columns": ("Juego", "Compatibilidad", "Fluidez", "Versión del juego",
                    "Versión de Kaichii", "Sistema operativo", "Tarjeta gráfica", "Fecha",
                    "Informe"),
        "ratings": {"Perfect": "Perfecto", "Playable": "Jugable", "Runs": "Funciona",
                    "Loads": "Carga", "Unplayable": "Injugable"},
        "smoothness": {"Perfect": "Perfecta", "Smooth": "Fluida", "Stutters": "Tirones puntuales",
                       "Choppy": "Poco fluida", "Poor": "Muy mala"},
        "unknown": "Desconocido",
        "by_system": "Por sistema operativo",
        "by_system_intro": "El informe más reciente de cada juego en cada sistema: su "
                           "compatibilidad, su fluidez y cuántos informes hay. Pulsa la "
                           "compatibilidad para abrir ese informe.",
        "game": "Juego",
        "no_report": "Sin informes",
        "families": {"Windows": "Windows", "Linux": "Linux", "macOS": "macOS",
                     "": "Sistema desconocido"},
    },
}


def clean(value: str) -> str:
    """A report value on one line, unescaped from its Markdown table cell, and kept short."""
    value = value.replace("\\|", "|").replace("<br>", " ")
    value = " ".join(value.split())
    if value in ("Unknown", "Not given"):
        return ""
    return value if len(value) <= CELL_LIMIT else value[:CELL_LIMIT - 1] + "…"


def os_family(name: str) -> str:
    """Windows, Linux or macOS for an operating system's name, or "" when it can't tell."""
    lowered = name.casefold()
    if "windows" in lowered:
        return "Windows"
    if any(word in lowered for word in MACOS_NAMES):
        return "macOS"
    if any(word in lowered for word in LINUX_NAMES):
        return "Linux"
    return ""


def table_cells(line: str) -> list:
    """The cells of a Markdown table row, splitting only on unescaped pipes."""
    inner = line.strip()
    if not (inner.startswith("|") and inner.endswith("|")):
        return []
    return [cell.strip() for cell in re.split(r"(?<!\\)\|", inner[1:-1])]


def parse_report(title: str, body: str) -> dict:
    """What a report says: its rating, game and system. Hand-written reports give what they can."""
    body = body or ""
    report = {"rating": "", "game": "", "version": "", "build": "", "gpu": "", "os": "",
              "program": "", "smoothness": "", "fps": ""}
    match = re.search(r"\*\*Compatibility:\*\*\s*(" + "|".join(RATINGS) + r")\b", body)
    if match:
        report["rating"] = match.group(1)
    match = re.search(r"\*\*Smoothness:\*\*\s*(" + "|".join(SMOOTHNESS) + r")\b", body)
    if match:
        report["smoothness"] = match.group(1)
    match = re.search(r"\*\*Frame rate:\*\*[ \t]*([^\n]*?)[ \t]*(?:fps)?[ \t]*$", body,
                      flags=re.MULTILINE)
    if match and FRAME_RATE.match(match.group(1).strip()):
        report["fps"] = match.group(1).strip()
    rows = {"Game": "game", "Game version": "version", "Kaichii build": "build", "GPU": "gpu",
            "Operating system": "os", "Program ID": "program"}
    for line in body.splitlines():
        cells = table_cells(line)
        if len(cells) == 2 and cells[0] in rows and not report[rows[cells[0]]]:
            report[rows[cells[0]]] = clean(cells[1])
    # The title Kaichii writes: "[Compatibility] <game> (<version>): <rating>".
    title_match = re.match(r"^\s*\[Compatibility\]\s*(.*?)\s*(?:\(([^()]*)\))?\s*"
                           r"(?::\s*(" + "|".join(RATINGS) + r"))?\s*$", title or "")
    if title_match:
        report["game"] = report["game"] or clean(title_match.group(1) or "")
        report["version"] = report["version"] or clean(title_match.group(2) or "")
        report["rating"] = report["rating"] or (title_match.group(3) or "")
    if not re.fullmatch(r"[0-9A-Fa-f]{16}", report["program"]):
        report["program"] = ""
    report["program"] = report["program"].upper()
    report["family"] = os_family(report["os"])
    return report


def listed(issue: dict) -> bool:
    """Whether an issue is a report to list: not a pull request, not dismissed."""
    if "pull_request" in issue:
        return False
    if issue.get("state_reason") == "not_planned":
        return False
    labels = {label.get("name", "").lower() for label in issue.get("labels", [])}
    return LABEL in labels and not labels & SKIPPED_LABELS


def fetch_issues(repo: str, token: str) -> list:
    """Every issue labelled `compatibility`, open or closed."""
    issues = []
    for page in range(1, 51):
        url = (f"https://api.github.com/repos/{repo}/issues?labels={LABEL}&state=all"
               f"&per_page=100&page={page}")
        request = urllib.request.Request(url, headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "Kaichii guide",
            **({"Authorization": f"Bearer {token}"} if token else {}),
        })
        with urllib.request.urlopen(request, timeout=30) as response:
            batch = json.load(response)
        issues.extend(batch)
        if len(batch) < 100:
            break
    return issues


def render(repo: str, issues, language: str) -> str:
    """The list page in `language`. `issues` is None when they couldn't be loaded."""
    text = TEXT[language]
    reports_url = f"https://github.com/{repo}/issues?q=label%3A{LABEL}"
    lines = [f"# {text['title']}", "", text["intro"], "", text["send"], ""]
    if issues is None:
        lines += [f"{text['unavailable']} [{text['browse']}]({reports_url}).", ""]
        return "\n".join(lines)
    rows = []
    for issue in issues:
        if not listed(issue):
            continue
        report = parse_report(issue.get("title", ""), issue.get("body", ""))
        number = issue.get("number")
        if not isinstance(number, int):
            continue
        rows.append((report, number, str(issue.get("created_at", ""))[:10]))
    if not rows:
        lines += [text["none"], ""]
        return "\n".join(lines)
    # Newest first, then by game (both sorts are stable).
    rows.sort(key=lambda row: row[2], reverse=True)
    rows.sort(key=lambda row: row[0]["game"].casefold())

    escape = html.escape
    unknown = escape(text["unknown"])

    def badge(rating, number=None):
        name = escape(text["ratings"][rating]) if rating else unknown
        kind = (rating or "unknown").lower()
        span = f'<span class="kaichii-rating kaichii-rating--{kind}">{name}</span>'
        if number is None:
            return span
        return (f'<a class="kaichii-rating-link" '
                f'href="https://github.com/{repo}/issues/{number}">{span}</a>')

    def smooth(report):
        """The smoothness badge and frame rate, or nothing for a report without them."""
        parts = []
        if report["smoothness"]:
            parts.append(f'<span class="kaichii-smooth kaichii-smooth--'
                         f'{report["smoothness"].lower()}">'
                         f'{escape(text["smoothness"][report["smoothness"]])}</span>')
        if report["fps"]:
            parts.append(f'<span class="kaichii-fps">{escape(report["fps"])} fps</span>')
        return " ".join(parts)

    counts = {rating: 0 for rating in RATINGS}
    for report, _, _ in rows:
        if report["rating"] in counts:
            counts[report["rating"]] += 1
    lines.append('<div class="kaichii-ratings">')
    lines.append(f'<span>{escape(text["count"].format(n=len(rows)))}</span>')
    for rating in RATINGS:
        if counts[rating]:
            lines.append(f'<span class="kaichii-rating kaichii-rating--{rating.lower()}">'
                         f'{escape(text["ratings"][rating])}: {counts[rating]}</span>')
    lines += ["</div>", "", text["legend"], ""]

    # One row per game: reports with a program ID are one game whatever their titles say.
    games = {}
    for report, number, _ in rows:
        key = report["program"] or report["game"].casefold()
        game = games.setdefault(key, {"name": report["game"], "newest": {}, "count": {}})
        family = report["family"]
        if family:
            game["count"][family] = game["count"].get(family, 0) + 1
            # Rows are newest first within a game, so the first one seen is the newest.
            game["newest"].setdefault(family, (report, number))
    lines += [f"## {escape(text['by_system'])}", "", escape(text["by_system_intro"]), "",
              '<table class="kaichii-compatibility kaichii-by-system">', "<thead><tr>",
              f"<th>{escape(text['game'])}</th>"]
    lines += [f"<th>{escape(text['families'][family])}</th>" for family in FAMILIES]
    lines += ["</tr></thead>", "<tbody>"]
    for game in sorted(games.values(), key=lambda g: g["name"].casefold()):
        cells = [escape(game["name"]) or unknown]
        for family in FAMILIES:
            if family in game["newest"]:
                report, number = game["newest"][family]
                details = smooth(report)
                cells.append(f'{badge(report["rating"], number)} '
                             f'<span class="kaichii-count">({game["count"][family]})</span>'
                             + (f'<br>{details}' if details else ""))
            else:
                cells.append(f'<span class="kaichii-none">{escape(text["no_report"])}</span>')
        lines.append("<tr>" + "".join(f"<td>{cell}</td>" for cell in cells) + "</tr>")
    lines += ["</tbody>", "</table>", ""]

    # Then every report, one section per system.
    for family in (*FAMILIES, ""):
        section = [row for row in rows if row[0]["family"] == family]
        if not section:
            continue
        lines += [f"## {escape(text['families'][family])}", "",
                  f'<p class="kaichii-section-count">'
                  f'{escape(text["count"].format(n=len(section)))}</p>',
                  "", '<table class="kaichii-compatibility">', "<thead><tr>"]
        lines += [f"<th>{escape(column)}</th>" for column in text["columns"]]
        lines += ["</tr></thead>", "<tbody>"]
        for report, number, date in section:
            cells = [escape(report["game"]) or unknown, badge(report["rating"]),
                     smooth(report) or '<span class="kaichii-none">—</span>',
                     escape(report["version"]) or unknown, escape(report["build"]) or unknown,
                     escape(report["os"]) or unknown, escape(report["gpu"]) or unknown,
                     escape(date),
                     f'<a href="https://github.com/{repo}/issues/{number}">#{number}</a>']
            lines.append("<tr>" + "".join(f"<td>{cell}</td>" for cell in cells) + "</tr>")
        lines += ["</tbody>", "</table>", ""]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--repo", required=True, help="owner/name")
    parser.add_argument("--out", required=True, type=pathlib.Path, help="the guide's folder")
    args = parser.parse_args()
    if not REPOSITORY.match(args.repo):
        print(f"compatibility: not a repository: {args.repo}", file=sys.stderr)
        return 2
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN") or ""
    try:
        issues = fetch_issues(args.repo, token)
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError) as error:
        print(f"compatibility: reports unavailable ({error})", file=sys.stderr)
        issues = None
    (args.out / "Compatibility.md").write_text(render(args.repo, issues, "en"), encoding="utf-8")
    (args.out / "Compatibility.es.md").write_text(render(args.repo, issues, "es"),
                                                 encoding="utf-8")
    listed_count = sum(1 for issue in issues or [] if listed(issue))
    print(f"compatibility: {listed_count} reports listed" if issues is not None
          else "compatibility: wrote the unavailable page")
    return 0


if __name__ == "__main__":
    sys.exit(main())
