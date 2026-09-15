#!/usr/bin/env bash
set -euo pipefail
find src app -name '*.h' -o -name '*.cpp' | xargs clang-format -i
echo "Formatted all C++ source files."
