#!/usr/bin/env python3
"""Check that every header carrying Q_OBJECT is a source of its CMake target.

The meta-object compiler only runs over files CMake knows about. A header
that declares Q_OBJECT and is not a source of its target is never processed,
so the class it declares has no static meta-object and no virtual table. That
is a *link* failure, and only in whatever links that library - the whole
project compiles cleanly first.

src/transport shipped in that state: five Q_OBJECT headers, none of them a
source, an empty mocs_compilation.cpp, and a link failure that only appeared
when a test finally pulled the library in.

A module passes if its CMakeLists either globs its headers in or names each
one.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

GLOBS_HEADERS = re.compile(r"file\s*\(\s*GLOB[^)]*\.h\s*\)", re.S | re.I)


def main():
    problems = []

    for module in sorted(p for p in SRC.iterdir() if p.is_dir()):
        lists = module / "CMakeLists.txt"
        if not lists.is_file():
            continue

        # Comments are stripped first: a header named only in a comment
        # explaining why it is absent is still absent.
        text = "\n".join(
            line.split("#", 1)[0]
            for line in lists.read_text(encoding="utf-8").splitlines()
        )
        if GLOBS_HEADERS.search(text):
            continue

        for header in sorted(module.rglob("*.h")):
            if "Q_OBJECT" not in header.read_text(encoding="utf-8", errors="ignore"):
                continue
            if header.name not in text:
                problems.append(
                    "%s declares Q_OBJECT and is not a source of %s"
                    % (header.relative_to(ROOT), lists.relative_to(ROOT))
                )

    for line in problems:
        print(line)
    print("%d header(s) the meta-object compiler would never see" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
