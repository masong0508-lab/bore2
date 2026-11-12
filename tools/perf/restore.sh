#!/bin/bash
# One command to get the PS1 test tools back after the machine is replaced (instead of rebuilding them from source):
#   tools/perf/restore.sh
# 1. The test emulator: ps1-toolchain-1.tar.xz from the repo's ps1-toolchain branch is unpacked to the path every test script uses,
#    /home/claude/tools/ds/b/bin: duckstation-regtest (DuckStation's regression runner with regtest_base.patch, regtest_pcsample.patch
#    and regtest_card_wav.patch), resources/ (its game database) and bios/openbios.bin (OpenBIOS: no proprietary BIOS).
#    It needs libcurl.so.4 from the system (apt install libcurl4t64); the C++ runtime is linked in.
# 2. The compiler and PSn00bSDK 0.24, the way .github/workflows/build.yml downloads them, both into /opt/psn00bsdk (the layout the
#    test scripts build with: -DPSN00BSDK_TC=/opt/psn00bsdk), and /opt/mipsel as a link to it (tools/perf and test/*.py call
#    /opt/mipsel/bin/mipsel-none-elf-nm). Skipped when already there; FORCE=1 installs again. SDK=<dir> installs somewhere else.
# Then: export PATH=/opt/psn00bsdk/bin:$PATH PSN00BSDK_LIBS=/opt/psn00bsdk/lib/libpsn00b; test/ds.sh <cue> <name> [frames] [script]
# To rebuild the emulator from source instead: tools/perf/build_runner.sh (about 15 minutes).
set -e
cd "$(dirname "$0")"
ROOT=${ROOT:-/}; SDK=${SDK:-/opt/psn00bsdk}
git fetch --depth 1 origin ps1-toolchain
git show FETCH_HEAD:ps1-toolchain-1.tar.xz | xz -d | tar -x -C "$ROOT"
R="${ROOT%/}/home/claude/tools/ds/b/bin/duckstation-regtest"
if ldd "$R" | grep -q "not found"; then echo "missing libraries (apt install libcurl4t64):"; ldd "$R" | grep "not found"; fi
if [ -n "$FORCE" ] || [ ! -x "$SDK/bin/mipsel-none-elf-gcc" ] || [ ! -d "$SDK/lib/libpsn00b" ]; then
  T=$(mktemp -d); B=https://github.com/Lameguy64/PSn00bSDK/releases/download/v0.24
  curl -fsSL $B/gcc-mipsel-none-elf-12.3.0-Linux.zip -o "$T/gcc.zip"
  curl -fsSL $B/PSn00bSDK-0.24-Linux.zip -o "$T/sdk.zip"
  unzip -q "$T/sdk.zip" -d "$T"
  rm -rf "$SDK"; mkdir -p "$(dirname "$SDK")"; mv "$T/PSn00bSDK-0.24-Linux" "$SDK"
  unzip -q -o "$T/gcc.zip" -d "$SDK"
  chmod -R a+rx "$SDK"; rm -rf "$T"
  echo "installed: the compiler and PSn00bSDK 0.24 in $SDK"
fi
if [ "$SDK" = /opt/psn00bsdk ] && [ ! -e /opt/mipsel ]; then ln -s /opt/psn00bsdk /opt/mipsel; fi
echo "restored: $R"
"$R" -version 2>&1 | grep -a "Regression Test Runner" || true
