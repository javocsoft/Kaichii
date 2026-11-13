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
# Issues closed with these, or carrying these labels, are not reports to list.
SKIPPED_LABELS = {"duplicate", "invalid", "spam"}
CELL_LIMIT = 80
REPOSITORY = re.compile(r"^[A-Za-z0-9-]+/[A-Za-z0-9._-]+$")

TEXT = {
    "en": {
        "title": "Compatibility list",
        "intro": "How games run in Kaichii, as players report them. Each row is one report, newest "
                 "first for each game. Results depend on the computer: check the graphics card "
                 "and the build.",
        "send": "Played a game? [Send a report](CompatibilityReports.md).",
        "count": "{n} reports",
        "none": "No reports yet: yours can be the first.",
        "unavailable": "The reports couldn't be loaded when this page was built.",
        "browse": "Browse them on GitHub",
        "columns": ("Game", "Rating", "Game version", "Kaichii build", "Graphics card", "Date",
                    "Report"),
        "ratings": {r: r for r in RATINGS},
        "unknown": "Unknown",
    },
    "es": {
        "title": "Lista de compatibilidad",
        "intro": "Cómo funcionan los juegos en Kaichii, según los informes de los jugadores. Cada "
                 "fila es un informe, los más recientes primero en cada juego. El resultado "
                 "depende del equipo: fíjate en la tarjeta gráfica y la versión.",
        "send": "¿Has jugado a algo? [Envía un informe](CompatibilityReports.es.md).",
        "count": "{n} informes",
        "none": "Todavía no hay informes: el tuyo puede ser el primero.",
        "unavailable": "No se pudieron cargar los informes al construir esta página.",
        "browse": "Consúltalos en GitHub",
        "columns": ("Juego", "Valoración", "Versión del juego", "Versión de Kaichii",
                    "Tarjeta gráfica", "Fecha", "Informe"),
        "ratings": {"Perfect": "Perfecto", "Playable": "Jugable", "Runs": "Funciona",
                    "Loads": "Carga", "Unplayable": "Injugable"},
        "unknown": "Desconocido",
    },
}


def clean(value: str) -> str:
    """A report value on one line, unescaped from its Markdown table cell, and kept short."""
    value = value.replace("\\|", "|").replace("<br>", " ")
    value = " ".join(value.split())
    if value in ("Unknown", "Not given"):
        return ""
    return value if len(value) <= CELL_LIMIT else value[:CELL_LIMIT - 1] + "…"


def table_cells(line: str) -> list:
    """The cells of a Markdown table row, splitting only on unescaped pipes."""
    inner = line.strip()
    if not (inner.startswith("|") and inner.endswith("|")):
        return []
    return [cell.strip() for cell in re.split(r"(?<!\\)\|", inner[1:-1])]


def parse_report(title: str, body: str) -> dict:
    """What a report says: its rating, game and system. Hand-written reports give what they can."""
    body = body or ""
    report = {"rating": "", "game": "", "version": "", "build": "", "gpu": ""}
    match = re.search(r"\*\*Compatibility:\*\*\s*(" + "|".join(RATINGS) + r")\b", body)
    if match:
        report["rating"] = match.group(1)
    rows = {"Game": "game", "Game version": "version", "Kaichii build": "build", "GPU": "gpu"}
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
    # By game, newest first within each (both sorts are stable).
    rows.sort(key=lambda row: row[2], reverse=True)
    rows.sort(key=lambda row: row[0]["game"].casefold())

    counts = {rating: 0 for rating in RATINGS}
    for report, _, _ in rows:
        if report["rating"] in counts:
            counts[report["rating"]] += 1
    escape = html.escape
    lines.append('<div class="kaichii-ratings">')
    lines.append(f'<span>{escape(text["count"].format(n=len(rows)))}</span>')
    for rating in RATINGS:
        if counts[rating]:
            lines.append(f'<span class="kaichii-rating kaichii-rating--{rating.lower()}">'
                         f'{escape(text["ratings"][rating])}: {counts[rating]}</span>')
    lines += ["</div>", "", '<table class="kaichii-compatibility">', "<thead><tr>"]
    lines += [f"<th>{escape(column)}</th>" for column in text["columns"]]
    lines += ["</tr></thead>", "<tbody>"]
    unknown = escape(text["unknown"])
    for report, number, date in rows:
        rating = report["rating"]
        badge = (f'<span class="kaichii-rating kaichii-rating--{rating.lower()}">'
                 f'{escape(text["ratings"][rating])}</span>' if rating
                 else f'<span class="kaichii-rating kaichii-rating--unknown">{unknown}</span>')
        cells = [escape(report["game"]) or unknown, badge,
                 escape(report["version"]) or unknown, escape(report["build"]) or unknown,
                 escape(report["gpu"]) or unknown, escape(date),
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
