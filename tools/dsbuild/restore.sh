#!/bin/bash
# One command to get the prebuilt DS toolchain back into /tmp (instead of rebuilding it from source):
#   tools/dsbuild/restore.sh
# It takes ds-toolchain-1.tar.xz from the repo's ds-toolchain branch and unpacks into /tmp:
#   /tmp/dsb (clang glue, picolibc, compiler-rt, libnds9.a, ndstool, crt0, ds.ld, a7.bin), /tmp/libnds182,
#   /tmp/crtls_old, /tmp/dsrun (headless melonDS), /tmp/melonDS + /tmp/mb/src (to relink dsrun).
# Host needs: clang-18, ld.lld, llvm-objcopy, python3 (apt install clang lld llvm python3).
# If dsrun will not start (other libstdc++), relink it with the g++ line in tools/dsbuild/README.md.
set -e
cd "$(dirname "$0")"
git fetch --depth 1 origin ds-toolchain
git show FETCH_HEAD:ds-toolchain-1.tar.xz | xz -d | tar -x -C /tmp
echo "restored: build with tools/dsbuild/mkp.sh, run with /tmp/dsrun rom.nds FRAMES out.ppm [script] [frames]"
