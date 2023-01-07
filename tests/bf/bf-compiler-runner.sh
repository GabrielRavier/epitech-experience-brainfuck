#!/usr/bin/env bash
set -euo pipefail

TMP_EXECUTABLE=$(mktemp)
$(dirname "$0")/../../bf-compiler "$1" "$TMP_EXECUTABLE"
"$TMP_EXECUTABLE"
rm "$TMP_EXECUTABLE"
