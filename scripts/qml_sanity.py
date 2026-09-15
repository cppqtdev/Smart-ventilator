#!/usr/bin/env python3
"""Static sanity checks for the QML tree.

This is not a substitute for `qmllint` - run that in CI where Qt is
available. It catches the class of mistakes that are cheap to make in bulk
edits and expensive to find at runtime, on a machine with no Qt installed:

  * unbalanced braces, brackets or parentheses
  * unterminated string literals
  * references to Theme singleton properties that do not exist
  * icon names passed to AppIcon that have no SVG behind them
  * property names the QML engine refuses ("Illegal property name")
  * assignments to font sub-properties the QML font value type does not have
  * assignments to implicitWidth/implicitHeight on types where QML makes them
    read-only
  * inline components reaching for an id declared outside them, which QML
    cannot resolve
  * adjacent string literals, which concatenate in C++ and are a syntax error
    in JavaScript
  * object declarations inside a ternary, which QML cannot parse
  * non-ASCII characters that are being used as icons

Usage:  python3 scripts/qml_sanity.py [root]
Exit code is non-zero when anything is reported.
"""
from __future__ import annotations

import os
import re
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
THEME_DIR = os.path.join(ROOT, "ui", "Theme")
ICON_DIR = os.path.join(ROOT, "ui", "Assets", "icons")
SKIP_DIRS = {"_legacy", "_to_delete", "build", ".git", "node_modules"}

PAIRS = {"}": "{", "]": "[", ")": "("}
OPENERS = set(PAIRS.values())

# qqmlirbuilder.cpp refuses these as property names because they collide with
# JavaScript globals. The failure mode is nasty: the whole Theme module goes
# unavailable and every type that imports it fails to load, so the error the
# engine reports is several levels away from the actual mistake.
ILLEGAL_PROPERTY_NAMES = {
    "Array", "Boolean", "Date", "Function", "JSON", "Math", "Number", "Object",
    "RegExp", "String", "Qt", "decodeURI", "decodeURIComponent", "encodeURI",
    "encodeURIComponent", "escape", "eval", "gc", "isFinite", "isNaN",
    "parseFloat", "parseInt", "print", "unescape",
}

# The complete set of Accessible attached properties in Qt Quick. Assigning
# one that is not here takes the whole type out with "Cannot assign to
# non-existent property", and the engine reports it several levels away from
# the file that carries the mistake.
ACCESSIBLE_PROPERTIES = {
    "role", "name", "description", "checkable", "checked", "editable",
    "focusable", "focused", "multiLine", "passwordEdit", "pressed",
    "readOnly", "searchEdit", "selectable", "selected", "ignored",
    "onPressAction", "onToggleAction", "onIncreaseAction", "onDecreaseAction",
    "onScrollUpAction", "onScrollDownAction", "onScrollLeftAction",
    "onScrollRightAction", "onPreviousPageAction", "onNextPageAction",
}

ACCESSIBLE_ASSIGN = re.compile(r"\bAccessible\.(\w+)\s*:")

# Members of the QtQuick.Templates controls that exist in C++ but are not
# assignable from QML: protected virtuals, slots and signals. Assigning one
# reads perfectly and then takes the whole type out at load with "Cannot
# assign to non-existent property", reported several levels away from the
# file that carries it.
NON_ASSIGNABLE_MEMBERS = {
    "nextCheckState", "toggle", "increase", "decrease",
    "toggled", "clicked", "pressed", "released", "canceled",
    "buttonChange", "mirrorChange", "itemChange", "geometryChange",
}
NON_ASSIGNABLE_ASSIGN = re.compile(r"^\s*(\w+)\s*:\s*")

# Redeclaring one of these shadows the inherited Item property of the same
# name. The component still loads, so the failure shows up as a layout that
# silently collapses rather than as an error.
SHADOWED_ITEM_PROPERTIES = {
    "x", "y", "z", "width", "height", "opacity", "visible", "enabled",
    "parent", "children", "scale", "rotation", "clip", "focus", "anchors",
    "state", "states", "transitions", "transform", "baselineOffset",
    "childrenRect", "activeFocus", "antialiasing", "smooth",
}

PROPERTY_DECL = re.compile(
    r"^\s*(?:readonly\s+|default\s+|required\s+)*property\s+[\w<>.]+\s+(\w+)\s*[:{]")

# QQuickFontValueType. QFont has more members than this - font.families is the
# classic trap, since it exists on QFont but is not exposed to QML - and
# assigning one of the others fails the whole component at load time.
FONT_SUBPROPERTIES = {
    "family", "styleName", "bold", "weight", "italic", "underline", "overline",
    "strikeout", "pointSize", "pixelSize", "letterSpacing", "wordSpacing",
    "capitalization", "hintingPreference", "kerning", "preferShaping",
    "variableAxes", "features", "contextFontMerging", "preferTypoLineMetrics",
}

