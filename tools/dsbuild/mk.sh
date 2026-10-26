#!/bin/bash
# mk.sh name : build name.c -> name.nds
set -e
cd /tmp/dsb/t; C=/tmp/crtls_old
/tmp/dsb/cc9.sh $1.c $1.o
ld.lld -T $C/dsi_arm9.mem -T ds.ld --gc-sections --no-check-sections --defsym=fake_heap_start=__end__ --defsym=fake_heap_end=__eheap_end -o $1.elf /tmp/dsb/ds_arm9_crt0.o $1.o glue.o /tmp/dsb/libnds9.a /tmp/dsb/pl/lib/libc.a /tmp/dsb/libgcc.a
llvm-objcopy -O binary $1.elf $1.full; tail -c +2049 $1.full > $1.bin
/tmp/dsb/bin/ndstool -c $1.nds -9 $1.bin -7 a7.bin -r9 0x02000000 -e9 0x02000800 -r7 0x02380000 -e7 0x02380000 >/dev/null
