#!/bin/bash
clang --target=${TGT:-armv5te-none-eabi} ${CPU:--mcpu=arm946e-s} -x assembler-with-cpp -DARM9 -I/tmp/libnds182/include -I/tmp/libnds182/source/common -c "$1" -o "$2"
