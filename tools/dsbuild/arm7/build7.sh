#!/bin/bash
# Builds the real ARM7 (touch, buttons, clock, sound) from libnds 1.8.2 ARM7 sources + main7.c -> ../a7.bin. Needs clang, ld.lld, llvm-objcopy and libnds at /tmp/libnds182.
set -e; F=/tmp/libnds182; D=$(cd "$(dirname "$0")"; pwd); W=/tmp/a7b; mkdir -p $W; cd $W; cp $D/main7.c $D/cc7.sh .
A="--target=armv4t-none-eabi -mcpu=arm7tdmi -x assembler-with-cpp -DARM7 -I$F/include -I$F/source/common"
for f in audio clock firmware input spi system touch userSettings systemShutDown; do ./cc7.sh $F/source/arm7/$f.c $f.o; done
for f in codec.twl microphone microphone.twl sdmmc.twl card.twl i2c.twl; do ./cc7.sh $F/source/arm7/$f.c t_$f.o; done
for f in fifosystem interrupts timers; do ./cc7.sh $F/source/common/$f.c c_$f.o; done; ./cc7.sh $F/source/common/libnds_exit.arm.c c_exit.o
# thumb files: the assembler wants flag-setting forms on ARMv4
sed 's/^\tmov\tr0, r1/\tadds\tr0, r1, #0/; s/^\tmov\tr2, r0/\tadds\tr2, r0, #0/; s/^\tmov\t\(r[0-7]\), #/\tmovs\t\1, #/' $F/source/common/biosCalls.s > bios.s; clang $A -c bios.s -o s_biosCalls.o
sed 's/^\tmov\t\(r[0-7]\), #/\tmovs\t\1, #/' $F/source/common/swiSoftReset.s > ssr.s; clang $A -c ssr.s -o s_ssr.o
clang $A -c $F/source/arm7/intrwait.s -o intrwait.o; clang $A -c $F/source/common/interruptDispatcher.s -o s_interruptDispatcher.o
sed 's/mov\tr3, #(__sync_end-__sync_start)/ldr\tr3, =(__sync_end-__sync_start)/' /tmp/crtls_old/ds_arm7_crt0.s > crt7.s; clang $A -c crt7.s -o crt0.o
./cc7.sh main7.c main7.o
ld.lld -T /tmp/crtls_old/ds_arm7.ld --gc-sections --no-check-sections -o a7.elf crt0.o main7.o audio.o clock.o firmware.o input.o spi.o system.o touch.o userSettings.o c_fifosystem.o c_interrupts.o c_timers.o intrwait.o s_biosCalls.o s_interruptDispatcher.o t_i2c.twl.o s_ssr.o t_codec.twl.o t_microphone.o t_microphone.twl.o t_sdmmc.twl.o t_card.twl.o c_exit.o systemShutDown.o
llvm-objcopy -R .twl -R .twl_bss -O binary a7.elf $D/../a7.bin; ls -la $D/../a7.bin
