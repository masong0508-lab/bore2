#!/usr/bin/env bash
# Run the actual shared skateboard controller for both PS1 and Dreamcast.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cd "$ROOT"
for port in ps1 dreamcast; do
  defs=()
  if [ "$port" = dreamcast ]; then defs=(-DBORE_DC); fi
  cc -std=gnu99 -O2 -Wall -Wextra -Werror -fno-strict-aliasing \
    -Wno-misleading-indentation -Wno-unused-function \
    -Wno-unused-parameter -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
    "${defs[@]}" -Iports/dreamcast/psx -ffunction-sections -fdata-sections \
    test/skate_controls_test.c src/skate.c src/platform/compat_math.c \
    -Wl,--gc-sections -o "$TMP/skate_controls_$port"
  "$TMP/skate_controls_$port" "$@"
done
