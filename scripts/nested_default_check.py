#!/usr/bin/env python3
"""Find a nested struct with default member initializers used as a default
argument inside the class that encloses it.

    class Outer {
    public:
        struct Options { bool flag = true; };
        void run(const Options &options = Options{});   // does not compile
    };

The initializers are not complete until the enclosing class is, so the
compiler refuses the default argument. Clang's message names the member
rather than the pattern, which sends you looking at the wrong line. Moving
the struct to namespace scope fixes it.

This has cost the project two build failures. It is cheap to check.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP = {"build", ".git", "_to_delete", "reference", ".claude"}

CLASS_OPEN = re.compile(r"^\s*(?:class|struct)\s+(\w+)")
NESTED_OPEN = re.compile(r"^\s+struct\s+(\w+)\s*(?:\{|$)")
MEMBER_INIT = re.compile(r"^\s+[\w:<>,\s\*&]+\s+\w+\s*=\s*[^;]+;")
DEFAULT_ARG = re.compile(r"=\s*(\w+)\s*\{\s*\}")


def scan(path):
    """Walks one header tracking brace depth, class scope and nested scope.

    A class whose opening brace sits on its own line is still at the same
    depth as its declaration, so the scope is only treated as open once the
    depth has actually risen. Closing on the declaration line was the first
    version's bug and it made the check silently find nothing.
    """
    problems = []
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")

    depth = 0
    outer = None
    outer_depth = 0
    outer_open = False
    nested = None
    nested_depth = 0
    nested_open = False
    risky = {}

    for number, line in enumerate(lines, 1):
        if line.strip().startswith("//"):
            depth += line.count("{") - line.count("}")
            continue

        if outer is None:
            match = CLASS_OPEN.match(line)
            if match:
                outer = match.group(1)
                outer_depth = depth
                outer_open = False
        elif nested is None and outer_open:
            match = NESTED_OPEN.match(line)
            if match:
                nested = match.group(1)
                nested_depth = depth
                nested_open = False

        if nested is not None and nested_open and MEMBER_INIT.match(line):
            risky[nested] = number

        for match in DEFAULT_ARG.finditer(line):
            name = match.group(1)
            if name in risky:
                problems.append(
                    "%s:%d: %s{} is a default argument inside %s, and %s has a "
                    "default member initializer at line %d - move the struct to "
                    "namespace scope"
                    % (os.path.relpath(path, ROOT), number, name, outer or "?",
                       name, risky[name]))

        depth += line.count("{") - line.count("}")

        if outer is not None and not outer_open and depth > outer_depth:
            outer_open = True
        if nested is not None and not nested_open and depth > nested_depth:
            nested_open = True

        if nested is not None and nested_open and depth <= nested_depth:
            nested = None
            nested_open = False
        if outer is not None and outer_open and depth <= outer_depth:
            outer = None
            outer_open = False
            risky = {}

    return problems


def main():
    problems = []
    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in files:
            if name.endswith((".h", ".hpp")):
                problems += scan(os.path.join(base, name))

    for line in problems:
        print(line)
    print("\n%d nested default argument problem(s)" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
