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
