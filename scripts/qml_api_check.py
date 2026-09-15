#!/usr/bin/env python3
"""Checks that every property assigned at a call site exists on the component.

QML resolves a component's properties at load time, so assigning one that no
longer exists takes the whole screen down with "Cannot assign to non-existent
property". Renaming or dropping a property is therefore a breaking change for
every call site, and nothing in the build catches it.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI = os.path.join(ROOT, "ui")
SKIP_DIRS = {"_legacy", "__pycache__"}

DECL = re.compile(
    r"^\s*(?:readonly\s+|required\s+|default\s+)*property\s+(?:alias\s+|[\w.<>]+\s+)([A-Za-z_]\w*)",
    re.M)
SIGNAL = re.compile(r"^\s*signal\s+([A-Za-z_]\w*)", re.M)
FUNC = re.compile(r"^\s*function\s+([A-Za-z_]\w*)", re.M)
ROOT_TYPE = re.compile(r"^([A-Z]\w*)(?:\.\w+)?\s*\{", re.M)
INLINE_COMPONENT = re.compile(r"^\s*component\s+([A-Z]\w*)\s*:", re.M)

# Properties and signals every QML item carries. Assigning one of these is
# always fine, so they are not worth resolving through the type system.
BASE = {
    # Item
    "id", "objectName", "x", "y", "z", "width", "height", "opacity", "visible",
    "enabled", "parent", "children", "data", "resources", "state", "states",
    "transitions", "transform", "rotation", "scale", "clip", "focus",
    "activeFocus", "activeFocusOnTab", "antialiasing", "smooth", "baselineOffset",
    "implicitWidth", "implicitHeight", "childrenRect", "anchors", "layer",
    "containmentMask", "visibleChildren",
    # Rectangle
    "color", "gradient", "radius", "border", "topLeftRadius", "topRightRadius",
    "bottomLeftRadius", "bottomRightRadius",
    # Text
    "text", "font", "horizontalAlignment", "verticalAlignment", "wrapMode",
    "elide", "maximumLineCount", "lineHeight", "lineHeightMode", "textFormat",
    "style", "styleColor", "linkColor", "renderType", "padding", "topPadding",
    "bottomPadding", "leftPadding", "rightPadding", "fontSizeMode",
    "minimumPixelSize", "minimumPointSize",
    # Image / ColorImage
    "source", "sourceSize", "fillMode", "asynchronous", "cache", "mipmap",
    "mirror", "autoTransform", "sourceClipRect", "defaultColor",
    # Control / AbstractButton
    "checked", "checkable", "down", "pressed", "hovered", "hoverEnabled",
    "autoExclusive", "autoRepeat", "icon", "display", "indicator",
    "contentItem", "background", "spacing", "action", "ButtonGroup",
    "leftInset", "rightInset", "topInset", "bottomInset", "palette",
    "focusPolicy", "wheelEnabled", "locale", "mirrored",
    # Popup / Dialog
    "modal", "dim", "closePolicy", "title", "standardButtons", "parentWindow",
    # Loader / Repeater
    "active", "sourceComponent", "item", "asynchronous", "model", "delegate",
    # MouseArea / Timer / Animation
    "acceptedButtons", "cursorShape", "drag", "propagateComposedEvents",
    "preventStealing", "pressAndHoldInterval", "interval", "repeat", "running",
    "triggeredOnStart", "duration", "easing", "loops", "from", "to", "target",
    "property", "properties", "velocity", "alwaysRunToEnd", "paused",
    # Flickable / views
    "contentWidth", "contentHeight", "contentX", "contentY", "flickableDirection",
    "boundsBehavior", "interactive", "orientation", "currentIndex", "count",
    "header", "footer", "section", "cacheBuffer", "snapMode", "highlight",
    "highlightMoveDuration", "currentItem", "keyNavigationEnabled",
    # Shapes
    "strokeColor", "strokeWidth", "fillColor", "capStyle", "joinStyle",
    "dashPattern", "startX", "startY", "containsMode", "preferredRendererType",
    # misc
    "value", "stepSize", "position", "visualPosition", "live", "wrap",
    "editable", "validator", "inputMethodHints", "selectByMouse", "readOnly",
    "placeholderText", "echoMode", "cursorVisible", "selectionColor",
    "selectedTextColor", "activeFocusOnPress", "persistentSelection",
    "volume", "muted", "loops", "audioOutput", "playbackRate",
    "maximumLength", "inputMask", "passwordCharacter", "passwordMaskDelay",
    "selectByKeyboard", "overwriteMode", "autoScroll", "canPaste", "canUndo",
    "canRedo", "hoveredLink", "renderType", "bottomInset", "implicitBackgroundWidth",
}

BASE_SIGNALS = {
    "clicked", "pressed", "released", "doubleClicked", "pressAndHold",
    "canceled", "toggled", "entered", "exited", "positionChanged",
    "wheel", "triggered", "loaded", "statusChanged", "completed",
    "accepted", "rejected", "closed", "opened", "aboutToShow", "aboutToHide",
    "activated", "deactivated", "linkActivated", "linkHovered",
    "textEdited", "editingFinished", "moved", "valueModified",
}


def qml_files():
    main = os.path.join(ROOT, "main.qml")
    if os.path.exists(main):
        yield main
    for base, dirs, names in os.walk(UI):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for name in sorted(names):
            if name.endswith(".qml") and name != "qmldir":
                yield os.path.join(base, name)


def component_api():
    """component name -> (declared names, declared signals, root type)."""
    api = {}
    for path in qml_files():
        text = open(path, encoding="utf-8").read()
        name = os.path.splitext(os.path.basename(path))[0]
        if name.endswith(".anchored") or "." in name:
            continue
        declared = set(DECL.findall(text)) | set(FUNC.findall(text))
        signals = set(SIGNAL.findall(text))
        match = ROOT_TYPE.search(text)
        api[name] = (declared, signals, match.group(1) if match else None)
    return api


def resolve(name, api, seen=None):
    """Declared names and signals of a component including its project bases."""
    seen = seen or set()
    if name in seen or name not in api:
        return set(), set()
    seen.add(name)
    declared, signals, base = api[name]
    if base and base in api:
        parent_declared, parent_signals = resolve(base, api, seen)
        declared = declared | parent_declared
        signals = signals | parent_signals
    return declared, signals


def block_for(text, open_index):
    depth = 1
    i = open_index
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return text[open_index:i - 1]


def main():
    api = component_api()
    inline = set()
    for path in qml_files():
        text = open(path, encoding="utf-8").read()
        inline |= set(INLINE_COMPONENT.findall(text))

    problems = []
    assignment = re.compile(r"^[ \t]*([A-Za-z_]\w*)\s*:(?!:)", re.M)

    # A component that redeclares one of its base component's properties
    # fails to load. The base here is a project file, so it is resolvable.
    for path in qml_files():
        rel = os.path.relpath(path, ROOT)
        text = open(path, encoding="utf-8").read()
        name = os.path.splitext(os.path.basename(path))[0]
        if name not in api:
            continue
        _, _, base = api[name]
        if not base or base not in api:
            continue
        inherited, _ = resolve(base, api)

        # Only the root object's own properties can clash; a property declared
        # inside a Repeater delegate belongs to the delegate, not this type.
        depth = 0
        for line_no, line in enumerate(text.split("\n"), 1):
            stripped = line.strip()
            found = re.match(r"^(?:readonly\s+|required\s+|default\s+)*"
                             r"property\s+(?:alias\s+|[\w.<>]+\s+)([A-Za-z_]\w*)",
                             stripped)
            if found and depth == 1 and found.group(1) in inherited:
                problems.append(
                    "%s:%d: redeclares '%s', which %s already has"
                    % (rel, line_no, found.group(1), base))
            depth += line.count("{") - line.count("}")


    for path in qml_files():
        rel = os.path.relpath(path, ROOT)
        text = open(path, encoding="utf-8").read()
        self_name = os.path.splitext(os.path.basename(path))[0]

        for match in re.finditer(r"(?<![\w.])([A-Z]\w*)\s*\{", text):
            used = match.group(1)
            if used not in api or used in inline or used == self_name:
                continue
            declared, signals = resolve(used, api)
            block = block_for(text, match.end())
            # Only the block's own top level; nested children belong to others.
            depth = 0
            for line in block.split("\n"):
                stripped = line.strip()
                if depth == 0:
                    found = assignment.match(line)
                    if found:
                        prop = found.group(1)
                        ok = (prop in declared or prop in BASE
                              or prop in signals or "." in stripped.split(":")[0])
                        if not ok and prop.startswith("on") and len(prop) > 2:
                            handler = prop[2].lower() + prop[3:]
                            ok = (handler in signals or handler in BASE_SIGNALS
                                  or handler.endswith("Changed"))
                        if not ok:
                            line_no = text[:match.end()].count("\n") + 1 \
                                + block[:block.index(line)].count("\n") + 1
                            problems.append(
                                f"{rel}:{line_no}: {used} has no property "
                                f"'{prop}'")
                depth += line.count("{") - line.count("}")

    for line in problems:
        print(line)
    print(f"\n{len(problems)} call-site problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
