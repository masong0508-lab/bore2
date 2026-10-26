#!/bin/bash
F=/tmp/libnds182
clang --target=armv4t-none-eabi -mcpu=arm7tdmi -marm -mfloat-abi=soft -Os -ffreestanding -fno-builtin -fomit-frame-pointer -DARM7 -DNDEBUG -D__NDS__ -w -I$F/include -I$F/source/common -I/tmp/dsb/pl/include -I/tmp/dsb/stub -include /tmp/dsb/stub/nds_compat.h $EXTRA -c "$1" -o "$2"
