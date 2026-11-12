#!/bin/bash
# build_runner.sh: the PS1 test emulator from source (the slow way; tools/perf/restore.sh unpacks the built one).
# DuckStation's regression runner only, headless: no Qt, no OpenGL/Vulkan, no audio output, no SDL. Then OpenBIOS.
#   sources:  /home/claude/tools/ds (DuckStation), /home/claude/tools/nugget (OpenBIOS)       result: /home/claude/tools/ds/b/bin
# Needs: git cmake ninja g++ libcurl4-openssl-dev (apt), and the PS1 compiler in /opt/psn00bsdk/bin for OpenBIOS.
# DuckStation a003832e (2023-04-29): the last commit before the runner's build broke for three months; its regtest_host.cpp
# is the one the patches here were made for (unchanged from e5039055, 2023-01-17, to 5b980daf, 2023-08-15).
# OpenBIOS: pcsx-redux/nugget (pcsx-redux's src/mips) 821a3ff (2026-10-18), the last commit whose plain `make` builds with this
# C compiler. Later ones embed a C++ shell (it needs a C++ library; OPENBIOS_SHELL=shell still builds) and leave a test build
# short of RAM (test/oldsave2.sh "50 59b5752") to crash while it loads.
set -e
HERE=$(cd "$(dirname "$0")" && pwd); T=${T:-/home/claude/tools}
DS=a003832e33bb66857a5d9fe2d0ca11e9463771ed; OB=821a3ff0248a85bb0330221794f3b9556c078a6e
mkdir -p $T; cd $T
[ -d ds/.git ] || git clone --filter=blob:none --no-checkout https://github.com/stenzek/duckstation ds
cd ds; git checkout -q -f --detach $DS
R=src/duckstation-regtest
patch -p1 -d $R < "$HERE/regtest_base.patch"; patch -p1 -d $R < "$HERE/regtest_pcsample.patch"; patch -p1 -d $R < "$HERE/regtest_card_wav.patch"
# (without X11 nothing includes these for PlaySoundAsync)
sed -i 's|^#include <cinttypes>$|#include <cinttypes>\n#include <spawn.h>\n#include <unistd.h>|' src/frontend-common/platform_misc_unix.cpp
cmake -G Ninja -S . -B b -DCMAKE_BUILD_TYPE=Release -DBUILD_QT_FRONTEND=OFF -DBUILD_NOGUI_FRONTEND=OFF -DBUILD_REGTEST=ON \
  -DENABLE_CUBEB=OFF -DENABLE_OPENGL=OFF -DENABLE_VULKAN=OFF -DENABLE_DISCORD_PRESENCE=OFF -DENABLE_CHEEVOS=OFF \
  -DUSE_SDL2=OFF -DUSE_X11=OFF -DUSE_EGL=OFF -DUSE_WAYLAND=OFF "-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc"
ninja -C b duckstation-regtest
cd $T
[ -d nugget/.git ] || git clone --filter=blob:none --no-checkout https://github.com/pcsx-redux/nugget.git nugget
cd nugget; git checkout -q -f --detach $OB
PATH=/opt/psn00bsdk/bin:$PATH make -C openbios -j2 PREFIX=mipsel-none-elf
mkdir -p $T/ds/b/bin/bios; cp -f openbios/openbios.bin $T/ds/b/bin/bios/openbios.bin   # (the runner takes the BIOS it finds in bios/ beside itself)
echo "built: $T/ds/b/bin/duckstation-regtest"
# To pack it for restore.sh (then put the file on the orphan branch ps1-toolchain):
#   tar -C / -cf - home/claude/tools/ds/b/bin/duckstation-regtest home/claude/tools/ds/b/bin/resources home/claude/tools/ds/b/bin/bios | xz -9 > ps1-toolchain-1.tar.xz
