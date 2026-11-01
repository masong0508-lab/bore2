#!/bin/bash
# sh-elf gcc 15.2.0, C only, one multilib (m4-single little endian, what KOS uses).
# Like kos-chain's gcc-pass1; the C-only pass 2 is skipped (see newlib step: fake-kos.o is removed from libgcc.a).
set -e
export PATH=/tmp/dc/sh-elf/bin:/tmp/dc/host/bin:$PATH
P=/tmp/dc/sh-elf
mkdir -p /tmp/dc/build/gcc1 && cd /tmp/dc/build/gcc1
[ -f Makefile ] || /tmp/dc/src/gcc/configure --target=sh-elf --prefix=$P \
  --with-gnu-as --with-gnu-ld --without-headers --with-newlib \
  --enable-languages=c --disable-libssp --enable-checking=release \
  --with-endian=little --with-cpu=m4-single --disable-multilib \
  --disable-nls --disable-lto --disable-libquadmath --disable-libgomp --disable-libatomic \
  --disable-libstdcxx --disable-bootstrap --disable-shared --disable-threads \
  MAKEINFO=missing CC="gcc -std=gnu17" CXX="g++ -std=gnu++17"
mkdir -p /tmp/dc/build/gcc1/gcc/d /tmp/dc/build/gcc1/gcc/rust   # genhooks writes d/ and rust/ hook headers even for C-only
make -j2 all-gcc MAKEINFO=true
make -j2 all-target-libgcc MAKEINFO=true
make install-gcc install-target-libgcc MAKEINFO=true
echo GCC_DONE
