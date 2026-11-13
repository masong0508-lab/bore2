MEMORY { OVL (rwx) : ORIGIN = 0x801bfc60, LENGTH = 82368 }
SECTIONS {
  .text : { KEEP(*(.ovl_head)) *(.text .text.*) } > OVL
  .rodata : { *(.rodata .rodata.*) } > OVL
  .data : { *(.data .data.*) *(.sdata .sdata.*) } > OVL
  .bss : { *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON) } > OVL
  /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.gnu.attributes) *(.pdr) *(.mdebug.*) }
}