FONT_ASSIGN = re.compile(r"\bfont\.(\w+)\s*:")

# QQuickImplicitSizeItem descendants re-declare implicitWidth and
# implicitHeight as read-only for QML. Assigning one fails the whole component
# at load time, and the engine reports it against the file that *uses* the
# component, several levels away from the mistake.
READONLY_IMPLICIT_SIZE = {
    "Text", "Image", "ColorImage", "AnimatedImage", "BorderImage",
    "TextInput", "TextEdit", "TextField", "TextArea", "AppIcon", "IconImage",
}

# Matches both `Rectangle {` and `background: Rectangle {`.
ELEMENT_OPEN = re.compile(r"^(\s*)(?:\w+\s*:\s*)?([A-Z]\w*)\s*\{")
IMPLICIT_ASSIGN = re.compile(r"^\s*(implicitWidth|implicitHeight)\s*:")



# Typographic characters that are text, not icons: the true minus sign and
# the multiplication sign read correctly in a monospace face and have no
# ASCII equivalent of the same width.
TYPOGRAPHIC_GLYPHS = {"\u2212", "\u00d7", "\u2013", "\u2014"}


def strip_code(text: str):
    """Yields (line_no, code_only_line) with comments and strings removed."""
    out = []
    in_block_comment = False
    for line_no, raw in enumerate(text.split("\n"), 1):
        buf = []
        i = 0
        in_string = None
        while i < len(raw):
            two = raw[i:i + 2]
            if in_block_comment:
                if two == "*/":
                    in_block_comment = False
                    i += 2
                    continue
                i += 1
                continue
            if in_string:
                if raw[i] == "\\":
                    i += 2
                    continue
                if raw[i] == in_string:
                    in_string = None
                buf.append(" ")
                i += 1
                continue
            if two == "//":
                break
            if two == "/*":
                in_block_comment = True
                i += 2
                continue
            if raw[i] in "\"'":
                in_string = raw[i]
                i += 1
                continue
            buf.append(raw[i])
            i += 1
        if in_string:
            out.append((line_no, "".join(buf), "unterminated string literal"))
        else:
            out.append((line_no, "".join(buf), None))
    return out


def theme_tokens() -> dict[str, set[str]]:
    """Reads the declared property and function names of each Theme singleton."""
    decl = re.compile(r"^\s*(?:readonly\s+)?property\s+\S+\s+(\w+)\s*:", re.M)
    func = re.compile(r"^\s*function\s+(\w+)\s*\(", re.M)
    alias = re.compile(r"^\s*property\s+alias\s+(\w+)\s*:", re.M)
    tokens: dict[str, set[str]] = {}
    if not os.path.isdir(THEME_DIR):
        return tokens
    for name in os.listdir(THEME_DIR):
        if not name.endswith(".qml"):
            continue
        text = open(os.path.join(THEME_DIR, name), encoding="utf-8").read()
        found = set(decl.findall(text)) | set(func.findall(text)) | set(alias.findall(text))
        tokens[name[:-4]] = found
    return tokens


def icon_names() -> set[str]:
    if not os.path.isdir(ICON_DIR):
        return set()
    return {f[:-4] for f in os.listdir(ICON_DIR) if f.endswith(".svg")}


def qml_files(root: str):
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for f in files:
            if f.endswith(".qml"):
                yield os.path.join(base, f)


def blank_comments(text: str) -> str:
    """Blanks comments while preserving every character offset.

    Offsets are preserved so a match found here maps straight back onto the
    original source.
    """
    out = []
    i, n, state = 0, len(text), None
    while i < n:
        two, c = text[i:i + 2], text[i]
        if state == "line":
            out.append("\n" if c == "\n" else " ")
            if c == "\n":
                state = None
            i += 1
            continue
        if state == "block":
            out.append("\n" if c == "\n" else " ")
            if two == "*/":
                out.append(" ")
                state = None
                i += 2
                continue
            i += 1
            continue
        if state in ('"', "'"):
            out.append(c)
            if c == "\\":
                if i + 1 < n:
                    out.append(text[i + 1])
                i += 2
                continue
            if c == state:
                state = None
            i += 1
            continue
        if two == "//":
            state = "line"
            out.append("  ")
            i += 2
            continue
        if two == "/*":
            state = "block"
            out.append("  ")
            i += 2
            continue
        if c in "\"'":
            state = c
        out.append(c)
        i += 1
    return "".join(out)


