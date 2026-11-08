#!/usr/bin/env python3
# ovl.py: the OVERLAYS (src/ovl.h).
#   ovl.py keep OUT.c NM obj...                     what the overlays need from BORE2.EXE, as a table that keeps it through -gc-sections
#   ovl.py link OUT.OVL MAIN.elf GCC NM OBJCOPY obj...  link one overlay at the crowd arena's OVL_AT against BORE2.EXE's symbols
import subprocess, sys, re, os
OVL_AT, ROOM, ARENA = 73728, 192704 - 73728, 156096
def syms(nm, objs, kind):
    out = set()
    for o in objs:
        for l in subprocess.run([nm, o], capture_output=True, text=True).stdout.splitlines():
            p = l.split()
            if kind == 'U' and len(p) == 2 and p[0] == 'U': out.add(p[1])
            if kind == 'D' and len(p) == 3 and p[1].isupper() and p[1] != 'U': out.add(p[2])
    return out
cmd = sys.argv[1]
if cmd == 'keep':
    out, nm, objs = sys.argv[2], sys.argv[3], sys.argv[4:]
    need = sorted(syms(nm, objs, 'U') - syms(nm, objs, 'D'))
    need = ['__wrap_' + s if s in ('memcpy', 'memmove') else s for s in need]
    need = [s for s in need if not s.startswith('__') or s.startswith('__wrap_')]   # (libgcc's helpers come with each overlay)
    with open(out + '.tmp', 'w') as f:
        f.write('// made by tools/ovl.py: what the overlays use from BORE2.EXE (taking its address keeps it through -gc-sections)\n')
        for s in need: f.write('extern char %s;\n' % s)
        f.write('void *const ovl_keep_tab[] __attribute__((used)) = {\n' + ''.join('    &%s,\n' % s for s in need) + '    0 };\n')
    if not os.path.exists(out) or open(out).read() != open(out + '.tmp').read(): os.replace(out + '.tmp', out)
    else: os.remove(out + '.tmp')
elif cmd == 'link':
    out, elf, gcc, nm, objcopy, objs = sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7:]
    base = None
    for l in subprocess.run([nm, elf], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 3 and p[2] == 'arena_mem': base = int(p[0], 16) & 0xffffffff
    if base is None: sys.exit('ovl.py: no arena_mem in ' + elf)
    low = os.path.basename(out).startswith('GOTCHI')   # (the toy's screens need no realm land: it loads at the start of the arena, with all of its 152 KB)
    at = base + (0 if low else OVL_AT); room = ARENA if low else ROOM
    ld = out + '.ld'
    open(ld, 'w').write('''MEMORY { OVL (rwx) : ORIGIN = 0x%08x, LENGTH = %d }
SECTIONS {
  .text : { KEEP(*(.ovl_head)) *(.text .text.*) } > OVL
  .rodata : { *(.rodata .rodata.*) } > OVL
  .data : { *(.data .data.*) *(.sdata .sdata.*) } > OVL
  .bss : { *(.bss .bss.*) *(.sbss .sbss.*) *(COMMON) } > OVL
  /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.gnu.attributes) *(.pdr) *(.mdebug.*) }
}
''' % (at, room))
    e = out + '.elf'
    r = subprocess.run([gcc, '-nostdlib', '-static', '-G0', '-T', ld, '-Wl,--just-symbols=' + elf, '-Wl,--wrap=memcpy', '-Wl,--wrap=memmove', '-Wl,--no-warn-rwx-segments', '-o', e] + objs + ['-lgcc'], capture_output=True, text=True)
    if r.returncode: sys.exit(r.stderr)
    subprocess.check_call([objcopy, '-O', 'binary', e, out])
    n = os.path.getsize(out)
    if (n + 2047) // 2048 * 2048 > room: sys.exit('ovl.py: %s is %d bytes, the room is %d' % (out, n, room))
    end = at   # (the end of the overlay's own sections, its zeroed data too)
    ro = nm.replace('-nm', '-readelf')
    for l in subprocess.run([ro, '-S', '-W', e], capture_output=True, text=True).stdout.splitlines():
        m = re.search(r'\]\s+(\.\S+)\s+\S+\s+([0-9a-f]{8})\s+[0-9a-f]+\s+([0-9a-f]+)', l)
        if m and m.group(1) in ('.text', '.rodata', '.data', '.bss'): end = max(end, int(m.group(2), 16) + int(m.group(3), 16))
    if end - at > room - 64: sys.exit('ovl.py: %s needs %d bytes with its data, the room is %d' % (out, end - at, room))
    print('%s: %d bytes at 0x%08x' % (os.path.basename(out), n, at))
