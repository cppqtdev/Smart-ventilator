#!/usr/bin/env python3
"""Keep the interface on one typeface.

Every reference screen draws its data, labels, units, buttons and tabs in a
monospaced face. Thirty-eight places used the proportional one, which is why
a screen would look, in the words of the person who kept finding them,
cheap: two faces on one panel read as two designs.

Three places are proportional in the reference and stay that way:

  - the alarm message, on the banner and the annunciator. It is a sentence
    for a person to read at a glance, not a value to line up in a column,
    and the reference sets it proportional and larger.
  - the command toast, which is the same kind of sentence.
  - the company wordmark on the start-up screen.

Anywhere else, Typography.family is the wrong face. Use monoFamily.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UI = ROOT / "ui"

# Files whose proportional text the reference actually draws proportional.
ALLOWED = {
    "ui/Components/AlarmBanner.qml",
    "ui/Components/AlarmAnnunciator.qml",
    "ui/Components/CommandToast.qml",
    "ui/Screens/SplashScreen.qml",
}

USE = re.compile(r"\b(?:font\.family|fontFamily)\s*:\s*Typography\.family\b")


def main():
    problems = []

    for path in sorted(UI.rglob("*.qml")):
        relative = path.relative_to(ROOT).as_posix()
        if relative in ALLOWED:
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if USE.search(line):
                problems.append(
                    "%s:%d: the proportional face outside the alarm message "
                    "and the wordmark - use Typography.monoFamily"
                    % (relative, number)
                )

    for line in problems:
        print(line)
    print("%d place(s) on the wrong typeface" % len(problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
