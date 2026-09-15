#!/usr/bin/env python3
"""Flags qBound/std::clamp calls whose bounds are computed, not literal.

qBound(min, value, max) asserts in a debug build when max < min, and
std::clamp is undefined behaviour in the same case. With literal bounds that
can never happen. With bounds that come from two separate expressions it can,
and it did: a patient category paired with a body weight from a different
category produced a floor above the ceiling and took the application down on
launch.

A computed pair is allowed once it is visibly ordered - wrapped in
qMin/qMax/std::min/std::max on the line, or named so the ordering is explicit.
"""
from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP = {"_legacy", "build", ".git", "node_modules", "tests"}

CALL = re.compile(r"\b(qBound|std::clamp)\s*(?:<[^>]*>)?\s*\(")

# qBound<qint64>(0, value, 100) does not compile: the explicit argument fixes
# one parameter type while the literals deduce another, and both two-type
# overloads then match. Either drop the explicit type or make every argument
# that type.
EXPLICIT = re.compile(r"\bqBound\s*<[^>]+>\s*\(")
LITERAL = re.compile(r"^-?(?:\d+\.?\d*[fu]?|0x[0-9A-Fa-f]+)$")
ORDERED = re.compile(r"\b(qMin|qMax|std::min|std::max)\s*(?:<[^>]*>)?\s*\(")


def split_arguments(text: str) -> list[str]:
    args, depth, current = [], 0, ""
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            if depth == 0:
                break
            depth -= 1
        if ch == "," and depth == 0:
            args.append(current.strip())
            current = ""
            continue
        current += ch
    if current.strip():
        args.append(current.strip())
    return args


def bounds_of(name: str, args: list[str]) -> tuple[str, str] | None:
    if len(args) != 3:
        return None
    # qBound(min, value, max); std::clamp(value, min, max)
    return (args[0], args[2]) if name == "qBound" else (args[1], args[2])


def main() -> int:
    problems = []
    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in files:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(base, name)
            for line_no, line in enumerate(
                    open(path, encoding="utf-8").read().split("\n"), 1):
                if line.strip().startswith("//"):
                    continue
                for match in EXPLICIT.finditer(line):
                    args = split_arguments(line[match.end():])
                    if len(args) >= 3 and (LITERAL.match(args[0])
                                           or LITERAL.match(args[2])):
                        problems.append(
                            "%s:%d: qBound with an explicit type and a plain "
                            "literal bound is an ambiguous overload - drop the "
                            "explicit type or cast every argument"
                            % (os.path.relpath(path, ROOT), line_no))

                for match in CALL.finditer(line):
                    args = split_arguments(line[match.end():])
                    pair = bounds_of(match.group(1), args)
                    if pair is None:
                        continue
                    low, high = pair
                    if LITERAL.match(low) and LITERAL.match(high):
                        continue
                    if ORDERED.search(low) or ORDERED.search(high):
                        continue
                    # A literal floor against an ordered ceiling is already
                    # safe; the ordering is visible on the ceiling.
                    if LITERAL.match(low) and ORDERED.search(high):
                        continue
                    problems.append(
                        "%s:%d: %s bounds '%s' and '%s' are computed - order "
                        "them with qMin/qMax or the range can invert"
                        % (os.path.relpath(path, ROOT), line_no,
                           match.group(1), low, high))

    for line in problems:
        print(line)
    print(f"\n{len(problems)} unguarded clamp(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
