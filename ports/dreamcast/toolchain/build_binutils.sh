#!/bin/bash
# sh-elf binutils for the Dreamcast (source: NetBSD/src external/gpl3/binutils/dist, GNU binutils 2.46.0)
set -e
export PATH=/tmp/dc/host/bin:$PATH   # flex from build_flex.sh (NetBSD strips the generated lexers)
P=/tmp/dc/sh-elf
S=/tmp/dc/src/netbsd/external/gpl3/binutils/dist
# NetBSD writes $> (bmake) in bfd's generated-header rules; GNU make needs $<
sed -i 's/< \$> /< $< /' $S/bfd/Makefile.in
mkdir -p /tmp/dc/build/binutils && cd /tmp/dc/build/binutils
[ -f Makefile ] || /tmp/dc/src/netbsd/external/gpl3/binutils/dist/configure --target=sh-elf --prefix=$P \
  --disable-werror --disable-nls --disable-gdb --disable-sim --disable-gprof --disable-gprofng \
  --disable-libctf --disable-plugins MAKEINFO=true CFLAGS="-O2 -std=gnu17"
make -j2 MAKEINFO=true
make install MAKEINFO=true
echo BINUTILS_DONE