# Two adjacent string literals concatenate in C++ and are a syntax error in
# JavaScript, which is what QML expressions are. The engine reports it as
# "Unexpected token `string literal'" - accurate, but it does not say why.
ADJACENT_STRINGS = re.compile(r"([\"'])\s*\n\s*([\"'])")

# `gradient: hollow ? null : Gradient { ... }` looks reasonable and is not
# valid QML - an object declaration is not an expression, so it cannot appear
# in a ternary. The engine reports it as "Expected token `,'", pointing at the
# brace rather than at the ternary, so it is easy to misread.
#
# Matches a `?` followed later on the same line by Type{ . Deliberately narrow:
# only a capitalised identifier immediately before an opening brace counts, so
# object literals ({a: 1}) and blocks do not trip it.
TERNARY_OBJECT = re.compile(r"\?[^\n]*?:\s*(?:null\s*:\s*)?[A-Z]\w*\s*\{\s*$")


INLINE_COMPONENT = re.compile(r"^\s*component\s+(\w+)\s*:")
ID_DECL = re.compile(r"^\s*id:\s*(\w+)")
ID_REF = re.compile(r"\b([a-z]\w*)\.")

# Singletons and JS globals are reachable from anywhere.
GLOBAL_SCOPE = {
    "Colors", "Spacing", "Radius", "Typography", "Metrics", "Icons",
    "Qt", "Math", "JSON", "Number", "String", "Date", "Screen", "console",
}


def inline_component_scope_problems(rel: str, text: str) -> list[str]:
    """Inline components do not share scope with their enclosing file.

    Referencing an enclosing id from inside one is a runtime ReferenceError,
    not a load-time failure, so it survives a clean start and shows up as a
    control that silently does nothing.
    """
    lines = text.split("\n")
    outer_ids, inline_ids = set(), set()

    depth = 0
    inside = None
    for raw in lines:
        if INLINE_COMPONENT.match(raw):
            inside = depth
        m = ID_DECL.match(raw)
        if m:
            (inline_ids if inside is not None else outer_ids).add(m.group(1))
        depth += raw.count("{") - raw.count("}")
        if inside is not None and depth <= inside:
            inside = None

    problems = []
    depth = 0
    inside = None
    name = None
    for line_no, raw in enumerate(lines, 1):
        m = INLINE_COMPONENT.match(raw)
        if m:
            inside = depth
            name = m.group(1)
        if inside is not None:
            for token in set(ID_REF.findall(raw.split("//")[0])):
                if token in outer_ids and token not in inline_ids \
                        and token not in GLOBAL_SCOPE:
                    problems.append(
                        f"{rel}:{line_no}: inline component {name} references "
                        f"'{token}', an id declared outside it")
        depth += raw.count("{") - raw.count("}")
        if inside is not None and depth <= inside:
            inside = None
            name = None
    return problems


