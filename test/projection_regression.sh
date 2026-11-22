#!/usr/bin/env bash
# Compile the real renderer paths against the Dreamcast port's C GTE.
# Function-section GC drops unrelated game code; no PS1 SDK or ROM is needed.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
# dc_psx.c includes kos.h but uses no KOS declarations in these math functions.
printf '/* Host-only include stub; no runtime functions are replaced. */\n' > "$TMP/kos.h"
cd "$ROOT"
# The full production TUs contain existing 32-bit GPU-address casts. Suppress
# their host-only warnings; all code retained by this test uses native pointers.
cc -std=gnu99 -O2 -DBORE_DC -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-attributes \
  -Iports/dreamcast/psx -I"$TMP" -ffunction-sections -fdata-sections \
  test/projection_test.c test/projection_near.c test/projection_far.c \
  test/projection_crowd.c src/gfx.c ports/dreamcast/dc_psx.c \
  -Wl,--gc-sections -lm -o "$TMP/projection_test"
"$TMP/projection_test"
