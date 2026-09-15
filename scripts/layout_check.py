#!/usr/bin/env python3
"""Flags layout items whose preferred size will be silently ignored.

Layout.fillWidth defaults to FALSE for a plain item and TRUE for a layout
item (ColumnLayout, RowLayout, GridLayout...). So a ColumnLayout nested in a
RowLayout with Layout.preferredWidth set still stretches, absorbs the row's
spare space, and squeezes its siblings to nothing - with no warning. That is
how the metric sidebar grew to fill the window and left the centre column a
few pixels wide.

Setting the fill flag explicitly is the fix, and saying so at every such site
is what keeps the layout readable.
"""
from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI = os.path.join(ROOT, "ui")
SKIP = {"_legacy", "__pycache__"}

LAYOUT_TYPES = ("ColumnLayout", "RowLayout", "GridLayout", "StackLayout")
POSITIONERS = ("Column", "Row", "Grid", "Flow")
BANNED_IN_COLUMN = ("anchors.fill", "anchors.centerIn", "anchors.top",
                    "anchors.bottom", "anchors.verticalCenter")
BANNED_IN_ROW = ("anchors.fill", "anchors.centerIn", "anchors.left",
                 "anchors.right", "anchors.horizontalCenter")
POSITIONER_OPENS = re.compile(r"^(\s*)(%s)\s*\{" % "|".join(POSITIONERS))
OPENS = re.compile(r"^(\s*)(%s)\s*\{" % "|".join(LAYOUT_TYPES))

PAIRS = (("preferredWidth", "fillWidth"), ("preferredHeight", "fillHeight"))


def main() -> int:
    problems = []
    for base, dirs, files in os.walk(UI):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in sorted(files):
            if not name.endswith(".qml"):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT)
            lines = open(path, encoding="utf-8").read().split("\n")
            for i, line in enumerate(lines):
                match = OPENS.match(line)
                if not match:
                    continue
                indent = len(match.group(1))
                depth, block = 1, []
                for j in range(i + 1, len(lines)):
                    depth += lines[j].count("{") - lines[j].count("}")
                    block.append(lines[j])
                    if depth <= 0:
                        break
                own = "\n".join(l for l in block
                                if len(l) - len(l.lstrip()) == indent + 4)
                for axis, fill in PAIRS:
                    if f"Layout.{axis}" in own and f"Layout.{fill}" not in own:
                        problems.append(
                            "%s:%d: %s sets Layout.%s but not Layout.%s, which "
                            "defaults to true for a layout and overrides it"
                            % (rel, i + 1, match.group(2), axis, fill))

    # A positioner lays its children out itself, so a child that anchors to
    # it is refused at runtime with a warning and the positioner stops
    # working. The warning is easy to miss in a busy log.
    for base, dirs, files in os.walk(UI):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in sorted(files):
            if not name.endswith(".qml"):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT)
            lines = open(path, encoding="utf-8").read().split("\n")
            for i, line in enumerate(lines):
                match = POSITIONER_OPENS.match(line)
                if not match:
                    continue
                kind = match.group(2)
                banned = BANNED_IN_ROW if kind in ("Row",) else BANNED_IN_COLUMN
                indent = len(match.group(1))
                depth, block = 1, []
                for j in range(i + 1, len(lines)):
                    depth += lines[j].count("{") - lines[j].count("}")
                    block.append((j, lines[j]))
                    if depth <= 0:
                        break
                for line_no, child in block:
                    if len(child) - len(child.lstrip()) != indent + 8:
                        continue
                    for bad in banned:
                        if child.strip().startswith(bad):
                            problems.append(
                                "%s:%d: %s child sets %s - a positioner "
                                "refuses anchored children and stops laying out"
                                % (rel, line_no + 1, kind, bad))

    for line in problems:
        print(line)
    print(f"\n{len(problems)} layout(s) with an ignored preferred size")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
