#!/usr/bin/env python3
"""Keep every button on one of the three heights.

Nine were in use across the screens - 28, 30, 32, 34, 36, 44, 45, 56 and 64
design pixels - which is why an odd-sized button kept turning up in review:
with nine heights there is no rule for one to be the odd one out from.

Three, named in Metrics:

    buttonHeightSmall      a chip in a dense row
    buttonHeightStandard   an action
    buttonHeightPrimary    one the operator must not miss under pressure

A tab strip keeps navHeight, which the reference measures separately and
reference_geometry.py pins.

This reads the enclosing type from the indentation above the line, which is
enough for QML written the way this project writes it, and it only judges
the button types named below.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UI = ROOT / "ui"

BUTTONS = {"AppButton", "ChipButton", "PrimaryButton", "AppTabButton", "IconButton"}

ALLOWED = {
    "Metrics.buttonHeightSmall",
    "Metrics.buttonHeightStandard",
    "Metrics.buttonHeightPrimary",
    # A tab is as tall as the strip it sits in.
    "Metrics.navHeight",
    # Kept for the sixteen call sites that already read as the action height.
    "Metrics.actionButtonHeight",
}

HEIGHT = re.compile(r"Layout\.preferredHeight:\s*(.+?)\s*$")
OPENS = re.compile(r"\s*([A-Z][A-Za-z]*)\s*\{")


def enclosing(lines, index):
    for i in range(index, -1, -1):
        match = OPENS.match(lines[i])
        if match:
            return match.group(1)
    return ""


def main():
    problems = []

    for path in sorted(UI.rglob("*.qml")):
        lines = path.read_text(encoding="utf-8").splitlines()
        for number, line in enumerate(lines):
            match = HEIGHT.search(line)
            if not match:
                continue
            if enclosing(lines, number) not in BUTTONS:
                continue
            value = match.group(1)
            if value in ALLOWED:
                continue
            problems.append(
                "%s:%d: a button at %s - use buttonHeightSmall, "
                "buttonHeightStandard or buttonHeightPrimary"
                % (path.relative_to(ROOT).as_posix(), number + 1, value)
            )

    for line in problems:
        print(line)
    print("%d button(s) off the height scale" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
