#!/usr/bin/env python3
"""Compare the layout clamp in AppSettings against HomeLayouts.

A layout id the settings clamp away never reaches the picker, so the old
selection stays lit next to the one that was pressed. The two numbers live in
different languages, so nothing else catches them drifting apart.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def main() -> int:
    layouts = (ROOT / "ui" / "Screens" / "Home" / "HomeLayouts.qml").read_text()
    settings = (ROOT / "src" / "core" / "AppSettings.cpp").read_text()

    ids = [int(value) for value in re.findall(r"^\s*id:\s*(\d+),", layouts, re.M)]
    match = re.search(r"kMonitoringLayoutCount\s*=\s*(\d+)", settings)

    problems = []
    if not ids:
        problems.append("HomeLayouts.qml declares no layout ids")
    if match is None:
        problems.append("AppSettings.cpp has no kMonitoringLayoutCount")

    if not problems:
        declared = int(match.group(1))
        highest = max(ids)
        if declared != highest:
            problems.append(
                f"AppSettings clamps layout ids to {declared} but HomeLayouts "
                f"offers {len(ids)}, the highest being {highest}")
        expected = list(range(1, len(ids) + 1))
        if sorted(ids) != expected:
            problems.append(f"HomeLayouts ids are {sorted(ids)}, expected {expected}")

    for problem in problems:
        print(f"  {problem}")
    print(f"{len(problems)} layout count problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
