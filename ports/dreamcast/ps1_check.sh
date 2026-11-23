#!/usr/bin/env bash
# Independently compile the native PlayStation target from this source tree,
# even when this job mainly builds Dreamcast. Print the output image checksum
# for regression tracking. This is a build guard, NOT a byte-for-byte A/B
# comparison to main (which uses a separate checkout and must be measured).
# Usage: ps1_check.sh [source dir] [build dir]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRCDIR=${1:-"$ROOT"}
B=${2:-"$ROOT/out-ps1-dc-compat"}
SDK=${PS1_SDK:-/opt/psn00bsdk}
test -s "$SRCDIR/CMakeLists.txt" || { echo "Missing PS1 CMakeLists under $SRCDIR" >&2; exit 2; }
test -s "$SDK/lib/libpsn00b/cmake/sdk.cmake" || { echo "Missing PSn00bSDK under $SDK" >&2; exit 2; }
test -x "$SDK/bin/mipsel-none-elf-gcc" || { echo "Missing native PS1 compiler under $SDK" >&2; exit 2; }
export PATH="$SDK/bin:$PATH"
mkdir -p "$B"
cmake -S "$SRCDIR" -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_TOOLCHAIN_FILE="$SDK/lib/libpsn00b/cmake/sdk.cmake" \
   -DPSN00BSDK_TC="$SDK" > "$B/config.log" 2>&1 || {
       tail -70 "$B/config.log" >&2; exit 2;
   }
cmake --build "$B" --target bore2 -j 2 > "$B/build.log" 2>&1 || {
    tail -100 "$B/build.log" >&2; exit 2;
}
E="$B/bore2.elf"
test -s "$E" || { echo "PS1 ELF missing" >&2; exit 2; }
mipsel-none-elf-objcopy -O binary -R .MIPS.abiflags "$E" "$B/bore2.bin.chk"
test -s "$B/bore2.bin.chk" || { echo "PS1 output image missing" >&2; exit 2; }
end=$(mipsel-none-elf-nm "$E" | awk '$3=="_end" {print $1; found=1} END {if (!found) exit 1}')
echo "PS1 COMPAT BUILD PASS: _end $end  image sha256 $(sha256sum "$B/bore2.bin.chk" | awk '{print $1}')"
