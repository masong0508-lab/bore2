#!/bin/bash
# The PS1 must not change because of the Dreamcast port: build the PS1 Release elf (bore2 target only) from a source tree and print
# its _end and a checksum of its loadable bytes. Usage: ps1_check.sh <source dir> <build dir>
set -e
SRCDIR=$1; B=$2
export PATH=/opt/psn00bsdk/bin:$PATH
mkdir -p $B
[ -f $B/build.ninja ] || cmake -S $SRCDIR -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_TOOLCHAIN_FILE=/opt/psn00bsdk/lib/libpsn00b/cmake/sdk.cmake -DPSN00BSDK_TC=/opt/psn00bsdk -DPSN00BSDK_TARGET=mipsel-none-elf > $B/cfg.log
ninja -C $B -j2 bore2 > $B/build.log 2>&1 || { tail -20 $B/build.log; exit 1; }
E=$(ls $B/bore2.elf)
mipsel-none-elf-objcopy -O binary -R .MIPS.abiflags $E $B/bore2.bin.chk
echo "_end $(mipsel-none-elf-nm $E | awk '$3=="_end"{print $1}')  image sha1 $(sha1sum < $B/bore2.bin.chk | cut -c1-16)"
