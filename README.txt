BORE 2 DS toolchain, prebuilt (Linux x86_64).
ds-toolchain-1.tar.xz unpacks into /tmp:
  dsb        clang glue scripts, picolibc, compiler-rt (libgcc.a), libnds9.a, ndstool, crt0, linker script, a7.bin
  libnds182  libnds 1.8.2 headers and sources
  crtls_old  devkitarm-crtls 333d62c (dsi_arm9.mem, specs)
  dsrun      the headless melonDS runner (tools/dsemu/run.cpp)
  melonDS, mb/src  melonDS sources + libcore.a, libteakra.a to relink dsrun
Restore with tools/dsbuild/restore.sh on main. The host needs clang-18, ld.lld, llvm-objcopy, python3.
