#!/bin/bash
# Patch gcc 15.2.0 (gcc-mirror/gcc tag releases/gcc-15.2.0) for KOS, as kos-chain's patch.mk does,
# and put GMP/MPFR/MPC (NetBSD/src external/lgpl3/*/dist) in-tree.
set -e
G=/tmp/dc/src/gcc
K=/tmp/dc/kos/utils/kos-chain/patches
cd $G
if [ ! -f .kos_patched ]; then
  patch -N -p1 -i $K/targets/gcc-15.2.0-kos.diff
  touch .kos_patched
fi
cp $K/gcc/gthr-kos.h libgcc/gthr-kos.h
cp $K/gcc/fake-kos.c libgcc/config/fake-kos.c
L=/tmp/dc/src/netbsd/external/lgpl3
ln -sfn $L/gmp/dist gmp
ln -sfn $L/mpfr/dist mpfr
ln -sfn $L/mpc/dist mpc
echo PREP_OK
