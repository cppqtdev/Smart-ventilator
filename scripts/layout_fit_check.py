#!/usr/bin/env python3
"""Flags a row whose children cannot fit the width its parent hands it.

Qt Quick Layouts honour Layout.minimumWidth over Layout.preferredWidth. When
the minimums of a row add up to more than the row is given, the children do
not shrink - they overflow, and whatever is drawn later covers whatever is
drawn earlier. Three separate overlaps on this device were that bug: the rail
dial one pixel wider than the rail, the Tools chips pushing under the loop
chart, and the layout picker growing a row past its fixed height.

The rule is deliberately narrow, because a general solution needs the whole
layout tree at runtime. It catches the case that has actually bitten: a
RowLayout with a fixed width whose children declare minimums in the same
file.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP = {"_legacy", "_to_delete", "build", ".git", "node_modules"}

PX = re.compile(r"Metrics\.px\(\s*(\d+)\s*\)")
FIXED_WIDTH = re.compile(r"Layout\.(?:preferredWidth|maximumWidth)\s*:\s*Metrics\.px\(\s*(\d+)\s*\)")
MIN_WIDTH = re.compile(r"Layout\.minimumWidth\s*:\s*(.+)$")
SPACING = re.compile(r"spacing\s*:\s*(?:Metrics\.px\(\s*(\d+)\s*\)|Metrics\.gutter|Spacing\.(\w+))")

SPACING_TOKENS = {"xxs": 2, "xs": 4, "sm": 8, "md": 12, "lg": 16, "xl": 24}
GUTTER = 16


def indent_of(line: str) -> int:
    return len(line) - len(line.lstrip())


def main() -> int:
    problems = []

    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in files:
            if not name.endswith(".qml"):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT)
            lines = open(path, encoding="utf-8").read().split("\n")

            for index, line in enumerate(lines):
                if "RowLayout {" not in line:
                    continue

                open_indent = indent_of(line)
                width = None
                spacing = 0
                minimums = []
                depth = 0

                for follow in lines[index + 1:]:
                    stripped = follow.strip()
                    if not stripped:
                        continue
                    if indent_of(follow) <= open_indent and stripped.startswith("}"):
                        break

                    match = FIXED_WIDTH.search(follow)
                    if match and indent_of(follow) == open_indent + 4:
                        width = int(match.group(1))

                    match = SPACING.search(follow)
                    if match and indent_of(follow) == open_indent + 4:
                        if match.group(1):
                            spacing = int(match.group(1))
                        elif match.group(2):
                            spacing = SPACING_TOKENS.get(match.group(2), 0)
                        else:
                            spacing = GUTTER

                    match = MIN_WIDTH.search(follow)
                    if match:
                        value = PX.search(match.group(1))
                        if value:
                            minimums.append(int(value.group(1)))

                    depth += follow.count("{") - follow.count("}")
                    if depth < 0:
                        break

                if width is None or len(minimums) < 2:
                    continue

                needed = sum(minimums) + spacing * (len(minimums) - 1)
                if needed > width:
                    problems.append(
                        "%s:%d: a row fixed at %d holds children whose minimum widths "
                        "come to %d - layouts honour the minimum, so they will overflow "
                        "rather than shrink"
                        % (rel, index + 1, width, needed))

    for problem in problems:
        print(f"  {problem}")
    print(f"{len(problems)} row(s) that cannot fit")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