def main() -> int:
    tokens = theme_tokens()
    icons = icon_names()
    problems: list[str] = []

    token_ref = re.compile(r"\b(Colors|Spacing|Radius|Typography|Metrics|Icons)\.(\w+)")
    icon_ref = re.compile(r"""\bname:\s*["'](?!\s*$)([a-z0-9\-]+)["']""")
    nonascii = re.compile(r"[^\x00-\x7F]")

    for path in sorted(qml_files(root=os.path.join(ROOT, "ui"))) + \
                sorted(p for p in qml_files(ROOT) if os.path.dirname(p) == ROOT):
        rel = os.path.relpath(path, ROOT)
        text = open(path, encoding="utf-8").read()

        stack = []
        for line_no, code, err in strip_code(text):
            if err:
                problems.append(f"{rel}:{line_no}: {err}")
            for ch in code:
                if ch in OPENERS:
                    stack.append((ch, line_no))
                elif ch in PAIRS:
                    if not stack:
                        problems.append(f"{rel}:{line_no}: stray '{ch}'")
                    elif stack[-1][0] != PAIRS[ch]:
                        problems.append(
                            f"{rel}:{line_no}: '{ch}' closes '{stack[-1][0]}' "
                            f"opened at line {stack[-1][1]}")
                        stack.pop()
                    else:
                        stack.pop()
            # Theme token existence
            for singleton, prop in token_ref.findall(code):
                known = tokens.get(singleton)
                if known and prop not in known:
                    problems.append(
                        f"{rel}:{line_no}: {singleton}.{prop} is not declared "
                        f"in ui/Theme/{singleton}.qml")

        if stack:
            ch, line_no = stack[-1]
            problems.append(f"{rel}: unclosed '{ch}' opened at line {line_no}")

        problems.extend(inline_component_scope_problems(rel, text))

        blanked = blank_comments(text)
        for m in ADJACENT_STRINGS.finditer(blanked):
            line_no = blanked[:m.start()].count("\n") + 1
            problems.append(
                f"{rel}:{line_no}: adjacent string literals - these concatenate "
                f"in C++ but are a syntax error in JavaScript; use + or join them")

        element_stack = []
        root_is_qtobject = bool(re.search(r'^QtObject\s*{', text, re.M))
        for line_no, raw in enumerate(text.split("\n"), 1):
            member = NON_ASSIGNABLE_ASSIGN.match(raw)
            if member and member.group(1) in NON_ASSIGNABLE_MEMBERS:
                problems.append(
                    f"{rel}:{line_no}: '{member.group(1)}' is not assignable "
                    f"from QML - it is a C++ member of the Templates control, "
                    f"and assigning it takes the type out at load")

            decl = PROPERTY_DECL.match(raw)
            if decl:
                name = decl.group(1)
                if name in SHADOWED_ITEM_PROPERTIES and not root_is_qtobject:
                    problems.append(
                        f"{rel}:{line_no}: property '{name}' shadows the "
                        f"inherited Item property of the same name")
                elif name in ILLEGAL_PROPERTY_NAMES:
                    problems.append(
                        f"{rel}:{line_no}: property '{name}' is an illegal QML "
                        f"property name - it collides with a JavaScript global")
                elif name[0].isupper():
                    problems.append(
                        f"{rel}:{line_no}: property '{name}' starts with a capital; "
                        f"QML reserves those for type names")
                elif name.startswith("on") and len(name) > 2 and name[2].isupper():
                    problems.append(
                        f"{rel}:{line_no}: property '{name}' will be read as a "
                        f"signal handler")
            for attached in ACCESSIBLE_ASSIGN.finditer(raw):
                if attached.group(1) not in ACCESSIBLE_PROPERTIES:
                    problems.append(
                        f"{rel}:{line_no}: Accessible.{attached.group(1)} is not "
                        f"an Accessible attached property")

            element = ELEMENT_OPEN.match(raw)
            if element:
                element_stack.append((len(element.group(1)), element.group(2)))
            elif raw.strip().startswith("}") and element_stack:
                indent = len(raw) - len(raw.lstrip())
                while element_stack and element_stack[-1][0] >= indent:
                    element_stack.pop()

            implicit = IMPLICIT_ASSIGN.match(raw)
            if implicit and element_stack \
                    and element_stack[-1][1] in READONLY_IMPLICIT_SIZE:
                problems.append(
                    f"{rel}:{line_no}: {implicit.group(1)} is read-only on "
                    f"{element_stack[-1][1]} - set sourceSize or width/height instead")

            ternary_object = TERNARY_OBJECT.search(raw.split("//")[0])
            if ternary_object:
                problems.append(
                    f"{rel}:{line_no}: object declaration inside a ternary - "
                    f"QML cannot parse this; declare the object separately and "
                    f"bind to it by id")

            font_assign = FONT_ASSIGN.search(raw)
            if font_assign and font_assign.group(1) not in FONT_SUBPROPERTIES:
                problems.append(
                    f"{rel}:{line_no}: font.{font_assign.group(1)} is not a "
                    f"property of the QML font value type")
            for icon in icon_ref.findall(raw):
                if icons and icon not in icons:
                    problems.append(
                        f"{rel}:{line_no}: icon '{icon}' has no SVG in ui/Assets/icons")
            for ch in nonascii.findall(raw):
                if ord(ch) > 0x2000 and ch not in TYPOGRAPHIC_GLYPHS:
                    problems.append(
                        f"{rel}:{line_no}: U+{ord(ch):04X} '{ch}' - use an "
                        f"AppIcon rather than a text glyph")

    # ColorImage keeps only the alpha channel, so an icon drawn in two tones
    # collapses into one solid shape. Every icon has to be single-colour.
    icon_dir = os.path.join(ROOT, "ui", "Assets", "icons")
    if os.path.isdir(icon_dir):
        tone = re.compile(r'(?:fill|stroke)="(#[0-9A-Fa-f]{3,6})"')
        for name in sorted(os.listdir(icon_dir)):
            if not name.endswith(".svg"):
                continue
            text = open(os.path.join(icon_dir, name), encoding="utf-8").read()
            tones = {c.upper() for c in tone.findall(text)}
            tones.discard("#FFF")
            if len(tones) > 1:
                problems.append(
                    f"ui/Assets/icons/{name}: {len(tones)} colours "
                    f"({', '.join(sorted(tones))}) - ColorImage keeps only the "
                    f"alpha, so this renders as one solid shape")

    for line in problems:
        print(line)
    print(f"\n{len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
