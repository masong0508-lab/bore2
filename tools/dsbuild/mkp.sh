#!/bin/bash
# mkp.sh : build ports/dsi/source/main.c + bigmap.c -> $OUT/bore.nds (default /tmp/dsb/t/bore.nds)
#   R=checkout (default: this script's repo), OUT=build dir (default /tmp/dsb/t), XTRA="-DDSTEST" for a test build
set -e
R=${R:-$(cd "$(dirname "$0")/../.." && pwd)}; OUT=${OUT:-/tmp/dsb/t}; C=/tmp/crtls_old
mkdir -p "$OUT"; for f in ds.ld glue.o a7.bin; do [ -f "$OUT/$f" ] || cp /tmp/dsb/t/$f "$OUT/"; done
cd "$OUT"
EXTRA="-I$R/ports/host/shim -I$R/src" /tmp/dsb/cc9.sh $R/ports/dsi/source/main.c bore_main.o
EXTRA="-I$R/ports/host/shim -I$R/src" /tmp/dsb/cc9.sh $R/src/bigmap.c bore_map.o
ld.lld -z muldefs -T $C/dsi_arm9.mem -T ds.ld --gc-sections --no-check-sections --defsym=fake_heap_start=__end__ --defsym=fake_heap_end=__eheap_end -o bore.elf /tmp/dsb/ds_arm9_crt0.o bore_main.o bore_map.o glue.o /tmp/dsb/libnds9.a /tmp/dsb/pl/lib/libc.a /tmp/dsb/libgcc.a
llvm-objcopy -O binary bore.elf bore.full; tail -c +2049 bore.full > bore.bin
/tmp/dsb/bin/ndstool -c bore.nds -g BR2D 01 BORE2 -h 0x4000 -9 bore.bin -7 a7.bin -r9 0x02000000 -e9 0x02000800 -r7 0x02380000 -e7 0x02380000 >/dev/null
python3 $R/tools/dsbuild/addmusic.py bore.nds $R/assets/dsmusic.bin
ls -la bore.nds
