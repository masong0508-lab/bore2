# DS build chain (no devkitPro needed)

Built from source with clang-18 (ARM target), ld.lld, picolibc, compiler-rt,
libnds 1.8.2 (pre-calico), devkitarm-crtls commit 333d62c, and ndstool.
Scripts here are the glue; paths point at /tmp/dsb and /tmp/libnds182.

- cc9.sh / cc9asm.sh: compile ARM9 C / asm files
- stub/: tiny header stubs libnds needs without devkitARM
- glue.c: newlib-ish hooks (stdout, sbrk, heap, argv)
- arm7/: the real ARM7 (touch screen, buttons, clock, sound) built from libnds ARM7 sources: arm7/build7.sh -> a7.bin. (a7.s is the old handshake-only stub, no longer used.)
- mk.sh NAME: build NAME.c to NAME.nds. ndstool adds the 0x800 secure
  stub itself, so the first 0x800 bytes of the linked binary are dropped.
- tools/dsemu/run.cpp: headless melonDS runner (NDS mode, FreeBIOS):
  `dsrun rom.nds FRAMES out.ppm` dumps both screens.

## Rebuilding /tmp from source (new container)
Check first: if /tmp/dsb, /tmp/libnds182, /tmp/crtls_old and /tmp/mb are there, only `dsrun` may need rebuilding.
- picolibc 1.8.12: `cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=tc.cmake -DCMAKE_INSTALL_PREFIX=/tmp/dsb/pl` (pip has no meson here, cmake works), then `ninja install`.
- compiler-rt builtins (llvm 18) for armv5te -> /tmp/dsb/libgcc.a.
- libnds v1.8.2 -> /tmp/libnds182; libnds9.a = source/arm9 (+dldi, system) and source/common built with cc9.sh / cc9asm.sh (not keyboard.c, it needs grit).
- devkitarm-crtls at commit 333d62c -> /tmp/crtls_old. `ds_arm9_crt0.o` = crt0.s here (crtls ds_arm9_crt0.s with `subnes` written `subsne` for clang).
  `/tmp/dsb/t/ds.ld` = crtls ds_arm9.ld with `>ewram` added to the two `:twl` / `:NONE` section lines (ld.lld needs a region).
- /tmp/dsb/t also holds glue.o (glue.c), a7.bin and copies of mk.sh / mkp.sh; /tmp/dsb/stub is stub/ here.
- ndstool from github devkitPro/ndstool -> /tmp/dsb/bin/ndstool.
- melonDS core: build melonDS with cmake into /tmp/mb (libcore.a, teakra), then
  `g++ -std=c++17 -O2 -DARCHITECTURE_x86_64=1 -DGDBSTUB_ENABLED -I/tmp/melonDS/src -I/tmp/melonDS/src/teakra/include tools/dsemu/run.cpp /tmp/mb/src/libcore.a /tmp/mb/src/teakra/src/libteakra.a -lpthread -o /tmp/dsrun`
- Runner extras: `BORE_SAV=file` keeps an 8 KB EEPROM save between runs; argv[4] is a touch/button script, argv[5] a list of frames to dump.
