#!/bin/bash
# One command to get the prebuilt Dreamcast toolchain back into /tmp/dc (instead of hours of rebuilding):
#   ports/dreamcast/toolchain/restore.sh
# It takes dc-toolchain-1.tar.xz from the repo's dc-toolchain branch (a GitHub release could not be made from the build session)
# and unpacks sh-elf (binutils/gcc/newlib), KallistiOS (built), host flex, the flycast core and dcrun (in /tmp/dc/pack/bin).
set -e
cd "$(dirname "$0")"
git fetch --depth 1 origin dc-toolchain
git show FETCH_HEAD:dc-toolchain-1.tar.xz | xz -d | tar -x -C /tmp
cp /tmp/dc/pack/bin/environ.sh /tmp/dc/environ.sh
echo "restored: source /tmp/dc/environ.sh; emulator: /tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so <elf> <frames> <outprefix> [script]"
