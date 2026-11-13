#!/bin/bash
# One command to get the prebuilt Dreamcast toolchain back into /tmp/dc (instead of hours of rebuilding):
#   ports/dreamcast/toolchain/restore.sh
# It takes dc-toolchain-1.tar.xz from the repo's dc-toolchain branch (a GitHub release could not be made from the build session)
# and unpacks sh-elf (binutils/gcc/newlib), KallistiOS (built), host flex, the flycast core and dcrun (in /tmp/dc/pack/bin).
# The tarball's dcrun is OLD (it does not print the game's log: no fps, no road audit result), so dcrun is rebuilt from tools/dcrun.c
# here (needs gcc and curl; libretro.h comes from libretro-common). About 10 seconds in all.
set -e
cd "$(dirname "$0")"
git fetch --depth 1 origin dc-toolchain
git show FETCH_HEAD:dc-toolchain-1.tar.xz | xz -d | tar -x -C /tmp
cp /tmp/dc/pack/bin/environ.sh /tmp/dc/environ.sh
mkdir -p /tmp/dc/lrh
[ -s /tmp/dc/lrh/libretro.h ] || curl -sS -o /tmp/dc/lrh/libretro.h https://raw.githubusercontent.com/libretro/libretro-common/master/include/libretro.h
# (the newer libretro.h calls it D3D11; dcrun.c only uses the name to refuse that context)
gcc -O2 -w -DRETRO_HW_CONTEXT_DIRECT3D=RETRO_HW_CONTEXT_D3D11 -o /tmp/dc/pack/bin/dcrun ../../../tools/dcrun.c -I/tmp/dc/lrh -ldl
echo "restored: source /tmp/dc/environ.sh; emulator: /tmp/dc/pack/bin/dcrun /tmp/dc/pack/bin/flycast_libretro.so <elf> <frames> <outprefix> [script]"
