#!/usr/bin/env python3
"""Keep the two bootstraps answering to one name.

Every persisted thing on this device is found through the organisation and
application names: QSettings keys its store by them, and AppDataLocation
builds the directory holding the SQLite database and the log file from them.
Two bootstraps setting them differently is two devices sharing one machine,
and the audit trail splits between them.

They are declared once, in sv/common/AppIdentity.h. This refuses any other
file that sets them, or builds an AppDataLocation path of its own.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP = {"build", ".git", "_to_delete", "node_modules"}

IDENTITY_HEADER = os.path.join("src", "common", "include", "sv", "common", "AppIdentity.h")
IDENTITY_SOURCE = os.path.join("src", "common", "src", "AppIdentity.cpp")

SETS_NAME = re.compile(r"\bsetOrganizationName\s*\(|\bsetApplicationName\s*\(")
BUILDS_PATH = re.compile(r"QStandardPaths::writableLocation\s*\(\s*QStandardPaths::AppDataLocation")
SETTINGS_CTOR = re.compile(r"QSettings\s*\(\s*QStringLiteral")


def main() -> int:
    problems = []
    bootstraps = set()

    for base, dirs, files in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP]
        for name in files:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(base, name)
            rel = os.path.relpath(path, ROOT)
            if rel in (IDENTITY_HEADER, IDENTITY_SOURCE):
                continue

            text = open(path, encoding="utf-8").read()
            if "applyApplicationIdentity" in text and name == "main.cpp":
                bootstraps.add(rel)

            for line_no, line in enumerate(text.split("\n"), 1):
                if line.strip().startswith(("//", "*")):
                    continue
                if SETS_NAME.search(line):
                    problems.append(
                        f"{rel}:{line_no}: sets the application identity by hand - "
                        f"call sv::common::applyApplicationIdentity() instead")
                if BUILDS_PATH.search(line):
                    problems.append(
                        f"{rel}:{line_no}: builds its own AppDataLocation path - "
                        f"use sv::common::applicationDataDirectory() instead")
                if SETTINGS_CTOR.search(line) and "identity::" not in line:
                    problems.append(
                        f"{rel}:{line_no}: names its own QSettings scope - "
                        f"use sv::common::identity::organizationName() and "
                        f"applicationName()")

    expected = {"main.cpp", os.path.join("app", "main.cpp")}
    for entry in sorted(expected - bootstraps):
        problems.append(f"{entry}: does not call sv::common::applyApplicationIdentity()")

    for problem in problems:
        print(f"  {problem}")
    print(f"{len(problems)} identity problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
