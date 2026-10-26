.arm
.global _start
_start:
  ldr r12, =0x04000000
  mov r0, #0x900
  str r0, [r12, #0x180]
1: ldr r1, [r12, #0x180]
  and r1, r1, #15
  cmp r1, #0xA
  bne 1b
  mov r0, #0xB00
  str r0, [r12, #0x180]
2: ldr r1, [r12, #0x180]
  and r1, r1, #15
  cmp r1, #0xC
  bne 2b
  mov r0, #0xD00
  str r0, [r12, #0x180]
3: ldr r1, [r12, #0x180]
  and r1, r1, #15
  cmp r1, #1
  bhi 3b
  mov r0, #0
  str r0, [r12, #0x180]
4: b 4b
.ltorg
