#!/usr/bin/env python3
"""Refuse a size the screen scale never reaches.

Every length on this device goes through Metrics.px, so the whole interface
grows and shrinks together with the panel it is drawn on. A raw number does
not: it stays the same while everything around it moves, which is how a
button ends up the wrong size on a screen nobody tested on.

Nine of them were left - 64, 24, 260, 16, 58, 52, 42 - in the emergency
screen, the shutdown screen and the loop chart, all of which are reachable.

A one or two digit number that is not a length is fine and common:
border.width, z, opacity, a duration, a loop count. Only the properties
below are checked, and only where the value is ten or more, which is where a
raw length stops being plausible as a hairline.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UI = ROOT / "ui"

LENGTHS = (
    "width", "height",
    "implicitWidth", "implicitHeight",
    "Layout.preferredWidth", "Layout.preferredHeight",
    "Layout.minimumWidth", "Layout.minimumHeight",
    "Layout.maximumWidth", "Layout.maximumHeight",
    "spacing", "rowSpacing", "columnSpacing",
    "anchors.margins", "anchors.leftMargin", "anchors.rightMargin",
    "anchors.topMargin", "anchors.bottomMargin",
    "Layout.topMargin", "Layout.bottomMargin",
    "Layout.leftMargin", "Layout.rightMargin",
)

# Anchored to the start of the line: "height: 170" is a property, while
# "value: ... ? ... .height : 170" is the tail of a conditional and not one.
RAW = re.compile(
    r"^\s*(%s)\s*:\s*(\d{2,})\s*$" % "|".join(re.escape(name) for name in LENGTHS)
)


def main():
    problems = []

    for path in sorted(UI.rglob("*.qml")):
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            match = RAW.search(line.split("//")[0])
            if not match:
                continue
            problems.append(
                "%s:%d: %s is %s, a length the screen scale never reaches - "
                "wrap it in Metrics.px() or use a Spacing token"
                % (path.relative_to(ROOT).as_posix(), number,
                   match.group(1), match.group(2))
            )

    for line in problems:
        print(line)
    print("%d length(s) that ignore the screen scale" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
