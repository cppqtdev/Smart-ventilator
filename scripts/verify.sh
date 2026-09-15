#!/usr/bin/env bash
#
# The one gate. Static checks, then a build, then the tests.
#
# The static checkers stand in for a compiler in environments that have no Qt
# toolchain, and they are not one: a runtime failure got past them once and
# took the application down. This script is what makes a change trusted, and
# it is what continuous integration runs on every push.
#
# Usage: scripts/verify.sh [preset]   (default: tests-debug)
#
set -euo pipefail

PRESET="${1:-tests-debug}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "=============================================="
echo " 1/3  Static checks"
echo "=============================================="
./scripts/check.sh

echo
echo "=============================================="
echo " 2/3  Build  (preset: $PRESET)"
echo "=============================================="
cmake --preset="$PRESET"
cmake --build --preset="$PRESET"

echo
echo "=============================================="
echo " 3/3  Tests"
echo "=============================================="
ctest --preset="$PRESET" --output-on-failure

echo
echo "All checks, the build and the tests passed."
