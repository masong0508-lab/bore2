# BORE 2 on the Dreamcast

Status: **boots and runs in an emulator.** Hello screen, the title with the BORE 2 logo, and SAN BOREAS drawn on the PowerVR at 640x480
from the game's own world code (`src/bigmap.c`, compiled unchanged). See `docs/DREAMCAST.md` for what works and what is next.
Nothing under `src/` is touched; the PS1 build does not know this directory exists.

## Files
- `dc_main.c` boot / title / life stages. `dc_gfx.c/.h` the PowerVR layer (2D in the game's 320x240 coordinates doubled, 3D polygons with near-plane clipping, text from the shared 8x8 font, the pad). `dc_life.c` SAN BOREAS from `bm_*`. `dc_stubs.c` the few PS1 services shared code calls. `dc_shared_pre.h` force-included before shared sources (renames the game's `link` that clashes with libc).
- `Makefile` (KOS). `toolchain/` the scripts that built the toolchain. `../../tools/dcrun.c` the headless emulator runner.

## Toolchain (rebuilt from source in /tmp/dc, C only, -j2; about 1.5 hours on 2 cores)
Only GitHub is reachable here, so the GNU sources come from mirrors on GitHub. gnu.org, sourceware.org, gcc.gnu.org, PyPI-style package hosts, conda and Ubuntu apt were refused, and were not worked around.
1. `git clone --depth 1 https://github.com/KallistiOS/kallistios /tmp/dc/kos` (the toolchain builder is `utils/kos-chain`; its profile "stable" = binutils 2.45.1, gcc 15.2.0, newlib 4.6.0).
2. Sources: binutils 2.46.0, GMP 6.2.1, MPFR 4.2.1, MPC 1.3.1 and flex 2.6.4 from `NetBSD/src` (`external/gpl3/binutils/dist`, `external/lgpl3/*/dist`, `external/bsd/flex/dist`; sparse checkout). gcc from `gcc-mirror/gcc` tag `releases/gcc-15.2.0` (sparse; keep `gcc/testsuite/selftests`, `contrib/texi2pod.pl`, `gcc/d/*.def`, `gcc/rust/*.def`). newlib from `cygwin/cygwin` tag `newlib-4.6.0` (sparse; keep `newlib/doc`). bminor/* mirrors were not reachable.
3. Run, in order (they patch with KOS's own diffs): `toolchain/build_flex.sh`, `build_binutils.sh`, `prep_gcc.sh`, `build_gcc.sh`, `build_newlib.sh`. (Fixes baked in: NetBSD's bfd Makefile uses bmake's `$>`; NetBSD strips generated lexers, hence host flex; gcc wants empty `gcc/d` and `gcc/rust` build dirs.) One multilib, m4-single little-endian. gcc is C only (no C++).
4. `source toolchain/environ.sh` (paths to /tmp/dc), then `make -C /tmp/dc/kos/kernel`, `make -C /tmp/dc/kos/addons`, `make -C /tmp/dc/kos/utils/{scramble,makeip,bin2c}`.
5. `cd ports/dreamcast && source /tmp/dc/environ.sh && make` gives `bore2_dc.elf`.

## Emulator (headless, scriptable)
flycast (`flyinghead/flycast`, libretro core, `-DLIBRETRO=ON -DUSE_VULKAN=OFF -DUSE_LUA=OFF -DUSE_BREAKPAD=OFF`, submodules libchdr tinygettext(+tinycmmc) asio xbyak rcheevos SDL). No GL dev headers here: `GL/gl.h` was a stand-in (`#include <SDL_opengl.h>` with `NO_SDL_GLEXT`, Khronos `glext.h`, `khrplatform.h`). The core runs the ELF straight in its built-in HLE BIOS (no Sega BIOS needed).
`tools/dcrun.c` is the frontend: surfaceless EGL on Mesa llvmpipe, scripted pad, PPM screenshots.
`gcc -O1 -o dcrun tools/dcrun.c -I<libretro.h dir> -ldl` then `dcrun flycast_libretro.so bore2_dc.elf 300 /tmp/shot script.txt` (script lines `frame start|a|up...` or `frame shot`; a shot is also saved at the end). A frame takes about 1 s on llvmpipe, so 300 frames is a few minutes.

## Controls
START or A on the title. In SAN BOREAS: D-pad walks and turns, A runs, X/Y zoom, B back to the title.
