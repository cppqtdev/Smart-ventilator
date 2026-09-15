#!/usr/bin/env bash
set -euo pipefail
PRESET="${1:-dev-all-debug}"
echo "Configuring with preset: $PRESET"
cmake --preset="$PRESET"
echo "Building..."
cmake --build --preset="$PRESET"
echo "Build complete."
