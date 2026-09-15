#!/usr/bin/env bash
set -euo pipefail
PRESET="${1:-tests-debug}"
cmake --preset="$PRESET"
cmake --build --preset="$PRESET"
ctest --preset="$PRESET" --output-on-failure
