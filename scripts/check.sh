#!/usr/bin/env bash
# Static checks that run without a Qt toolchain.
#
# These are not a substitute for building, and not a substitute for qmllint
# and clazy in CI where Qt is available. They catch the classes of mistake
# that are cheap to make during a large refactor and expensive to find at
# runtime: unbalanced QML, references to theme tokens or C++ properties that
# do not exist, icons with no SVG behind them, and members declared but never
# defined.
set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
status=0

run() {
    echo "==> $1"
    shift
    if ! "$@"; then
        status=1
    fi
    echo
}

run "QML structure, theme tokens and icon names" \
    python3 "$root/scripts/qml_sanity.py" "$root"

run "QML call sites against the components they use" \
    python3 "$root/scripts/qml_api_check.py"

run "layout items whose preferred size would be ignored" \
    python3 "$root/scripts/layout_check.py"

run "QML bindings against the C++ objects behind them" \
    python3 "$root/scripts/qml_binding_check.py"

run "C++ declarations against their definitions" \
    python3 "$root/scripts/cpp_decl_check.py"

run "clamp and qBound ranges that could invert" \
    python3 "$root/scripts/clamp_check.py"

run "qrc: URLs against the resource file" \
    python3 "$root/scripts/qrc_check.py"

run "layout tokens against the reference screens" \
    python3 "$root/scripts/reference_geometry.py"

run "nested structs used as default arguments" \
    python3 "$root/scripts/nested_default_check.py"

run "the layout clamp against the layouts on offer" \
    python3 "$root/scripts/layout_count.py"

run "one application identity across both bootstraps" \
    python3 "$root/scripts/identity_check.py"

run "rows whose children cannot fit the width they are given" \
    python3 "$root/scripts/layout_fit_check.py"

run "Q_OBJECT headers the meta-object compiler would never see" \
    python3 "$root/scripts/metaobject_check.py"

run "one typeface across the interface" \
    python3 "$root/scripts/typeface_check.py"

if [ "$status" -eq 0 ]; then
    echo "All static checks passed."
else
    echo "Static checks reported problems."
fi
exit "$status"
