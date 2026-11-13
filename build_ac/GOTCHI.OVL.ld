MEMORY { OVL (rwx) : ORIGIN = 0x801adbc8, LENGTH = 156096 }
SECTIONS {
  .text : { KEEP(*(.ovl_head)) *(.text .text.*) } > OVL
  .rodata : { *(.rodata .rodata.*) } > OVL
  .data : { *(.data .data.*) *(.sdata .sdata.*) } > OVL
  .bss : { *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON) } > OVL
  /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.gnu.attributes) *(.pdr) *(.mdebug.*) }
}
