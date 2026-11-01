#!/bin/bash
# host flex (gcc from git needs it for gengtype-lex.l); source: NetBSD/src external/bsd/flex/dist
set -e
D=/tmp/dc/src/netbsd/external/bsd/flex/dist
mkdir -p $D/tools && printf 'all:\ninstall:\n' > $D/tools/Makefile.in   # NetBSD drops tools/
mkdir -p /tmp/dc/build/flex && cd /tmp/dc/build/flex
$D/configure --prefix=/tmp/dc/host --disable-nls --disable-shared MAKEINFO=true
# NetBSD ships the pre-made scanner/parser as init*.c
cp $D/src/initscan.c src/scan.c; cp $D/src/initparse.c src/parse.c; cp $D/src/initparse.h src/parse.h
touch src/scan.c src/parse.c src/parse.h
make -C src flex MAKEINFO=true ACLOCAL=true AUTOCONF=true AUTOMAKE=true AUTOHEADER=true
mkdir -p /tmp/dc/host/bin && cp src/flex /tmp/dc/host/bin/
/tmp/dc/host/bin/flex --version
