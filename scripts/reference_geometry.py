#!/usr/bin/env python3
"""Compare the layout tokens in ui/Theme/Metrics.qml and ui/Theme/Typography.qml
against measurements taken from the reference screens in reference/.

The measurements below were read off the reference images pixel by pixel and
then divided by the frame scale (1422 image pixels across a 1024 design pixel
window, 1064 down a 768 design pixel window). Each entry records what the
reference draws so that a later edit to a token has to justify itself.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOLERANCE = 2.0

MEASURED_METRICS = {
    "screenPadding": 16.0,
    "gutter": 16.0,
    "sidebarWidth": 222.0,
    "railWidth": 210.0,
    "headerHeight": 68.0,
    "navHeight": 45.0,
    "navGap": 9.0,
    "tileGap": 19.0,
    "dialSize": 103.0,
    "dialStepWidth": 38.0,
    "dialStepHeight": 66.0,
    "actionButtonWidth": 108.0,
    "actionButtonHeight": 44.0,
    "pageTabWidth": 183.0,
}

MEASURED_TYPOGRAPHY = {
    "tileLabel": 15.0,
    "tileValue": 32.0,
    "tileLimit": 17.0,
    "tileUnit": 18.0,
    "dialLabel": 18.0,
    "dialValue": 28.0,
    "readoutLabel": 16.0,
    "readoutValue": 33.0,
    "readoutUnit": 15.0,
    "channelLabel": 18.0,
    "axisTick": 13.0,
    "tabLabel": 15.0,
}

TOKEN = re.compile(r"property\s+int\s+(\w+)\s*:\s*(?:metrics|typography)\.px\((\d+)\)")


def read_tokens(path):
    found = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = TOKEN.search(line)
        if match:
            found[match.group(1)] = float(match.group(2))
    return found


def compare(name, tokens, measured):
    problems = []
    for key, reference in measured.items():
        if key not in tokens:
            problems.append("%s: %s is missing" % (name, key))
            continue
        actual = tokens[key]
        if abs(actual - reference) > TOLERANCE:
            problems.append("%s: %s is %g, the reference measures %g"
                            % (name, key, actual, reference))
    return problems


def main():
    metrics = read_tokens(ROOT / "ui" / "Theme" / "Metrics.qml")
    typography = read_tokens(ROOT / "ui" / "Theme" / "Typography.qml")

    problems = compare("Metrics", metrics, MEASURED_METRICS)
    problems += compare("Typography", typography, MEASURED_TYPOGRAPHY)

    for problem in problems:
        print(problem)
    print("%d token(s) away from the reference" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
