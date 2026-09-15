#!/usr/bin/env python3
"""Regenerate qml.qrc from the files on disk."""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {"_legacy", "__pycache__"}
KEEP_EXT = {".qml", ".svg", ".png", ".wav", ".ttf", ".otf", ".json", ".dbc"}


def collect():
    entries = ["main.qml"]
    for base in ("ui",):
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, base)):
            dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
            rel_dir = os.path.relpath(dirpath, ROOT).replace(os.sep, "/")
            for name in sorted(filenames):
                if name == "qmldir":
                    entries.append(f"{rel_dir}/{name}")
                    continue
                if os.path.splitext(name)[1] in KEEP_EXT:
                    entries.append(f"{rel_dir}/{name}")
    assets = [e for e in entries if "/Assets/" in e]
    qmldirs = [e for e in entries if e.endswith("qmldir")]
    rest = [e for e in entries if e not in assets and e not in qmldirs and e != "main.qml"]
    return ["main.qml"] + assets + qmldirs + rest


def main():
    lines = ["<RCC>", '    <qresource prefix="/">']
    for entry in collect():
        lines.append(f"        <file>{entry}</file>")
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(ROOT, "qml.qrc"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))
    print(f"qml.qrc: {len(collect())} entries")


if __name__ == "__main__":
    main()
