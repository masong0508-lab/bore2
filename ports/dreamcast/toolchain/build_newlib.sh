#!/bin/bash
# newlib 4.6.0 (KOS-patched) for sh-elf, then the KOS include fixups kos-chain does
set -e
export PATH=/tmp/dc/sh-elf/bin:$PATH
P=/tmp/dc/sh-elf
K=/tmp/dc/kos
/tmp/dc/prep_newlib.sh
mkdir -p /tmp/dc/build/newlib && cd /tmp/dc/build/newlib
[ -f Makefile ] || /tmp/dc/src/newlib/configure --disable-newlib-supplied-syscalls --target=sh-elf --prefix=$P \
  --with-multilib-list=m4-single --with-endian=little --with-cpu=m4-single \
  --enable-newlib-io-c99-formats --disable-nls \
  CC_FOR_TARGET=sh-elf-gcc MAKEINFO=true
make -j2 MAKEINFO=true
make install MAKEINFO=true
# fixup-newlib (kos-chain/scripts/newlib.mk)
I=$P/sh-elf/include
mkdir -p $I/sys $I/machine
cp $K/include/pthread.h $I
cp $K/include/sys/_pthreadtypes.h $I/sys
cp $K/include/sys/dirent.h $I/sys
cp $K/include/machine/time.h $I/machine
ln -nsf $K/include/kos $I
echo NEWLIB_DONE
