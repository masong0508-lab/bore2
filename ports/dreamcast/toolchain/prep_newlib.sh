#!/bin/bash
# Patch newlib 4.6.0 (cygwin/cygwin tag newlib-4.6.0) for KOS like kos-chain's patch.mk.
set -e
N=/tmp/dc/src/newlib
K=/tmp/dc/kos
cd $N
if [ ! -f .kos_patched ]; then
  patch -N -p1 -i $K/utils/kos-chain/patches/targets/newlib-4.6.0.20260123-kos.diff
  touch .kos_patched
fi
mkdir -p newlib/libc/machine/sh/sys
cp $K/include/sys/lock.h newlib/libc/machine/sh/sys/lock.h
cp $K/include/sys/lock.h newlib/libc/include/sys/lock.h
echo PREP_OK
