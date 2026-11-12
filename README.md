# ps1-toolchain

`ps1-toolchain-1.tar.xz`: the built PS1 test emulator for BORE 2, unpacked by `tools/perf/restore.sh` (on main) to
`/home/claude/tools/ds/b/bin`: `duckstation-regtest` (DuckStation a003832e, regression runner only, headless, with
`tools/perf/regtest_base.patch`, `regtest_pcsample.patch` and `regtest_card_wav.patch`), `resources/` and
`bios/openbios.bin` (OpenBIOS, pcsx-redux/nugget 821a3ff). Linux x86-64 (built on Ubuntu 24.04), needs libcurl.so.4.
Built from source by `tools/perf/build_runner.sh`. DuckStation and OpenBIOS keep their own licences (see their repositories).
