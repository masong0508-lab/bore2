#!/bin/bash
# cc9.sh file.[cs] out.o : compile for the DS ARM9 (clang, ARM mode)
F=/tmp/libnds182
clang --target=armv5te-none-eabi -mcpu=arm946e-s -marm -mfloat-abi=soft -O2 -g0 -ffunction-sections -fdata-sections -fomit-frame-pointer -ffreestanding -fno-builtin-printf -DARM9 -DNDEBUG -D__NDS__ -w \
  -I$F/include -I$F/source/common -I/tmp/dsb/pl/include -I/tmp/dsb/stub -include /tmp/dsb/stub/nds_compat.h $EXTRA $XTRA -c "$1" -o "$2"
