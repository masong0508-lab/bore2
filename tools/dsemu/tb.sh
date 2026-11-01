#!/bin/bash
# tb.sh outdir [scenes...] : build a -DDSTEST ROM into $B (default /tmp/dsb/tt) and run the frame-rate bench into outdir
R=$(cd "$(dirname "$0")/../.." && pwd); B=${B:-/tmp/dsb/tt}
OUT=$B XTRA="-DDSTEST $XTRA" bash "$R/tools/dsbuild/mkp.sh" > "$B.log" 2>&1 || { grep -E "error" "$B.log" | head -20; exit 1; }
grep -E "error" "$B.log" | head -5
python3 "$R/tools/dsemu/bench.py" "$B/bore.nds" "$B/bore.elf" "$@"
