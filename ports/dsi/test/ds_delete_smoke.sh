#!/usr/bin/env bash
# Real headless melonDS save-deletion safety check. Run against a built ROM:
# DSRUN=/tmp/dsrun bash ports/dsi/test/ds_delete_smoke.sh out/
# Verifies that a first tap does not erase any EEPROM save journal, while two
# distinct taps inside three seconds do erase both save headers.
set -euo pipefail
D=${1:-out}
RUN=${DSRUN:-/tmp/dsrun}
test -x "$RUN" && test -s "$D/bore.nds"
S="$D/title_delete.sav"
rm -f "$S"
LIVE="100-103:0:212,140;200-203:1:-1,-1"
MENU="128,185"
MAKE="$LIVE;400-403:0:$MENU;440-443:0:43,75;500-503:0:115,88;540-542:1:-1,-1;600-603:0:88,8;640-643:0:$MENU;680-683:0:215,75"
BORE_SAV="$S" "$RUN" "$D/bore.nds" 760 "$D/title_save.ppm" "$MAKE" >/dev/null 2>&1
check() {
    python3 - "$S" "$1" <<'PY'
import sys
from pathlib import Path
data = Path(sys.argv[1]).read_bytes()
assert len(data) >= 8192, "EEPROM save truncated"
has_save = any(data[i:i+4] == b"BR2S" for i in (0, 4096))
assert has_save == (sys.argv[2] == "present"), "unexpected save journal: present=" + str(has_save)
PY
}
check present
# One accidental press must not change the persisted EEPROM.
BORE_SAV="$S" "$RUN" "$D/bore.nds" 260 "$D/title_one_tap.ppm" "140-142:0:175,140" >/dev/null 2>&1
check present
# A real second touch, separated by a release, confirms the operation.
BORE_SAV="$S" "$RUN" "$D/bore.nds" 260 "$D/title_two_taps.ppm" "140-142:0:175,140;164-166:0:175,140" >/dev/null 2>&1
check absent
echo "DS DELETE SAFETY PASS: single tap preserved save; second tap erased it"
